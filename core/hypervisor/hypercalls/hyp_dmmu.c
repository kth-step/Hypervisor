#include "hyper.h"
#include "dmmu.h"
#include "mmu.h"

extern virtual_machine *curr_vm;
#if 0
#define DEBUG_MMU_L1_Free
#define DEBUG_MMU_L1_CREATE
#define DEBUG_MMU_L1_SWITCH
#define DEBUG_MMU_SET_L1
#endif

/*Get physical address from Linux virtual address*/
#define LINUX_PA(va) ((va) - (addr_t)(curr_vm->config->firmware->vstart) + (addr_t)(curr_vm->config->firmware->pstart))
/*Get virtual address from Linux physical address*/
#define LINUX_VA(pa) ((pa) - (addr_t)(curr_vm->config->firmware->pstart) + (addr_t)(curr_vm->config->firmware->vstart))

addr_t linux_pt_get_empty_l2();

void dump_L1pt(virtual_machine * curr_vm)
{
	uint32_t *guest_pt_va;
	addr_t guest_pt_pa;
	COP_READ(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0,
		 (uint32_t) guest_pt_pa);
	guest_pt_va = mmu_guest_pa_to_va(guest_pt_pa, curr_vm->config);
	uint32_t index;
	for (index = 0; index < 4096; index++) {
		if (*(guest_pt_va + index) != 0x0)
			printf("add %x %x \n", index, *(guest_pt_va + index));
	}
}

void dump_L2pt(addr_t l2_base, virtual_machine * curr_vm)
{
	uint32_t l2_idx, l2_desc_pa_add, l2_desc_va_add, l2_desc;
	for (l2_idx = 0; l2_idx < 512; l2_idx++) {
		l2_desc_pa_add = L2_DESC_PA(l2_base, l2_idx);
		l2_desc_va_add =
		    mmu_guest_pa_to_va(l2_desc_pa_add, curr_vm->config);
		l2_desc = *((uint32_t *) l2_desc_va_add);
		printf("L2 Entry at %x + %x is %x \n", l2_base, l2_idx,
		       l2_desc);
	}
}

void hypercall_dyn_switch_mm(addr_t table_base, uint32_t context_id)
{
#ifdef DEBUG_MMU_L1_SWITCH
	printf("Hypercall switch PGD\t table_base:%x context_id:%x\n",
	       table_base, context_id);
#endif

	/*Switch the TTB and set context ID */
	/*
	if (dmmu_switch_mm(table_base & L1_BASE_MASK)) {
		printf("\n\tCould not switch MM\n");
		dmmu_switch_mm(table_base & L1_BASE_MASK);
	}*/
	push_request(request_dmmu_switch_mm(table_base & L1_BASE_MASK));
	COP_WRITE(COP_SYSTEM, COP_CONTEXT_ID_REGISTER, context_id);	//Set context ID
	isb();

}

/* Free Page table, Make it RW again */
void hypercall_dyn_free_pgd(addr_t * pgd_va)
{
#ifdef DEBUG_MMU_L1_Free
	printf("\n\t\t\tHypercall FREE PGD\n\t\t pgd:%x \n", pgd_va);
#endif
	uint32_t i, clean_va;

	uint32_t page_offset = curr_vm->guest_info.page_offset;

	/*First get the physical address of the lvl 2 page by
	 * looking at the index of the pgd location. Then set
	 * 4 lvl 2 pages to read write*/

	addr_t *master_pgd_va;
	/*Get master page table */
	master_pgd_va =
	    (addr_t *) (curr_vm->config->pa_initial_l1_offset + page_offset);
	addr_t *l1_pt_entry_for_desc =
	    (addr_t *) & master_pgd_va[(addr_t) pgd_va >> MMU_L1_SECTION_SHIFT];
	uint32_t l1_desc_entry = *l1_pt_entry_for_desc;

	/*Get level 2 page table address */

	uint32_t table2_pa = MMU_L1_PT_ADDR(l1_desc_entry);
	uint32_t table2_idx =
	    (table2_pa - (table2_pa & L2_BASE_MASK)) >> MMU_L1_PT_SHIFT;
	table2_idx *= 0x100;	/*256 pages per L2PT */

	uint32_t l2_entry_idx = (((uint32_t) pgd_va << 12) >> 24) + table2_idx;

	uint32_t *l2_page_entry =
	    (addr_t *) (mmu_guest_pa_to_va(table2_pa & L2_BASE_MASK,
					   (curr_vm->config)));
	uint32_t page_pa = MMU_L2_SMALL_ADDR(l2_page_entry[l2_entry_idx]);

	uint32_t attrs = MMU_L2_TYPE_SMALL;
	attrs |= (MMU_FLAG_B | MMU_FLAG_C);
	attrs |= MMU_AP_USER_RW << MMU_L2_SMALL_AP_SHIFT;
	
	
	for (i = l2_entry_idx; i < l2_entry_idx + 4; i++) {
		/*
		if (dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i))
			printf("\n\tCould not unmap L2 entry in new PGD\n");*/
		push_request(request_dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i));
	}
	
	/*
	if (dmmu_unmap_L1_pt(LINUX_PA((addr_t) pgd_va)))
		printf("\n\tCould not unmap L1 PT in free pgd\n");*/
	push_request(request_dmmu_unmap_L1_pt(LINUX_PA((addr_t) pgd_va)));

	for (i = l2_entry_idx; i < l2_entry_idx + 4; i++, page_pa += 0x1000) {
		/*
		if (dmmu_l2_map_entry
		    (table2_pa & L2_BASE_MASK, i, page_pa, attrs))
			printf("\n\tCould not map L2 entry in new pgd\n");
		*/
		push_request(request_dmmu_l2_map_entry(table2_pa & L2_BASE_MASK, i, page_pa, attrs));

		clean_va = LINUX_VA(MMU_L2_SMALL_ADDR(l2_page_entry[i]));
		CacheDataInvalidateBuff(&l2_page_entry[i], 4);
		dsb();

		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, clean_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, clean_va);	/*Update cache with new values */
		dsb();
		isb();
	}

#ifdef AGGRESSIVE_FLUSHING_HANDLERS
	hypercall_dcache_clean_area((uint32_t) pgd_va, 0x4000);
#endif
}

/*New pages for processes, copys kernel space from master pages table
 *and cleans the cache, set these pages read only for user */
//#define DEBUG_MMU_L1_CREATE 
void hypercall_dyn_new_pgd(addr_t * pgd_va)
{
#ifdef DEBUG_MMU_L1_CREATE
	printf("*** Hypercall new PGD pgd: va:0x%x pa:0x%x\n", pgd_va,
	       LINUX_PA((addr_t) pgd_va));
#endif

	/*If the requested page is in a section page, we need to modify it to lvl 2 pages
	 *so we can modify the access control granularity */
	uint32_t i, end, table2_idx, table2_pa, err = 0;

	addr_t *master_pgd_va;
	addr_t phys_start = curr_vm->config->firmware->pstart;
	addr_t page_offset = curr_vm->guest_info.page_offset;
	addr_t linux_va;
	/*Get master page table */
	master_pgd_va =
	    (addr_t *) (curr_vm->config->pa_initial_l1_offset + page_offset);
	addr_t *l1_pt_entry_for_desc =
	    (addr_t *) & master_pgd_va[(addr_t) pgd_va >> MMU_L1_SECTION_SHIFT];
	uint32_t l1_desc_entry = *l1_pt_entry_for_desc;

	if (l1_desc_entry & MMU_L1_TYPE_SECTION){ 
		printf("****ERROR: Linux 1-to-1 memory mapped by section\n");
		return -1;
	}
	
	//TODO: Exit if the l1 descriptor is unmapped.

	/*Get the index of the page entry to make read only */

	table2_pa = MMU_L1_PT_ADDR(l1_desc_entry);
	table2_idx =
	    (table2_pa - (table2_pa & L2_BASE_MASK)) >> MMU_L1_PT_SHIFT;
	table2_idx *= 0x100;	/*256 pages per L2PT */

	uint32_t l2_entry_idx =
	    (((uint32_t) pgd_va << 12) >> 24) + table2_idx;

	uint32_t *l2_page_entry =
	    (addr_t *) (mmu_guest_pa_to_va(table2_pa & L2_BASE_MASK,
					   (curr_vm->config)));
	uint32_t page_pa =
	    MMU_L2_SMALL_ADDR(l2_page_entry[l2_entry_idx]);

	addr_t clean_va;

	//TODO: This must be done only if the ap is 3 (RW)
#ifdef DEBUG_MMU_L1_CREATE
	printf("\n\tRemapping as read only the page:%x\n",page_pa);
#endif
	for (i = l2_entry_idx; i < l2_entry_idx + 4;
	     i++, page_pa += 0x1000) {
		/*
		if ((err =
		     dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i)))
			printf
			    ("\n\tCould not unmap L2 entry in new PGD err:%x\n",
			     err);*/
		push_request(request_dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i));
		uint32_t ro_attrs =
		    0xE | (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT);
		/*
		if ((err =
		     dmmu_l2_map_entry(table2_pa & L2_BASE_MASK, i,
				       page_pa, ro_attrs)))
			printf
			    ("\n\tCould not map L2 entry in new pgd err:%x\n",
			     err);*/
		push_request(request_dmmu_l2_map_entry(table2_pa & L2_BASE_MASK, i,
				       page_pa, ro_attrs));

		clean_va =
		    LINUX_VA(MMU_L2_SMALL_ADDR(l2_page_entry[i]));
		CacheDataInvalidateBuff(&l2_page_entry[i], 4);
		dsb();

		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, clean_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, clean_va);	/*Update cache with new values */
		dsb();
		isb();
	}

	/* Page table 0x0 - 0x4000
	 * Reset user space 0-0x2fc0
	 * 0x2fc0 = 0xBF000000 END OF USER SPACE
	 * Copy kernel, IO and hypervisor mappings
	 * 0x2fc0 - 0x4000
	 * */
	uint32_t l1_va_add =
	    mmu_guest_pa_to_va(LINUX_PA((addr_t) pgd_va), curr_vm->config);
	memset((void *)l1_va_add, 0, 0x2fc0);
	memcpy((void *)(l1_va_add + 0x2fC0),
	       (uint32_t *) ((uint32_t) (master_pgd_va) + 0x2fc0), 0x1040);
	/*Clean dcache on whole table */
	clean_and_invalidate_cache();
	/*
	if ((err = dmmu_create_L1_pt(LINUX_PA((addr_t) pgd_va)))) {
		printf("\n\tCould not create L1 pt in new pgd in %x err:%d\n", LINUX_PA((addr_t) pgd_va), err);
		printf("\n\tMaster PGT:%d\n", LINUX_PA((addr_t) master_pgd_va));
		print_all_pointing_L1(LINUX_PA((addr_t) pgd_va), 0xfff00000);
		while (1) ;
	}*/
	push_request(request_dmmu_create_L1_pt(LINUX_PA((addr_t) pgd_va)));
}

/*In ARM linux pmd refers to pgd, ARM L1 Page table
 *Linux maps 2 pmds at a time  */
// #define DEBUG_SET_PMD
void hypercall_dyn_set_pmd(addr_t * pmd, uint32_t desc)
{
#ifdef DEBUG_SET_PMD
	printf("*** Hypercall set_pmd: va:0x%x pa:0x%x\n", pmd,
	       LINUX_PA((addr_t) pmd));
#endif
	uint32_t switch_back = 0;
	addr_t l1_entry, *l1_pt_entry_for_desc;
	addr_t curr_pgd_pa, *pgd_va, attrs;
	uint32_t l1_pt_idx_for_desc, l1_desc_entry, phys_start;

	phys_start = curr_vm->config->firmware->pstart;
	addr_t page_offset = curr_vm->guest_info.page_offset;
	uint32_t page_offset_idx = (page_offset >> MMU_L1_SECTION_SHIFT) * 4;

	/*Page attributes */
	uint32_t l2_rw_attrs = MMU_L2_TYPE_SMALL;
	l2_rw_attrs |= (MMU_FLAG_B | MMU_FLAG_C);
	l2_rw_attrs |= MMU_AP_USER_RW << MMU_L2_SMALL_AP_SHIFT;

	/*Get page table for pmd */
	pgd_va = (addr_t *) (((addr_t) pmd) & L1_BASE_MASK);	/*Mask with 16KB alignment to get PGD */

	/*Get current page table */
	COP_READ(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0,
		 (uint32_t) curr_pgd_pa);
	addr_t master_pgd_va =
	    (curr_vm->config->pa_initial_l1_offset + page_offset);

	/*Switch to the page table that we want to modify if we are not in it */
	if ((LINUX_PA((addr_t) pmd & L1_BASE_MASK)) != (curr_pgd_pa)) {
		/*This means that we are setting a pmd on another pgd, current
		 * API does not allow that, so we have to switch the physical ttb0
		 * back and forth */
		COP_WRITE(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, LINUX_PA((addr_t) pgd_va));	// Set TTB0
		isb();
		switch_back = 1;
	}

	/*Page table entry to set, if the desc is 0 we have to
	 * get it from the pgd*/
	if (desc != 0)
		l1_entry = desc;
	else
		l1_entry = *pmd;

	l1_pt_idx_for_desc =
	    ((l1_entry - phys_start) >> MMU_L1_SECTION_SHIFT) * 4;
	l1_pt_entry_for_desc =
	    (addr_t *) ((addr_t) pgd_va + l1_pt_idx_for_desc + page_offset_idx);
	l1_desc_entry = *l1_pt_entry_for_desc;

	if (l1_desc_entry & MMU_L1_TYPE_SECTION){ 
		printf("****ERROR: Linux 1-to-1 memory mapped by section\n");
		return -1;
	}
	//TODO: Exit if the l1 entry is unmapped.

	addr_t *desc_va = LINUX_VA(MMU_L1_PT_ADDR(l1_entry));

	COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, desc_va);
	COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, desc_va);
	dsb();
	isb();

	/*We need to make sure the new L2 PT is unreferenced */
#ifdef DEBUG_SET_PMD
	printf("\t desc is 0x%x\n", desc);
	printf("\t desc_va is 0x%x\n", desc_va);
#endif
	addr_t desc_va_idx = MMU_L1_SECTION_IDX((addr_t) desc_va);

	addr_t l2pt_pa = MMU_L1_PT_ADDR(pgd_va[desc_va_idx]);
	addr_t *l2pt_va =
	    (addr_t *) (mmu_guest_pa_to_va(l2pt_pa, (curr_vm->config)));

	uint32_t l2_idx = ((uint32_t) l1_entry << 12) >> 24;
	uint32_t l2entry_desc = l2pt_va[l2_idx];

	/*Get index of physical L2PT */
	uint32_t table2_idx =
	    (l2pt_pa - (l2pt_pa & L2_BASE_MASK)) >> MMU_L1_PT_SHIFT;
	table2_idx *= 0x100;	/*256 pages per L2PT */

	//TODO: Fix the method to read the ap
	/*If page entry for L2PT is RW, unmap it and make it RO so we can create a L2PT */
	if (((l2entry_desc >> 4) & 0xff) == 3) {
#ifdef DEBUG_SET_PMD
		printf("\n\tRemapping as read only the page:%x\n",l2entry_desc);
#endif
		uint32_t ph_block = PA_TO_PH_BLOCK(l2entry_desc);
		dmmu_entry_t *bft_entry = get_bft_entry_by_block_idx(ph_block);
#ifdef DEBUG_SET_PMD
		printf("\n\tCounter :%d\n",bft_entry->refcnt);
#endif
		if (dmmu_l2_unmap_entry
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx))
			printf("\n\tCould not unmap L2 entry in set PMD\n");
		//push_request(request_dmmu_l2_unmap_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx));
		uint32_t desc_pa = MMU_L2_SMALL_ADDR(desc);
		uint32_t ro_attrs =
		    0xE | (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT);
		
		if (dmmu_l2_map_entry
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		     desc_pa, ro_attrs))
			printf("\n\tCould not map L2 entry in set PMD\n");
		//push_request(request_dmmu_l2_map_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		//     desc_pa, ro_attrs));
	}
	uint32_t err;
	if (desc != 0) {
		
		if ((err = (dmmu_create_L2_pt(MMU_L2_SMALL_ADDR(desc))))) {
			printf("\n\tCould not create L2PT in set pmd at %x %d\n",
			       MMU_L2_SMALL_ADDR(desc), err);

#ifdef DEBUG_MMU_SET_L1
			printf("Hypercall set PMD pmd:%x val:%x \n", pmd, desc);
#endif

			print_all_pointing_L1(MMU_L2_SMALL_ADDR(desc),
					      0xfffff000);
			print_all_pointing_L2(MMU_L2_SMALL_ADDR(desc),
					      0xfffff000);
			
			while (1) ;
		}
		//push_request(request_dmmu_create_L2_pt(MMU_L2_SMALL_ADDR(desc)));
	}

	attrs = desc & 0x3FF;	/*Mask out addresss */

	/*Get virtual address of the translation for pmd */
	addr_t virt_transl_for_pmd =
	    (addr_t) ((pmd - pgd_va) << MMU_L1_SECTION_SHIFT);

	if (desc == 0) {
		uint32_t linux_va = pmd[0] - phys_start + page_offset;
		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();
		
		if (dmmu_unmap_L1_pageTable_entry(virt_transl_for_pmd))
			printf("\n\tCould not unmap L1 entry in set PMD\n");
		//push_request(request_dmmu_unmap_L1_pageTable_entry(virt_transl_for_pmd));

		
		if (dmmu_unmap_L1_pageTable_entry
		    (virt_transl_for_pmd + SECTION_SIZE))
			printf("\n\tCould not unmap L1 entry in set PMD\n");
		//push_request(request_dmmu_unmap_L1_pageTable_entry(virt_transl_for_pmd + SECTION_SIZE));

#ifdef AGGRESSIVE_FLUSHING_HANDLERS
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA,
			  (uint32_t) pmd);
#endif

		/*We need to make the l2 page RW again so that
		 *OS can reuse the address */
		
		if (dmmu_l2_unmap_entry
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx))
			printf("\n\tCould not unmap L2 entry in set PMD\n");
		if (dmmu_unmap_L2_pt(MMU_L2_SMALL_ADDR((uint32_t) * pmd)))
			printf("\n\tCould not unmap L2 pt in set PMD\n");
		if (dmmu_l2_map_entry
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		     MMU_L2_SMALL_ADDR((uint32_t) * pmd), l2_rw_attrs))
			printf("\n\tCould not map L2 entry in set PMD\n");
		
		//push_request(request_dmmu_l2_unmap_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx));
		//push_request(request_dmmu_unmap_L2_pt((uint32_t) * pmd));
		//push_request(request_dmmu_l2_map_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		//     MMU_L2_SMALL_ADDR((uint32_t) * pmd), l2_rw_attrs));

		//COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, (uint32_t)&l2pt_va[l2_idx]);
		CacheDataInvalidateBuff((uint32_t) & l2pt_va[l2_idx], 4);
		/*Flush entry */
		dsb();
	} else {
		uint32_t linux_va = desc - phys_start + page_offset;
		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();

		/*Flush entry */
		
		err =
		    dmmu_l1_pt_map(virt_transl_for_pmd,
				   MMU_L2_SMALL_ADDR(desc), attrs);
		if (err)
			printf("Could not map L1 PT in set PMD err:%d\n", err);
		err =
		    dmmu_l1_pt_map(virt_transl_for_pmd + SECTION_SIZE,
				   MMU_L2_SMALL_ADDR(desc) + 0x400, attrs);
		if (err)
			printf("Could not map L1 PT in set PMD err:%d\n", err);
		
		/*
		push_request(request_dmmu_l1_pt_map(virt_transl_for_pmd,
				   MMU_L2_SMALL_ADDR(desc), attrs));
		push_request(request_dmmu_l1_pt_map(virt_transl_for_pmd + SECTION_SIZE,
				   MMU_L2_SMALL_ADDR(desc) + 0x400, attrs));
		*/

		/*Flush entry */
		CacheDataInvalidateBuff((uint32_t) pmd, 4);
		CacheDataInvalidateBuff((uint32_t) & l2pt_va[l2_idx], 4);
		dsb();

	}
	if (switch_back) {
		COP_WRITE(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, curr_pgd_pa);	// Set TTB0
		isb();
	}
	/*Flush entry */
	clean_and_invalidate_cache();
}

#define USE_REQUEST
#define USE_REQUEST_SET_PTE_UNMAP

/*va is the virtual address of the page table entry for linux pages
 *the physical pages are located 0x800 below */
void hypercall_dyn_set_pte(addr_t * l2pt_linux_entry_va, uint32_t linux_pte,
			   uint32_t phys_pte)
{
#ifdef DEBUG_MMU
	printf("Hypercall set PTE va:%x linux_pte:%x phys_pte:%x \n",
	       l2pt_linux_entry_va, phys_pte, linux_pte);
#endif
	addr_t phys_start = curr_vm->config->firmware->pstart;
	uint32_t page_offset = curr_vm->guest_info.page_offset;
	uint32_t guest_size = curr_vm->config->firmware->psize;
#if 0				/*Linux 3.10.1 */
	uint32_t *l2pt_hw_entry_va =
	    (addr_t *) ((addr_t) l2pt_linux_entry_va + 0x800);
#else				/*Linux 2.6 */
	uint32_t *l2pt_hw_entry_va =
	    (addr_t *) ((addr_t) l2pt_linux_entry_va - 0x800);
#endif
	addr_t l2pt_hw_entry_pa =
	    ((addr_t) l2pt_hw_entry_va - page_offset + phys_start);

	/*Security Checks */
	uint32_t pa = MMU_L2_SMALL_ADDR(phys_pte);

	/*Check virtual address */
	if ((uint32_t) l2pt_hw_entry_va < page_offset
	    || (uint32_t) l2pt_linux_entry_va >
	    (uint32_t) (HAL_VIRT_START - sizeof(uint32_t))) {
		hyper_panic
		    ("Page table entry reside outside of allowed address space !\n",
		     1);
		printf("%s ErRor va:%x pgf:%x", __func__,
		       (uint32_t) l2pt_hw_entry_va, page_offset);
	}

	if (phys_pte != 0) {	/*If 0, then its a remove mapping */
		/*Check physical address */
		if (!(pa >= (phys_start) && pa < (phys_start + guest_size))) {
			printf("Address: %x\n", pa);
			hyper_panic
			    ("Guest trying does not own pte physical address",
			     1);
		}
	}

	/*Get index of physical L2PT */
	uint32_t entry_idx = ((addr_t) l2pt_hw_entry_va & 0xFFF) >> 2;
	/*Small page with CB on and RW */
	uint32_t attrs = phys_pte & 0xFFF;	/*Mask out address */
	uint32_t err;
	if (phys_pte != 0) {
#ifndef USE_REQUEST
/*
		if ((err =
		     (dmmu_l2_map_entry
		      (l2pt_hw_entry_pa & L2_BASE_MASK, entry_idx,
		       MMU_L1_PT_ADDR(phys_pte), attrs)))) {
			if (err == ERR_MMU_PT_NOT_UNMAPPED) {
				//So DMMU API does not allow changing attributes or remapping an entry if its not empty
				//this is a workaround 
				dmmu_l2_unmap_entry(l2pt_hw_entry_pa &
						    L2_BASE_MASK, entry_idx);
				dmmu_l2_map_entry(l2pt_hw_entry_pa &
						  L2_BASE_MASK, entry_idx,
						  MMU_L1_PT_ADDR(phys_pte),
						  attrs);
			} else {
				printf
				    ("\n\tCould not map l2 entry in set pte hypercall\n");
			}
		}
*/
		dmmu_l2_unmap_entry(l2pt_hw_entry_pa & L2_BASE_MASK, entry_idx);
		dmmu_l2_map_entry(l2pt_hw_entry_pa &
						  L2_BASE_MASK, entry_idx,
						  MMU_L1_PT_ADDR(phys_pte),
						  attrs);
#else
		push_request(request_dmmu_l2_unmap_entry(l2pt_hw_entry_pa &
						    L2_BASE_MASK, entry_idx));
		push_request(request_dmmu_l2_map_entry(l2pt_hw_entry_pa &
						  L2_BASE_MASK, entry_idx,
						  MMU_L1_PT_ADDR(phys_pte),
						  attrs));
#endif
	} else {
		/*Unmap */
#ifndef USE_REQUEST_SET_PTE_UNMAP
		if (dmmu_l2_unmap_entry
		    (l2pt_hw_entry_pa & L2_BASE_MASK, entry_idx))
			printf
			    ("\n\tCould not unmap l2 entry in set pte hypercall\n");
#else
		push_request(request_dmmu_l2_unmap_entry(l2pt_hw_entry_pa & L2_BASE_MASK, entry_idx));
#endif
	}
	/*Do we need to use the DMMU API to set Linux pages? */
	*l2pt_linux_entry_va = linux_pte;


	COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA,
		  (uint32_t) l2pt_hw_entry_va);
	clean_and_invalidate_cache();
}

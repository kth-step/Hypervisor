#include "hyper.h"
#include "dmmu.h"
#include "mmu.h"

extern virtual_machine *curr_vm;
/*
#define DEBUG_MMU_L1_CREATE
#define DEBUG_MMU_L1_SWITCH
#define DEBUG_MMU_SET_PTE
#define DEBUG_MMU_SET_PMD
#define DEBUG_MMU_L1_Free
#define DEBUG_MMU_SET_L1
*/

/*Get physical address from Linux virtual address*/
#define LINUX_PA(va) ((va) - (addr_t)(curr_vm->config->firmware->vstart) + (addr_t)(curr_vm->config->firmware->pstart))
/*Get virtual address from Linux physical address*/
#define LINUX_VA(pa) ((pa) - (addr_t)(curr_vm->config->firmware->pstart) + (addr_t)(curr_vm->config->firmware->vstart))

//uint32_t va_limit_x = 0xc2000000;
//uint32_t va_limit_x = 0xc07c714c;
//uint32_t va_limit_x = 0xc0469000;
#define pa_in_kernel_code(pa) (pa < curr_vm->config->firmware->pstart + (curr_vm->config->runtime_kernel_ex_va_top - curr_vm->config->firmware->vstart))
#define pa_below_1to1_l2s(pa) (pa < curr_vm->config->firmware->pstart + curr_vm->config->pa_initial_l2_offset)
#define number_of_1to1_l2s (curr_vm->config->firmware->psize >> 20)
#define size_of_1to1_l2s (number_of_1to1_l2s*2*1024)
#define pa_above_1to1_l2s(pa) (pa >= curr_vm->config->firmware->pstart + curr_vm->config->pa_initial_l2_offset + size_of_1to1_l2s)

uint32_t remove_ex_from_1to1_non_kernel_text(uint32_t table2_pa, uint32_t page_pa, uint32_t attrs) {
	uint32_t attrs1 = attrs;
	// We are not mapping kernel code memory
	if (!pa_in_kernel_code(page_pa)) {
		if (!pa_below_1to1_l2s(table2_pa)) {
			if (!pa_above_1to1_l2s(table2_pa)) {
				// It is one of the L2 pts built by linux_init for the 1to1 mapping
				attrs1 |= 0b1;
			}
		}
	}
	return attrs1;
}

uint32_t L2_MAP(uint32_t table2_pa, uint32_t i, uint32_t page_pa, uint32_t attrs) {
	return dmmu_l2_map_entry (table2_pa, i, page_pa, remove_ex_from_1to1_non_kernel_text(table2_pa, page_pa, attrs));
}

hypercall_request_t local_request_dmmu_l2_map_entry(uint32_t table2_pa, uint32_t i, uint32_t page_pa, uint32_t attrs) {
	return request_dmmu_l2_map_entry (table2_pa, i, page_pa, remove_ex_from_1to1_non_kernel_text(table2_pa, page_pa, attrs));
}


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

#define USE_REQUEST_LIST_SWITCH
void hypercall_dyn_switch_mm(addr_t table_base, uint32_t context_id)
{
#ifdef DEBUG_MMU_L1_SWITCH
	printf("*** Hypercall switch PGD\t table_base:%x context_id:%x\n",
	       table_base, context_id);
#endif

	/*Switch the TTB and set context ID */
#ifndef USE_REQUEST_LIST_SWITCH
	if (dmmu_switch_mm(table_base & L1_BASE_MASK)) {
		printf("\n\tCould not switch MM\n");
		dmmu_switch_mm(table_base & L1_BASE_MASK);
	}
#else
	push_request(request_dmmu_switch_mm(table_base & L1_BASE_MASK));
#endif

	COP_WRITE(COP_SYSTEM, COP_CONTEXT_ID_REGISTER, context_id);	//Set context ID
	isb();

}

#define USE_REQUEST_LIST_FREE_PGD
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

#ifndef USE_REQUEST_LIST_FREE_PGD
		if (dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i))
			printf("\n\tCould not unmap L2 entry in new PGD\n");
#else
		push_request(request_dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i));
#endif	
	}
	
#ifndef USE_REQUEST_LIST_FREE_PGD
	
	if (dmmu_unmap_L1_pt(LINUX_PA((addr_t) pgd_va)))
		printf("\n\tCould not unmap L1 PT in free pgd\n");
#else
	push_request(request_dmmu_unmap_L1_pt(LINUX_PA((addr_t) pgd_va)));
#endif

	for (i = l2_entry_idx; i < l2_entry_idx + 4; i++, page_pa += 0x1000) {
#ifndef USE_REQUEST_LIST_FREE_PGD
		if (L2_MAP(table2_pa & L2_BASE_MASK, i, page_pa, attrs))
			printf("\n\tCould not map L2 entry in new pgd\n");
#else
		push_request(local_request_dmmu_l2_map_entry(table2_pa & L2_BASE_MASK, i, page_pa, attrs));
#endif

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
#define USE_REQUEST_LIST_NEW_PGD
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
#ifndef USE_REQUEST_LIST_NEW_PGD
		
		if ((err =
		     dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i)))
			printf
			    ("\n\tCould not unmap L2 entry in new PGD err:%x\n",
			     err);
#else
		push_request(request_dmmu_l2_unmap_entry(table2_pa & L2_BASE_MASK, i));
#endif
		uint32_t ro_attrs =
		    0xE | (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT);
#ifndef USE_REQUEST_LIST_NEW_PGD
		if ((err =
		     L2_MAP(table2_pa & L2_BASE_MASK, i,
				       page_pa, ro_attrs)))
			printf
			    ("\n\tCould not map L2 entry in new pgd err:%x\n",
			     err);
#else
		push_request(local_request_dmmu_l2_map_entry(table2_pa & L2_BASE_MASK, i,
				       page_pa, ro_attrs));
#endif

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
#ifndef USE_REQUEST_LIST_NEW_PGD
	if ((err = dmmu_create_L1_pt(LINUX_PA((addr_t) pgd_va)))) {
		printf("\n\tCould not create L1 pt in new pgd in %x err:%d\n", LINUX_PA((addr_t) pgd_va), err);
		printf("\n\tMaster PGT:%d\n", LINUX_PA((addr_t) master_pgd_va));
		print_all_pointing_L1(LINUX_PA((addr_t) pgd_va), 0xfff00000);
		while (1) ;
	}
#else
	push_request(request_dmmu_create_L1_pt(LINUX_PA((addr_t) pgd_va)));
#endif

}

/*In ARM linux pmd refers to pgd, ARM L1 Page table
 *Linux maps 2 pmds at a time  */

#define USE_REQUEST_LIST_SET_PMD
#define DISABLE_CACHE

void hypercall_dyn_set_pmd(addr_t * pmd, uint32_t desc)
{
#ifdef DEBUG_MMU_SET_PMD
	printf("*** Hypercall set_pmd: va:0x%x pa:0x%x\n", pmd,
	       LINUX_PA((addr_t) pmd));
	printf("   descriptor mapping 0x%x\n", (((LINUX_PA((addr_t) pmd)) % (16 * 1024)) / 4) << 20);
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
		
		//COP_WRITE(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, LINUX_PA((addr_t) pgd_va));	// Set TTB0
		//isb();

#ifndef USE_REQUEST_LIST_SET_PMD
		dmmu_switch_mm(LINUX_PA((addr_t) pgd_va));
#else
		push_request(request_dmmu_switch_mm(LINUX_PA((addr_t) pgd_va)));
#endif

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

#ifndef DISABLE_CACHE
	COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, desc_va);
	COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, desc_va);
	dsb();
	isb();
#endif

	/*We need to make sure the new L2 PT is unreferenced */
#ifdef DEBUG_MMU_SET_PMD
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
#ifdef DEBUG_MMU_SET_PMD
		printf("\n\tRemapping as read only the page:%x\n",l2entry_desc);
#endif
		uint32_t ph_block = PA_TO_PH_BLOCK(l2entry_desc);
		dmmu_entry_t *bft_entry = get_bft_entry_by_block_idx(ph_block);
#ifdef DEBUG_MMU_SET_PMD
		printf("\n\tCounter :%d\n",bft_entry->refcnt);
#endif

#ifndef USE_REQUEST_LIST_SET_PMD
		if (dmmu_l2_unmap_entry
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx))
			printf("\n\tCould not unmap L2 entry in set PMD\n");
#else
		push_request(request_dmmu_l2_unmap_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx));
#endif
		uint32_t desc_pa = MMU_L2_SMALL_ADDR(desc);
		uint32_t ro_attrs =
		    0xE | (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT);

#ifdef DEBUG_MMU_SET_PMD
		printf("%x\n", desc_pa);
#endif

#ifndef USE_REQUEST_LIST_SET_PMD
		if (L2_MAP
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		     desc_pa, ro_attrs))
			printf("\n\tCould not map L2 entry in set PMD\n");
#else
		push_request(local_request_dmmu_l2_map_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		     desc_pa, ro_attrs));
#endif
	}
	uint32_t err;
	if (desc != 0) {

#ifndef USE_REQUEST_LIST_SET_PMD
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
#else
		push_request(request_dmmu_create_L2_pt(MMU_L2_SMALL_ADDR(desc)));
#endif
	}

	attrs = desc & 0x3FF;	/*Mask out addresss */

	/*Get virtual address of the translation for pmd */
	addr_t virt_transl_for_pmd =
	    (addr_t) ((pmd - pgd_va) << MMU_L1_SECTION_SHIFT);

	if (desc == 0) {
		uint32_t linux_va = pmd[0] - phys_start + page_offset;
#ifndef DISABLE_CACHE
		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();
#endif

#ifndef USE_REQUEST_LIST_SET_PMD
		if (dmmu_unmap_L1_pageTable_entry(virt_transl_for_pmd))
			printf("\n\tCould not unmap L1 entry in set PMD\n");
	
		if (dmmu_unmap_L1_pageTable_entry
		    (virt_transl_for_pmd + SECTION_SIZE))
			printf("\n\tCould not unmap L1 entry in set PMD\n");
#else
		push_request(request_dmmu_unmap_L1_pageTable_entry(virt_transl_for_pmd));
		push_request(request_dmmu_unmap_L1_pageTable_entry(virt_transl_for_pmd + SECTION_SIZE));
#endif

#ifndef DISABLE_CACHE
#ifdef AGGRESSIVE_FLUSHING_HANDLERS
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA,
			  (uint32_t) pmd);
#endif
#endif

		/*We need to make the l2 page RW again so that
		 *OS can reuse the address */
		
#ifndef USE_REQUEST_LIST_SET_PMD
		if (dmmu_l2_unmap_entry
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx))
			printf("\n\tCould not unmap L2 entry in set PMD\n");
		if (dmmu_unmap_L2_pt(MMU_L2_SMALL_ADDR((uint32_t) * pmd)))
			printf("\n\tCould not unmap L2 pt in set PMD\n");

		if (L2_MAP
		    ((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		     MMU_L2_SMALL_ADDR((uint32_t) * pmd), l2_rw_attrs))
			printf("\n\tCould not map L2 entry in set PMD\n");
#else
		push_request(request_dmmu_l2_unmap_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx));
		push_request(request_dmmu_unmap_L2_pt(MMU_L2_SMALL_ADDR((uint32_t) * pmd)));
		push_request(local_request_dmmu_l2_map_entry((uint32_t) l2pt_pa & L2_BASE_MASK, table2_idx + l2_idx,
		     MMU_L2_SMALL_ADDR((uint32_t) * pmd), l2_rw_attrs));
#endif

#ifndef DISABLE_CACHE
		//COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, (uint32_t)&l2pt_va[l2_idx]);
		CacheDataInvalidateBuff((uint32_t) & l2pt_va[l2_idx], 4);
		/*Flush entry */
		dsb();
#endif
	} else {
		uint32_t linux_va = desc - phys_start + page_offset;
#ifndef DISABLE_CACHE
		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();
#endif
		/*Flush entry */
		
#ifndef USE_REQUEST_LIST_SET_PMD
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
		
#else
		push_request(request_dmmu_l1_pt_map(virt_transl_for_pmd,
				   MMU_L2_SMALL_ADDR(desc), attrs));
		push_request(request_dmmu_l1_pt_map(virt_transl_for_pmd + SECTION_SIZE,
				   MMU_L2_SMALL_ADDR(desc) + 0x400, attrs));
		
#endif

#ifndef DISABLE_CACHE
		/*Flush entry */
		CacheDataInvalidateBuff((uint32_t) pmd, 4);
		CacheDataInvalidateBuff((uint32_t) & l2pt_va[l2_idx], 4);
		dsb();
#endif

	}
	if (switch_back) {
		//COP_WRITE(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, curr_pgd_pa);	// Set TTB0
		//isb();

#ifndef USE_REQUEST_LIST_SET_PMD
		dmmu_switch_mm(curr_pgd_pa);
#else
		push_request(request_dmmu_switch_mm(curr_pgd_pa));
#endif
	}

#ifndef DISABLE_CACHE
	/*Flush entry */
	clean_and_invalidate_cache();
#endif
}


#define number_of_1to1_l2s (curr_vm->config->firmware->psize >> 20)

//#define DEBUG_EMULATOR

void remove_writable_mapping_from(uint32_t pa) {
	uint32_t ph_block_pg = PA_TO_PH_BLOCK(pa);
	dmmu_entry_t *bft_entry_pg = get_bft_entry_by_block_idx(ph_block_pg);
#ifdef DEBUG_EMULATOR
	printf("****** EMULATOR, removing writable access to 0x%x\n", pa);
	printf("  counter %d,  ex counter %d\n", bft_entry_pg->refcnt, bft_entry_pg->x_refcnt);
#endif
	if (bft_entry_pg->refcnt == 0)
		return;

	uint32_t linux_va = LINUX_VA(pa);
	/*Get master page table */
	uint32_t page_offset = curr_vm->guest_info.page_offset;
	addr_t *master_pgd_va = (addr_t *) (curr_vm->config->pa_initial_l1_offset + page_offset);
	addr_t *l1_pt_entry_for_desc = (addr_t *) & master_pgd_va[(addr_t) linux_va >> MMU_L1_SECTION_SHIFT];
	uint32_t l1_desc_entry = *l1_pt_entry_for_desc;
	uint32_t l2_base_pa = MMU_L1_PT_ADDR(l1_desc_entry);
	uint32_t table2_idx = (l2_base_pa - (l2_base_pa & L2_BASE_MASK)) >> MMU_L1_PT_SHIFT;
	table2_idx *= 0x100;	/*256 pages per L2PT */
	uint32_t l2_idx = (((uint32_t) linux_va << 12) >> 24) + table2_idx;

	uint32_t *l2_page_entry = (addr_t *) (mmu_guest_pa_to_va(l2_base_pa & L2_BASE_MASK, (curr_vm->config)));
	uint32_t l2_desc = l2_page_entry[l2_idx];

	uint32_t l2_type = l2_desc & DESC_TYPE_MASK;

	if ((l2_type != 2) && (l2_type != 3))
		return;

	l2_small_t *pg_desc = (l2_small_t *) (&l2_desc);
	uint32_t l2_pointed_pa = START_PA_OF_SPT(pg_desc);
	uint32_t ap = GET_L2_AP(pg_desc);

	if ((0xfffff000 & l2_pointed_pa) != (0xfffff000 & pa))
		return;
	if (ap != 3)
		return;
	l2_desc = l2_desc & (~(0b1 << 4));

	push_request(request_dmmu_l2_unmap_entry(l2_base_pa, l2_idx));
	push_request(request_dmmu_l2_map_entry(l2_base_pa, l2_idx, l2_pointed_pa, l2_desc));

#if 0
	uint32_t l2block;
	for (l2block = 0; l2block < number_of_1to1_l2s / 2; l2block++) {
		uint32_t l2_base_pa = curr_vm->config->firmware->pstart + curr_vm->config->pa_initial_l2_offset + (l2block << 12);
		uint32_t l2_base_va = mmu_guest_pa_to_va(l2_base_pa, curr_vm->config);
		uint32_t l2_idx;
		for (l2_idx=0; l2_idx<512; l2_idx++) {
			uint32_t l2_desc_va = l2_base_va + l2_idx*4;
			uint32_t l2_desc = *((uint32_t *) l2_desc_va);
			uint32_t l2_type = l2_desc & DESC_TYPE_MASK;

			if ((l2_type != 2) && (l2_type != 3))
				continue;

			l2_small_t *pg_desc = (l2_small_t *) (&l2_desc);
			uint32_t l2_pointed_pa = START_PA_OF_SPT(pg_desc);
			uint32_t ap = GET_L2_AP(pg_desc);

			if ((0xfffff000 & l2_pointed_pa) != (0xfffff000 & pa))
				continue;
			if (ap != 3)
				continue;
			l2_desc = l2_desc & (~(0b1 << 4));

			push_request(request_dmmu_l2_unmap_entry(l2_base_pa, l2_idx));
			push_request(request_dmmu_l2_map_entry(l2_base_pa, l2_idx, l2_pointed_pa, l2_desc));
		}		
	}
#endif
#ifdef DEBUG_EMULATOR
	printf("  counter %d,  ex counter %d\n", bft_entry_pg->refcnt, bft_entry_pg->x_refcnt);
#endif
	return 0;	
}

#define USE_REQUEST_LIST_SET_PTE

/*va is the virtual address of the page table entry for linux pages
 *the physical pages are located 0x800 below */
void hypercall_dyn_set_pte(addr_t * l2pt_linux_entry_va, uint32_t linux_pte,
			   uint32_t phys_pte)
{
#ifdef DEBUG_MMU_SET_PTE
	printf("*** Hypercall set PTE va:%x phys_pte:%x linux_pte:%x  \n",
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
#ifndef USE_REQUEST_LIST_SET_PTE

		if ((err =
		     (L2_MAP
		      (l2pt_hw_entry_pa & L2_BASE_MASK, entry_idx,
		       MMU_L1_PT_ADDR(phys_pte), attrs)))) {
			if (err == ERR_MMU_PT_NOT_UNMAPPED) {
				//So DMMU API does not allow changing attributes or remapping an entry if its not empty
				//this is a workaround 
				dmmu_l2_unmap_entry(l2pt_hw_entry_pa &
						    L2_BASE_MASK, entry_idx);
				L2_MAP(l2pt_hw_entry_pa &
						  L2_BASE_MASK, entry_idx,
						  MMU_L1_PT_ADDR(phys_pte),
						  attrs);
			} else {
				printf
				    ("\n\tCould not map l2 entry in set pte hypercall\n");
			}
		}

#else
		// If it is executable, we must remove the 1-to-1 existing writable mappings
/*
		if ((attrs & 0b1) == 0b0) {
			remove_writable_mapping_from(MMU_L1_PT_ADDR(phys_pte));
		}
*/
		push_request(request_dmmu_l2_unmap_entry(l2pt_hw_entry_pa &
						    L2_BASE_MASK, entry_idx));
		push_request(local_request_dmmu_l2_map_entry(l2pt_hw_entry_pa &
						  L2_BASE_MASK, entry_idx,
						  MMU_L1_PT_ADDR(phys_pte),
						  attrs));
#endif
	} else {
		/*Unmap */
#ifndef USE_REQUEST_LIST_SET_PTE
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

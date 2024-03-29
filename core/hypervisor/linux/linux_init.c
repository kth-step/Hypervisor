#include "linux_signal.h"
#include "hyper_config.h"
#include "hyper.h"
#include "mmu.h"
#include "dmmu.h"

extern uint32_t *flpt_va;
extern uint32_t *slpt_va;

extern uint32_t l2_index_p;
extern virtual_machine *curr_vm;

/*We copy the signal codes to the vector table that Linux uses here */

const unsigned long sigreturn_codes[7] = {
	MOV_R7_NR_SIGRETURN, SWI_SYS_SIGRETURN, SWI_THUMB_SIGRETURN,
	MOV_R7_NR_RT_SIGRETURN, SWI_SYS_RT_SIGRETURN, SWI_THUMB_RT_SIGRETURN,
};

const unsigned long syscall_restart_code[2] = {
	SWI_SYS_RESTART,	/* swi  __NR_restart_syscall */
	0xe49df004,		/* ldr  pc, [sp], #4 */
};

void clear_linux_mappings()
{
	uint32_t PAGE_OFFSET = curr_vm->guest_info.page_offset;
	uint32_t VMALLOC_END = curr_vm->guest_info.vmalloc_end;
	uint32_t guest_size = curr_vm->guest_info.guest_size;
	uint32_t MODULES_VADDR =
	    (curr_vm->guest_info.page_offset - 16 * 1024 * 1024);
	uint32_t address;

	uint32_t offset = 0;
	uint32_t *pgd = flpt_va;

	/*
	 * Clear out all the mappings below the kernel image. Maps
	 */
	for (address = 0; address < MODULES_VADDR; address += 0x200000) {
		offset = address >> 21;	//Clear pages 2MB at time
		pgd[offset] = 0;
		pgd[offset + 1] = 0;
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, pgd);
	}

	for (; address < PAGE_OFFSET; address += 0x200000) {
		offset = address >> 21;
		pgd[offset] = 0;
		pgd[offset + 1] = 0;
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, pgd);
	}

	/*
	 * Clear out all the kernel space mappings, except for the first
	 * memory bank, up to the end of the vmalloc region.
	 */
	if (VMALLOC_END > HAL_VIRT_START)
		hyper_panic("Cannot clear out hypervisor page\n", 1);

	for (address = PAGE_OFFSET + guest_size; address < VMALLOC_END;
	     address += 0x200000) {
		offset = address >> 21;
		pgd[offset] = 0;
		pgd[offset + 1] = 0;
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, pgd);
	}
}

#define number_of_1to1_l2s (curr_vm->config->firmware->psize >> 20)
#define pa_in_kernel_code_and_init(pa) (pa < curr_vm->config->firmware->pstart + (curr_vm->config->init_kernel_ex_va_top - curr_vm->config->firmware->vstart))
#define pa_in_kernel_code(pa) (pa < curr_vm->config->firmware->pstart + (curr_vm->config->runtime_kernel_ex_va_top - curr_vm->config->firmware->vstart))

dmmu_clear_linux_mappings()
{
	addr_t guest_vstart = curr_vm->config->firmware->vstart;
	addr_t guest_psize = curr_vm->config->firmware->psize;
	uint32_t address;

	uint32_t VMALLOC_END = curr_vm->guest_info.vmalloc_end;

	/*
	 * Clear out all the mappings below the kernel image.
	 */

	for (address = 0; address < guest_vstart; address += SECTION_SIZE) {
		dmmu_unmap_L1_pageTable_entry(address);
	}

	/*
	 * Clear out all the kernel space mappings, except for the first
	 * memory bank, up to the end of the vmalloc region.
	 */
	if (VMALLOC_END > HAL_VIRT_START)
		hyper_panic("Cannot clear out hypervisor page\n", 1);

	for (address = guest_vstart + guest_psize; address < VMALLOC_END;
	     address += SECTION_SIZE) {
		dmmu_unmap_L1_pageTable_entry(address);
	}

	// Remove the executable flag from all L2 used for the 1-to-1 mapping
	// if it is pointing outside the Linux TEXT (+ init?)
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
			if (pa_in_kernel_code_and_init(l2_pointed_pa))
				continue;

			if (pg_desc->xn)
				continue;
			
			dmmu_l2_unmap_entry(l2_base_pa, l2_idx);
			dmmu_l2_map_entry(l2_base_pa, l2_idx, l2_pointed_pa, l2_desc | 0b1);
		}		
	}
}

uint32_t hypercall_linux_init_end() {
	// Remove the executable flag from all L2 used for the 1-to-1 mapping
	// if it is pointing outside the Linux TEXT (+ init?)
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
			if (pa_in_kernel_code(l2_pointed_pa))
				continue;

			if (pg_desc->xn)
				continue;
			
			dmmu_l2_unmap_entry(l2_base_pa, l2_idx);
			dmmu_l2_map_entry(l2_base_pa, l2_idx, l2_pointed_pa, l2_desc | 0b1);
		}		
	}
	return 0;
}

void init_linux_sigcode()
{
	memcpy((void *)KERN_SIGRETURN_CODE, sigreturn_codes,
	       sizeof(sigreturn_codes));
	memcpy((void *)KERN_RESTART_CODE, syscall_restart_code,
	       sizeof(syscall_restart_code));
}

void init_linux_page()
{
	uint32_t *p, i;

	/*Map the first pages for the Linux kernel OS specific
	 * These pages must cover the whole boot phase before the setup arch
	 * in Linux, in case of built in Ramdisk its alot bigger*/

	uint32_t linux_phys_ram = HAL_PHYS_START + 0x1000000;
	pt_create_coarse(flpt_va, 0xc0000000, linux_phys_ram, 0x100000,
			 MLT_USER_RAM);

	for (i = 1; i < 0x50; i++) {
		pt_create_section(flpt_va, 0xC0000000 + (i * (1 << 20)),
				  linux_phys_ram + i * (1 << 20), MLT_USER_RAM);
	}

	uint32_t phys = 0;
	p = (uint32_t *) ((uint32_t) slpt_va + ((l2_index_p - 1) * 0x400));	/*256 pages * 4 bytes for each lvl 2 page descriptor */
	/*Modify the master swapper page global directory to read only */

	/*Maps first eight pages 0-0x8000 */
	for (i = 0x4; i < 0x8; i++, phys += 0x1000) {
		p[i] &= PAGE_MASK;
		/*This maps Linux kernel pages to the hypervisor and sets it read only */
		p[i] |= (uint32_t) (GET_PHYS(flpt_va) + phys);

		p[i] &= ~(MMU_L2_SMALL_AP_MASK << MMU_L2_SMALL_AP_SHIFT);
		p[i] |= (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT);	//READ only
	}
}

uint32_t linux_l2_index_p = 0;

addr_t linux_pt_get_empty_l2()
{
	uint32_t pa_l2_pt =
	    curr_vm->config->firmware->pstart +
	    curr_vm->config->pa_initial_l2_offset;
	uint32_t va_l2_pt = mmu_guest_pa_to_va(pa_l2_pt, curr_vm->config);
	if ((linux_l2_index_p * 0x400) > SECTION_SIZE) {	// Set max size of L2 pages
		printf("No more space for more L2s\n");
		while (1) ;	//hang
		return 0;
	} else {
		addr_t index = linux_l2_index_p * 0x400;
		// memset((uint32_t *) ((uint32_t) va_l2_pt + index), 0, 0x400);
		uint32_t l2_base_add = (uint32_t) (pa_l2_pt + index);

		// assuming that va_l2_pt is 4kb aligned
		linux_l2_index_p++;
		if ((linux_l2_index_p & 0x2) == 0x2)
			linux_l2_index_p = (linux_l2_index_p & (~0x3)) + 4;

		return l2_base_add;
	}

}

#define MAP_LINUX_USING_L2

void linux_init_dmmu()
{
	uint32_t error;
	uint32_t sect_attrs, sect_attrs_ro, small_attrs, small_attrs_ro,
	    small_attrs_xn, page_attrs, table2_idx, i;
	addr_t table2_pa;
	addr_t guest_vstart = curr_vm->config->firmware->vstart;
	addr_t guest_pstart = curr_vm->config->firmware->pstart;
	addr_t guest_psize = curr_vm->config->firmware->psize;
	printf("Linux mem info %x %x %x \n", guest_vstart, guest_pstart,
	       guest_psize);
	/*Linux specific mapping */
	/*Section page with user RW in kernel domain with Cache and Buffer */
	sect_attrs = MMU_L1_TYPE_SECTION;
	sect_attrs |= MMU_AP_USER_RW << MMU_SECTION_AP_SHIFT;
	sect_attrs |= (HC_DOM_KERNEL << MMU_L1_DOMAIN_SHIFT);
	sect_attrs |= (MMU_FLAG_B | MMU_FLAG_C);

	sect_attrs_ro = MMU_L1_TYPE_SECTION;
	sect_attrs_ro |= MMU_AP_USER_RO << MMU_SECTION_AP_SHIFT;
	sect_attrs_ro |= (HC_DOM_KERNEL << MMU_L1_DOMAIN_SHIFT);
	sect_attrs_ro |= (MMU_FLAG_B | MMU_FLAG_C);

	/*L1PT attrs */
	page_attrs = MMU_L1_TYPE_PT;
	page_attrs |= (HC_DOM_KERNEL << MMU_L1_DOMAIN_SHIFT);

	/*Small page with CB on and RW */
	small_attrs = MMU_L2_TYPE_SMALL;
	small_attrs |= (MMU_FLAG_B | MMU_FLAG_C);
	small_attrs_ro |= small_attrs | (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT);
	small_attrs |= MMU_AP_USER_RW << MMU_L2_SMALL_AP_SHIFT;
	small_attrs_xn = small_attrs | 0b1;

	// clear the memory reserved for L2s
	addr_t reserved_l2_pts_pa =
	    curr_vm->config->pa_initial_l2_offset + guest_pstart;
	/*Set whole 1MB reserved address region in Linux as L2_pt */
	addr_t reserved_l2_pts_va =
	    mmu_guest_pa_to_va(reserved_l2_pts_pa, curr_vm->config);

	/*Memory setting the reserved L2 pages to 0
	 *We clean not the whole MB   */
	memset((addr_t *) reserved_l2_pts_va, 0, 0x100000);
	for (i = reserved_l2_pts_pa; i < reserved_l2_pts_pa + 0x10000;
	     i += PAGE_SIZE) {
		if ((error = dmmu_create_L2_pt(i)))
			printf("\n\tCould not map L2 PT: %d %x\n", error, i);
	}
	

	uint32_t offset;
	/*Can't map from offset = 0 because start addresses contains page tables */
	/*Maps PA-PA for boot */

#ifndef MAP_LINUX_USING_L2
	for (offset = SECTION_SIZE;
	     offset + SECTION_SIZE <= guest_psize; offset += SECTION_SIZE) {

		dmmu_map_L1_section(guest_pstart + offset,
				    guest_pstart + offset, sect_attrs);
	}
	/*Maps VA-PA for kernel */
	for (offset = SECTION_SIZE;
	     offset + SECTION_SIZE <= (guest_psize - SECTION_SIZE * 16);
	     offset += SECTION_SIZE) {
		dmmu_map_L1_section(guest_vstart + offset,
				    guest_pstart + offset, sect_attrs);
	}
	/*Map last 16MB as coarse */
	for (; offset + SECTION_SIZE <= guest_psize; offset += SECTION_SIZE) {
		table2_pa = linux_pt_get_empty_l2();	/*pointer to private L2PTs in guest */
		if ((error =
		     dmmu_l1_pt_map((addr_t) guest_vstart + offset, table2_pa,
				    page_attrs)))
			printf("\n\tCould not map L1 PT in set PMD: %d\n",
			       error);

		/*Get index of physical L2PT */
		table2_idx =
		    (table2_pa - (table2_pa & L2_BASE_MASK)) >> MMU_L1_PT_SHIFT;
		table2_idx *= 0x100;	/*256 pages per L2PT */
		uint32_t end = table2_idx + 0x100;
		uint32_t page_pa;
		for (i = table2_idx, page_pa = offset; i < end;
		     i++, page_pa += 0x1000) {
			if (dmmu_l2_map_entry
			    (table2_pa, i, page_pa + guest_pstart, small_attrs))
				printf
				    ("\n\tCould not map L2 entry in new pgd\n");
		}

	}
#else

	for (offset = SECTION_SIZE;
	     offset + SECTION_SIZE <= guest_psize; offset += SECTION_SIZE) {
		table2_pa = linux_pt_get_empty_l2(); /*pointer to private L2PTs in guest*/
		dmmu_create_L2_pt(table2_pa & L2_BASE_MASK);
		dmmu_l1_pt_map(guest_pstart + offset, table2_pa, page_attrs);
		dmmu_l1_pt_map(guest_vstart + offset, table2_pa, page_attrs);
    		table2_idx = (table2_pa - (table2_pa & L2_BASE_MASK)) >> MMU_L1_PT_SHIFT;
    		table2_idx *= 0x100; /*256 pages per L2PT*/
    		uint32_t end = table2_idx + 0x100;
    		uint32_t page_pa = guest_pstart + offset;

		for(i = table2_idx; i < end;i++, page_pa+=0x1000) {
			// Do not map executable the part of the memory above .text and .init of linux
			if (page_pa <= guest_pstart + (curr_vm->config->initial_kernel_ex_va_top - guest_vstart))
				dmmu_l2_map_entry(table2_pa, i, page_pa,  small_attrs);
			else {
				dmmu_l2_map_entry(table2_pa, i, page_pa,  small_attrs_xn);
				dmmu_entry_t *e = get_bft_entry_by_block_idx(page_pa >> 12);
				if (e->x_refcnt > 0)
					printf("\t %x ref_xnt > 0 %d\n", (page_pa >> 12), e->x_refcnt);
			}
		}
	}
#endif


#if 1				//Linux 3.10
	/*New ATAG (3.10) at end of image */
	dmmu_map_L1_section(0x9FE00000, 0x9FE00000, sect_attrs);
	//dmmu_map_L1_section(0xFA400000, 0x87000000, sect_attrs);
#endif
	/*special mapping for start address */

	table2_pa = linux_pt_get_empty_l2();	/*pointer to private L2PTs in guest */

	dmmu_create_L2_pt(table2_pa);

	dmmu_l1_pt_map(guest_pstart, table2_pa, page_attrs);
	dmmu_l1_pt_map(guest_vstart, table2_pa, page_attrs);

	uint32_t page_pa = guest_pstart;

	table2_idx = (table2_pa - (table2_pa & L2_BASE_MASK)) >> MMU_L1_PT_SHIFT;
	table2_idx *= 0x100; /*256 pages per L2PT*/
	uint32_t end = table2_idx + 0x100;

	for(i = table2_idx; i < end; i++, page_pa+=0x1000) {
    		if((i%256) >=4 && (i%256) <=7) {
			uint32_t ro_attrs =
			    0xE | (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT);
			dmmu_l2_map_entry(table2_pa, i, page_pa, ro_attrs);
		} else
			dmmu_l2_map_entry(table2_pa, i, page_pa, small_attrs);
	}
}

void linux_init()
{
	/*Initalise the page tables for Linux kernel */
	init_linux_page();
	/*Copy the signal codes into the vector table */
	init_linux_sigcode();
}

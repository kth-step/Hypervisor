#include <hw.h>
#include "hyper.h"
#include "guest_blob.h"
#include "mmu.h"
#include "hw_core_mem.h"
#include "dmmu.h"

//#define DEBUG_PG_CONTENT
#define DEBUG_L1_PG_TYPE
/*
 * Function prototypes
 */

void change_guest_mode();
void start();
void board_init();
/*Handlers */
void prefetch_abort_handler();
void data_abort_handler();
void swi_handler();
void irq_handler();
void undef_handler();

/*Init guest*/
void linux_init();
/****************************/
/*
 * Globals
 */

extern int __hyper_pt_start__;
extern uint32_t l2_index_p;

/*Pointers to start of  first and second level Page tables
 *Defined in linker script  */
//& is necessary to get the virtual address corresponding to __hyper_pt_start__
//since __hyper_pt_start__ is not a variable with an associated memory cell but
//identifies a location.
uint32_t *flpt_va = (uint32_t *) (&__hyper_pt_start__);
uint32_t *slpt_va = (uint32_t *) ((uint32_t) & __hyper_pt_start__ + 0x4000);	//16k Page offset

extern memory_layout_entry *memory_padr_layout;

//Static VM - May change to dynamic in future
virtual_machine vm_0;
virtual_machine *curr_vm;

extern void start_();
extern uint32_t _interrupt_vector_table;

#ifdef LINUX			//TODO remove ifdefs for something nicer
extern hc_config linux_config;
#endif
#ifdef MINIMAL
extern hc_config minimal_config;
#endif
#ifdef DTEST
extern hc_config minimal_config;
#endif

/*****************************/
/* DEBUG */
void dump_mmu(addr_t adr)
{
	uint32_t *t = (uint32_t *) adr;
	int i;

	for (i = 0; i < 4096; i++) {
		uint32_t x = t[i];
		switch (x & 3) {
		case 2:
			printf("SEC %x -> %x : %x DOM=%d C=%d B=%d AP=%d\n",
			       i << 20, x, (x & 0xFFF00000), (x >> 5) & 15,
			       (x >> 3) & 1, (x >> 2) & 1, (x >> 10) & 3);
			break;
		case 1:
			printf("COR %x -> %x : %x DOM=%d C=%d B=%d\n",
			       i << 20, x, (x & 0xFFFFF000),
			       (x >> 5) & 15, (x >> 3) & 1, (x >> 2) & 1);
			break;
		case 3:
			printf("FIN %x -> %x\n", i << 20, x);
			break;
		}
	}
	printf("\n");
}

/*****************************/

//Invalidates entire instruction TLB, data TLB by ASID = 0, and instruction
//cache; invalidates and cleans/flushes (read observations) all data cache
//levels.
void memory_commit()
{
	//Invalidates entire instruction TLB, and data TLB by ASID = 0.
	mem_mmu_tlb_invalidate_all(TRUE, TRUE);
	//Invalidates instruction cache, invalidates and cleans/flushes (read
	//observations) all data cache levels.
	mem_cache_invalidate(TRUE, TRUE, TRUE);	//instr, data, writeback
}

void memory_init()
{
	/*Setup heap pointer */
	core_mem_init();

	uint32_t j, va_offset;

	cpu_type type;
	cpu_model model;

	cpu_get_type(&type, &model);

	/* Start with simple access control
	 * Only seperation between hypervisor and user address space
	 *
	 * Here hypervisor already runs in virtual address since boot.S, now just setup guests
	 */

	// We clear the memory that contains the L2s that can be created in the 32KB of slpt_va
	memset(slpt_va, 0, 0x8000);

	memory_layout_entry *list =
	    (memory_layout_entry *) (&memory_padr_layout);

	for (;;) {
		if (!list)
			break;
		switch (list->type) {
		case MLT_IO_RW_REG:
		case MLT_IO_RO_REG:
		case MLT_IO_HYP_REG:
			/*All IO get coarse pages */
			pt_create_coarse(flpt_va,
					 IO_VA_ADDRESS(PAGE_TO_ADDR
						       (list->page_start)),
					 PAGE_TO_ADDR(list->page_start),
					 (list->page_count -
					  list->page_start) << PAGE_BITS,
					 list->type);
			break;
		case MLT_USER_RAM:
			/* do this later */
			break;
		case MLT_HYPER_RAM:
		case MLT_TRUSTED_RAM:
			/* own memory */
			j = (list->page_start) >> 8;	/*Get L1 Page index */
			for (; j < ((list->page_count) >> 8); j++) {
				pt_create_section(flpt_va,
						  (j << 20) - HAL_OFFSET,
						  j << 20, list->type);
			}
			break;
		case MLT_NONE:
			break;
		}
		if (list->flags & MLF_LAST)
			break;
		list++;
	}

	/*map 0xffff0000 to Vector table, interrupt have been relocated to this address */
	pt_map(0xFFFF0000, (uint32_t) GET_PHYS(&_interrupt_vector_table),
	       0x1000, MLT_USER_ROM);
	memory_commit();
	mem_cache_set_enable(TRUE);
	mem_mmu_set_domain(0x55555555);	//Start with access to all domains
}

void setup_handlers()
{
	/*Direct the exception to the hypervisor handlers */
	cpu_set_abort_handler((cpu_callback) prefetch_abort_handler,
			      (cpu_callback) data_abort_handler);
	printf("abort_handler is READY \n");
	cpu_set_swi_handler((cpu_callback) swi_handler);
	printf("swi_handler is READY \n");
	cpu_set_undef_handler((cpu_callback) undef_handler);
	printf("undef_handler is READY \n");

	/* Start the timer and direct interrupts to hypervisor irq handler */
//      gp_timer_tick_start((cpu_callback) irq_handler);

}

//Defines page tables enabling the hypervisor to access linux memory via the
//virtual address space (guest_psize = 0x0700_0000=112MB)
//[0xE800_0000, 0xEF10_0000) -> [0x8100_0000, 0x8810_0000)
//
//Copies level-1 descriptors from hypervisor page table to linux page table,
//and updates DMMU data structures (and some sanity checking).
//
//Configures page tables to map all physically allocated Linux memory
//(112MB) as an identity mapping for boot up to
//arch/arm/kernel/head.S:__create_page_tables and as the linear virtual
//mapping setup by arch/arm/kernel/head.S:__create_page_tables_
//-Boot: [0x8110_0000, 0x8800_0000) -> [0x8110_0000, 0x8800_0000) (Linux uses
// sections but only level-2 pages are used by DMMU.)
//-After MMU enabled: [0xC010_0000, 0xC700_0000) -> [0x8110_0000, 0x8800_0000)
// (Linux uses a mix of sections and small pages depending on how much memory is
// mapped.)
//
//Initializes the guest structure curr_vm.
//
//Stores address of guests registers of curr_vm in 
//core/hw/cpu/arm/arm_common/family_context_bottom.inc:context_stack:def_context1.
//Used by the exception handlers.
//
//Summary:
//1.	Defines page tables enabling the hypervisor to access linux memory via
//		the virtual address space
//		[0xE800_0000, 0xEF10_0000) -> [0x8100_0000, 0x8810_0000)
//2.	Initializes DMMU data structures.
//3.	Configures page tables to map all physically allocated Linux memory
//		(112MB) as an identity mapping for boot up to and as the linear virtual
//		mapping:
//		-Before MMU: [0x8110_0000, 0x8800_0000) -> [0x8110_0000, 0x8800_0000)
//		-After MMU: [0xC010_0000, 0xC700_0000) -> [0x8110_0000, 0x8800_0000)
//4.	Initializes the guest structure curr_vm.
//5.	Stores address of register context data structure of guest used by
//		exception handlers.
void guests_init()
{
	uint32_t i, guest = 0;
	//All assignments to vm_0 initializes the structure representing a guest
	//system.
	vm_0.id = 0;
	vm_0.next = &vm_0;	//Only one VM

	/*Start with VM_0 as the current VM */
	curr_vm = &vm_0;

	printf("HV pagetable before guests initialization:\n");	// DEBUG
	//    dump_mmu(flpt_va); // DEBUG

	/* show guest information */
	printf("We have %d guests in physical memory area start = %x and end = %x\n",
	       guests_db.count, guests_db.pstart, guests_db.pend);

	for (i = 0; i < guests_db.count; i++) {
		printf("Guest_%d: PA=%x+%x VA=%x FWSIZE=%x OFFSET=%x\n", i,
			// initial physical address of the guest
			guests_db.guests[i].pstart,
			// size in bytes of the guest
			guests_db.guests[i].psize,
			// initial virtual address of the 1-to-1 mapping
			guests_db.guests[i].vstart,
			// size in byte of the binary that has been copied
			guests_db.guests[i].fwsize,
			guests_db.guests[i].offset);
	}
/////////////////////////////
	printf("initial address where guest is stored at pstart: %x\n", guests_db.guests[0].pstart);
	printf("initial address where guest is copied at vstart: %x\n", guests_db.guests[0].vstart);
	printf("final address where guest is copied to at psize: %x\n", guests_db.guests[0].psize);
	printf("amount of bytes to copy at fwsize: %x\n", guests_db.guests[0].fwsize);
///////////////////////////////////////////

#ifdef LINUX
	//Get linux configuration structure: core/hypervisor/guest_config/linux_config.c:linux_config:
	//guest_entry_offset = 0x10000:					Consistent with guest_data.S offset. Used by arm_guest_blob to move the guest binary.
	//reserved_va_for_pt_access_start = 0xE8000000:	128MB of virtual address space mapping guest memory starting at 0xE800_0000.
	//pa_initial_l1_offset = 0x00004000:			Offset from virtual start address of to master page table in Linux
	//pa_initial_l2_offset = 0x0:					Updated below by the physical size of Linux = 112MB to point to the memory region where the L2 page tables are stored.
	//initial_kernel_ex_va_top = 0xc2000000:		
	//runtime_kernel_ex_va_top = 0xc0469000:		
	//init_kernel_ex_va_top = 0xc07c8000:			

////////////////////////////////////////////////////////////////////////////////
//Avoids the assignment of PAGE_OFFSET in///////////////////////////////////////
//core/hypervisor/hypercalls.c:hypercall_guest_init.////////////////////////////
curr_vm->guest_info.page_offset = 0xC0000000;///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


	vm_0.config = &linux_config;
	//Get guest zero (core/hw/common/guest_blob.c:guest_binary containing pstart, vstart, psize, fwsize).
	vm_0.config->firmware = get_guest(guest++);
	//Updates pa_initial_l2_offset by the size of the guest gives 0x0700_0000.
	curr_vm->config->pa_initial_l2_offset += curr_vm->config->firmware->psize;
	//  linux_init();

#else
	vm_0.config = &minimal_config;
	vm_0.config->firmware = get_guest(1 + guest++);
#endif
	addr_t guest_vstart = curr_vm->config->firmware->vstart;
	addr_t guest_pstart = curr_vm->config->firmware->pstart;
	addr_t guest_psize = curr_vm->config->firmware->psize;
	/* KTH CHANGES */
	/* - The hypervisor must be always able to read/write the guest PTs */
	/*   for now, the guest PTS can be written everywhere into the guest memory */
	/*   in the future we probably need more master page tables, one for each guest that uses the mmu */
	/*   so that the virtual reserved addresses can be different */

	/*   we constraint that for the minimal guests, the page tables */
	/*   are between physical addresses 0x01000000 and 0x014FFFFF (that are the five megabytes of the guest) */
	/*   of memory reserved to the guest */
	/*   these address are mapped by the virtual address  0x00000000 and 0x004FFFFF */
	/*   TODO: this must be accessible only to the hypervisor */
	// this must be a loop
	uint32_t va_offset;
	//Defines page tables enabling the hypervisor to access linux memory via the
	//virtual address space (guest_psize = 0x0700_0000=112MB) and the
	//second-level page tables that are allocated in one MB after Linux:
	//[0xE800_0000, 0xE800_0000 + 112MB + 1MB) -> [0x8100_0000, 0x8100_0000 + 112MB + 1MB) =
	//[0xE800_0000, 0xE800_0000 + 0x0700_0000 + 0x0010_0000) -> [0x8100_0000, 0x8100_0000 + 0x0700_0000 + 0x0010_0000) =
	//[0xE800_0000, 0xEF00_0000 + 0x0010_0000) -> [0x8100_0000, 0x8800_0000 + 0x0010_0000) =
	//[0xE800_0000, 0xEF10_0000) -> [0x8100_0000, 0x8810_0000)
	for (va_offset = 0; va_offset <= guest_psize; va_offset += SECTION_SIZE) { /*+ 1MB at end for L1PT */
		uint32_t offset, pmd;
		//va is in [0xE800_0000, 0xE800_0000 + 0x07100000) = [0xE800_0000, 0xEF10_0000)
		uint32_t va = vm_0.config->reserved_va_for_pt_access_start + va_offset;
		//guest_pstart =
		//curr_vm->config->firmware->pstart =
		//initialized by arm_guest_blob.S.
		uint32_t pa = guest_pstart + va_offset;
//printf("Hypervisor guests_init: va = %x, pa = %x\n", va, pa);
		//flpt_va =
		//Virtual address of the first-level page table =
		//initialized to &__hyper_pt_start__ in this file.
		//Maps the physical addresses of the guest into the virtual address
		//range [0xE800_0000, 0xEF10_0000) -> [0x8100_0000, 0x8810_0000) only accessible to the hypervisor.
		pt_create_section(flpt_va, va, pa, MLT_HYPER_RAM);

		/* Invalidate the new created entries */
		//Calculates the MB index of the current virtual address, and multiplied
		//by 4, since each page table entry is 4 bytes, to obtain the page table
		//byte index.
		offset = ((va >> MMU_L1_SECTION_SHIFT) * 4);
		//Virtual address of the current page table entry.
		pmd = (uint32_t *) ((uint32_t) flpt_va + offset);
		//"mcr p15, 0, <r>, c7, c10, 1", dccmvac. Updates page table entry in
		//memory by cleaning cache.
		/* "A cache clean operation ensures that updates made by an observer
		 *  that controls the cache are made visible to other observers that can
		 *  access memory at the point to which the operation is performed. Once
		 *  the Clean has completed, the new memory values are guaranteed to be
		 *  visible to the point to which the operation is performed, for
		 *  example to the point of unification."
		 *
		 * Point of unification is point of coherence:
		 * "For a particular MVA, the PoC is the point at which all agents that
		 *  can access memory are guaranteed to see the same copy of a memory
		 *  location."
		 */
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, pmd);

		// printf("%x -> %x\n", va, pa); // DEBUG
	}

//while (1);
	//Invalidates instruction and data TLB, ASID = 0.
	//Invalidates instruction cache and invalidates and cleans/flushes (read
	//observations) all data cache levels.
	memory_commit();

	printf("HV pagetable after guests initialization:\n");	// DEBUG
	//    dump_mmu(flpt_va); // DEBUG

	// We pin the L2s that can be created in the 32KB are of slpt_va
	dmmu_entry_t *bft = (dmmu_entry_t *) DMMU_BFT_BASE_VA;
	for (i = 0; i * 4096 < 0x8000; i++) {
		//Second-level page table is located 0x4000 = 16kB above first-level
		//page table. A first-level page table addresses 4GB, with each entry, 4
		//bytes, addressing 1MB, leading to 4096 entries = 16kB.
		bft[PA_TO_PH_BLOCK ((uint32_t) GET_PHYS(slpt_va) + i * 4096)].type = PAGE_INFO_TYPE_L2PT;
		bft[PA_TO_PH_BLOCK ((uint32_t) GET_PHYS(slpt_va) + i * 4096)].refcnt = 1;
	}

	// END initialization of the MATER PAGE TABLE
	// START initialization of the FIRST guest PT

	// now the master page table is ready
	// it contains
	// - the virtual mapping to the hypervisor code and data
	// - a fixed virtual mapping to the guest PT
	// - some reserved mapping that for now we ignore, e.g. IOREGS
	// - a 1-1 mapping to the guest memory (as defined in the board_mem.c) writable and readable by the user
	// - THIS SETUP MUST BE FIXED, SINCE THE GUEST IS NOT ALLOWED TO WRITE IN TO ITS WHOLE MEMORY

	/* - Create a copy of the master page table for the guest in the physical address: pa_initial_l1 */
	uint32_t *guest_pt_va;
	addr_t guest_pt_pa;
	//Guest start physical address = 0x8100_0000, and page tables are located at
	//0x8100_4000, and text starts at 0x8100_8000. Hence,
	//pa_initial_l1_offset = 0x4000.
	guest_pt_pa = guest_pstart + vm_0.config->pa_initial_l1_offset;
	//mmu_guest_pa_to_va(guest_pt_pa, vm_0.config) =
	//guest_pt_pa - vm_0.config->firmware->pstart + vm_0.config->reserved_va_for_pt_access_start =
	//guest_pstart + vm_0.config->pa_initial_l1_offset - vm_0.config->firmware->pstart + vm_0.config->reserved_va_for_pt_access_start =
	//curr_vm->config->firmware->pstart + vm_0.config->pa_initial_l1_offset - vm_0.config->firmware->pstart + vm_0.config->reserved_va_for_pt_access_start =
	//vm_0.config->config->firmware->pstart + vm_0.config->pa_initial_l1_offset - vm_0.config->firmware->pstart + vm_0.config->reserved_va_for_pt_access_start =
	//vm_0.config->pa_initial_l1_offset + vm_0.config->reserved_va_for_pt_access_start =
	//0x0000_4000 + 0xE8000000 =
	//virtual address mapped to physical address 0x8100_4000, where Linux stores
	//its page tables (in our case level-1 page tables, with level-2 page tables
	//after Linux physical memory after 112MB).
	guest_pt_va = mmu_guest_pa_to_va(guest_pt_pa, vm_0.config);

	printf("COPY %x %x %x\n", guest_pt_pa, guest_pt_va, flpt_va);
	//Copies the level-1 page table to where Linux stores its page tables =
	//16kB = 4096 4-byte entries covering 4096 * 1MB = 4GB = the whole address
	//space: Source = flpt_va. Destination = guest_pt_va (see library/uc/include/uclib_stdlib.h).
	memcpy(guest_pt_va, flpt_va, 1024 * 16);

	printf("vm_0 pagetable:\n");	// DEBUG    
	//    dump_mmu(guest_pt_va); // DEBUG

	/* activate the guest page table */
	memory_commit();	//Invalides TLB and cache.
	//Sets TTBR0 to the page table location in Linux memory.
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, guest_pt_pa);	// Set TTB0.
	//"Instruction Synchronization Barrier flushes the pipeline in the
	// processor, so that all instructions following the ISB are fetched from
	// cache or memory, after the instruction has been completed."
	isb();
	memory_commit();	//Invalides TLB and cache.

	// Calling the create_L1_pt API to check the correctness of the L1 content and to change the page table type to 1
	//Copies level-1 descriptors from hypervisor page table to linux page table,
	//and updates DMMU data structures (and some sanity checking).
	dmmu_create_L1_pt(guest_pt_pa);

#ifdef DEBUG_L1_PG_TYPE
	uint32_t index;
	for (index = 0; index < 4; index++)
		printf("Initial L1 page table's page type:%x \n", bft[PA_TO_PH_BLOCK(guest_pt_pa) + index].type);
#endif
	// Initialize the datastructures with the type for the initial L1
	// create the attribute that allow the guest to read/write/execute
	uint32_t attrs;

#ifdef LINUX
	/*Maps PA-PA for boot and VA-PA for kernel
	 * First MB of pa mapped as L2PT with page 1-7 RO (private L2PT and master page)*/

	//Configures page tables to map all physically allocated Linux memory
	//(112MB) as an identity mapping for boot up to
	//arch/arm/kernel/head.S:__create_page_tables and as the linear virtual
	//mapping setup by arch/arm/kernel/head.S:__create_page_tables.
	linux_init_dmmu();
#else
	attrs = 0x12;		// 0b1--10
	attrs |= MMU_AP_USER_RW << MMU_SECTION_AP_SHIFT;
	attrs = (attrs & (~0x10)) | 0xC | (HC_DOM_KERNEL << MMU_L1_DOMAIN_SHIFT);
	// As default the guest has a 1-to-1 mapping to all its memory
	uint32_t offset;
	for (offset = 0; offset + SECTION_SIZE <= guest_psize; offset += SECTION_SIZE) {
		dmmu_map_L1_section(guest_vstart + offset, guest_pstart + offset, attrs);
	}
#endif
	printf("vm_0 pagetable after initialization:\n");	// DEBUG
	//dump_mmu(guest_pt_va); // DEBUG

	//Invalidates entire instruction TLB, and data TLB by ASID = 0.
	mem_mmu_tlb_invalidate_all(TRUE, TRUE);	//Clears TLB
	//Invalidates instruction cache, invalidates and cleans/flushes (read
	//observations) all data cache levels.
	mem_cache_invalidate(TRUE, TRUE, TRUE);	//instr, data, writeback
	//Enables/disables instruction and data caches.
	mem_cache_set_enable(TRUE);

#ifdef DEBUG_PG_CONTENT
	for (index = 0; index < 4096; index++) {
		if (*(guest_pt_va + index) != 0x0)
			printf("add %x %x \n", index, *(guest_pt_va + index));	//(flpt_va + index)
	}
#endif
	/* END KTH CHANGES */

#ifdef TRUSTED
	get_guest(guest++);
	curr_vm->mode_states[HC_GM_TRUSTED].ctx.sp = curr_vm->config->rpc_handlers->sp;
	curr_vm->mode_states[HC_GM_TRUSTED].ctx.psr = ARM_INTERRUPT_MASK | ARM_MODE_USER;
#endif

	guest = 0;

	//Initializes the guest structure curr_vm = vm_0.
	// Init the context with the physical addresses
	do {
		/*Init default values */
		for (i = 0; i < HC_NGUESTMODES; i++) {
			//curr_vm->config->guest_modes[i] =
			//linux_config->guest_modes[i] =
			//[HC_DOMAC_TRUSTED, HC_DOMAC_KERNEL, HC_DOMAC_TASK]
			//Guest execution mode, including the domain access control bits
			//that are set when entering the mode.
			curr_vm->mode_states[i].mode_config = (curr_vm->config->guest_modes[i]);
			//curr_vm->mode_states[i].rpc_for = MODE_NONE;
			//curr_vm->mode_states[i].rpc_to  = MODE_NONE;
		}
		curr_vm->current_guest_mode = MODE_NONE;
		curr_vm->interrupted_mode = MODE_NONE;
		curr_vm->current_mode_state = 0;
		//curr_vm->mode_states[HC_GM_INTERRUPT].ctx.psr= ARM_MODE_USER;
		//Next guest, which is this guest; see assignments above:
		//vm_0.next = &vm_0;
		//curr_vm = &vm_0
		curr_vm = curr_vm->next;

		// let guest know where it is located
		curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[3] = curr_vm->config->firmware->pstart;
		curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[4] = curr_vm->config->firmware->vstart;
		// initial page table location
	} while (curr_vm != &vm_0);

	//Invalidates instruction and data TLB, ASID = 0.
	//Invalidates instruction cache and invalidates and cleans/flushes (read
	//observations) all data cache levels.
	memory_commit();

	//Stores address of guests registers of curr_vm in 
	//core/hw/cpu/arm/arm_common/family_context_bottom.inc:context_stack:def_context1.
	//Used by the exception handlers.
	cpu_context_initial_set(&curr_vm->mode_states[HC_GM_KERNEL].ctx);
}

void start_guest()
{

	/*Change guest mode to KERNEL before going into guest */
	change_guest_mode(HC_GM_KERNEL);

	/*Starting Guest */
	start();

}

void start_()
{
	cpu_init();

	/*Setting up pagetable rules */
	memory_init();
	/* Initialize hardware */
	board_init();
	soc_init();
	/* Setting up exception handlers and starting timer */
	setup_handlers();
	/* dmmu init */
	dmmu_init();
	/* Initialize hypervisor guest modes and data structures
	 * according to config file in guest*/
	guests_init();
	/*Test crypto */

	printf("Hypervisor initialized2.0\n Entering Guest\n");
	//Start execution of guest.
	start_guest();
}

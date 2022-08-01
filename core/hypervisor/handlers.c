#include <hw.h>
#include "hypercalls.h"
#if defined(LINUX) && defined(CPSW)
#include <soc_cpsw.h>
#endif
#include "dmmu.h"

extern virtual_machine *curr_vm;

#define USE_DMMU

// Disabling aggressive flushing
#define AGGRESSIVE_FLUSHING_HANDLERS

//Hypercall for writing buffer descriptors to NIC.
#define HYPERCALL_CPSW_WRITE_BD 1045

void clean_and_invalidate_cache()
{
#ifdef AGGRESSIVE_FLUSHING_HANDLERS
	isb();
	mem_mmu_tlb_invalidate_all(TRUE, TRUE);
	CacheDataCleanInvalidateAll();
	dsb();
#endif
}

void swi_handler(uint32_t param0, uint32_t param1, uint32_t param2,
		 uint32_t hypercall_number)
{
//  if ((hypercall_number == HYPERCALL_RESTORE_REGS) || (hypercall_number == HYPERCALL_RESTORE_LINUX_REGS) && (hypercall_number != HYPERCALL_SET_TLS_ID) ){
//      printf("SWI ENTER hypercall_number = %d %x %x %x\n", hypercall_number, param0, param1, param2);
//  }
	/*TODO Added check that controls if it comes from user space, makes it pretty inefficient, remake later */
	/*Testing RPC from user space, remove later */
	if (curr_vm->current_guest_mode == HC_GM_TASK) {
		if (hypercall_number == 1020) {
			//ALLOWED RPC OPERATION
			hypercall_rpc(param0, (uint32_t *) param1);
			return;
		} else if (hypercall_number == 1021) {
			hypercall_end_rpc();
			return;
		}
	}
	if (curr_vm->current_guest_mode == HC_GM_TASK) {	//Linux application running.
/////////////////////
		if ((curr_vm->current_mode_state->ctx.psr & IRQ_MASK) != 0)
			printf
			    ("ERROR (interrupt delivered when disabled) IRQ_MASK = %x\n",
			     curr_vm->current_mode_state->ctx.psr);
/////////////////////

		//  debug("\tUser process made system call:\t\t\t %x\n",  curr_vm->mode_states[HC_GM_TASK].ctx.reg[7] );
		change_guest_mode(HC_GM_KERNEL);				//Linux now executing in kernel mode.
		/*The current way of saving context by the hypervisor is very inefficient,
		 * can be improved alot with some hacking and shortcuts (for the FUTURE)*/
		curr_vm->current_mode_state->ctx.sp -= (72 + 8);	//FRAME_SIZE (18 registers to be saved) + 2 swi args
		uint32_t *context, *sp, i;

		context = &curr_vm->mode_states[HC_GM_TASK].ctx.reg[0];
		sp = (uint32_t *) curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;

		*sp++ = curr_vm->mode_states[HC_GM_TASK].ctx.reg[4];
		*sp++ = curr_vm->mode_states[HC_GM_TASK].ctx.reg[5];

		i = 17;		//r0-lr

		while (i > 0) {
			*sp++ = *context++;
			i--;
		}

		*sp = curr_vm->mode_states[HC_GM_TASK].ctx.reg[0];	//OLD_R0

		//update CR for alignment fault
		//Enable IRQ
		curr_vm->current_mode_state->ctx.psr &= ~(IRQ_MASK);

		curr_vm->current_mode_state->ctx.lr = curr_vm->exception_vector[V_RET_FAST_SYSCALL];
		//curr_vm->handlers.syscall.ret_fast_syscall;
		//copy task context to kernel context. syscall supports 6 arguments

		/*system call nr in r7 */
		curr_vm->current_mode_state->ctx.reg[7] = curr_vm->mode_states[HC_GM_TASK].ctx.reg[7];

		if (curr_vm->mode_states[HC_GM_TASK].ctx.reg[7] < curr_vm->guest_info.nr_syscalls) {
			/*Regular system call, restore params */
			for (i = 0; i <= 5; i++)
				curr_vm->current_mode_state->ctx.reg[i] = curr_vm->mode_states[HC_GM_TASK].ctx.reg[i];
			/*Set PC to systemcall function */
			curr_vm->current_mode_state->ctx.pc = *((uint32_t *) (curr_vm->exception_vector[V_SWI] + (curr_vm->current_mode_state->ctx.reg[7] << 2)));
		} else {
			//TODO Have not added check that its a valid private arm syscall, done anyways inside arm_syscall
			//if(curr_vm->current_mode_state->ctx.reg[7] >= 0xF0000){ //NR_SYSCALL_BASE
			/*Arm private system call */
			curr_vm->current_mode_state->ctx.reg[0] = curr_vm->mode_states[HC_GM_TASK].ctx.reg[7];
			curr_vm->current_mode_state->ctx.reg[1] = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp + 8;	//Adjust sp with S_OFF, contains regs
			curr_vm->current_mode_state->ctx.pc = curr_vm->exception_vector[V_ARM_SYSCALL];
		}
	} else if (curr_vm->current_guest_mode != HC_GM_TASK) {
		//    printf("\tHypercallnumber: %d (%x, %x) called\n", hypercall_number, param0, param);
		uint32_t res;
		switch (hypercall_number) {
			/* TEMP: DMMU TEST */
		case 666:

			clean_and_invalidate_cache();

			res = dmmu_handler(param0, param1, param2);
			curr_vm->current_mode_state->ctx.reg[0] = res;

			clean_and_invalidate_cache();

			return;
		case HYPERCALL_DBG:
			printf("To here %x\n", param0);
			return;
		case HYPERCALL_GUEST_INIT:
			hypercall_guest_init(param0);
			return;
		case HYPERCALL_INTERRUPT_SET:
			hypercall_interrupt_set(param0, param1);
			return;
		case HYPERCALL_END_INTERRUPT:
			hypercall_end_interrupt(param0);
			return;
		case HYPERCALL_INTERRUPT_CTRL:
			hypercall_interrupt_ctrl(param0, param1);
			return;
		case HYPERCALL_CACHE_OP:
			hypercall_cache_op(param0, param1, param2);
			return;
		case HYPERCALL_SET_TLS_ID:
			hypercall_set_tls(param0);
			return;
		case HYPERCALL_SET_CTX_ID:
			COP_WRITE(COP_SYSTEM, COP_CONTEXT_ID_REGISTER, param0);
			isb();
			return;
			/*Context */
		case HYPERCALL_RESTORE_LINUX_REGS:
			hypercall_restore_linux_regs(param0, param1);
			return;
		case HYPERCALL_RESTORE_REGS:
			hypercall_restore_regs((uint32_t *) param0);
			return;

			/*Page table operations */
		case HYPERCALL_SWITCH_MM:
			clean_and_invalidate_cache();
			hypercall_dyn_switch_mm(param0, param1);
			//clean_and_invalidate_cache();
			return;

		case HYPERCALL_NEW_PGD:
			printf("HYPERCALL_NEW_PGD %x\n", param0);
			clean_and_invalidate_cache();
			hypercall_dyn_new_pgd((uint32_t *) param0);
//if (get_bft_entry_by_block_idx(PA_TO_PH_BLOCK(0x82DE0000))->type == 2) printf("HYPERCALL_NEW_PGD = %d\n", get_bft_entry_by_block_idx(PA_TO_PH_BLOCK(0x82DE0000))->type);
			//clean_and_invalidate_cache();
			return;
		case HYPERCALL_FREE_PGD:
			clean_and_invalidate_cache();
			hypercall_dyn_free_pgd((uint32_t *) param0);
			//clean_and_invalidate_cache();
			return;
		case HYPERCALL_CREATE_SECTION:
			{
//				printf("HYPERVISOR hypercall_number = %d %x %x %x NOT IMPLEMENTED!\n", HYPERCALL_CREATE_SECTION, param0, param1, param2);
//while (1);
				//printf("SWI ENTER hypercall_number = %d %x %x %x\n", hypercall_number, param0, param1, param2);
				return;
			}

		case HYPERCALL_SET_PMD:
//			printf("HYPERCALL_SET_PMD1 = %x %x!\n", param0, param1);
//print_specific_L2();
//virtual_address_mapped(0xC7000000, curr_vm);

			clean_and_invalidate_cache();
			hypercall_dyn_set_pmd(param0, param1);
//if (get_bft_entry_by_block_idx(PA_TO_PH_BLOCK(0x82DE0000))->type == 2) printf("HYPERCALL_SET_PMD = %d\n", get_bft_entry_by_block_idx(PA_TO_PH_BLOCK(0x82DE0000))->type);
//print_specific_L2();
//virtual_address_mapped(0xC7000000, curr_vm);
//			printf("HYPERCALL_SET_PMD2 = %x %x!\n", param0, param1);
			//clean_and_invalidate_cache();
			return;
		case HYPERCALL_SET_PTE: {
			//param0 = va of l2 pte, param1 = Linux pte, param2 = Hardware pte.
			addr_t * l2_pte_va = (addr_t *) param0;
			uint32_t lpte = param1;
			uint32_t hpte = param2;

//			printf("HYPERCALL_SET_PTE1 = VA of L2 PTE = 0x%x, LPTE = 0x%x, HWPTE = 0x%x!\n", l2_pte_va, lpte, hpte);
//if (l2_pte_va == 0xC1DE17FC) print_specific_L2();

//			print_ap(hpte);
			hpte = adjust_aps(hpte);
//			print_ap(hpte);
			clean_and_invalidate_cache();
//uint32_t type = get_bft_entry_by_block_idx(PA_TO_PH_BLOCK(0x82DE0000))->type;
			hypercall_dyn_set_pte(l2_pte_va, lpte, hpte);
//if (l2_pte_va == 0xC1DE17FC) print_specific_L2();
//if (type != 2 && get_bft_entry_by_block_idx(PA_TO_PH_BLOCK(0x82DE0000))->type == 2) {
//	printf("HYPERCALL_SET_PTE = %d\n", get_bft_entry_by_block_idx(PA_TO_PH_BLOCK(0x82DE0000))->type);
//	printf("HYPERCALL_SET_PTE1 = VA of L2 PTE = 0x%x, LPTE = 0x%x, HWPTE = 0x%x!\n", l2_pte_va, lpte, hpte);
//	printf("l1desc = 0x%x\n", *((uint32_t *) (0xC0004000 + 0xE9D*4)));
//	while (1);
//}
//			printf("HYPERCALL_SET_PTE2 = VA of L2 PTE = 0x%x, LPTE = 0x%x, HWPTE = 0x%x!\n", l2_pte_va, lpte, hpte);
			//clean_and_invalidate_cache();
			return;
		}

    /****************************/
		 /*RPC*/ case HYPERCALL_RPC:
			hypercall_rpc(param0, (uint32_t *) param1);
			return;
		case HYPERCALL_END_RPC:
			hypercall_end_rpc();
			return;
			//  /*VFP Test**********************/
			//case HYPERCALL_VFP:
			//  hypercall_vfp_op(param0, param1, param2);
			//  return;
		case HYPERCALL_LINUX_INIT_END:	//NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			hypercall_linux_init_end();
			return;

		case HYPERCALL_UPDATE_MONITOR:
			{
				uint32_t number_of_signatures = param0;
				uint32_t *kernel_image = (uint32_t *) param1;
				uint32_t *kernel_image_signature =
				    (uint32_t *) param2;
				printf
				    ("HYPERVISOR: HYPERCALL_UPDATE_MONITOR\n");
				printf
				    ("HYPERVISOR: number_of_signatures = %d\n",
				     number_of_signatures);
				printf("HYPERVISOR: kernel_image = %x\n",
				       kernel_image);
				printf
				    ("HYPERVISOR: kernel_image_signature = %x\n",
				     kernel_image_signature);

				//Successful update of the monitor.
				curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[0] =
				    0;

				return;
			}
		case HYPERCALL_QUERY_BFT:
			res = dmmu_query_bft(param0);
			curr_vm->current_mode_state->ctx.reg[0] = res;
			return;			

		case HYPERCALL_CPSW_WRITE_BD: {
			printf("HYPERVISOR PRINTF: p0 = %x, p1 = %x, p2 = %x\n", param0, param1, param2);

//			unsigned int *__atags_pointer = (unsigned int *)param0;
//			printf("HYPERVISOR PRINTF: *__atags_pointer = %x\n", *__atags_pointer);
//			unsigned int *__atags_pointer_va = (unsigned int *)(param0 + 0x3F000000);
//			printf("HYPERVISOR PRINTF: *__atags_pointer_va = %x\n", *__atags_pointer_va);



/*			unsigned int *atags_vaddr = (unsigned int *)param1;
			unsigned int atags_word0 = *atags_vaddr;
			unsigned int atags_word1 = *(atags_vaddr + 1);


			printf("HYPERVISOR PRINTF: atags_word0 = %x, atags_word1 = %x\n", atags_word0, atags_word1);
*/
/*while (1);

			uint32_t desc = (param0 >> 16) + 0xFA402000;
			uint32_t len = param0 & 0x0000FFFF;
			uint32_t buffer = param1;
			uint32_t token = param2;

			BOOL ret = soc_check_bd_write(desc, len, buffer, token);

			if (!ret)
				printf("HYPERVISOR_CPSW_WRITE_BD ERROR WRITE BD: desc = %x, len = %x, buffer = %x, token = %x\n", desc, len, buffer, token);
*/
//BOOL ret = 0;
//			curr_vm->current_mode_state->ctx.reg[0] = ret;
			return;
			}

		case 1046: {		//Read CPU ID
			unsigned int cpuid_reg = (unsigned int) param0;
			unsigned int cpu_id;
			unsigned int *cpuid_out_ptr = (unsigned int *) param1;
			switch (cpuid_reg) {
			case 0: //CPUID_ID
				asm volatile ("mrc	p15, 0, %0, c0, c0, 0" : "=r" (cpu_id) : : "cc"); break;
			case 1:	//CPUID_CACHETYPE
				asm volatile ("mrc	p15, 0, %0, c0, c0, 1" : "=r" (cpu_id) : : "cc"); break;
			case 2: //CPUID_TCM
				asm volatile ("mrc	p15, 0, %0, c0, c0, 2" : "=r" (cpu_id) : : "cc"); break;
			case 3:	//CPUID_TLBTYPE
				asm volatile ("mrc	p15, 0, %0, c0, c0, 3" : "=r" (cpu_id) : : "cc"); break;
			case 4: //CPUID_MPUIR
				asm volatile ("mrc	p15, 0, %0, c0, c0, 4" : "=r" (cpu_id) : : "cc"); break;
			case 5:	//CPUID_MPIDR
				asm volatile ("mrc	p15, 0, %0, c0, c0, 5" : "=r" (cpu_id) : : "cc"); break;
			case 6: //CPUID_REVIDR
				asm volatile ("mrc	p15, 0, %0, c0, c0, 6" : "=r" (cpu_id) : : "cc"); break;
			default: printf("HYPERVISOR READ CPU ID ERROR!\n"); break;
			}
			printf("HYPERVISOR READ CPU ID = %x\n", cpu_id);
			*cpuid_out_ptr = cpu_id;
			return;
		}

		case 1047: {		//Read extended CPU ID
			char *ext_reg = (char *) param0;
			unsigned int *__val = (unsigned int *) param1;
			unsigned int ext_id;
			//"c1, 0", "c1, 1", ... "c1, 7"
			if (ext_reg[0] == 'c' && ext_reg[1] == '1' && ext_reg[2] == ',' && ext_reg[3] == ' ') {
				switch (ext_reg[4]) {
				case '0': //CPUID_EXT_PFR0
					asm volatile ("mrc p15,0,%0,c0,c1,0" : "=r" (ext_id) : : "memory"); break;
				case '1': //CPUID_EXT_PFR1
					asm volatile ("mrc p15,0,%0,c0,c1,1" : "=r" (ext_id) : : "memory"); break;
				case '2': //CPUID_EXT_DFR0
					asm volatile ("mrc p15,0,%0,c0,c1,2" : "=r" (ext_id) : : "memory"); break;
				case '3': //CPUID_EXT_AFR0
					asm volatile ("mrc p15,0,%0,c0,c1,3" : "=r" (ext_id) : : "memory"); break;
				case '4': //CPUID_EXT_MMFR0
					asm volatile ("mrc p15,0,%0,c0,c1,4" : "=r" (ext_id) : : "memory"); break;
				case '5': //CPUID_EXT_MMFR1
					asm volatile ("mrc p15,0,%0,c0,c1,5" : "=r" (ext_id) : : "memory"); break;
				case '6': //CPUID_EXT_MMFR2
					asm volatile ("mrc p15,0,%0,c0,c1,6" : "=r" (ext_id) : : "memory"); break;
				case '7': //CPUID_EXT_MMFR3
					asm volatile ("mrc p15,0,%0,c0,c1,7" : "=r" (ext_id) : : "memory"); break;
				default: printf("HYPERVISOR READ EXTENDED CPU ID ERROR!\n"); break;
				}
			} else if (ext_reg[0] == 'c' && ext_reg[1] == '2' && ext_reg[2] == ',' && ext_reg[3] == ' ') {
				switch (ext_reg[4]) {
				case '0': //CPUID_EXT_ISAR0
					asm volatile ("mrc p15,0,%0,c0,c2,0" : "=r" (ext_id) : : "memory"); break;
				case '1': //CPUID_EXT_ISAR1
					asm volatile ("mrc p15,0,%0,c0,c2,1" : "=r" (ext_id) : : "memory"); break;
				case '2': //CPUID_EXT_ISAR2
					asm volatile ("mrc p15,0,%0,c0,c2,2" : "=r" (ext_id) : : "memory"); break;
				case '3': //CPUID_EXT_ISAR3
					asm volatile ("mrc p15,0,%0,c0,c2,3" : "=r" (ext_id) : : "memory"); break;
				case '4': //CPUID_EXT_ISAR4
					asm volatile ("mrc p15,0,%0,c0,c2,4" : "=r" (ext_id) : : "memory"); break;
				case '5': //CPUID_EXT_ISAR5
					asm volatile ("mrc p15,0,%0,c0,c2,5" : "=r" (ext_id) : : "memory"); break;
				default: printf("HYPERVISOR READ EXTENDED CPU ID ERROR!\n"); break;
				}
			} else
				printf("HYPERVISOR READ EXTENDED CPU ID ERROR ARGUMENT: %c%c%c%c%c\n",
						ext_reg[0], ext_reg[1], ext_reg[2], ext_reg[3], ext_reg[4]);
			printf("HYPERVISOR READ EXTENDED CPU ID = %x\n", ext_id);
			*__val = ext_id;
			return;
		}

		case 1048: {		//Read SCTLR.
			uint32_t cr;
			uint32_t *cr_ptr = (uint32_t *) param0;
			asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (cr) : : "cc");
			printf("HYPERVISOR READ SCTLR = %x\n", cr);
			*cr_ptr = cr;
			return;
		}

		case 1049: {		//Set Cache selection register.
			uint32_t *cache_selector = (uint32_t *) param0;
			asm volatile("mcr p15, 2, %0, c0, c0, 0" : : "r" (*cache_selector));
			printf("HYPERVISOR SETS CACHE SELECTION REGISTER = %x\n", *cache_selector);
			return;
		}

		case 1050: {		//Reads Cache Size ID Register (CCSIDR)
			uint32_t *val = (uint32_t *) param0;
			asm volatile("mrc p15, 1, %0, c0, c0, 0" : "=r" (*val));
			printf("HYPERVISOR READS CACHE SIZE ID REGISTER = %x\n", *val);
			return;
		}

		case 1051: {	//Reads "ID Code Register", "device ID code that contains information about the processor."
			uint32_t id_code_reg;
			asm volatile ("mrc p15, 0, %0, c0, c0" : "=r" (id_code_reg) : : "cc");
			printf("HYPERVISOR READS ID Code Register = %x\n", id_code_reg);
			curr_vm->current_mode_state->ctx.reg[9] = id_code_reg;
			return;
		}

		case 1052: {	//Hypercall invoked from assembly code in linux.
			hypercall_cache_op(FLUSH_D_CACHE_AREA, param0, param1);
			return;
		}

		case 1053: {
			//Hypercall 1: Initial virtual section mapping during Linux boot,
			//invoked from head.S.
			uint32_t PAGE_OFFSET = param0;
			uint32_t kernel_sec_start = param1;
			uint32_t kernel_sec_end = param2;
			printf("Hypercall 1: Initial boot virtual Linux map PAGE_OFFSET = %x\n", PAGE_OFFSET);
			printf("Hypercall 1: Initial boot virtual Linux map start va = %x\n", PAGE_OFFSET);
			printf("Hypercall 1: Initial boot virtual Linux map end va = %x\n", PAGE_OFFSET + kernel_sec_end - kernel_sec_start);
			printf("Hypercall 1: Initial boot virtual Linux map start pa = %x\n", kernel_sec_start);
			printf("Hypercall 1: Initial boot virtual Linux map end pa = %x\n", kernel_sec_end);
			linux_boot_virtual_map_section(param0, param1, param2);
			return;
		}

		case 1054: {	//Hypercall 2: Map of DTB.
			printf("Hypercall 2: Linux DTB virtual map fdt_base_pa = %x\n", param0);
			printf("Hypercall 2: Linux DTB virtual map fdt_base_va = %x\n", param1);
			linux_boot_virtual_map_fdt(param0, param1);
			return;
		}

		case 1055: {	//Hypercall 3: Clears page tables.
			printf("Hypercall 3: Linux clear initial mapping: start = 0x%x; end = 0x%x\n", param0, param1);
			linux_clear_virtual_map_section(param0, param1);
			return;
		}

		case 1056: {
			if (param1 == 0x48000000 || param1 == 0x44C00000) {	//Hypercall 4: Maps peripherals on BBB.
				printf("Hypercall 5: Second initial boot virtual Linux I/O map: ");
				printf("start va = %x, end va = %x, start pa = %x, end pa = %x\n", param0, param0 + param2 - param1, param1, param2);
				linux_map_per_wkup(param0, param1, param2);
			} else {
				//Hypercall 5: Remapping of initial virtual mapping during Linux
				//boot, with more fine-grained page table mappings and page table
				//configurations, invoked by mmu.c:create_mapping.
				printf("Hypercall 4: Second initial boot virtual Linux section map start va = %x\n", param0);
				printf("Hypercall 4: Second initial boot virtual Linux section map end va = %x\n", param0 + param2 - param1);
				printf("Hypercall 4: Second initial boot virtual Linux section map start pa = %x\n", param1);
				printf("Hypercall 4: Second initial boot virtual Linux section map end pa = %x\n", param2);
printf("DEPRECATED Hypercall 4: SHOULD NOT BE INVOKED!\n");
while (1);
				linux_boot_second_virtual_map(param0, param1, param2);
			}
			return;
		}

		case 1057: {	//Reads ACTLR (Auxiliary Control Register)
			uint32_t *aux_cr = (uint32_t *) param0;
            asm("mrc p15, 0, %0, c1, c0, 1" : "=r" (*aux_cr));
			printf("HYPERVISOR READS Auxiliary Control Register = %x\n", *aux_cr);
			return;
		}

		case 1058: {	//Sets TLS register (TPIDRURO, User Read-Only Thread ID Register, VMSA)
			uint32_t tls = param0;
			asm volatile ("mcr	p15, 0, %0, c13, c0, 3		@ set TLS register" : "=r" (tls) : : "cc");
//			printf("HYPERVISOR SETS TLS Register = %x\n", r4);
			return;
		}

		case 1059: {	//Enable floating point by writing CPACR.
			uint32_t access;
			uint32_t coprocessor10_full_access = 3 << 10*2;
			uint32_t coprocessor11_full_access = 3 << 11*2;
			asm volatile("mrc p15, 0, %0, c1, c0, 2 @ get copro access" : "=r" (access) : : "cc");
			access = access | coprocessor10_full_access | coprocessor11_full_access;
			asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access" : : "r" (access) : "cc");
			printf("HYPERVISOR ENABLES FULL ACCESS TO COPROCESSORS 10 AND 11\n");
			return;
		}

		case 1060: {	//Reads floating-point associated register.
			uint32_t *val = (uint32_t *) param0;
			uint32_t reg = param1;

			switch (reg) {
			case 0 : asm(".fpu	vfpv2\n"   "vmrs	%0, FPSID" : "=r" (*val) : : "cc"); break;
			case 1 : asm(".fpu	vfpv2\n"   "vmrs	%0, FPSCR" : "=r" (*val) : : "cc"); break;
			case 6 : asm(".fpu	vfpv2\n"   "vmrs	%0, MVFR1" : "=r" (*val) : : "cc"); break;
			case 7 : asm(".fpu	vfpv2\n"   "vmrs	%0, MVFR0" : "=r" (*val) : : "cc"); break;
			case 8 : asm(".fpu	vfpv2\n"   "vmrs	%0, FPEXC" : "=r" (*val) : : "cc"); break;
			case 9 : asm(".fpu	vfpv2\n"   "vmrs	%0, FPINST" : "=r" (*val) : : "cc"); break;
			case 10: asm(".fpu	vfpv2\n"   "vmrs	%0, FPINST2" : "=r" (*val) : : "cc"); break;
			}
//if (reg != 8)
//			printf("Reads floating-point associated register %x: %x\n", reg, *val);
			return;
		}

		case 1061: {	//Writes floating-point associated register.
			uint32_t val = param0;
			uint32_t reg = param1;

			switch (reg) {
			case 0 : asm(".fpu	vfpv2\n"	"vmsr	FPSID, %0" : : "r" (val) : "cc"); break;
			case 1 : asm(".fpu	vfpv2\n"	"vmsr	FPSCR, %0" : : "r" (val) : "cc"); break;
			case 6 : asm(".fpu	vfpv2\n"	"vmsr	MVFR1, %0" : : "r" (val) : "cc"); break;
			case 7 : asm(".fpu	vfpv2\n"	"vmsr	MVFR0, %0" : : "r" (val) : "cc"); break;
			case 8 : asm(".fpu	vfpv2\n"	"vmsr	FPEXC, %0" : : "r" (val) : "cc"); break;
			case 9 : asm(".fpu	vfpv2\n"	"vmsr	FPINST, %0" : : "r" (val) : "cc"); break;
			case 10: asm(".fpu	vfpv2\n"	"vmsr	FPINST2, %0" : : "r" (val) : "cc"); break;
			}
//if (reg != 8)
//			printf("Writes floating-point associated register %x: %x\n", reg, val);
			return;
		}

		case 1062: {	//Checks that DMA re-mapping is not changed.
			uint32_t va = param0;
			uint32_t new_l2e = param1;

			uint32_t l1i = va >> 20;
			uint32_t l1e_va = 0xC0004000 + (l1i << 2);
			uint32_t l1e = *((uint32_t *) l1e_va);
			uint32_t l2_pt_pa = l1e & ~0x3FF;
			uint32_t l2_pt_va = mmu_guest_pa_to_va(l2_pt_pa, curr_vm->config);
			uint32_t l2i = ((va >> 12) & 0xFF) << 2;
			uint32_t l2e = *((uint32_t *)(l2_pt_va + l2i));

			if (l2e & ~0xFFF != new_l2e & ~0xFFF) {
				printf("Hypervisor discovers L2 entry update:\n");
				printf("l1i: 0x%x\n", l1i);
				printf("l1e_va: 0x%x\n", l1e_va);
				printf("l1e: 0x%x\n", l1e);
				printf("l2_pt_pa: 0x%x\n", l2_pt_pa);
				printf("l2_pt_va: 0x%x\n", l2_pt_va);
				printf("l2i: 0x%x\n", l2i);
				printf("l2e: 0x%x\n", l2e);
				printf("Old entry: 0x%x\n", l2e);
				printf("New entry: 0x%x\n", new_l2e);
				while (1);
			}

			return;
		}

		case 1063: {	//Sets alignment check enable bit.
			//Only bits 21, 14, 10 and 1 are allowed to be modified by Linux.
			#define SCTLR_A	(1 << 1)
			#define SCTLR_SW (1 << 10)
			#define SCTLR_RR (1 << 14)
			#define SCTLR_FI (1 << 21)
			#define SCTLR_WRITABLE	(SCTLR_FI | SCTLR_RR | SCTLR_SW | SCTLR_A)

			uint32_t sctlr_old;
			asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (sctlr_old) : : "cc");
			uint32_t sctlr = sctlr_old & ~SCTLR_WRITABLE;
			uint32_t sctlr_mod = param0 & SCTLR_WRITABLE;
			uint32_t sctlr_new = sctlr | sctlr_mod;
			if (sctlr_new != sctlr_old) {
				asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r" (sctlr_new) : "cc");
				isb();
				printf("Hypervisor updates: SCTLR to 0x%x\n", sctlr_new);
			}
			return;
		}
/*
		case 1064: {	//Reads DACR.
			printf("Hypervisor reads DACR.\n");
			uint32_t *domain = (uint32_t *) param0;
			uint32_t dependency_address = param1;	//To avoid compiler optimizing it away.
			asm volatile("mrc	p15, 0, %0, c3, c0	@ get domain" : "=r" (*domain) : "m" (dependency_address));

			return;
		}

		case 1065: {	//Writes DACR.
			printf("Hypervisor writes DACR.\n");
			uint32_t new_domain = param0;
			asm volatile("mcr	p15, 0, %0, c3, c0	@ set domain" : : "r" (val) : "memory");
			isb();

			return;
		}
*/
		case 1099: {	//Invoked when Linux kernel makes a panic.
			printf("LINUX PANIC!\n");
			while (1);
			return;
		}

		default:
			hypercall_num_error(hypercall_number);
		}
	}
	/*Control of virtual PSR */
#if 1
	if ((curr_vm->current_mode_state->ctx.psr & 0x1F) == 0x13 && curr_vm->current_guest_mode != HC_GM_KERNEL) {	/*virtual SVC mode */
		hyper_panic("PSR in SVC mode but guest mode is not KERNEL\n",
			    0);
	} else if ((curr_vm->current_mode_state->ctx.psr & 0x1F) == 0x10
		   && (curr_vm->current_guest_mode == HC_GM_KERNEL))
		hyper_panic("PSR in USR mode but guest mode is in KERNEL\n", 0);
#endif
}

//addr = IFAR
//status = IFSR
//address of faulting instruction
return_value prefetch_abort_handler(uint32_t addr, uint32_t status,
				    uint32_t unused)
{
#if 1
	if (addr >= 0xc0000000) {
		printf("Pabort: 0x%x Status: 0x%x, u = 0x%x \n", addr, status, unused);
		printf("LR:%x\n", curr_vm->current_mode_state->ctx.lr);
	}
#endif
	uint32_t interrupted_mode = curr_vm->current_guest_mode;

	/*Need to be in virtual kernel mode to access data abort handler */
	change_guest_mode(HC_GM_KERNEL);
#ifdef LINUX
	/*Set uregs, Linux kernel ususally sets these up in exception vector
	 * which we have to handle now*/

	uint32_t *sp = (uint32_t *) (curr_vm->mode_states[HC_GM_KERNEL].ctx.sp - 72);	/*FRAME_SIZE (18 registers to be saved) */
	uint32_t *context = curr_vm->mode_states[interrupted_mode].ctx.reg;
	uint32_t i;

	for (i = 0; i < 17; i++) {
		*sp++ = *context++;
	}
	*sp = 0xFFFFFFFF;	//ORIG_R0
	curr_vm->mode_states[HC_GM_KERNEL].ctx.sp -= (72);	/*Adjust stack pointer */

	/*Prepare args for prefetchabort handler */
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[0] = addr;
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[1] = status;
	/*Linux saves the user registers in the stack */
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[2] = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;

	curr_vm->mode_states[HC_GM_KERNEL].ctx.psr |= IRQ_MASK;	/*Disable IRQ ALWAYS */
	/*Prepare pc for Linux handler */
	uint32_t *pabt_handler = (uint32_t *) (curr_vm->exception_vector[V_PREFETCH_ABORT]);
	if (interrupted_mode == HC_GM_TASK)
		pabt_handler++;	/*DABT_USR located +4 in Linux exception vector */

	curr_vm->current_mode_state->ctx.pc = *pabt_handler;
#endif
	return RV_OK;
}

//addr = DFAR: contains VA of accessed word.
//status = DFSR: contains information about fault.
//unused: contains VA of faulting instruction.
return_value data_abort_handler(uint32_t addr, uint32_t status, uint32_t unused)
{
	uint32_t interrupted_mode = curr_vm->current_guest_mode;

	BOOL emulated = FALSE;
	if (interrupted_mode == HC_GM_KERNEL && addr < HAL_VIRT_START)
		emulated = emulate_write(addr, unused);

	if (emulated) {
		printf("Dabort: Emulated access to 0x%x now made writable (when executing instruction at 0x%x).\n", addr, unused);
		return RV_OK;	//Do not propagate error to Linux, but let Linux reexecute the instruction and continue to execute as if the exception had not happen.
//		while (1);
	}
#if 1
	if (addr >= 0xC0000000 && !emulated) {
		printf("Dabort: 0x%x Status: 0x%x, u = 0x%x \n", addr, status, unused);
while (1);
		uint32_t l1_pt_pa;
		COP_READ(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, l1_pt_pa);
		uint32_t l1_pt_va = mmu_guest_pa_to_va(l1_pt_pa, curr_vm->config);
		uint32_t l1i = addr >> 20;
		uint32_t l1e_va = l1_pt_va + (l1i << 2);
		uint32_t l1e = *((uint32_t *) (l1e_va));
		uint32_t l2_pt_pa = l1e & 0xFFFFFC00;
		uint32_t l2i = (addr >> 12) & 0xFF;

		if ((l1e & 0x3) == 0x1) {
			uint32_t l2_pt_va = mmu_guest_pa_to_va(l2_pt_pa, curr_vm->config);
			uint32_t l2e_va = l2_pt_va + (l2i << 2);
			uint32_t l2e = *((uint32_t *) l2e_va);
			uint32_t l2e_ap = ((l2e & 0x200) >> 7) | ((l2e & 0x30) >> 4);
			printf("l1i = 0x%x, &l1e = 0x%x, l1e = 0x%x, l2_pt_pa = 0x%x, l2i = 0x%x, &l2e = 0x%x, l2e = 0x%x, l2e.AP[2:0] = 0x%x\n", l1i, l1e_va, l1e, l2_pt_pa, l2i, l2e_va, l2e, l2e_ap);


//ska det vara:
//dmmu_l2_map_entry(l2_pt_pa & 0xFFFFF000
//har vi rätt index??? + 256?
//cache

			if ((l2e & 0x3) == 0) {
				l2e = l2e | 0x2;
				uint32_t pti = ((l2_pt_pa - (l2_pt_pa & 0xFFFFF000)) >> 10)*256;
				l2i = pti + ((addr >> 12) & 0xFF);
				uint32_t page_pa = l2e & 0xFFFFF000;
				uint32_t small_attrs = l2e & 0x00000FFD;
				uint32_t err = dmmu_l2_map_entry(l2_pt_pa & 0xFFFFF000, l2i, page_pa, small_attrs);
				if (err) {
					printf("INVALID L2 COULD NOT BE MADE VALID: err = 0x%x.\n", err);
					while (1);
				} else
					printf("INVALID L2 MADE VALID.\n");
			}
		} else
			printf("l1i = 0x%x, &l1e = 0x%x, l1e = 0x%x, l2_pt_pa = 0x%x, l2i = 0x%x\n", l1i, l1e_va, l1e, l2_pt_pa, l2i);
	}
#endif
////////
//  printf("Hypervisor, data abort handler: VA of addressed word:%x Information about data abort:%x, VA of faulting instruction: %x\n", addr, status, unused);
////////

////////
#if defined(LINUX) && defined(CPSW)
	//If accessed address is within the mapped Ethernet Subsystem memory
	//regions.
	if (interrupted_mode == HC_GM_KERNEL && 0xFA400000 <= addr && addr < 0xFA404000) {
		//Checks the access. If it is valid it is carried out, otherwise a
		//message is printed and the system freezes.
		BOOL ret = soc_check_cpsw_access(addr, curr_vm->current_mode_state->ctx.pc);

		//If the access is invalid the system freezes.
		if (!ret) {
			printf("FAILURE AT STH CPSW DRIVER!\n");
			for (;;) ;
		}
		//Increment program counter to point to instruction following the
		//failing one.
		curr_vm->current_mode_state->ctx.pc += 4;

		//Returns to exception_bottom which restores the guest to exeucte the
		//instruction following the failed one.
		return RV_OK;
	}// else if (addr >= 0xc0000000)
//		printf("Dabort:%x Status:%x, u=%x \n", addr, status, unused);
#endif
////////
	/*Must be in virtual kernel mode to access kernel handlers */
	change_guest_mode(HC_GM_KERNEL);
#ifdef LINUX

	/*Set uregs, Linux kernel ususally sets these up in exception vector
	 * which we have to handle now*/

	uint32_t *sp = (uint32_t *) (curr_vm->mode_states[HC_GM_KERNEL].ctx.sp - 72);	/*FRAME_SIZE (18 registers to be saved) */
	uint32_t *context = curr_vm->mode_states[interrupted_mode].ctx.reg;
	uint32_t i;

	/*Save context in sp */
	for (i = 0; i < 17; i++) {
		*sp++ = *context++;
	}
	*sp = 0xFFFFFFFF;	//ORIG_R0
	curr_vm->mode_states[HC_GM_KERNEL].ctx.sp -= (72);	/*Adjust stack pointer */

	/*Prepare args for dataabort handler */
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[0] = addr;
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[1] = status;
	/*Linux saves the user registers in the stack */
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[5] = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.psr;	/*spsr in r5 for linux kernel vector */

	curr_vm->mode_states[HC_GM_KERNEL].ctx.psr |= IRQ_MASK;	/*Disable IRQ ALWAYS */

	/*Prepare pc for handler and lr to return from handler */

	uint32_t *dabt_handler =
	    (uint32_t *) (curr_vm->exception_vector[V_DATA_ABORT]);
	if (interrupted_mode == HC_GM_TASK) {
////////
//    printf("HYPERVISOR: core/hypervisor/handlers.c:data_abort_handler(): The data abort occurred in a task!\n");
////////
		dabt_handler++;	//DABT_USR located +4
	}
////////
//  else {
//    printf("HYPERVISOR: core/hypervisor/handlers.c:data_abort_handler(): The data abort occurred in the kernel!\n");
//  }
////////

	curr_vm->current_mode_state->ctx.pc = *dabt_handler;
#if 0	 /*DEBUG*/
////////
	    //   printf("Task PC:%x LR:%x \n",curr_vm->mode_states[HC_GM_TASK].ctx.pc, curr_vm->mode_states[HC_GM_TASK].ctx.lr);
////////
	    printf("Kernel PC:%x LR:%x \n",
		   curr_vm->mode_states[HC_GM_KERNEL].ctx.pc,
		   curr_vm->mode_states[HC_GM_KERNEL].ctx.lr);
#endif
#endif
	return RV_OK;
}

#define LINUX_5_15_13
#ifndef LINUX_5_15_13
return_value irq_handler(uint32_t irq, uint32_t r1, uint32_t r2)
{
//HÄR mars 21:
//sök på CONFIG_SICS_HYPERVISOR
//kolla drivers/irqchip/irq-omap-intc.c
//kolla arch/arm/kernel/entry-armv.S


//	printf("IRQ handler called %d, interrupt handler: 0x%x\n", irq, );
	if (curr_vm->current_mode_state->ctx.psr & 0x80) {	/*Interrupts are off, return */
		printf("AVBROTT: FREEZE!\n");
		for (;;) ;
//              mask_interrupt(irq, 1); //Mask interrupt and mark pending
		return RV_OK;
	}

	uint32_t interrupted_mode = curr_vm->current_guest_mode;
	change_guest_mode(HC_GM_KERNEL);
#ifdef LINUX
	/*Prepare stack for nested irqs */
	uint32_t i;

	uint32_t *context = curr_vm->mode_states[interrupted_mode].ctx.reg;
	uint32_t *sp_push = (uint32_t *) (curr_vm->mode_states[HC_GM_KERNEL].ctx.sp - 72);	//FRAME_SIZE (18 registers to be saved)

	for (i = 0; i < 17; i++) {
		*sp_push++ = *context++;
	}
	*sp_push = 0xFFFFFFFF;	//ORIG_R0
	curr_vm->mode_states[HC_GM_KERNEL].ctx.sp -= (72);	//FRAME_SIZE (18 registers to be saved)

#if 0	 /*DEBUG*/
	    printf("IRQ handler called %x:%x:\n", irq,
		   curr_vm->mode_states[HC_GM_KERNEL].ctx.sp);
#endif
	curr_vm->interrupted_mode = interrupted_mode;
//här är fel.
//dacr måste vara på stacken också???
	curr_vm->current_mode_state->ctx.reg[0] = irq;
	curr_vm->current_mode_state->ctx.reg[1] = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[5] = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.psr;	/*spsr in r5 for linux kernel vector */

	uint32_t *irq_handler = (uint32_t *) (curr_vm->exception_vector[V_IRQ]);

	if (interrupted_mode == HC_GM_TASK)
		irq_handler++;	//IRQ_USR located +4
	printf("IRQ TEST %d, interrupt handler: 0x%x\n", irq, irq_handler);
	printf("IRQ handler called %d, interrupt handler: 0x%x\n", irq, irq_handler);
	printf("IRQ TEST %d, interrupt handler: 0x%x\n", irq, irq_handler);
	curr_vm->current_mode_state->ctx.pc = *irq_handler;
	curr_vm->current_mode_state->ctx.psr |= IRQ_MASK;
	curr_vm->current_mode_state->ctx.sp = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;

#endif
//      unmask_interrupt(irq, 0);
	return RV_OK;
}
#else
#if 0
return_value irq_handler_svc(void) {
	uint32_t interrupted_mode = curr_vm->current_guest_mode;
	//Set guest in kernel mode (should already be in kernel mode since this
	//handler is invoked when that is the case.
	change_guest_mode(HC_GM_KERNEL);

	uint32_t *context = curr_vm->mode_states[interrupted_mode].ctx.reg;

	//SVC_REGS_SIZE = 19. Makes room for everything except r0.
	uint32_t *sp = (uint32_t *) (curr_vm->mode_states[HC_GM_KERNEL].ctx.sp - (19*4 + 0 - 4));
	//After making room for r0, and SP[2] == 0, then SP[2] == 1 and not a
	//multiple of 8. Therefore sp is subtracted by 4.
	if (sp & 0x4 == 0)												//2.
		sp = sp - 4;												//3.

	//Stores r1-r12. sp starts at r1 and context at r0.
	for (i = 1; i <= 12; i++)										//4.
		*(sp + (i - 1)*4) = *(context + i*4);						//4.

	uint32_t r3 = *context;											//5. r3 := r0 before exception.
	uint32_t r4 = (uint32_t) curr_vm->mode_states[interrupted_mode].ctx.pc;	//5. r4 := lr = corrected and preferred return address.
	uint32_t r5 = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.psr;	//5. r5 := SPSR = kernel CPSR before exception.
	uint32_t *r7 = sp + 13*4 - 4;									//6.
	uint32_t r6 = -1;												//7.
	uint32_t r2 = sp + 19*4 + 0 - 4;								//8.
	if ((sp + 4) & 0x4 == 0)										//2.
		r2 = r2 + 4;												//9.

	*(sp - 4) = r3;													//10.
	sp = sp - 4;													//10.

	r3 = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.lr;		//11.

	*r7 = r2;														//12.
	r7 = r7 + 4;													//12.
	*r7 = r3;														//12.
	r7 = r7 + 4;													//12.
	*r7 = r4;														//12.
	r7 = r7 + 4;													//12.
	*r7 = r5;														//12.
	r7 = r7 + 4;													//12.
	*r7 = r6;														//12.
	r7 = r7 + 4;													//12.

	curr_vm->mode_states[HC_GM_KERNEL].ctx.sp = sp;					//10.


//här är fel.
//dacr måste vara på stacken också???
	curr_vm->current_mode_state->ctx.reg[1] = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[5] = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.psr;	/*spsr in r5 for linux kernel vector */

	uint32_t *irq_handler = (uint32_t *) (curr_vm->exception_vector[V_IRQ]);
	curr_vm->current_mode_state->ctx.pc = *irq_handler;
	curr_vm->current_mode_state->ctx.psr |= IRQ_MASK;
	curr_vm->current_mode_state->ctx.sp = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;

	return RV_OK;
}
#endif
return_value irq_handler(uint32_t irq, uint32_t r1, uint32_t r2)
{
//if (irq == 44 || irq == 45 || irq == 46 || irq == 72 || irq == 73 || irq == 74)
//	printf("IRQ handler called %d, interrupt handler: 0x%x\n", irq);
	if (curr_vm->current_mode_state->ctx.psr & 0x80) {	/*Interrupts are off, return */
		printf("AVBROTT: FREEZE!\n");
		for (;;) ;
//              mask_interrupt(irq, 1); //Mask interrupt and mark pending
		return RV_OK;
	}

	uint32_t interrupted_mode = curr_vm->current_guest_mode;
	change_guest_mode(HC_GM_KERNEL);
#ifdef LINUX
	/*Prepare stack for nested irqs */
	uint32_t i;

	uint32_t *context = curr_vm->mode_states[interrupted_mode].ctx.reg;
	uint32_t *sp_push = (uint32_t *) (curr_vm->mode_states[HC_GM_KERNEL].ctx.sp - 72);	//FRAME_SIZE (18 registers to be saved)

	for (i = 0; i < 17; i++) {
		*sp_push++ = *context++;
	}
	*sp_push = 0xFFFFFFFF;	//ORIG_R0
	curr_vm->mode_states[HC_GM_KERNEL].ctx.sp -= (72);	//FRAME_SIZE (18 registers to be saved)

	curr_vm->interrupted_mode = interrupted_mode;

//	curr_vm->current_mode_state->ctx.reg[0] = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;
//	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[5] = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.psr;	/*spsr in r5 for linux kernel vector */

	uint32_t *irq_handler = (uint32_t *) (curr_vm->exception_vector[V_IRQ]);

	if (interrupted_mode == HC_GM_TASK)
		irq_handler++;	//IRQ_USR located +4
//	printf("IRQ TEST %d, interrupt handler: 0x%x\n", irq, irq_handler);
//	printf("IRQ handler called %d, interrupt handler: 0x%x\n", irq, irq_handler);
//	printf("IRQ TEST %d, interrupt handler: 0x%x\n", irq, irq_handler);
	curr_vm->current_mode_state->ctx.pc = *irq_handler;
	curr_vm->current_mode_state->ctx.psr |= IRQ_MASK;
	curr_vm->current_mode_state->ctx.sp = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;

#endif
//      unmask_interrupt(irq, 0);
	return RV_OK;
}
#endif

/*Used for floating point emulation in Linux*/
return_value undef_handler(uint32_t instr, uint32_t unused, uint32_t addr)
{
#if 1
	printf("Undefined abort\n Address: 0x%x Instruction: 0x%x \n", addr, instr);
#endif
	uint32_t interrupted_mode = curr_vm->current_guest_mode;

	/*Must be in virtual kernel mode to access kernel handlers */
	change_guest_mode(HC_GM_KERNEL);
#ifdef LINUX
	/*Set uregs, Linux kernel ususally sets these up in exception vector
	 * which we have to handle now*/

	uint32_t *sp = (uint32_t *) (curr_vm->mode_states[HC_GM_KERNEL].ctx.sp - 72);	//FRAME_SIZE (18 registers to be saved)
	uint32_t *context = curr_vm->mode_states[interrupted_mode].ctx.reg;
	uint32_t i;

	for (i = 0; i < 17; i++) {
		*sp++ = *context++;
	}
	*sp = 0xFFFFFFFF;	//ORIG_R0
	curr_vm->mode_states[HC_GM_KERNEL].ctx.sp -= (72);	//FRAME_SIZE (18 registers to be saved)
	//Context saved in sp

	/*Prepare args for dataabort handler */
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[4] =
	    curr_vm->mode_states[interrupted_mode].ctx.pc;
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[5] = curr_vm->mode_states[interrupted_mode].ctx.psr;	/*spsr in r5 for linux kernel vector */
	/*Linux saves the user registers in the stack */

	curr_vm->mode_states[HC_GM_KERNEL].ctx.psr |= IRQ_MASK;	/*Disable IRQ ALWAYS */

	uint32_t *und_handler =
	    (uint32_t *) (curr_vm->exception_vector[V_UNDEF]);
	if (interrupted_mode == HC_GM_TASK)
		und_handler++;	//DABT_USR located +4

	curr_vm->current_mode_state->ctx.pc = *und_handler;
#endif
	return RV_OK;
}

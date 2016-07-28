#include <hw.h>
#include "hypercalls.h"
#if defined(LINUX) && defined(CPSW)
#include <soc_cpsw.h>
#endif

extern virtual_machine *curr_vm;

extern BOOL soc_check_cpsw_access(uint32_t, uint32_t);
#define USE_DMMU

// Disabling aggressive flushing
#define AGGRESSIVE_FLUSHING_HANDLERS
#define HC_DOMAC_ALL 0x55555555

typedef struct data
{
	uint32_t p0;
	uint32_t p1;
	uint32_t p2;
};

struct data params;

void clean_and_invalidate_cache()
{
#ifdef AGGRESSIVE_FLUSHING_HANDLERS
	isb();
	mem_mmu_tlb_invalidate_all(TRUE, TRUE);
	CacheDataCleanInvalidateAll();
	dsb();
#endif
}


uint32_t counter = 0;


#define MONITOR_ENABLED
//#define DEBUG_MONITOR_CALL

void swi_handler(uint32_t param0, uint32_t param1, uint32_t param2,
		 uint32_t hypercall_number)
{

	//Enabling all the domains
	uint32_t domac = HC_DOMAC_ALL;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
//  if ((hypercall_number == HYPERCALL_RESTORE_REGS) || (hypercall_number == HYPERCALL_RESTORE_LINUX_REGS) && (hypercall_number != HYPERCALL_SET_TLS_ID) ){
//      printf("SWI ENTER hypercall_number = %d %x %x %x\n", hypercall_number, param0, param1, param2);
//  }
	/*TODO Added check that controls if it comes from user space, makes it pretty inefficient, remake later */
	/*Testing RPC from user space, remove later */
	if (curr_vm->current_guest_mode == HC_GM_TASK) {
/////////////////////
		if ((curr_vm->current_mode_state->ctx.psr & IRQ_MASK) != 0)
			printf
			    ("ERROR (interrupt delivered when disabled) IRQ_MASK = %x\n",
			     curr_vm->current_mode_state->ctx.psr);
/////////////////////

		//  debug("\tUser process made system call:\t\t\t %x\n",  curr_vm->mode_states[HC_GM_TASK].ctx.reg[7] );
		change_guest_mode(HC_GM_KERNEL);
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

		curr_vm->current_mode_state->ctx.lr = curr_vm->exception_vector[V_RET_FAST_SYSCALL];	//curr_vm->handlers.syscall.ret_fast_syscall;
		//copy task context to kernel context. syscall supports 6 arguments

		/*system call nr in r7 */
		curr_vm->current_mode_state->ctx.reg[7] =
		    curr_vm->mode_states[HC_GM_TASK].ctx.reg[7];

		if (curr_vm->mode_states[HC_GM_TASK].ctx.reg[7] <
		    curr_vm->guest_info.nr_syscalls) {
			/*Regular system call, restore params */
			for (i = 0; i <= 5; i++)
				curr_vm->current_mode_state->ctx.reg[i] =
				    curr_vm->mode_states[HC_GM_TASK].ctx.reg[i];
			/*Set PC to systemcall function */
			curr_vm->current_mode_state->ctx.pc =
			    *((uint32_t *) (curr_vm->exception_vector[V_SWI] +
					    (curr_vm->current_mode_state->ctx.
					     reg[7] << 2)));
		} else {
			//TODO Have not added check that its a valid private arm syscall, done anyways inside arm_syscall
			//if(curr_vm->current_mode_state->ctx.reg[7] >= 0xF0000){ //NR_SYSCALL_BASE
			/*Arm private system call */
			curr_vm->current_mode_state->ctx.reg[0] =
			    curr_vm->mode_states[HC_GM_TASK].ctx.reg[7];
			curr_vm->current_mode_state->ctx.reg[1] = curr_vm->mode_states[HC_GM_KERNEL].ctx.sp + 8;	//Adjust sp with S_OFF, contains regs
			curr_vm->current_mode_state->ctx.pc =
			    curr_vm->exception_vector[V_ARM_SYSCALL];
		}
	} else if (curr_vm->current_guest_mode != HC_GM_TASK) {
		// printf("\tHypercallnumber: %d (%x) called\n", hypercall_number, param0);
		uint32_t res;
		uint32_t from_end_rpc = 0;
		switch (hypercall_number) {
			/* TEMP: DMMU TEST */
		case 666:
			params.p0 = param0;
			params.p1 = param1;
			params.p2 = param2;
			dmmu_handler_2(param0, param1, param2);
			break;
		case HYPERCALL_DBG:
			printf("To here %x\n", param0);
			break;
		case HYPERCALL_GUEST_INIT:
			hypercall_guest_init(param0);
			break;
		case HYPERCALL_INTERRUPT_SET:
			hypercall_interrupt_set(param0, param1);
			break;
		case HYPERCALL_END_INTERRUPT:
			hypercall_end_interrupt(param0);
			break;
		case HYPERCALL_INTERRUPT_CTRL:
			hypercall_interrupt_ctrl(param0, param1);
			break;
		case HYPERCALL_CACHE_OP:
			hypercall_cache_op(param0, param1, param2);
			break;
		case HYPERCALL_SET_TLS_ID:
			hypercall_set_tls(param0);
			return;
		case HYPERCALL_SET_CTX_ID:
			COP_WRITE(COP_SYSTEM, COP_CONTEXT_ID_REGISTER, param0);
			isb();
			break;
			/*Context */
		case HYPERCALL_RESTORE_LINUX_REGS:
			hypercall_restore_linux_regs(param0, param1);
			break;
		case HYPERCALL_RESTORE_REGS:
			hypercall_restore_regs((uint32_t *) param0);
			break;

			/*Page table operations */
		case HYPERCALL_SWITCH_MM:
			clean_and_invalidate_cache();
			hypercall_dyn_switch_mm(param0, param1);
			//clean_and_invalidate_cache();
			break;

		case HYPERCALL_NEW_PGD:
			clean_and_invalidate_cache();
			hypercall_dyn_new_pgd((uint32_t *) param0);
			//clean_and_invalidate_cache();
			break;
		case HYPERCALL_FREE_PGD:
			clean_and_invalidate_cache();
			hypercall_dyn_free_pgd((uint32_t *) param0);
			//clean_and_invalidate_cache();
			break;
		case HYPERCALL_CREATE_SECTION:
			{
				//printf("SWI ENTER hypercall_number = %d %x %x %x\n", hypercall_number, param0, param1, param2);
				break;
			}

		case HYPERCALL_SET_PMD:
			clean_and_invalidate_cache();
			hypercall_dyn_set_pmd(param0, param1);
			//clean_and_invalidate_cache();
			break;
		case HYPERCALL_SET_PTE:
			clean_and_invalidate_cache();
			hypercall_dyn_set_pte((uint32_t *) param0, param1,
					      param2);
			//clean_and_invalidate_cache();
			break;
		case HYPERCALL_END_RPC:
			res = curr_vm->current_mode_state->ctx.reg[0];
			hypercall_end_rpc(res);
			from_end_rpc = 0;
#ifdef DEBUG_MONITOR_CALL
			counter+=1;
			if (counter % 100 == 0)
			{
				printf("Monitor returned with result: %d\n", res);
				debug_current_request();
			}
#endif
			if (res == 0)
			{
				clean_and_invalidate_cache();
				uint32_t result = execute_next_request();
#ifdef DEBUG_MONITOR_CALL
				if (counter % 100 == 0)
				{
					printf("hypervisor returned with result: %d\n", result);
				}
#endif
				from_end_rpc = 1;			
				curr_vm->current_mode_state->ctx.reg[0] = res;
				clean_and_invalidate_cache();
			}
			else
				reset_requests();
			break;
			//  /*VFP Test**********************/
			//case HYPERCALL_VFP:
			//  hypercall_vfp_op(param0, param1, param2);
			//  return;
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

				break;
			}
		case HYPERCALL_QUERY_BFT:
			res = dmmu_query_bft(param0);
			curr_vm->current_mode_state->ctx.reg[0] = res;
			break;
		case HYPERCALL_PING_MONITOR:
			params.p0 = param0;
			params.p1 = param1;
			params.p2 = param2;
			hypercall_rpc(0, param0);
			break;
		case HYPERCALL_MAKE_REQ:
			params.p0 = param0;
			params.p1 = param1;
			params.p2 = param2;
			dmmu_handler_2(param0, param1, param2);
			break;
		default:
			hypercall_num_error(hypercall_number);
		}

		if (curr_vm->pending_request_index < curr_vm->pending_request_counter)
		{
#ifdef MONITOR_ENABLED
			hypercall_end_request();
#else
			clean_and_invalidate_cache();
			res = execute_all_requests();
			if (res != 0)
				printf("Hypervisor returned with result: %d\n", res);
			curr_vm->current_mode_state->ctx.reg[0] = res;
			clean_and_invalidate_cache();
#endif
		}
		else
			reset_requests();
			
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

	domac = curr_vm->current_mode_state->mode_config->domain_ac;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
}

void hypercall_end_request()
{
	hypercall_request_t request = get_request(curr_vm->pending_request_index);
	uint32_t l1_base_add;
	COP_READ(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, l1_base_add);
	request.curr_l1_base_add = l1_base_add;
	change_request(request, curr_vm->pending_request_index);
	hypercall_rpc(0, curr_vm->pending_request_index);
}

return_value prefetch_abort_handler(uint32_t addr, uint32_t status,
				    uint32_t unused)
{
	uint32_t domac = HC_DOMAC_ALL;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
#if 1
	if (addr >= 0xc0000000)
		printf("Pabort:%x Status:%x, u=%x \n", addr, status, unused);
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
	curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[2] =
	    (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;

	curr_vm->mode_states[HC_GM_KERNEL].ctx.psr |= IRQ_MASK;	/*Disable IRQ ALWAYS */
	/*Prepare pc for Linux handler */
	uint32_t *pabt_handler =
	    (uint32_t *) (curr_vm->exception_vector[V_PREFETCH_ABORT]);
	if (interrupted_mode == HC_GM_TASK)
		pabt_handler++;	/*DABT_USR located +4 in Linux exception vector */

	curr_vm->current_mode_state->ctx.pc = *pabt_handler;
#endif
	domac = curr_vm->current_mode_state->mode_config->domain_ac;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
	return RV_OK;
}

return_value data_abort_handler(uint32_t addr, uint32_t status, uint32_t unused)
{
	uint32_t domac = HC_DOMAC_ALL;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);

	uint32_t interrupted_mode = curr_vm->current_guest_mode;

#if defined(LINUX) && defined(CPSW)
	//If accessed address is within the mapped Ethernet Subsystem memory
	//regions.
	if (interrupted_mode == HC_GM_KERNEL && 0xFA400000 <= addr
	    && addr < 0xFA404000) {
		//Checks the access. If it is valid it is carried out, otherwise a
		//message is printed and the system freezes.
		BOOL ret = soc_check_cpsw_access(addr,
						 curr_vm->current_mode_state->
						 ctx.pc);

		//If the access is invalid the system freezes.
		if (!ret) {
			printf("FAILURE AT STH CPSW DRIVER!\n");
			for (;;) ;
		}
		//Increment program counter to point to instruction following the
		//failing one.
		curr_vm->current_mode_state->ctx.pc += 4;

	}
	//Returns to exception_bottom which restores the guest to exeucte the
	//instruction following the failed one.
	else {
		if (addr >= 0xc0000000)
			printf("Dabort:%x Status:%x, u=%x \n", addr, status, unused);
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
		dabt_handler++;	//DABT_USR located +4
	}

	curr_vm->current_mode_state->ctx.pc = *dabt_handler;
#endif
#if defined(LINUX) && defined(CPSW)
	}
#endif
	domac = curr_vm->current_mode_state->mode_config->domain_ac;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
	return RV_OK;
}

return_value irq_handler(uint32_t irq, uint32_t r1, uint32_t r2)
{
	uint32_t domac = HC_DOMAC_ALL;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
//      printf("IRQ handler called %d\n", irq);
	if (curr_vm->current_mode_state->ctx.psr & 0x80) {	/*Interrupts are off, return */

		printf("AVBROTT: FREEZE!\n");
		for (;;) ;

//              mask_interrupt(irq, 1); //Mask interrupt and mark pending
	}
	else{
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

		curr_vm->current_mode_state->ctx.reg[0] = irq;
		curr_vm->current_mode_state->ctx.reg[1] =
		    curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;
		curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[5] = (uint32_t) curr_vm->mode_states[HC_GM_KERNEL].ctx.psr;	/*spsr in r5 for linux kernel vector */

		uint32_t *irq_handler = (uint32_t *) (curr_vm->exception_vector[V_IRQ]);

		if (interrupted_mode == HC_GM_TASK)
			irq_handler++;	//IRQ_USR located +4

		curr_vm->current_mode_state->ctx.pc = *irq_handler;
		curr_vm->current_mode_state->ctx.psr |= IRQ_MASK;
		curr_vm->current_mode_state->ctx.sp =
		    curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;

#endif
	//      unmask_interrupt(irq, 0);
	}
	domac = curr_vm->current_mode_state->mode_config->domain_ac;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
	return RV_OK;
}

/*Used for floating point emulation in Linux*/
return_value undef_handler(uint32_t instr, uint32_t unused, uint32_t addr)
{
	uint32_t domac = HC_DOMAC_ALL;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
#if 1
	printf("Undefined abort\n Address:%x Instruction:%x \n", addr, instr);
	uint32_t l1_base_add;
	COP_READ(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, l1_base_add);
	dump_mmu(mmu_guest_pa_to_va(l1_base_add, curr_vm->config));

	printf("Undefined abort\n Address:%x Instruction:%x \n", addr, instr);
	uint32_t value = *((uint32_t *)addr);
	printf("value:%x requests:%d\n", value, curr_vm->pending_request_counter);

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
	domac = curr_vm->current_mode_state->mode_config->domain_ac;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
	return RV_OK;
}

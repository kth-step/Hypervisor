#include "hw.h"
#include "hyper.h"
#include "dmmu.h";

extern virtual_machine *curr_vm;
extern uint32_t *flpt_va;
extern uint32_t *slpt_va;

/* Change hypervisor guest mode. Domain AP and page AP will change .*/
void change_guest_mode(uint32_t mode)
{
	uint32_t domac;
	curr_vm->current_mode_state = &curr_vm->mode_states[mode];
	cpu_context_current_set(&(curr_vm->current_mode_state->ctx));
	curr_vm->current_guest_mode = mode;
	domac = curr_vm->current_mode_state->mode_config->domain_ac;
	COP_WRITE(COP_SYSTEM, COP_SYSTEM_DOMAIN, domac);
}

uint32_t boot = 0;

/* Guest use this hypercall to give information about the kernel attributes
 * and also gets hardware information from the hypervisor*/
void hypercall_guest_init(boot_info * info)
{
	uint32_t size = sizeof(boot_info);
	if (boot != 0)
		hyper_panic("Guest tried to set boot info twice\n", 1);
	if (((uint32_t) info < 0xC0000000)
	    || ((uint32_t) info > (uint32_t) (HAL_VIRT_START - size)))
		hyper_panic("Pointer given does not reside in kernel space\n",
			    1);

	boot++;
	uint32_t cpuid = 0, mmf = 0, cr = 0;
	COP_READ(COP_SYSTEM, COP_ID_CPU, cpuid);
	COP_READ(COP_SYSTEM, COP_ID_MEMORY_MODEL_FEAT, mmf);
	COP_READ(COP_SYSTEM, COP_SYSTEM_CONTROL, cr);

	info->cpu_id = cpuid;
	info->cpu_mmf = mmf;
	info->cpu_cr = cr;

	curr_vm->guest_info.nr_syscalls = (uint32_t) info->guest.nr_syscalls;
	curr_vm->guest_info.page_offset = info->guest.page_offset;
	curr_vm->guest_info.phys_offset = info->guest.phys_offset;
	curr_vm->guest_info.vmalloc_end = info->guest.vmalloc_end;
	curr_vm->guest_info.guest_size = info->guest.guest_size;

	/*Check if the exception vector address are within allowed values */
	int i, address;
	for (i = 0; i < 10; i++) {
		address = (uint32_t) info->guest.vector_table[i];
		if (((address < 0xC0000000) && address != 0)
		    || (address >
			(uint32_t) (HAL_VIRT_START - sizeof(uint32_t))))
			hyper_panic
			    ("Pointer given does not reside in kernel space\n",
			     1);

		curr_vm->exception_vector[i] = info->guest.vector_table[i];
	}

#ifdef LINUX
	dmmu_clear_linux_mappings();
#endif

}

void hypercall_restore_regs(uint32_t * regs)
{
	if ((uint32_t) regs <= 0xC0000000)
		hyper_panic("Pointer given reside in user space\n", 1);

	if ((uint32_t) regs > HAL_VIRT_START
	    && (uint32_t) regs < HAL_VIRT_START + MAX_TRUSTED_SIZE)
		hyper_panic
		    ("Pointer given reside in hypervisor or trusted space", 1);

	uint32_t *context;
	uint32_t i = 17;
	context = &curr_vm->current_mode_state->ctx.reg[0];

	while (i > 0) {
		*context++ = *regs++;
		i--;
	}

}

/* Linux context switches are very fast and to maintain its speed,
 * this function is adapted to the context switching system of Linux.
 * Not portable to other guest OS */
void hypercall_restore_linux_regs(uint32_t return_value, BOOL syscall)
{
	uint32_t offset = 0, mode, i, kernel_space = 0, stack_pc;
	uint32_t *sp, *context;
	if (syscall)
		offset = 8;

	sp = (uint32_t *) curr_vm->mode_states[HC_GM_KERNEL].ctx.sp;
	/*when in syscall mode, it means its returning from a systemcall which means we have 2 extra registers
	 *for systemcall arguments and we have to add a offset */
	mode = *((uint32_t *) (sp + 16 + (offset / 4)));	//PSR register
	stack_pc = *((uint32_t *) (sp + 15));	//pc register
	if ((mode & 0x1F) == 0x10)
		kernel_space = 0;
	else if ((mode & 0x1F) == 0x13)
		kernel_space = 1;
	else {
		printf("In %s with mode is set to %x stack:%x\n", __func__,
		       (mode & 0x1F), sp);
		dump_L1pt(curr_vm);
		dump_L2pt(0x88009000, curr_vm);
		hyper_panic("Unknown mode, halting system", 1);
	}

#if 0
	/*Kuser helper is from user space */
	if (stack_pc < 0xC0000000
	    || (stack_pc < 0xFFFF1000 && stack_pc >= 0xFFFF0FA0))
		kernel_space = 0;
	else
		kernel_space = 1;
#endif
	/*Virtual kernel mode */
	if (kernel_space) {

		//debug("Switching to KERNEL mode!\n");
		context = &curr_vm->mode_states[HC_GM_KERNEL].ctx.reg[0];
		i = 13;

		/*Restore register r0-r12, reuse sp and lr
		 * (same in virtual kernel mode)*/

		while (i > 0) {
			*context++ = *sp++;
			i--;
		}
		*++context = *++sp;	//this sets LR, jumps over SP which is already set

		/*Code originaly run in SVC mode, however only make sure it
		 * can run in virtual kernel mode*/
		curr_vm->mode_states[HC_GM_KERNEL].ctx.psr = mode;
		curr_vm->mode_states[HC_GM_KERNEL].ctx.pc = stack_pc;

		/*Adjust kernel stack pointer */
		curr_vm->mode_states[HC_GM_KERNEL].ctx.sp += (18 * 4);	// Frame size
		change_guest_mode(HC_GM_KERNEL);
	}
	/*Virtual user mode */
	else if (!(kernel_space)) {
		//debug("Switching to USER mode!\n");
		if (syscall) {	//this mean skip r0
			curr_vm->mode_states[HC_GM_TASK].ctx.reg[0] =
			    return_value;
			context = &curr_vm->mode_states[HC_GM_TASK].ctx.reg[1];
			i = 15;	//saves r1-pc
			sp += 3;	//adjust sp to skip arg 4, 5 and r0
		} else {
			context =
			    (uint32_t *) & curr_vm->mode_states[HC_GM_TASK].ctx;
			i = 16;	//saves r0-pc
		}

		/*Restore register user registers r0/r1-lr */

		while (i > 0) {
			*context++ = *sp++;
			i--;
		}

		curr_vm->mode_states[HC_GM_TASK].ctx.psr = mode;

		/*Adjust kernel stack pointer */
		curr_vm->mode_states[HC_GM_KERNEL].ctx.sp += (18 * 4 + offset);	// Frame size + offset (2 swi args)
		change_guest_mode(HC_GM_TASK);
	}
}

//      ERROR HANDLING CODE -----------------------------------

void terminate(uint32_t number)
{
	printf("\n Hypervisor terminated with error code: %i\n", number);
	while (1) ;		//get stuck here
}

void hypercall_num_error(uint32_t hypercall_num)
{
	uint32_t addr = (curr_vm->current_mode_state->ctx.pc - 4);
	printf("Unknown hypercall %d originated at 0x%x, aborting",
	       hypercall_num, addr);
	terminate(1);
}

void hyper_panic(char *msg, uint32_t exit_code)
{
	printf("\n\n***************************************\n"
	       "HYPERVISOR PANIC: \n"
	       "%s\n" "***************************************\n", msg);
	terminate(exit_code);
}

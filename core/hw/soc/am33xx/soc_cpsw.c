//Includes types declarations.
#include <types.h>
//Includes declarations for data structures related to the data structure virtual_machine.
#include <hyper.h>
//Includes declarations for the macros of block types, the data structure dmmu_entry_t and the function get_bft_entry_by_block_idx.
#include <dmmu.h>

//printf declaration.
//extern void printf();

//Reference to externally (relative to this file) defined virtual machine.
//Used to access registers of the guest that the guest might refer to at when
//accessing a register.
extern virtual_machine *curr_vm;

//#define PRINT_QUEUE

//The number to add to physical address in the Ethernet Subsystem to get the
//corresponding virtual address.
#define CPSW_OFFSET (0xFA400000 - 0x4A100000)
//Returns the virtual address corresponding to the physical address that
//belongs to the Ethernet Subsystem.
#define phys_to_virt(phys) (phys + CPSW_OFFSET)
//Returns the physical address corresponding to the virtual address that
//belongs to the Ethernet Subsystem.
#define virt_to_phys(virt) (virt - CPSW_OFFSET)

//Maximum length of a new buffer descriptor queue to add for transmission or
//reception. 256 buffer descriptors * 4 words per buffer descriptor * 4 bytes
//per word = 4KB. That is the half of CPPI_RAM. One half for transmission and
//one half for reception.
#define MAX_QUEUE_LENGTH 256

//Accesses the value at physical address phys which is of type uint32_t *.
#define word_at_phys_addr(phys) (*((uint32_t *) phys_to_virt(phys)))
//Accesses the value at physical address phys which is of type uint32_t *.
#define word_at_virt_addr(virt) (*((uint32_t *) virt))

//Bit 31 is the SOP bit in a buffer descriptor.
#define SOP (1 << 31)
//Bit 30 is the EOP bit in a buffer descriptor.
#define EOP (1 << 30)
//Bit 29 is the OWNER bit in a buffer descriptor.
#define OWNER (1 << 29)
//Bit 28 is the EOQ bit in a buffer descriptor.
#define EOQ (1 << 28)
//Bit 27 is the Teardown bit in a buffer descriptor.
#define TD (1 << 27)
//Bit 26 is the CRC bit in a buffer descriptor.
#define CRC (1 << 26)

//Bits [10:0] is the Buffer Length field for reception buffer descriptors.
#define RX_BL 0x7FF
//Bits [26:16] is the Buffer Offset field for receive buffer descriptors.
#define RX_BO (0x7FF << 16)

//Bits [15:0] is the Buffer Length field for transmission buffer descriptors.
#define TX_BL 0xFFFF
//Bits [26:16] is the Buffer Offset field for transmission buffer descriptors.
#define TX_BO (0xFFFF << 16)

//Bits [10:0] is the Packet Length field for buffer descriptors.
#define PL 0x7FF

//The Next Descriptor Pointer has an offset 0 from the start of a buffer descriptor.
#define NEXT_DESCRIPTOR_POINTER 0
//The Buffer Pointer has an offset 4 from the start of a buffer descriptor.
#define BUFFER_POINTER 4
//The Buffer Offset and Buffer Length word has an offset 8 from the start of a buffer descriptor.
#define BOBL 8
//The word with flags has an offset 12 from the start of a buffer descriptor.
#define FLAGS 12

//Macros for retrieving the content of a certain word of a buffer descriptor with physical address physical_address.
#define get_next_descriptor_pointer(physical_address) (word_at_phys_addr(physical_address + NEXT_DESCRIPTOR_POINTER))
#define get_buffer_pointer(physical_address) (word_at_phys_addr(physical_address + BUFFER_POINTER))
#define get_buffer_offset_and_length(physical_address) (word_at_phys_addr(physical_address + BOBL))
#define get_flags(physical_address) (word_at_phys_addr(physical_address + FLAGS))
#define get_rx_buffer_length(physical_address) (word_at_phys_addr(physical_address + BOBL) & RX_BL)
#define get_tx_buffer_length(physical_address) (word_at_phys_addr(physical_address + BOBL) & TX_BL)
#define get_packet_length(physical_address) (word_at_phys_addr(physical_address + FLAGS) & PL)

//Macros returning truth values depending on flag values of the given physical buffer descriptor address.
#define is_sop(physical_address) ((word_at_phys_addr(physical_address + 12) & SOP) != 0)
#define is_eop(physical_address) ((word_at_phys_addr(physical_address + 12) & EOP) != 0)
#define is_released(physical_address) ((word_at_phys_addr(physical_address + 12) & OWNER) == 0)
#define is_eoq(physical_address) ((word_at_phys_addr(physical_address + 12) & EOQ) != 0)
#define is_td(physical_address) ((word_at_phys_addr(physical_address + 12) & TD) != 0)

//Macro that checks word alignment.
#define is_word_aligned(address) ((address & 0x3) == 0)

//Teardown acknowledgement value.
#define TD_INT 0xFFFFFFFC

//Macros for classifying the type of queue.
#define TRANSMIT TRUE
#define RECEIVE FALSE

//Macros used for adding/removing a buffer descriptor to/from an active queue and marking the update in alpha.
#define ADD TRUE
#define REMOVE FALSE

//Macros used when manipulating SOP or EOP buffer descriptors.
#define SOP_BD TRUE
#define EOP_BD FALSE

//Macros for NIC register accesses:
#define DMACONTROL_PHYSICAL_ADDRESS 0x4A100820
#define is_dmacontrol_virtual_address(address) (address == phys_to_virt(DMACONTROL_PHYSICAL_ADDRESS))
#define DMACONTROL (word_at_phys_addr(DMACONTROL_PHYSICAL_ADDRESS))

#define CPDMA_SOFT_RESET_PHYSICAL_ADDRESS 0x4A10081C
#define is_cpdma_soft_reset_virtual_address(address) (address == phys_to_virt(CPDMA_SOFT_RESET_PHYSICAL_ADDRESS))
#define CPDMA_SOFT_RESET (word_at_phys_addr(CPDMA_SOFT_RESET_PHYSICAL_ADDRESS))

#define is_hdp_register_virtual_address(address) (phys_to_virt(0x4A100A00) <= address && address < phys_to_virt(0x4A100A40))

#define TX0_HDP_PHYSICAL_ADDRESS 0x4A100A00
#define is_tx0_hdp_virtual_address(address) (address == phys_to_virt(TX0_HDP_PHYSICAL_ADDRESS))
#define TX0_HDP (word_at_phys_addr(TX0_HDP_PHYSICAL_ADDRESS))

#define RX0_HDP_PHYSICAL_ADDRESS 0x4A100A20
#define is_rx0_hdp_virtual_address(address) (address == phys_to_virt(RX0_HDP_PHYSICAL_ADDRESS))
#define RX0_HDP (word_at_phys_addr(RX0_HDP_PHYSICAL_ADDRESS))

#define is_cp_register_virtual_address(address) (phys_to_virt(0x4A100A40) <= address && address < phys_to_virt(0x4A100A80))

#define TX0_CP_PHYSICAL_ADDRESS 0x4A100A40
#define is_tx0_cp_virtual_address(address) (address == phys_to_virt(TX0_CP_PHYSICAL_ADDRESS))
#define TX0_CP (word_at_phys_addr(TX0_CP_PHYSICAL_ADDRESS))

#define RX0_CP_PHYSICAL_ADDRESS 0x4A100A60
#define is_rx0_cp_virtual_address(address) (address == phys_to_virt(RX0_CP_PHYSICAL_ADDRESS))
#define RX0_CP (word_at_phys_addr(RX0_CP_PHYSICAL_ADDRESS))

#define TX_TEARDOWN_PHYSICAL_ADDRESS 0x4A100808
#define is_tx_teardown_virtual_address(address) (address == phys_to_virt(TX_TEARDOWN_PHYSICAL_ADDRESS))
#define TX_TEARDOWN (word_at_phys_addr(TX_TEARDOWN_PHYSICAL_ADDRESS))

#define RX_TEARDOWN_PHYSICAL_ADDRESS 0x4A100818
#define is_rx_teardown_virtual_address(address) (address == phys_to_virt(RX_TEARDOWN_PHYSICAL_ADDRESS))
#define RX_TEARDOWN (word_at_phys_addr(RX_TEARDOWN_PHYSICAL_ADDRESS))

#define CPPI_RAM_START_PHYSICAL_ADDRESS 0x4A102000
#define CPPI_RAM_END_PHYSICAL_ADDRESS 0x4A104000
#define is_cppi_ram_virtual_address(address) (phys_to_virt(CPPI_RAM_START_PHYSICAL_ADDRESS) <= address && address < phys_to_virt(CPPI_RAM_END_PHYSICAL_ADDRESS))
#define CPPI_RAM(physical_address) (word_at_phys_addr(physical_address))

#define RX_BUFFER_OFFSET_PHYSICAL_ADDRESS 0x4A100828
#define is_rx_buffer_offset_virtual_address(address) (address == phys_to_virt(RX_BUFFER_OFFSET_PHYSICAL_ADDRESS))
#define RX_BUFFER_OFFSET (word_at_phys_addr(RX_BUFFER_OFFSET_PHYSICAL_ADDRESS))

//Maximum value of rho_nic.
#define MAX_RHO_NIC 15

//Enums used to denote what sort of overlap a CPPI_RAM does with respect to a queue.
typedef enum OVERLAP { ZEROED_NEXT_DESCRIPTOR_POINTER_OVERLAP, ILLEGAL_OVERLAP, NO_OVERLAP} bd_overlap;

//Physical addresses of buffer descriptors of the hypervisor's view of where
//the transmitter and receiver are in processing buffer descriptors. That is,
//the hypervisor's view of the active queues. They are updated on every
//CPPI_RAM access to give an accurate view of the current state of CPPI_RAM so
//that an access is not classified as accessing a buffer descriptor in use when
//the buffer descriptor in queustion has actually been released by the NIC
//hardware.
//
//tx0_active_queue and rx0_active_queue always point to a SOP.
static uint32_t tx0_active_queue = 0, rx0_active_queue = 0;
static BOOL initialized = FALSE, tx0_hdp_initialized = FALSE, rx0_hdp_initialized = FALSE, tx0_cp_initialized = FALSE, rx0_cp_initialized = FALSE, tx0_tearingdown = FALSE, rx0_tearingdown = FALSE;

//For each 32-bit word aligned word in CPPI_RAM, true means that word is a part of an active queue, and false not.
#define alpha_SIZE 64
//This data structure assumes that no buffer descriptors overlap.
//By the definition of the C language is alpha initialized to contain only zeros.
static uint32_t alpha[alpha_SIZE];

//The data structure recv_bd_nr_blocks records for each buffer descriptor how many blocks it accesses.
#define recv_bd_nr_blocks_SIZE 2048
//By the definition of the C language is recv_bd_nr_blocks initialized to contain only zeros.
static uint32_t recv_bd_nr_blocks[recv_bd_nr_blocks_SIZE];
//Used to index recv_bd_nr_blocks from a word aligned address.

//Input is a word aligned address in CPPI_RAM. The result is true if and only if that word is in used by the NIC, as seen by reading that word's bit in alpha.
/*
 1. WORD_INDEX := (CPPI_RAM_WORD - CPPI_RAM_START) >> 2
 2. uint32_t_index := WORD_INDEX / 32
 3. bit_offset := WORD_INDEX % 32
 4. (alpha[uint32_t_index] & (1 << bit_offset)) != 0

Optimized:
 1. uint32_t_index := ((CPPI_RAM_WORD - CPPI_RAM_START) >> 7)
 2. bit_offset := (((CPPI_RAM_WORD - CPPI_RAM_START) >> 2) & 0x1F)
 3. (alpha[(CPPI_RAM_WORD - CPPI_RAM_START) >> 7] & (1 << (((CPPI_RAM_WORD - CPPI_RAM_START) >> 2) & 0x1F)))
*/
#define IS_ACTIVE_CPPI_RAM(CPPI_RAM_WORD) (alpha[(CPPI_RAM_WORD - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 7] & (1 << (((CPPI_RAM_WORD - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 2) & 0x1F)))

//Input is a word aligned address in CPPI_RAM. The corresponding bit in alpha is set to 1.
#define SET_ACTIVE_CPPI_RAM(CPPI_RAM_WORD) (alpha[(CPPI_RAM_WORD - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 7] |= (1 << (((CPPI_RAM_WORD - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 2) & 0x1F)))

//Input is a word aligned address in CPPI_RAM. The corresponding bit in alpha is set to 0.
#define CLEAR_ACTIVE_CPPI_RAM(CPPI_RAM_WORD) (alpha[(CPPI_RAM_WORD - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 7] &= (~(1 << (((CPPI_RAM_WORD - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 2) & 0x1F))))

/*
 *  Local functions only used in this file.
 */
static BOOL stateram_handler(uint32_t);
static BOOL write_nic_register_handler(uint32_t, uint32_t);
static BOOL dmacontrol_handler(uint32_t);
static BOOL cpdma_soft_reset_handler(uint32_t);
static BOOL tx0_hdp_handler(uint32_t);
static BOOL rx0_hdp_handler(uint32_t);
static BOOL tx0_cp_handler(uint32_t);
static BOOL rx0_cp_handler(uint32_t);
static BOOL tx_teardown_handler(uint32_t);
static BOOL rx_teardown_handler(uint32_t);
static BOOL cppi_ram_handler(uint32_t, uint32_t);
static BOOL rx_buffer_offset_handler(uint32_t);

static void initialization_performed(void);
static void update_active_queue(BOOL);
static void handle_potential_misqueue_condition(BOOL, uint32_t, uint32_t);
static void decrement_rho_nic_update_alpha_queue(uint32_t, BOOL, BOOL);
static void decrement_rho_nic_update_alpha(uint32_t, BOOL, BOOL);
static void decrement_rho_nic(uint32_t);
static BOOL no_overflow_and_updated_rho_nic(uint32_t);
static BOOL is_queue_secure(uint32_t, BOOL);
static BOOL is_valid_length_in_cppi_ram_alignment_no_active_queue_overlap(uint32_t);
static BOOL is_queue_self_overlap(uint32_t bd_ptr);
static BOOL is_transmit_SOP_EOP_packet_length_fields_set_correctly(uint32_t);
static BOOL is_data_buffer_secure_queue(uint32_t, BOOL);
static BOOL is_data_buffer_secure(uint32_t, BOOL);
static BOOL is_secure_linux_memory(BOOL, uint32_t, uint32_t);
static BOOL inline is_L1_or_L2_block(uint32_t);
static BOOL inline is_D_block(uint32_t);
static BOOL inline is_executable_block(uint32_t);
static bd_overlap type_of_cppi_ram_access_overlap(uint32_t, uint32_t);
static void set_and_clear_word(uint32_t, uint32_t, uint32_t, uint32_t);
static void set_and_clear_word_on_sop_or_eop(uint32_t, uint32_t, uint32_t,
					     uint32_t, BOOL);
#if defined(PRINT_QUEUE)
static void print_queue(uint32_t);
#endif

#if 0
uint32_t read_register_at_physical_address(uint32_t physical_address)
{
	return word_at_phys_addr(physical_address);
}

void write_register_at_physical_address(uint32_t physical_address,
					uint32_t value)
{
	word_at_phys_addr(physical_address) = value;
}

BOOL rx_int_core = FALSE;
BOOL rx_int_dma = FALSE;
BOOL rx_dma = FALSE;
#endif

/*
 *  @accessed_va: Virtual address of accessed word that belongs to the Ethernet
 *  Subsystem. That is, its physical address is in [0x4A100000, 0x4A104000).
 *
 *  @instruction_va: Virtual address of the instruction that accessed virtual
 *  address @accessed_va.
 *
 *  Function: Handles an aborted instruction that tried to write a register in
 *  the memory belonging to the Ethernet Subsystem.
 *
 *  Returns: True if and only if the intended operations was actually executed
 *  and if it was successful.
 */
BOOL soc_check_cpsw_access(uint32_t accessed_va, uint32_t instruction_va)
{
	uint32_t instruction_encoding = *((uint32_t *) instruction_va);
	uint32_t *context, t, n, imm, rt, rn;

	//If the accessed address according to the MMU is not word aligned, then
	//this is a fault.
	if (!is_word_aligned(accessed_va)) {
		printf("STH CPSW: ACCESSED ADDRESS IS NOT WORD ALIGNED!");
		return FALSE;
	}
	//Checks the type of instruction that was executed and takes appropriate
	//actions.
	switch (0xFFF00000 & instruction_encoding) {
		//STR  Rt, [Rn, #+imm32] = mem32[Regs[Rn] + imm32] := Regs[Rt]
	case 0xE5800000:
		//Retrieves the source register indexes and the store address
		//offset.
		t = (0x0000F000 & instruction_encoding) >> 12;
		n = (0x000F0000 & instruction_encoding) >> 16;
		imm = 0x00000FFF & instruction_encoding;

		//Retrieves the contents of the registers that are used when
		//the instruction is to be re-executed. The address base register
		//is set to its original value added with the address offset.
		context = curr_vm->current_mode_state->ctx.reg;
		rt = *(context + t);
		rn = *(context + n) + imm;

		//If the memory location to store a value to differs from what was
		//reported by the MMU, then there is some bug.
		if (rn != accessed_va) {
			printf
			    ("STH CPSW ERROR: Base register Regs[R%d] = %x distinct "
			     "from accessed address accessed_va = %x\n", n,
			     rn, accessed_va);
			return FALSE;
		}
		//-----------------------------------------------------------------
		//Now it is known that the address to access is word aligned.
		//-----------------------------------------------------------------

///////////////////
#if 0
//      write_register_at_physical_address(0x4A100800 + 0x14, 1);    //DMA RX CONTROL, Forbid all but 1
//      write_register_at_physical_address(0x4A100800 + 0xAC, 0xFF);  //RX_INTMASK_CLEAR, Forbid everything
//      write_register_at_physical_address(0x4A101200 + 0x14, 0xFF);  //Core enable, Forbid all but FF

		if (phys_to_virt(0x4A100814) == rn && rt == 0) {
			printf("STH CPSW: PREVENTED DMA RX CONTROL DISABLE\n");
			rx_dma = FALSE;
			return TRUE;
		} else if (phys_to_virt(0x4A100814) == rn && rt == 1) {
			rx_dma = TRUE;
			printf("STH CPSW: INTERVEANED DMA RX CONTROL ENABLE\n");
		} else if (phys_to_virt(0x4A1008A8) == rn) {
			printf("STH CPSW: INTERVEANED RX INTERRUPT ENABLE\n");
			rx_int_dma = TRUE;
		} else if (phys_to_virt(0x4A1008AC) == rn) {
			printf("STH CPSW: PREVENTED RX INTERRUPT DISABLE\n");
			rx_int_dma = FALSE;
			return TRUE;
		} else if (phys_to_virt(0x4A101214) == rn && rt == 0) {
			printf
			    ("STH CPSW: PREVENTED CORE RX INTERRUPT DISABLE\n");
			rx_int_core = FALSE;
			return TRUE;
		} else if (phys_to_virt(0x4A101214) == rn && rt == 0xFF) {
			printf
			    ("STH CPSW: INTERVEANED CORE RX INTERRUPT ENABLE\n");
			rx_int_core = TRUE;
		}
#endif
///////////////////

		/*
		 *  If the accessed word is outside CPPI_RAM, the Head Descriptor
		 *  Pointer registers, the CPDMA Logic reset register, and the
		 *  CPDMA teardown registers, then the instruction is simply
		 *  re-executed.
		 *
		 *  The undagerous registers are tested first since it is assumed
		 *  that that is the most common case.
		 */
		if (is_cppi_ram_virtual_address(rn))	//CPPI_RAM
			return cppi_ram_handler(virt_to_phys(rn), rt);
		else if (is_tx0_cp_virtual_address(rn))	//TX0_CP
			return tx0_cp_handler(rt);
		else if (is_rx0_cp_virtual_address(rn))	//RX0_CP
			return rx0_cp_handler(rt);
		else if (is_tx0_hdp_virtual_address(rn))	//TX0_HDP
			return tx0_hdp_handler(rt);
		else if (is_rx0_hdp_virtual_address(rn))	//RX0_HDP
			return rx0_hdp_handler(rt);
		else if (is_cpdma_soft_reset_virtual_address(rn))	//CPDMA_SOFT_RESET
			return cpdma_soft_reset_handler(rt);
		else if (is_dmacontrol_virtual_address(rn))	//DMACONTROL
			return dmacontrol_handler(rt);
		else if (is_tx_teardown_virtual_address(rn))	//TX_TEARDOWN
			return tx_teardown_handler(rt);
		else if (is_rx_teardown_virtual_address(rn))	//RX_TEARDOWN
			return rx_teardown_handler(rt);
		else if (is_rx_buffer_offset_virtual_address(rn))	//RX_BUFFER_OFFSET
			return rx_buffer_offset_handler(rt);
		else if (is_hdp_register_virtual_address(rn))	//Tests T/X[i]_HDP := 0, must be after the T/RX0_HDP test above.
			return stateram_handler(rt);
		else if (is_cp_register_virtual_address(rn))	//Tests T/X[i]_CP := 0, must be after the T/RX0_CP test above.
			return stateram_handler(rt);
		else		//The rest of the registers.
			return write_nic_register_handler(rn, rt);

		printf("STH CPSW ERROR: NOT REACHABLE CODE!\n");
		return FALSE;
		break;
	default:
		printf("STH CPSW ERROR: UNKNOWN INSTRUCTION TYPE WHEN ACCESSING CPSW REGISTER!\n");
		return FALSE;
		break;
	}

	printf("STH CPSW ERROR: UNREACHABLE POINT WAS REACHED IN HYPERVISOR CPSW DRIVER!\n");
	return FALSE;
}

/*
 *  Purpose: Letting the guest initialize the TX[i]_HDP, RX[i]_HDP, TX[i]_CP
 *  and RX[i]_CP registers that are not used: 0 < i < 8.
 */
static BOOL stateram_handler(uint32_t val)
{
	if (val != 0 || !initialized)
		return FALSE;
	else
		return TRUE;
}

/*
 *  @val: The value to write the DMACONTROL register.
 *
 *  Function: Checks that the write to DMACONTROL is secure.
 *
 *  THE RX_OWNERSHIP BIT MUST NOT BE CHANGED SINCE CORRECT UPDATES OF
 *  rx0_active_queue DEPENDS ON IT.
 *
 *  @return: True if and only if the write was performed.
 */
static BOOL dmacontrol_handler(uint32_t val)
{
	if (!initialized || val != 0)
		return FALSE;
	else
		return TRUE;
}

/*
 *  Function: Executes *rn := rt.
 */
static BOOL write_nic_register_handler(uint32_t rn, uint32_t rt)
{
	//Syntax:
	//*asm is for inline assembly
	//*volatile is to tell the compiler not to optimize this code
	//*Character string for assembly code to execute. Operands are
	// referenced by a percent sign followed by a single digit. %0
	// refers to the first, %1 to the second operand and so forth,
	// where the operands are referenced with the output operand
	// list first.
	//*Output operand list. In this case the list is empty.
	//*Input operand list. Each input and output operand is
	// described by a constraint string followed by a C expression
	// in parantheses. 'r' is for a general register and constraint
	// characters may be prepended by a single constraint modifier.
	// Contraints without a modifier specify read-only operands,
	// which applies in this case.
	//*Clobber list which tells the compiler which modifications
	// this code does. If registers are used, which had not been
	// passed as operands, this information must be provided by
	// adding those registers to the clobber list. The 'memory'
	// This special clobber informs the compiler that the assembler
	// code may modify any memory location. It forces the compiler
	// to update all variables for which the contents are currently
	// held in a register before executing the assembler code. And
	// of course, everything has to be reloaded again after this
	// code.
	////STR  Rt, [Rn, #+imm32] = mem32[Regs[R2] + 4] := Regs[R1]
/*        asm volatile("mov R0, %0      \n\t"
         "mov R1, %1      \n\t"
         "str R0, [R1]      \n\t"
         ::"r" (rt), "r" (rn) : "memory", "r0", "r1");
*/
	/*
	 *  Since the Ethernet Subsystem memory region is mapped with
	 *  TEX[2:0] = 000, C = 0 and B = 0, this means that the mapped memory
	 *  region is classified as Strongly-ordered. Strongly-ordered memory means
	 *  that "Address locations marked as Device or Strongly-ordered are never
	 *  held in a cache." This can be seen in
	 *  core/hw/cpu/arm/arm_common/arm_mmu_pt.c:pt_create_coarse at the label
	 *  MLT_IO_RO_REG where flags is set to 1 and at the assignment of pte.
	 *
	 *  Hence no volatile is needed but is used just in case.
	 */
	/*
	 *  volatile uint32_t * means that rn is casted to a type that
	 *  points to an unsigned 32 bit integer which has the
	 *  qualifier volatile. This means that the access to the value
	 *  pointed to by rn will not be optimized, and hence the
	 *  writing will occur.
	 */
	*(( /*volatile */ uint32_t *) rn) = rt;

	return TRUE;
}

/*
 *  @val: The value to set the reset bit to.
 *
 *  Function: Sets the set reset bit in the CPDMA_SOFT_RESET register.
 *
 *  Returns: True if and only if the writing the reset bit succeeded.
 */
static BOOL cpdma_soft_reset_handler(uint32_t val)
{
	if ((val & 1) == 0)
		return TRUE;
	else if (!initialized || tx0_tearingdown || rx0_tearingdown)
		return FALSE;
	else {
		initialized = FALSE;
		tx0_hdp_initialized = FALSE;
		rx0_hdp_initialized = FALSE;
		tx0_cp_initialized = FALSE;
		rx0_cp_initialized = FALSE;
		CPDMA_SOFT_RESET = 1;	//Sets reset bit.
		return TRUE;
	}
}

/*
 *  @bd_ptr: Physical address of the buffer descriptor that shall be written to
 *  TX0_HDP.
 *
 *  Function: Sets TX0_HDP to @bd_ptr if deemed appropriate.
 *
 *  Returns: True if and only if TX0_HDP is securely set to @bd_ptr.
 */
static BOOL tx0_hdp_handler(uint32_t bd_ptr)
{
	if (!initialized) {	//Performing initialization.
		if (CPDMA_SOFT_RESET == 1 || bd_ptr != 0)
			return FALSE;
		else {
			TX0_HDP = 0;	//bd_ptr = 0
			tx0_hdp_initialized = TRUE;

			if (rx0_hdp_initialized && tx0_cp_initialized && rx0_cp_initialized)
				initialization_performed();

			return TRUE;
		}
	} else {		//Initialization is done.
		if (TX0_HDP != 0 || tx0_tearingdown)
			return FALSE;
		else {
			//In this case, the initialization, and transmit teardown NIC processes are idle.
			//Hence tx0_active_queue, and ACTIVE_CPPI_RAM are updated to zero for the
			//buffer descriptors in tx0_active_queue.
			update_active_queue(TRANSMIT);
			if (is_queue_secure(bd_ptr, TRANSMIT)) {
				tx0_active_queue = bd_ptr;
				decrement_rho_nic_update_alpha_queue(bd_ptr, TRANSMIT, ADD);
				TX0_HDP = bd_ptr;
				return TRUE;
			} else
				return FALSE;
		}
	}
}

/*
 *  @bd_ptr: Physical address of the buffer descriptor that shall be written to
 *  RX0_HDP.
 *
 *  Function: Sets RX0_HDP to @bd_ptr if deemed appropriate.
 *
 *  Returns: True if and only if RX0_HDP is securely set to @bd_ptr.
 */
static BOOL rx0_hdp_handler(uint32_t bd_ptr)
{
	if (!initialized) {	//Performing initialization.
		if (CPDMA_SOFT_RESET == 1 || bd_ptr != 0)
			return FALSE;
		else {
			RX0_HDP = 0;	//bd_ptr = 0
			rx0_hdp_initialized = TRUE;

			if (tx0_hdp_initialized && tx0_cp_initialized && rx0_cp_initialized)
				initialization_performed();

			return TRUE;
		}
	} else {
		if (RX0_HDP != 0 || rx0_tearingdown)
			return FALSE;
		else {
			//In this case, the initialization, and transmit teardown NIC processes are idle.
			//Hence rx0_active_queue, and alpha are updated to zero for the
			//buffer descriptors in rx0_active_queue.
			update_active_queue(RECEIVE);
			if (is_queue_secure(bd_ptr, RECEIVE)) {
				rx0_active_queue = bd_ptr;
				decrement_rho_nic_update_alpha_queue(bd_ptr, RECEIVE, ADD);
				RX0_HDP = bd_ptr;
				return TRUE;
			} else
				return FALSE;
		}
	}
}

/*
 *  Purpose: Prevent the NIC to be set into an undefined state. That is to
 *  prevent making the NIC go into a state that is not specified by the
 *  specification.
 *
 *  @val: The value to seto TX0_CP to.
 *
 *  Function: Sets TX0_CP to @value if deemed appropriate.
 *
 *  Returns: True if and only if TX0_CP is securely set to @value.
 */
static BOOL tx0_cp_handler(uint32_t val)
{
	if (!initialized) {
		if (CPDMA_SOFT_RESET == 1 || val != 0)
			return FALSE;
		else {
			TX0_CP = 0;	//val = 0.
			tx0_cp_initialized = TRUE;

			if (tx0_hdp_initialized && rx0_hdp_initialized && rx0_cp_initialized)
				initialization_performed();

			return TRUE;
		}
	} else {
		if (!tx0_tearingdown) {
			TX0_CP = val;
			return TRUE;
		}

		update_active_queue(TRANSMIT);	//If tx0_active_queue is 0, then is the teardown complete.
		if (tx0_tearingdown && tx0_active_queue == 0 && TX0_CP == TD_INT && TX0_HDP == 0 && val == TD_INT) {
			TX0_CP = TD_INT;	//val = TD_INT
			tx0_tearingdown = FALSE;
			return TRUE;
		} else
			return FALSE;
	}
}

/*
 *  Purpose: Prevent the NIC to be set into an undefined state. That is to
 *  prevent making the NIC go into a state that is not specified by the
 *  specification.
 *
 *  @val: The value to seto RX0_CP to.
 *
 *  Function: Sets RX0_CP to @value if deemed appropriate.
 *
 *  Returns: True if and only if RX0_CP is securely set to @value.
 */
static BOOL rx0_cp_handler(uint32_t val)
{
	if (!initialized) {
		if (CPDMA_SOFT_RESET == 1 || val != 0)
			return FALSE;
		else {
			RX0_CP = 0;
			rx0_cp_initialized = TRUE;

			if (tx0_hdp_initialized && rx0_hdp_initialized && tx0_cp_initialized)
				initialization_performed();

			return TRUE;
		}
	} else {
		if (!rx0_tearingdown) {
			RX0_CP = val;
			return TRUE;
		}

		update_active_queue(RECEIVE);
		if (rx0_tearingdown && rx0_active_queue == 0 && RX0_CP == TD_INT && RX0_HDP == 0 && val == TD_INT) {
			RX0_CP = TD_INT;
			rx0_tearingdown = FALSE;
			return TRUE;
		} else
			return FALSE;
	}
}

/*
 *  Purpose: Check valid behavior of the guest. Only channel zero is allowed to
 *  be used.
 *
 *  @channel: The DMA transmit channel to teardown.
 *
 *  Function: Initiate the teardown of DMA transmit channel zero.
 *
 *  Returns: True if and only if teardown was initiated for DMA transmit
 *  channel zero.
 */
static BOOL tx_teardown_handler(uint32_t channel)
{
	if (!initialized || tx0_tearingdown || (channel & 0x7) != 0)
		return FALSE;
	else {
		tx0_tearingdown = TRUE;
		TX_TEARDOWN = 0;	//Triggering the teardown for channel = 0.
		return TRUE;
	}
}

/*
 *  Purpose: Check valid behavior of the guest. Only channel zero is allowed to
 *  be used.
 *
 *  @channel: The DMA transmit channel to teardown.
 *
 *  Function: Initiate the teardown of DMA receive channel zero.
 *
 *  Returns: True if and only if teardown was initiated for DMA receive
 *  channel zero.
 */
static BOOL rx_teardown_handler(uint32_t channel)
{
	if (!initialized || rx0_tearingdown || (channel & 0x7) != 0)
		return FALSE;
	else {
		rx0_tearingdown = TRUE;
		RX_TEARDOWN = 0;	//Triggering the teardown for channel = 0.
		return TRUE;
	}
}

/*
 *  Purpose: Validity check of guest's access to CPPI_RAM. No non-guest memory
 *  is allowed to be accessed.
 *
 *  @pa: The physical address that is in the CPPI_RAM memory
 *  region that was tried to be written with the value @value.
 *
 *  @val: The value that the guest tried to write at physical address @pa.
 *
 *  Function: Making sure that the CPPI_RAM access cannot cause any disallowed
 *  memory accesses.
 *
 *  Returns: True if and only if the word @physical_address was set to @value.
 */
static BOOL cppi_ram_handler(uint32_t pa, uint32_t val)
{
	if (!initialized || tx0_tearingdown || rx0_tearingdown) {
		printf("INIT = %x | TX_TEARDOWN = %x | RX_TEARDOWN = %x\n", initialized, tx0_tearingdown, rx0_tearingdown);
		return FALSE;
	}
	//Updates the tx0_active_queue and rx0_active_queue variables. This gives
	//a snapshot of which transmit and receive buffer descriptors are in use by
	//the NIC.
	update_active_queue(TRANSMIT);
	update_active_queue(RECEIVE);

	//physical_address is checked to be word aligned and in CPPI_RAM (since it
	//is also word aligned) in the top-level function. If it does not overlap
	//an active buffer descriptor, then the value can be written at that
	//location.

	if (!IS_ACTIVE_CPPI_RAM(pa)) {
		word_at_phys_addr(pa) = val;
		return TRUE;
	}
	//Checks what kind of overlap the CPPI_RAM access made.
	bd_overlap transmit_overlap = type_of_cppi_ram_access_overlap(pa, tx0_active_queue);

	//An access cannot overlap both the active transmit and receive queue since
	//they are word aligned, the access is word aligned, and the transmit and
	//receive queues do not overlap.
	//
	//If a zeroed next descriptor pointer overlap was identified, then it is
	//checked if the appended queue is secure. If it is not secure, then false
	//is returned and if it is secure, then @value is written at
	//@physical_address and true is returned.
	//
	//If there was an illegal overlap, then false is returned.
	//
	//Otherwise the receive case is checked in a similar manner.
	if (transmit_overlap == ZEROED_NEXT_DESCRIPTOR_POINTER_OVERLAP) {
		if (is_queue_secure(val, TRANSMIT)) {
			decrement_rho_nic_update_alpha_queue(val, TRANSMIT, ADD);
			word_at_phys_addr(pa) = val;
			handle_potential_misqueue_condition(TRANSMIT, pa, val);
			return TRUE;
		}
	} else if (transmit_overlap == ILLEGAL_OVERLAP) {
		printf("STH CPSW: ILLEGAL TRANSMISSION OVERLAP!\n");
		return FALSE;
	} else {
		bd_overlap receive_overlap = type_of_cppi_ram_access_overlap(pa, rx0_active_queue);
		if (receive_overlap == ZEROED_NEXT_DESCRIPTOR_POINTER_OVERLAP && is_queue_secure(val, RECEIVE)) {
			decrement_rho_nic_update_alpha_queue(val, RECEIVE, ADD);
			word_at_phys_addr(pa) = val;
			handle_potential_misqueue_condition(RECEIVE, pa, val);
			return TRUE;
		} else {
			printf("STH CPSW: ILLEGAL RECEPTION OVERLAP!\n");
			return FALSE;
		}
	}
}

/*
 *  Purpose: Making sure that the RX_BUFFER_OFFSET register is not changed
 *  since the buffer length field of receive buffer descriptors must be greater
 *  than RX_BUFFER_OFFSET which might cause problems after data buffer memory
 *  accesses have been checked by is_data_buffer_secure().
 *
 *  @val: The value that the guest wants to set RX_BUFFER_OFFSET to.
 *
 *  Function: Checks that the guest does not try to changed the
 *  RX_BUFFER_OFFSET register to a value greater than zero.
 *
 *  Returns: True if and only if the NIC DMA hardware has been initialized and
 *  the value to write is zero.
 */
static BOOL rx_buffer_offset_handler(uint32_t val)
{
	//Only allow zero to be written for simplicity. Is used to guarantee that
	//nothing unspecified happens. The buffer offset field in receive buffer
	//descriptors must be greater than the value of this register. If this
	//RX_BUFFER_OFFSET register is changed after the data buffer memory
	//accesses check, it might affect the hardware operation if the buffer
	//length field is not greater than the RX_BUFFER_OFFSET register value.
	if (!initialized || val != 0)
		return FALSE;
	else
		return TRUE;
}

/*
 *  Function: Update the array containing information about the NIC usage
 *  status of CPPI_RAM words and the tx0/rx0_active_queue variables.
 */
static void initialization_performed(void)
{
	decrement_rho_nic_update_alpha_queue(tx0_active_queue, TRANSMIT, REMOVE);
	decrement_rho_nic_update_alpha_queue(rx0_active_queue, RECEIVE, REMOVE);
	tx0_active_queue = 0;
	rx0_active_queue = 0;
	initialized = TRUE;
}

/*
 *  Purpose: Update the snapshot view of where the NIC is with respect to
 *  transmission or reception.
 *
 *  Assumption: For transmission that the SOP and EOP bits are matching each
 *  other. For reception this is done by hardware.
 *
 *  @transmit: True if the tx0_active_queue shall be updated and false if the
 *  rx0_active_queue shall be updated.
 *
 *  Function: Update the hypervisor's view of which buffer descriptors are in
 *  use by the NIC, for transmission or reception depending on the value
 *  @transmit.
 *
 *  Returns: void.
 */
static void update_active_queue(BOOL transmit)
{
	uint32_t bd_ptr = transmit ? tx0_active_queue : rx0_active_queue;
	BOOL tearingdown = transmit ? tx0_tearingdown : rx0_tearingdown;
	BOOL released = TRUE, no_teardown = TRUE;

	//Exists a frame and its SOP buffer descriptor's Ownership bit has been
	//cleared.
	while (released && no_teardown && bd_ptr) {
		if (!is_released(bd_ptr))
			released = FALSE;
		//Checks teardown bit. If it is set then the buffer descriptors are
		//released and alpha must be updated.
		else if (is_td(bd_ptr) && tearingdown) {
			decrement_rho_nic_update_alpha_queue(bd_ptr, transmit, REMOVE);
			no_teardown = FALSE;
			bd_ptr = 0;
		} else {
			//Advances bd_ptr to point to the last buffer descriptor of the
			//current frame. That is one with EOP set.
			while (!is_eop(bd_ptr)) {
				decrement_rho_nic_update_alpha(bd_ptr, transmit, REMOVE);
				bd_ptr = get_next_descriptor_pointer(bd_ptr);
			}

			//Advances the buffer descriptor pointer to the next frame's SOP
			//buffer descriptor if there is one.
			decrement_rho_nic_update_alpha(bd_ptr, transmit, REMOVE);
			bd_ptr = get_next_descriptor_pointer(bd_ptr);
		}
	}

	//Updates the current head pointer to point to the first buffer
	//descriptor not released by the hardware for the queue.
//  printf("STH CPSW: update_active_queue, t/rx_active_queue = %x\n", i, bd_ptr);
	if (transmit)
		tx0_active_queue = bd_ptr;
	else
		rx0_active_queue = bd_ptr;
}

/*
 *  @transmit: True if and only if potential misqueue condition is to be
 *  considered for transmission. False for reception.
 *
 *  @last_bd_ptr: Physical address of the last buffer descriptor in the queue
 *	to be extended.
 *
 *  @new_queue: Physical address of the first buffer descriptor in the queue to be appended.
 *
 *  Function: Handles a misqueue condition if it has occurred.
 */
static void handle_potential_misqueue_condition(BOOL transmit, uint32_t last_bd_ptr, uint32_t new_queue)
{
	uint32_t bd_ptr = transmit ? tx0_active_queue : rx0_active_queue;
	uint32_t sop, eop = 0;

	//If no new queue is to be appended or there is no active queue, then can
	//no misqueue condition occur and nothing is done.
	if (new_queue == 0 || bd_ptr == 0)
		return;

	//If the buffer descriptor to write is a released SOP with no EOQ bit set,
	//then has no misqueue condition occurred.
	if ((is_sop(last_bd_ptr) && is_released(last_bd_ptr) && !is_eoq(last_bd_ptr)))
		return;
	//If the buffer descriptor to write is a released SOP with the EOQ bit set,
	//then has a misqueue condition occurred. Therefore is its EOQ bit cleared
	//to enable the simulation proof and the HDP register written with the new
	//queue.
	else if (is_sop(last_bd_ptr) && is_released(last_bd_ptr) && is_eoq(last_bd_ptr)) {
		word_at_phys_addr(last_bd_ptr + FLAGS) = (word_at_phys_addr(last_bd_ptr + FLAGS)) & (~EOQ);

		if (transmit)
			TX0_HDP = new_queue;
		else
			RX0_HDP = new_queue;

		return;
	}
	//Retrieves the SOP and EOP buffer descriptor addresses that belong to the
	//same frame that last_bd_ptr belong to. This is done by traversing the
	//transmission or reception buffer descriptor queue until the matching SOP
	//is found or a non-released SOP is encountered.
	while (eop != last_bd_ptr)
		//If a non-released SOP is encountered before the relevant SOP is
		//found, then can no misqueue condition have occurred.
		if (!is_released(bd_ptr))
			return;
	//Otherwise is the current SOP recorded in sop and its matching EOP in
	//eop. If the EOP matches the buffer descriptor to be written, as
	//tested in the while-test above for the next iteration, then is the
	//loop terminated. Then can it be tested whether the matching SOP of
	//the buffer descriptor that was written is released and whether the
	//EOQ bit of the written buffer descriptor has a valid meaning and
	//whether it is set.
		else {
			sop = bd_ptr;
			while (!is_eop(bd_ptr))
				bd_ptr = get_next_descriptor_pointer(bd_ptr);
			eop = bd_ptr;

			bd_ptr = get_next_descriptor_pointer(bd_ptr);
		}

	//If a misqueue condition has occurred, then is the EOQ bit cleared of the
	//written buffer descriptor to enable the simulation proof and the HDP
	//register written with the physical address of the queue that was supposed
	//to be appended to the original queue.
	if (is_released(sop) && is_eoq(eop)) {
		word_at_phys_addr(eop + FLAGS) = (word_at_phys_addr(eop + FLAGS)) & (~EOQ);

		if (transmit)
			TX0_HDP = new_queue;
		else
			RX0_HDP = new_queue;
	}
}

/*
 *  @bd_ptr: The physical address of the first buffer descriptor in a queue, in
 *  which all buffer descriptors are added or removed according to @add. For
 *	all buffer descriptors, rho_nic is decremented if @add is false, and alpha
 *	is updated depending on whether the buffer descriptors are added or removed.
 */
static void decrement_rho_nic_update_alpha_queue(uint32_t bd_ptr, BOOL transmit, BOOL add)
{
	while (bd_ptr) {
		decrement_rho_nic_update_alpha(bd_ptr, transmit, add);
		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}
}

/*
 *  @bd_ptr: The physical address of the buffer descriptor that is to be added
 *  or removed according to @add.
 *
 *	@transmit: True if the transmission queue is modified. False if it is the
 *	reception queue is modified.
 *
 *  @add: True if and only if the buffer descriptor at @bd_ptr is added to an
 *  active queue.
 *
 *  ASSUMES: bd_ptr is aligned and is in CPPI_RAM.
 *
 *  Function: If the buffer descriptor is added, alpha is updated. If the
 *	buffer descriptor is removed, rho_nic is decremented and alpha updated.
 *
 *  return: void.
 */
static void decrement_rho_nic_update_alpha(uint32_t bd_ptr, BOOL transmit, BOOL add)
{
	if (add) {
		SET_ACTIVE_CPPI_RAM(bd_ptr);
		SET_ACTIVE_CPPI_RAM(bd_ptr + 4);
		SET_ACTIVE_CPPI_RAM(bd_ptr + 8);
		SET_ACTIVE_CPPI_RAM(bd_ptr + 12);
	} else {
		CLEAR_ACTIVE_CPPI_RAM(bd_ptr);
		CLEAR_ACTIVE_CPPI_RAM(bd_ptr + 4);
		CLEAR_ACTIVE_CPPI_RAM(bd_ptr + 8);
		CLEAR_ACTIVE_CPPI_RAM(bd_ptr + 12);
	}

	//This function only updates rho_nic for removal. is_queue_secure updates
	//rho_nic for addition.
	if (!transmit && !add)
		decrement_rho_nic(bd_ptr);
}

/*
 *	@bd_ptr: Physical address of buffer the descriptor that is added or removed
 *	from the receive queue.
 *
 *	@add: True if the buffer descriptor is added to the receive queue and false
 *	otherwise.
 *
 *	Decrements rho_nic and resets recv_bd_nr_blocks for the buffer descriptor
 *	at @bd_ptr.
 */
static void decrement_rho_nic(uint32_t bd_ptr)
{
	uint32_t bp = get_buffer_pointer(bd_ptr);
	uint32_t start_bl = bp >> 12, end_bl, update;

	end_bl = start_bl + recv_bd_nr_blocks[(bd_ptr - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 2] - 1;
	recv_bd_nr_blocks[(bd_ptr - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 2] = 0;

	dmmu_entry_t *e = get_bft_entry_by_block_idx(start_bl);
	e->dev_refcnt--;

	if (start_bl != end_bl) {
		e = get_bft_entry_by_block_idx(end_bl);
		e->dev_refcnt--;
	}
}

/*
 *  Purpose: Determine whether a queue to be set in an HDP register or appended
 *  to an active queue is secure.
 *
 *  @bd_ptr: Physical address of the first buffer descriptor in the queue to be
 *  checked for security.
 *
 *  @transmit: True if and only if the queue pointed by @head_bd_ptr is for
 *  transmission. Otherwise it is for reception.
 *
 *  Function: Returns true if and only if the queue @head_bd_ptr is secure. A
 *  queue is secure if and only if the following conditions are satisfied by
 *  the queue:
 *  a)  It has a length of at most MAX_QUEUE_LENGTH buffer descriptors.
 *  b)  All buffer descriptors are completely located in CPPI_RAM.
 *  c)  All buffer descriptors are word aligned.
 *  d)  None of its buffer descriptors overlap any buffer descriptor in any
 *		active queue (tx0_active_queue or rx0_active_queue).
 *  e)  None of its buffer descriptors overlap each other.
 *  f)  For transmission only: All buffer descriptors must have correctly
 *		matching SOP and EOP bits.
 *  g)  All buffer descriptors only access the memory of the guest and that the
 *		buffer length field is greater than zero. For the monitor and receive
 *		queues, no buffer descriptor is allowed to access executable memory.
 *	h)	For reception only: Check that there are no update overflows in the
 *		reference counter rho_nic, and if so, update it. This can be done
 *		earliest at this point since if rho_nic is updated at this point,
 *		nothing can fail at a later stage that forces the update of rho_nic to
 *		be reversed.
 *  i)  For transmission only: The following fields must be set accordingly:
 *		-Ownership: 1. Only valid on SOP.
 *		-EOQ: 0. Only valid on EOP.
 *		-Teardown: 0. Only valid on SOP.
 *  j)  For reception only: The following fields must be set accordingly:
 *		-Buffer offset: 0. Only valid on SOP but should be cleared on all
 *		 buffer descriptors.
 *		-SOP: 0. Valid on all buffer descriptors.
 *		-EOP: 0. Valid on all buffer descriptors.
 *		-Ownership: 1. Valid on all buffer descriptors.
 *		-EOQ: 0. Valid on all buffer descriptors.
 *		-Teardown: 0. For initialization, should be cleared in all buffer
 *		 descriptors.
 *		-CRC: 0. For initialization, should be cleared in all buffer
 *		 descriptors.
 *
 *  Returns: True if and only if the queue pointed to by @bd_ptr is secure.
 *  Such a queue can be given to the NIC hardware.
 */
static BOOL is_queue_secure(uint32_t bd_ptr, BOOL transmit)
{
	//An empty queue is secure.
	if (bd_ptr == 0)
		return TRUE;

	//a), b), c), d).
	if (!is_valid_length_in_cppi_ram_alignment_no_active_queue_overlap(bd_ptr)) {
		printf("STH CPSW ERROR: Not valid length, in CPPI_RAM or alignment for queue at %x\n", bd_ptr);
		return FALSE;
	}
	//e).
	if (is_queue_self_overlap(bd_ptr)) {
		printf("STH CPSW ERROR: Queue overlaps itself for queue beginning at %x\n", bd_ptr);
		return FALSE;
	}
	//f).
	if (transmit && !is_transmit_SOP_EOP_packet_length_fields_set_correctly(bd_ptr)) {
		printf("STH CPSW ERROR: Queue is of transmit type and does not have correctly matching SOP and EOP bits for queue starting at %x\n", bd_ptr);
		return FALSE;
	}
	//g).
	if (!is_data_buffer_secure_queue(bd_ptr, transmit)) {
		printf("STH CPSW ERROR: Queue accesses memory outside the guest or buffer length is zero for queue starting at %x\n", bd_ptr);
		return FALSE;
	}

	//Now that it is known that the buffer descriptors are not overlapping,
	//nor used by the NIC, and the buffer descriptors are word aligned, they
	//can be recorded as given to the NIC (rho_nic and recv_bd_nr_blocks) and
	//manipulated to have correctly configured bits.

	//h).
	if (!transmit && !no_overflow_and_updated_rho_nic(bd_ptr)) {
		printf("STH CPSW ERROR: Too many buffer descriptors in new queue addresses the same memory block for queue starting at %x. Limit is %d.\n", bd_ptr, MAX_RHO_NIC);
		return FALSE;
	}
	//i).
	if (transmit) {
		set_and_clear_word_on_sop_or_eop(bd_ptr, FLAGS, OWNER, 0, SOP_BD);		//Sets Owner bit on SOP.
		set_and_clear_word_on_sop_or_eop(bd_ptr, FLAGS, 0, EOQ, EOP_BD);		//Clears EOQ bit on EOP.
		set_and_clear_word_on_sop_or_eop(bd_ptr, FLAGS, 0, TD, SOP_BD);			//Clears TD bit on SOP.
	}
	//j).
	else {
		set_and_clear_word(bd_ptr, BOBL, 0, RX_BO);								//Clears buffer offset.
		set_and_clear_word(bd_ptr, FLAGS, OWNER, SOP | EOP | EOQ | TD | CRC);	//Sets owner, clears SOP, EOP, EOQ, TD, CRC.
	}

	//No security violations and therefore true is returned.
	return TRUE;
}

/*
 *  Purpose: Modifies certain bits of certain words of SOP or EOP buffer
 *  descriptors only in a transmit queue. (Receive queues have both SOP and EOP
 *  bits cleared.)
 *
 *  @bd_ptr: Physical address of the head of the queue to be operated on.
 *
 *  @offset: Number of bytes of the word to be accessed from the start of the
 *  buffer descriptor. MUST BE WORD ALIGNED.
 *
 *  @set: A bit mask with ones at the positions that shall be set in the word
 *  with the byte offset indicated by @offset for every buffer descriptor in
 *  the queue that is of the type indicated by @sop. Should be disjunctive with
 *  @clear.
 *
 *  @clear: A bit mask with ones at the positions that shall be cleared in the
 *  word with the byte offset indicated by @offset for every buffer descriptor
 *  in the queue that is of the type indicated by @sop. Should be disjunctive
 *  with @set.
 *
 *  @modify_sop_or_eop: True if SOP buffer descriptors shall be manipulated,
 *	and false if EOP buffer descriptors shall be manipulated.
 *
 *  Termination: The queue pointed to by bd_ptr must be finite.
 *
 *  Function: For each buffer descriptor of type indicated by @sop in the queue
 *  pointed to by bd_ptr, the word with @offset bytes is set to:
 *  word := (word | set) & ~clear.
 *
 *  Returns: void.
 */
static void set_and_clear_word_on_sop_or_eop(uint32_t bd_ptr, uint32_t offset, uint32_t set, uint32_t clear, BOOL modify_sop_or_eop)
{
	while (bd_ptr) {
		//Remember that a buffer descriptor can be both of type SOP and EOP.
		if ((is_sop(bd_ptr) && modify_sop_or_eop) || (is_eop(bd_ptr) && !modify_sop_or_eop))
			word_at_phys_addr(bd_ptr + offset) = (word_at_phys_addr(bd_ptr + offset) | set) & ~clear;
		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}
}

/*
 *  Purpose: Modifies certain bits of certain words of all buffer descriptors
 *  in a queue.
 *
 *  @bd_ptr: Physical address of the head of the queue to be operated on.
 *
 *  @offset: Number of bytes of the word to be accessed from the start of the
 *  buffer descriptor. MUST BE WORD ALIGNED.
 *
 *  @set: A bit mask with ones at the positions that shall be set in the word
 *  with the byte offset indicated by @offset for every buffer descriptor in
 *  the queue. Should be disjunctive with @clear.
 *
 *  @clear: A bit mask with ones at the positions that shall be cleared in the
 *  word with the byte offset indicated by @offset for every byffer descriptor
 *  in the queue. Should be disjunctive with @set.
 *
 *  Termination: The queue pointed to by bd_ptr must be finite.
 *
 *  Function: For each buffer descriptor in the queue pointed to by bd_ptr the
 *  fourth word is set to: word := (word | set) & ~clear
 */
static void set_and_clear_word(uint32_t bd_ptr, uint32_t offset, uint32_t set, uint32_t clear)
{
	while (bd_ptr) {
		word_at_phys_addr(bd_ptr + offset) = (word_at_phys_addr(bd_ptr + offset) | set) & ~clear;
		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}
}

/*
 *  Purpose: Checks that a queue is finite by restricting its length to be at
 *  most 256 buffer descriptors, that it is completely located in CPPI_RAM, and
 *  that all buffer descriptors are word aligned.
 *
 *  @bd_ptr: Physical address of the first buffer descriptor of the queue to
 *  check.
 *
 *  Function: Checks that the queue headed by @bd_ptr satisfies:
 *  a)  It has a length of at most MAX_QUEUE_LENGTH buffer descriptors.
 *  b)  All buffer descriptors are completely located in CPPI_RAM.
 *  c)  All buffer descriptors are word aligned.
 *  d)  No overlap with an active queue.
 *
 *  Returns: True if and only if the queue headed by @bd_ptr is of valid,
 *  length, located in CPPI_RAM, word aligned, and does not overlap an active
 *  queue.
 */
static BOOL is_valid_length_in_cppi_ram_alignment_no_active_queue_overlap(uint32_t bd_ptr)
{
	int length = 0;
	while (bd_ptr != 0 && length < MAX_QUEUE_LENGTH)
		//Subtracts fifteen bytes from 0x4A104000 instead from bd_ptr to avoid
		//overflow, which would create a false truth value. Also checks that
		//all buffer descriptor pointers are word aligned.
		if ((0x4A102000 <= bd_ptr) && (bd_ptr < (0x4A104000 - 15)) && is_word_aligned(bd_ptr)) {
			if (IS_ACTIVE_CPPI_RAM(bd_ptr) || IS_ACTIVE_CPPI_RAM(bd_ptr + 4) || IS_ACTIVE_CPPI_RAM(bd_ptr + 8) || IS_ACTIVE_CPPI_RAM(bd_ptr + 12)) {
				printf("STH CPSW: New buffer descriptor overlaps active buffer descriptor.\n");
				return FALSE;
			}

			length += 1;
			bd_ptr = get_next_descriptor_pointer(bd_ptr);
		} else {
			printf("STH CPSW: Buffer descriptor not completely located in CPPI_RAM or is not word aligned.\n");
			return FALSE;
		}

	//If the queue consists of more than MAX_QUEUE_LENGTH buffer descriptors,
	//the function returns false.
	if (bd_ptr) {
		printf("STH CPSW: New queue is longer than %d!\n", MAX_QUEUE_LENGTH);
		return FALSE;
	}
	//No policy violations and true is returned.
	return TRUE;
}

/*
 *  Purpose: Checks if a queue does not overlap itself.
 *
 *  Assumption: The input queue is in CPPI_RAM and word aligned. Hence no
 *  overflow and only multiples of four needs to be compared for the addresses.
 *
 *  @bd_ptr: Physical address of the queue to be checked.
 *
 *  Termination: Guaranteed.
 *
 *  Function: Checks that the queue headed by @bd_ptr satisfies:
 *  e)  None of its buffer descriptors overlap each other.
 *
 *  Returns: True if and only if the queue pointed to by @bd_ptr overlaps itself.
 */
static BOOL is_queue_self_overlap(uint32_t bd_ptr)
{
	//Check self overlap. No form of cyclic queue is allowed. Alignment of all
	//buffer descriptors is assumed which is also true due to the test above.
	while (bd_ptr) {
		//Compares bd_ptr with all buffer descriptors following it.
		uint32_t other_bd_ptr = get_next_descriptor_pointer(bd_ptr);
		while (other_bd_ptr)
			if ((bd_ptr <= other_bd_ptr && other_bd_ptr < bd_ptr + 0x10) || (other_bd_ptr <= bd_ptr && bd_ptr < other_bd_ptr + 0x10)) {
				printf("STH CPSW: Buffer descriptor queue overlaps itself!\n");
				return TRUE;
			} else
				other_bd_ptr = get_next_descriptor_pointer(other_bd_ptr);

		//Advances bd_ptr to the next buffer descriptor that should be compared
		//with the buffer descriptors following it.
		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}

	//No self-overlap error was found.
	return FALSE;
}

/*
 *  Purpose: Check if a new queue overlaps an active queue.
 *
 *  Assumptions:
 *  *queue1_bd_ptr and queue2 are both completely located in
 *   CPPI_RAM. This prevents overflows in the address calculations in the
 *   comparisons of the inner while loop.
 *  *queue1_bd_ptr and queue2 only points to chains of buffer descriptors that
 *   are word aligned. Hence only multiples of four are needed in the address
 *   comparisons in the inner while loop.
 *
 *  @queue1_bd_ptr: Physical address of first buffer descriptor in queue1.
 *
 *  @queue2: Physical address of first buffer descriptor in queue2.
 *
 *  Function: Checks if queue1_bd_ptr and queue2 overlaps by checking for each
 *  buffer descriptor in queue1 if it overlaps any buffer descriptor in queue2.
 *
 *  Returns: True if and only if queue1 and queue2 do not overlap each other.
 */
/*static BOOL is_no_queue_overlap(uint32_t queue1_bd_ptr, uint32_t queue2) {
  if (queue1_bd_ptr == 0 || queue2 == 0)  //Optimization.
    return TRUE;

  while (queue1_bd_ptr) {
    uint32_t queue2_bd_ptr = queue2;

    while (queue2_bd_ptr)
      if (queue1_bd_ptr + 12 == queue2_bd_ptr || queue1_bd_ptr + 8 == queue2_bd_ptr || queue1_bd_ptr + 4 == queue2_bd_ptr || queue1_bd_ptr  == queue2_bd_ptr ||
        queue1_bd_ptr == queue2_bd_ptr + 4 || queue1_bd_ptr == queue2_bd_ptr + 8 || queue1_bd_ptr == queue2_bd_ptr + 12)
        return FALSE;
      else
        queue2_bd_ptr = get_next_descriptor_pointer(queue2_bd_ptr);

    queue1_bd_ptr = get_next_descriptor_pointer(queue1_bd_ptr);
  }

  //No overlap was found and true is therefore returned.
  return TRUE;
}*/

/*
 *  Purpose: Check that a transmit queue has its SOP and EOP bits set correctly
 *  such that updates of tx0_active_queue can be done correctly. Updates of
 *  tx0_active_queue depend on the Ownership bit which is only set in SOP
 *  buffer descriptors. To make tx0_active_queue point to the next unreleased
 *  buffer descriptor by the hardware, these bits must be set correctly.
 *
 *  Assumption: Input queue is for transmission in CPPI_RAM and the first
 *  buffer descriptor is a SOP.
 *    
 *  ATTENTION: THIS CHECK IS NECESSARY SINCE THE CALCULATION OF ACTIVE QUEUES
 *  DEPENDS ON CORRECT SETTING OF SOP AND EOP BITS.
 *
 *  @bd_ptr: Physical address of the first buffer descriptor in the transmit
 *  queue to check for correct setting of the SOP and EOP bits.
 *
 *  Function: Checks that the queue headed by @bd_ptr satisifies:
 *  f)  For transmission only: all buffer descriptors must have correctly
 *		matching SOP and EOP bits.
 *
 *  Returns: True if and only if all SOP and EOP bits matched correctly: Every
 *  SOP bit is followed by an EOP bit (could be in same buffer descriptor)
 *  before the next SOP bit and the queue always ends with an EOP bit.
 */
static BOOL is_transmit_SOP_EOP_packet_length_fields_set_correctly(uint32_t bd_ptr)
{
	//Goes through each frame's buffer descriptors, that is the whole queue but
	//one SOP ... EOP sequence at a time.
	//While there is a frame...
	while (bd_ptr) {
		//If the first buffer descriptor is not a SOP, then there is an error.
		if (!is_sop(bd_ptr)) {
			printf("STH CPSW: Transmit queue is rejected due to invalid SOP/EOP sequence of buffer descriptors!\n");
			return FALSE;
		}
		uint32_t buffer_length_sum = get_tx_buffer_length(bd_ptr);
		uint32_t packet_length = get_packet_length(bd_ptr);

		//If the first buffer descriptor is not an EOP, then such a buffer
		//descriptor must be found.
		if (!is_eop(bd_ptr)) {
			bd_ptr = get_next_descriptor_pointer(bd_ptr);
			//At this point the queue looks like: [BD.SOP], [BD.X] ... And bd_ptr
			//points to [BD.X].

			//Until a null pointer, SOP or EOP buffer descriptor is found, bd_ptr
			//is advanced.
			while (bd_ptr != 0 && !is_sop(bd_ptr) && !is_eop(bd_ptr)) {
				buffer_length_sum += get_tx_buffer_length(bd_ptr);
				bd_ptr = get_next_descriptor_pointer(bd_ptr);
			}
			if (bd_ptr)
				buffer_length_sum += get_tx_buffer_length(bd_ptr);

			//If it wasn't an EOP that stopped the processing, then it is an error.
			//Even if EOP was set, SOP must not be set for a valid matching.
			if (bd_ptr == 0 || is_sop(bd_ptr)) {
				printf("STH CPSW: Transmit queue is rejected due to invalid SOP/EOP sequence of buffer descriptors!\n");
				return FALSE;
			}
		}
		//Checks that the sum of the buffer length fields are equal to the
		//packet length field.
		if (packet_length != buffer_length_sum) {
			printf("STH CPSW: Transmit queue is rejected due to invalid packet length field of a transmit SOP buffer descriptors!\n");
			return FALSE;
		}
		//At this point, bd_ptr is an EOP buffer descriptor. Hence bd_ptr is
		//advanced to the next frame's first buffer descriptor.
		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}

	//No policy violation and therefore true is returned.
	return TRUE;
}

/*
 *  Purpose: Checks that a queue is valid: All buffer descriptors refer to data
 *  buffers that are in the guest's memory region, that the buffer length field
 *  is greater than zero and if the monitor is used, that receive buffer
 *  descriptors cannot access executable memory.
 *
 *  Assumptions: Queue is in CPPI_RAM and word aligned.
 *
 *  @bd_ptr: The physical address of the first buffer descriptor in the queue.
 *
 *  @transmit: True if and only if the queue is for transmission.
 *
 *  Termination: If the queue is of finite length.
 *
 *  Function: Checks that the queue headed by bd_ptr satisifes:
 *  g)  All buffer descriptors only access the memory of the guest and their
 *		buffer length field is greater than zero. For the monitor it is checked
 *		that receive buffer descriptors cannot access executable memory.
 *
 *  Returns: True if and only if the queue pointed to by @bd_ptr only access
 *  memory of the guest, the buffer length field is greater than zero, and if
 *  the monitor is used no executable memory is accessed.
 */
static BOOL is_data_buffer_secure_queue(uint32_t bd_ptr, BOOL transmit)
{
	while (bd_ptr) {
		if (!is_data_buffer_secure(bd_ptr, transmit)) {
			printf("STH CPSW: Insecure buffer descriptor rejected!\n");
			return FALSE;
		}

		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}

	//No error was found and hence true is returned.
	return TRUE;
}

/*
 *  Purpose: Checks that a buffer descriptor only refers to memory of the
 *  guest, that the buffer length field is greater than zero, and if the
 *  monitor is used no executable memory is accessed by receive buffer
 *  descriptors.
 *
 *  Assumptions:
 *  *Assumes that if @bd_ptr is a receive queue, then the buffer offset field
 *   of all buffer descriptors have been cleared. This could be dangerous if
 *   the hardware ORed the bits in the buffer descriptor with the
 *   RX_BUFFER_OFFSET register.
 *  *If @bd_ptr is belongs to a transmit queue and is a SOP buffer descriptor,
 *   then its related buffer descriptors belonging to the same frame, can be
 *   found by following the next descriptor pointer fields until the EOP buffer
 *   descriptor.
 *
 *  @bd_ptr: The physical address of the buffer descriptor.
 *
 *  @transmit: True if and only if the queue is of transmission type.
 *
 *  Termination: Guaranteed.
 *
 *  Function: Checks that the buffer descriptor, of the type indicated by
 *  @transmit, pointed to by @bd_ptr represents a data buffer that is within
 *  the guest's physical memory region, that the buffer length is greater than
 *  zero, and if monitor is used, then no executable memory is accessed by
 *  receive buffer descriptors.
 *
 *  Returns: True if and only if the buffer descriptor pointed to by bd_ptr
 *  represents a data buffer that is within the guests physical memory region,
 *  that the buffer length field is greater than zero, and if monitor is used,
 *  then no executable memory is accessed by receive buffer descriptors.
 */
static BOOL is_data_buffer_secure(uint32_t bd_ptr, BOOL transmit)
{
	uint32_t buffer_pointer = get_buffer_pointer(bd_ptr), buffer_offset, buffer_length;
	uint32_t buffer_offset_length = get_buffer_offset_and_length(bd_ptr);
	uint32_t guest_start_address, guest_end_address;

	//Retrieves the buffer offset and buffer length fields, the sizes of which
	//depends on whether they are part of a transmit or receive buffer descriptor.
	if (transmit) {		//If a transmit buffer descriptor.
		if (is_sop(bd_ptr))	//If a SOP buffer descriptor then must the buffer offset field be checked.
			buffer_offset = (buffer_offset_length & TX_BO) >> 16;
		else
			//Buffer offset is only relevant for SOP buffer descriptors and is zero for other ones.
			buffer_offset = 0;

		buffer_length = buffer_offset_length & TX_BL;
	} else {		//If a receive buffer descriptor.
		buffer_offset = 0;	//Buffer offset is only relevant for SOP buffer descriptors and should always be initialized to zero by
		//software. Overwritten by the hardware with the value in the RX_BUFFER_OFFSET register in SOP buffer
		//descriptors. The buffer length field indicates the size of the buffer and the RX_BUFFER_OFFSET register
		//cannot make data be stored outside the data buffer associated with the buffer descriptor.
		buffer_length = buffer_offset_length & RX_BL;
	}

	//Checks that buffer length is greater than zero.
	if (buffer_length == 0) {
		printf("STH CPSW: Buffer length is zero for buffer descriptor at %x!\n", bd_ptr);
		return FALSE;
	}
	//Checks overflow condition.
	//The buffer may not wrap.
	//There is an overflow if
	//buffer_pointer + buffer_offset + buffer_length - 1 > 0xFFFFFFFF <==>
	//buffer_pointer > 0xFFFFFFFF - buffer_offset - buffer_length + 1.
	//Now underflow since buffer_offset and buffer_length is at most 2^16 - 1
	//and their sum is at most 2^17 - 2.
	if (buffer_pointer > 0xFFFFFFFF - buffer_offset - buffer_length + 1) {
		printf("STH CPSW: Data buffer calculation overflow!\n");
		return FALSE;
	}

	uint32_t start_bl = buffer_pointer >> 12;
	uint32_t end_bl = (buffer_pointer + buffer_offset + buffer_length - 1) >> 12;
	if (!is_secure_linux_memory(transmit, start_bl, end_bl)) {
		printf("STH CPSW: Buffer descriptor is not addressing legal memory.\n");
		return FALSE;
	}
	//No errors, and hence the buffer descriptor is valid.
	return TRUE;
}

/*
 *	@transmit: True if the security check is with respect to a transmission
 *	buffer descriptor and false if it is with respect to a reception buffer
 *	descriptor.
 *
 *	@start_bl: Block index of the first block that shall be checked.
 *
 *	@end_bl: Block index of the last block that shall be checked.
 *
 *	Returns: True if and only if the blocks with indexes between current_bl and
 *	end_bl, inclusive, only access Linux RAM, and for reception also no page
 *	tables, NIC registers or executable memory.
 */
static BOOL is_secure_linux_memory(BOOL transmit, uint32_t start_bl, uint32_t end_bl)
{
	uint32_t current_bl;
	for (current_bl = start_bl; current_bl <= end_bl; current_bl++) {
		if (is_executable_block(current_bl)) {
			dmmu_entry_t *e = get_bft_entry_by_block_idx(current_bl);
			printf("\t addr %x executable reference %d\n", (current_bl << 12), e->x_refcnt);

			print_all_pointing_L1(current_bl << 12, 0xfffff000);
			print_all_pointing_L2(current_bl << 12, 0xfffff000);
		}
		if (transmit) {
			//The type of the block is not L1, L2 or D.
			if (!(is_L1_or_L2_block(current_bl) || is_D_block(current_bl)))
				return FALSE;
		} else {
			//The type of the block is either not D, or it is of type D but executable.
			if (!is_D_block(current_bl) || is_executable_block(current_bl))
				return FALSE;
		}
	}

	//The blocks address only allowed RAM and therefore are there no security issues.
	return TRUE;
}

/*
 *	@bl: Block index of the block whose type is to be checked.
 *
 *	@return: True if and only if the block with index @bl is of type L1 or L2.
 */
static BOOL inline is_L1_or_L2_block(uint32_t bl)
{
	dmmu_entry_t *e = get_bft_entry_by_block_idx(bl);

	if (e->type == PAGE_INFO_TYPE_L1PT || e->type == PAGE_INFO_TYPE_L2PT)
		return TRUE;
	else
		return FALSE;
}

/*
 *	@bl: Block index of the block that shall be checked to be of type D.
 *
 *	@return: True if and only if the block with index @bl is of type D.
 */
static BOOL inline is_D_block(uint32_t bl)
{
	dmmu_entry_t *e = get_bft_entry_by_block_idx(bl);

	if (e->type == PAGE_INFO_TYPE_DATA)
		return TRUE;
	else
		return FALSE;
}

/*
 *	@bl: Block index of the block whose execute permissions is to be checked.
 *
 *	@return: True if and only if the block with index @bl is mapped by the MMU
 *	as executable.
 */
static BOOL inline is_executable_block(uint32_t bl)
{
	dmmu_entry_t *e = get_bft_entry_by_block_idx(bl);

	if (e->x_refcnt > 0)
		return TRUE;
	else
		return FALSE;
}

/*
 *  Purpose: Determine what sort of access is made to a queue: zeroed next
 *  descriptor pointer, other accesses on the queue, and no access at all.
 *
 *  Assumptions: The queue identified by @bd_ptr and @physical_address are word
 *  aligned.
 *
 *  @accessed_address: Physical address of CPPI_RAM that is to be written.
 *
 *  @bd_ptr: The physical address of the first buffer descriptor in the
 *  queue to check overlap with.
 *
 *  Function: Returns a value of type bd_overlap which is an enum:
 *  *ZEROED_NEXT_DESCRIPTOR_POINTER_OVERLAP: @physical_address accesses a
 *   zeroed next descriptor pointer.
 *  *ILLEGAL_OVERLAP: @physical_address accesses a illegad buffer descriptor
 *   word.
 *  *NO_OVERLAP: @physical_address does not access any byte of any buffer
 *   descriptor.
 */
static bd_overlap type_of_cppi_ram_access_overlap(uint32_t accessed_address, uint32_t bd_ptr)
{
	while (bd_ptr)
		if (bd_ptr == accessed_address) {
			if (get_next_descriptor_pointer(bd_ptr) == 0)
				return ZEROED_NEXT_DESCRIPTOR_POINTER_OVERLAP;
			else
				return ILLEGAL_OVERLAP;
		} else if (bd_ptr < accessed_address && accessed_address <= bd_ptr + 0xF)
			return ILLEGAL_OVERLAP;
		else
			bd_ptr = get_next_descriptor_pointer(bd_ptr);

	return NO_OVERLAP;
}

/*
 *	@bd_ptr: Physical address of the first buffer descriptor in a reception
 *	queue to be given to the NIC.
 *
 *	@return: If there is no entry in rho_nic that is overflowing as a result of
 *	updating rho_nic by giving the queue to the NIC, rho_nic and
 *	recv_bd_nr_blocks are updated and true is returned. If some entry would
 *	overflow as a result of an update, rho_nic nor recv_bd_nr_blocks are
 *	updated and false is returned.
 */
static BOOL no_overflow_and_updated_rho_nic(uint32_t bd_ptr)
{
	uint32_t initial_bd_ptr = bd_ptr, overflow_bd_ptr = 0;

	while (bd_ptr && !overflow_bd_ptr) {
		//1. Identify which blocks the buffer descriptor addresses.
		uint32_t buffer_pointer = get_buffer_pointer(bd_ptr);
		uint32_t start_bl = buffer_pointer >> 12;
		uint32_t end_bl = (buffer_pointer + get_rx_buffer_length(bd_ptr) - 1) >> 12;

		//2. Check overflow of rho_nic (reference counter of NIC/devices) for
		//first addressed block.
		dmmu_entry_t *e1 = get_bft_entry_by_block_idx(start_bl);
		if (e1->dev_refcnt == MAX_RHO_NIC)
			overflow_bd_ptr = bd_ptr;	//Incrementing max value is an error.

		//3. Check if the buffer descriptor addresses a second block. A
		//reception buffer descriptor can address at most two blocks.
		dmmu_entry_t *e2;
		if (start_bl != end_bl) {
			e2 = get_bft_entry_by_block_idx(end_bl);
			if (e2->dev_refcnt == MAX_RHO_NIC)
				overflow_bd_ptr = bd_ptr;	//Incrementing max value is an error.
		}

		//4. If no overflow will occur by updating rho_nic, rho_nic is updated.
		if (!overflow_bd_ptr)
			e1->dev_refcnt++;
		if (!overflow_bd_ptr && start_bl != end_bl)
			e2->dev_refcnt++;

		//5. Update recv_bd_nr_blocks.
		recv_bd_nr_blocks[(bd_ptr - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 2] = end_bl - start_bl + 1;

		//6. Process next buffer descriptor.
		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}

	//If there was an overflow, then the update of rho_nic must be reversed.
	//This means that all buffer descriptors up to but excluding the one
	//causing the overflow (which did not update rho_nic) must be processed to
	//decrement rho_nic.
	if (overflow_bd_ptr) {
		bd_ptr = initial_bd_ptr;

		while (bd_ptr != overflow_bd_ptr) {
			//1. Identify which blocks the buffer descriptor addresses.
			uint32_t buffer_pointer = get_buffer_pointer(bd_ptr);
			uint32_t start_bl = buffer_pointer >> 12;
			uint32_t end_bl = (buffer_pointer + get_rx_buffer_length(bd_ptr) - 1) >> 12;

			//2. Decrement rho_nic (reference counter of NIC/devices) for first
			//addressed block.
			dmmu_entry_t *e1 = get_bft_entry_by_block_idx(start_bl);
			e1->dev_refcnt--;

			//3. Check if the buffer descriptor addresses a second block. A
			//reception buffer descriptor can address at most two blocks.
			if (start_bl != end_bl) {
				dmmu_entry_t *e2 = get_bft_entry_by_block_idx(end_bl);
				e2->dev_refcnt--;
			}

			//4. Reset recv_bd_nr_blocks.
			recv_bd_nr_blocks[(bd_ptr - CPPI_RAM_START_PHYSICAL_ADDRESS) >> 2] = 0;

			//5. Process next buffer descriptor.
			bd_ptr = get_next_descriptor_pointer(bd_ptr);
		}

		//Overflow and rho_nic is not updated. False is therefore returned.
		return FALSE;
	}
	//No overflow and updated rho_nic. True is therefore returned.
	else
		return TRUE;
}

/*
 *  @value: The value to write to the CPDMA_SOFT_RESET register.
 *
 *  Function: Used by the hypervisor to reset the NIC DMA logic, clear all Head
 *  Descriptor Pointer registers, and initialize STH CPSW related registers
 *  such as active_queues, during the hypervisor initialization phase. This
 *  should be done in case the guest does not perform any resetting (which it
 *  should).
 *
 *  Returns true if and only if the reset operation was successful.
 *
 *  Since the Ethernet Subsystem memory region is mapped with
 *  TEX[2:0] = 000, C = 0 and B = 0, this means that the mapped memory
 *  region is classified as Strongly-ordered. Strongly-ordered memory means
 *  that "Address locations marked as Device or Strongly-ordered are never
 *  held in a cache." This can be seen in
 *  core/hw/cpu/arm/arm_common/arm_mmu_pt.c:pt_create_coarse at the label
 *  MLT_IO_RO_REG where flags is set to 1 and at the assignment of pte.
 *
 *  Hence no volatile is needed.
 */
BOOL soc_cpsw_reset(uint32_t value)
{
	unsigned int timeout, i;

	//Checks if the SOFT_RESET field of the CPDMA_SOFT_RESET register is to be
	//set. This is to prevent the code below from performing a reset operation
	//when it actually should not be done.
	if (!(0x00000001 & value))
		return TRUE;

	/*
	 *  Starts the reset operation of the CPDMA logic.
	 */
	CPDMA_SOFT_RESET = 1;

	/*
	 *  Waits for the reset to complete.
	 *
	 *  On BeagleBone Black there is a one GHz clock and assuming one read
	 *  takes 100 ns since it goes to the bus. At least 20 ms is waited. No
	 *  accurate calculation. This gives:
	 *  20 ms / 100 ns = 2/10 * 10^6 = 2 * 10^5
	 *
	 *  When zero is returned, then the reset is complete.
	 */
	timeout = 0;
	while (CPDMA_SOFT_RESET && timeout < 100000)
		timeout++;

	/*
	 *  If the reset operation is not complete at this point, it is deemed to
	 *  have failed and therefore false is returned.
	 */
	if (CPDMA_SOFT_RESET) {
		printf("STH CPSW: Failed reset operation!\n");
		return FALSE;
	}

	/*
	 *  Clears the Head Descriptor Pointer and Completion Pointer registers,
	 *  and sets the hypervisor's view of active queues to no active queues.
	 */
	for (i = 0; i < 8; i++) {
		//TX[i]_HDP := 0
		//RX[i]_HDP := 0
		//TX[i]_CP := 0
		//RX[i]_CP := 0
		//Hypervisor's transmit active queues are zeroed.
		//Hypervisor's receive active queues are zeroed.
//    printf("STH CPSW: TX[%d]_HDP at 0x%x\n", i, 0x4A100A00 + 4*i);
		word_at_phys_addr(0x4A100A00 + 4 * i) = 0;
//    printf("STH CPSW: RX[%d]_HDP at 0x%x\n", i, 0x4A100A00 + 8*4 + 4*i);
		word_at_phys_addr(0x4A100A00 + 8 * 4 + 4 * i) = 0;
//    printf("STH CPSW: TX[%d]_CP at 0x%x\n", i, 0x4A100A00 + 16*4 + 4*i);
		word_at_phys_addr(0x4A100A00 + 16 * 4 + 4 * i) = 0;
//    printf("STH CPSW: RX[%d]_CP at 0x%x\n", i, 0x4A100A00 + 24*4 + 4*i);
		word_at_phys_addr(0x4A100A00 + 24 * 4 + 4 * i) = 0;

//    printf("STH CPSW: TX[%d]_HDP at 0x%x = %x\n", i, 0x4A100A00 + 4*i, word_at_phys_addr(0x4A100A00 + 4*i));
//    printf("STH CPSW: RX[%d]_HDP at 0x%x = %x\n", i, 0x4A100A00 + 8*4 + 4*i, word_at_phys_addr(0x4A100A00 + 8*4 + 4*i));
//    printf("STH CPSW: TX[%d]_CP at 0x%x = %x\n", i, 0x4A100A00 + 16*4 + 4*i, word_at_phys_addr(0x4A100A00 + 16*4 + 4*i));
//    printf("STH CPSW: RX[%d]_CP at 0x%x = %x\n", i, 0x4A100A00 + 24*4 + 4*i, word_at_phys_addr(0x4A100A00 + 24*4 + 4*i));

//    printf("STH CPSW: #################################\n");
	}

	RX_BUFFER_OFFSET = 0;

	initialized = TRUE;
	tx0_hdp_initialized = TRUE;
	rx0_hdp_initialized = TRUE;
	tx0_cp_initialized = TRUE;
	rx0_cp_initialized = TRUE;

	return TRUE;
}

#if defined(PRINT_QUEUE)
static void print_queue(uint32_t bd_ptr)
{
	printf
	    ("-----DEBUGGING INFORMATION: CURRENT ACTIVE QUEUE-------------------------------------------------\n");
	printf("TX0_HDP, RX0_HDP = (%x, %x)\n", TX0_HDP, RX0_HDP);
	while (bd_ptr) {
		uint32_t ndp = get_next_descriptor_pointer(bd_ptr);
		uint32_t bp = get_buffer_pointer(bd_ptr);
		uint32_t bobl = get_buffer_offset_and_length(bd_ptr);
		uint32_t mode = get_flags(bd_ptr);
		printf("########################################\n");
		printf("Buffer Descriptor at: %x\n", bd_ptr);
		printf("Next Buffer Descriptor Pointer: %x\n", ndp);
		printf("Buffer Pointer: %x\n", bp);
		printf("Buffer Offset and Buffer Length: %x\n", bobl);
		printf
		    ("SOP = %d EOP = %d OWNER = %d EOQ = %d Teardown = %d\n",
		     (mode & SOP) >> 31, (mode & EOP) >> 30,
		     (mode & OWNER) >> 29, (mode & EOQ) >> 28,
		     (mode & TD) >> 27);
		printf("########################################\n");
		bd_ptr = get_next_descriptor_pointer(bd_ptr);
	}
	printf
	    ("-------------------------------------------------------------------------------------------------\n");
}
#endif

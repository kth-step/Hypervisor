#include <types.h>
#include <mmu.h>

memory_layout_entry memory_padr_layout[] = {
	{ADDR_TO_PAGE(0x48002000), ADDR_TO_PAGE(0x48003000), MLT_IO_RO_REG,
	 MLF_READABLE}
	,			/* SYSTEM CONTROL MODULE 4K preferable RO */
	{ADDR_TO_PAGE(0x48004000), ADDR_TO_PAGE(0x48006000), MLT_IO_RO_REG,
	 MLF_READABLE}
	,			/* CLOCKS 16K (only 8k needed in Linux port) */
	{ADDR_TO_PAGE(0x4806a000), ADDR_TO_PAGE(0x4806b000), MLT_IO_RW_REG,
	 MLF_READABLE | MLF_WRITEABLE}
	,			/* UART1 4K */
	{ADDR_TO_PAGE(0x4806c000), ADDR_TO_PAGE(0x4806d000), MLT_IO_RW_REG,
	 MLF_READABLE | MLF_WRITEABLE}
	,			/* UART2 4K */
	{ADDR_TO_PAGE(0x48200000), ADDR_TO_PAGE(0x48201000), MLT_IO_HYP_REG,
	 MLF_READABLE | MLF_WRITEABLE}
	,			/*INTERRUPT CONTROLLER BASE 16KB (only 4k needed in Linux port) */
	{ADDR_TO_PAGE(0x48304000), ADDR_TO_PAGE(0x48305000), MLT_IO_RO_REG,
	 MLF_READABLE}
	,			/*L4-Wakeup (gp-timer in reserved ) 4KB */
	{ADDR_TO_PAGE(0x48306000), ADDR_TO_PAGE(0x48308000), MLT_IO_RO_REG,
	 MLF_READABLE}
	,			/*L4-Wakeup (power-reset manager) module A 8KB can be RO OMAP READS THE HW REGISTER TO SET UP CLOCKS */
	{ADDR_TO_PAGE(0x48320000), ADDR_TO_PAGE(0x48321000), MLT_IO_RO_REG,
	 MLF_READABLE}
	,			/*L4-Wakeup (32KTIMER module) 4KB RO */
	{ADDR_TO_PAGE(0x4830A000), ADDR_TO_PAGE(0x4830B000), MLT_IO_RO_REG,
	 MLF_READABLE}
	,			/*CONTROL MODULE ID CODE 4KB RO */
	{ADDR_TO_PAGE(0x49020000), ADDR_TO_PAGE(0x49021000), MLT_IO_RW_REG,
	 MLF_READABLE | MLF_WRITEABLE}
	,			/*UART 3 */
	{ADDR_TO_PAGE(0x80100000), ADDR_TO_PAGE(0x80500000), MLT_HYPER_RAM,
	 MLF_READABLE | MLF_WRITEABLE}
	,			// hypervisor ram
	{ADDR_TO_PAGE(0x80500000), ADDR_TO_PAGE(0x80600000), MLT_TRUSTED_RAM,
	 MLF_READABLE | MLF_WRITEABLE}
	,			// trusted ram
	{ADDR_TO_PAGE(0x81000000), ADDR_TO_PAGE(0x81000000 + 0x00500000),
	 MLT_USER_RAM, MLF_READABLE | MLF_WRITEABLE | MLF_LAST}
	,			// user ram
};

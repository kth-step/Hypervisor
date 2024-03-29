
#include <types.h>
#include <mmu.h>

memory_layout_entry memory_padr_layout[] = {
	{ADDR_TO_PAGE(0x00000000), ADDR_TO_PAGE(0x000fffff), MLT_HYPER_RAM, MLF_READABLE | MLF_WRITEABLE},			// hypervisor ram
	{ADDR_TO_PAGE(0x00100000), ADDR_TO_PAGE(0x001fffff), MLT_USER_RAM, MLF_READABLE | MLF_WRITEABLE},			// user ram
	{ADDR_TO_PAGE(0xA0000000), ADDR_TO_PAGE(0xA00FFFFF), MLT_IO_RW_REG, MLF_READABLE | MLF_WRITEABLE},			// IO, updated MLT_IO_REG to MLT_IO_RW_REG to make u8500 compilable.
	{ADDR_TO_PAGE(0x80000000), ADDR_TO_PAGE(0x8FFFFFFF), MLT_IO_RW_REG, MLF_READABLE | MLF_WRITEABLE}			// IO, updated MLT_IO_REG to MLT_IO_RW_REG to make u8500 compilable.
};

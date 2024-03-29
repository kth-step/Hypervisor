#include <types.h>
#include <mmu.h>

memory_layout_entry memory_padr_layout[] = {
	// these two are not really used at the moment
	//{ADDR_TO_PAGE(0x40000000), ADDR_TO_PAGE(0x4002BFFF), MLT_HYPER_RAM, MLF_READABLE                       }, // internal ROM
	//{ADDR_TO_PAGE(0x402F0000), ADDR_TO_PAGE(0x402FFFFF), MLT_HYPER_RAM, MLF_READABLE | MLF_WRITEABLE       }, // internal SRAM

	{ADDR_TO_PAGE(0x80100000), ADDR_TO_PAGE(0x80500000), MLT_HYPER_RAM,
	 MLF_READABLE | MLF_WRITEABLE}
	,			/* hypervisor ram */
	{ADDR_TO_PAGE(0x80500000), ADDR_TO_PAGE(0x80600000), MLT_TRUSTED_RAM,
	 MLF_READABLE | MLF_WRITEABLE}
	,			/* trusted ram */
	//{ADDR_TO_PAGE(0x81000000), ADDR_TO_PAGE(0x88000000), MLT_USER_RAM, MLF_READABLE | MLF_WRITEABLE | MLF_LAST}, /* user ram */
	{ADDR_TO_PAGE(0x81000000), ADDR_TO_PAGE(0x81000000 + 0x00500000),
	 MLT_USER_RAM, MLF_READABLE | MLF_WRITEABLE | MLF_LAST}
	,			// user ram
};

#include <hw.h>
#include "board.h"

extern uint32_t *flpt_va;

/*
#define L3_34XX_BASE			0x68000000
#define L4_34XX_BASE			0x48000000
#define OMAP34XX_GPMC_BASE		0x6E000000
#define OMAP343X_SMS_BASE		0x6C000000
#define OMAP343X_SDRC_BASE		0x6D000000
#define L4_PER_34XX_BASE		0x49000000
#define L4_EMU_34XX_BASE		0x54000000
#define L4_WK_AM33XX_BASE		0x44C00000
#define L3_44XX_BASE			0x44000000
#define L4_44XX_BASE			0x4a000000
#define L4_PER_44XX_BASE		0x48000000
#define L3_54XX_BASE			0x44000000
#define L4_54XX_BASE			0x4a000000
#define L4_WK_54XX_BASE			0x4ae00000
#define L4_PER_54XX_BASE		0x48000000
#define L4_CFG_MPU_DRA7XX_BASE	0x48210000
#define L3_MAIN_SN_DRA7XX_BASE	0x44000000
#define L4_PER1_DRA7XX_BASE		0x48000000
#define L4_PER2_DRA7XX_BASE		0x48400000
#define L4_PER3_DRA7XX_BASE		0x48800000
#define L4_CFG_DRA7XX_BASE		0x4A000000
#define L4_WKUP_DRA7XX_BASE		0x4ae00000

#define L3_34XX_PHYS			L3_34XX_BASE
#define L4_34XX_PHYS			L4_34XX_BASE
#define OMAP34XX_GPMC_PHYS		OMAP34XX_GPMC_BASE
#define OMAP343X_SMS_PHYS		OMAP343X_SMS_BASE
#define OMAP343X_SDRC_PHYS		OMAP343X_SDRC_BASE
#define L4_PER_34XX_PHYS		L4_PER_34XX_BASE
#define L4_EMU_34XX_PHYS		L4_EMU_34XX_BASE
#define L4_WK_AM33XX_PHYS		L4_WK_AM33XX_BASE
#define L3_44XX_PHYS			L3_44XX_BASE
#define L4_44XX_PHYS			L4_44XX_BASE
#define L4_PER_44XX_PHYS		L4_PER_44XX_BASE
#define L3_54XX_PHYS			L3_54XX_BASE
#define L4_54XX_PHYS			L4_54XX_BASE
#define L4_WK_54XX_PHYS			L4_WK_54XX_BASE
#define L4_PER_54XX_PHYS		L4_PER_54XX_BASE
#define L4_CFG_MPU_DRA7XX_PHYS	L4_CFG_MPU_DRA7XX_BASE
#define L3_MAIN_SN_DRA7XX_PHYS	L3_MAIN_SN_DRA7XX_BASE
#define L4_PER1_DRA7XX_PHYS		L4_PER1_DRA7XX_BASE
#define L4_PER2_DRA7XX_PHYS		L4_PER2_DRA7XX_BASE
#define L4_PER3_DRA7XX_PHYS		L4_PER3_DRA7XX_BASE
#define L4_CFG_DRA7XX_PHYS		L4_CFG_DRA7XX_BASE
#define L4_WKUP_DRA7XX_PHYS		L4_WKUP_DRA7XX_BASE

#define OMAP2_L3_IO_OFFSET		0x90000000
#define OMAP2_L4_IO_OFFSET		0xb2000000
#define OMAP2_EMU_IO_OFFSET		0xaa800000
#define AM33XX_L4_WK_IO_OFFSET	0xb5000000
#define OMAP4_L3_IO_OFFSET		0xb4000000

#define L3_34XX_VIRT			(L3_34XX_PHYS + 			OMAP2_L3_IO_OFFSET)
#define L4_34XX_VIRT			(L4_34XX_PHYS + 			OMAP2_L4_IO_OFFSET)
#define OMAP34XX_GPMC_VIRT		(OMAP34XX_GPMC_PHYS + 		OMAP2_L3_IO_OFFSET)
#define OMAP343X_SMS_VIRT		(OMAP343X_SMS_PHYS + 		OMAP2_L3_IO_OFFSET)
#define OMAP343X_SDRC_VIRT		(OMAP343X_SDRC_PHYS +	 	OMAP2_L3_IO_OFFSET)
#define L4_PER_34XX_VIRT		(L4_PER_34XX_PHYS + 		OMAP2_L4_IO_OFFSET)
#define L4_EMU_34XX_VIRT		(L4_EMU_34XX_PHYS + 		OMAP2_EMU_IO_OFFSET)
#define L4_WK_AM33XX_VIRT		(L4_WK_AM33XX_PHYS +	 	AM33XX_L4_WK_IO_OFFSET)
#define L3_44XX_VIRT			(L3_44XX_PHYS + 			OMAP4_L3_IO_OFFSET)
#define L4_44XX_VIRT			(L4_44XX_PHYS + 			OMAP2_L4_IO_OFFSET)
#define L4_PER_44XX_VIRT		(L4_PER_44XX_PHYS + 		OMAP2_L4_IO_OFFSET)
#define L3_54XX_VIRT			(L3_54XX_PHYS + 			OMAP4_L3_IO_OFFSET)
#define L4_54XX_VIRT			(L4_54XX_PHYS + 			OMAP2_L4_IO_OFFSET)
#define L4_WK_54XX_VIRT			(L4_WK_54XX_PHYS + 			OMAP2_L4_IO_OFFSET)
#define L4_PER_54XX_VIRT		(L4_PER_54XX_PHYS + 		OMAP2_L4_IO_OFFSET)
#define L4_CFG_MPU_DRA7XX_VIRT	(L4_CFG_MPU_DRA7XX_PHYS + 	OMAP2_L4_IO_OFFSET)
#define L3_MAIN_SN_DRA7XX_VIRT	(L3_MAIN_SN_DRA7XX_PHYS + 	OMAP4_L3_IO_OFFSET)
#define L4_PER1_DRA7XX_VIRT		(L4_PER1_DRA7XX_PHYS + 		OMAP2_L4_IO_OFFSET)
#define L4_PER2_DRA7XX_VIRT		(L4_PER2_DRA7XX_PHYS + 		OMAP2_L4_IO_OFFSET)
#define L4_PER3_DRA7XX_VIRT		(L4_PER3_DRA7XX_PHYS + 		OMAP2_L4_IO_OFFSET)
#define L4_CFG_DRA7XX_VIRT		(L4_CFG_DRA7XX_PHYS + 		OMAP2_L4_IO_OFFSET)
#define L4_WKUP_DRA7XX_VIRT		(L4_WKUP_DRA7XX_PHYS + 		OMAP2_L4_IO_OFFSET)

#define SZ_1M					0x00100000
#define SZ_2M					0x00200000
#define SZ_4M					0x00400000
#define SZ_8M					0x00800000

#define L3_34XX_SIZE			SZ_1M
#define L4_34XX_SIZE			SZ_4M
#define OMAP34XX_GPMC_SIZE		SZ_1M
#define OMAP343X_SMS_SIZE		SZ_1M
#define OMAP343X_SDRC_SIZE		SZ_1M
#define L4_PER_34XX_SIZE		SZ_1M
#define L4_EMU_34XX_SIZE		SZ_8M
#define L4_WK_AM33XX_SIZE		SZ_4M
#define L3_44XX_SIZE			SZ_1M
#define L4_44XX_SIZE			SZ_4M
#define L4_PER_44XX_SIZE		SZ_4M
#define L3_54XX_SIZE			SZ_1M
#define L4_54XX_SIZE			SZ_4M
#define L4_WK_54XX_SIZE			SZ_2M
#define L4_PER_54XX_SIZE		SZ_4M
#define L4_CFG_MPU_DRA7XX_SIZE	SZ_1M
#define L3_MAIN_SN_DRA7XX_SIZE	SZ_1M
#define L4_PER1_DRA7XX_SIZE		SZ_1M
#define L4_PER2_DRA7XX_SIZE		SZ_1M
#define L4_PER3_DRA7XX_SIZE		SZ_2M
#define L4_CFG_DRA7XX_SIZE		(SZ_1M + SZ_2M)
#define L4_WKUP_DRA7XX_SIZE		SZ_1M

#define PAGE_SHIFT				12
#define PHYS_PFN(x)				((unsigned long)((x) >> PAGE_SHIFT))
#define	__phys_to_pfn(paddr)	PHYS_PFN(paddr)
*/

//#define OMAP2_L4_IO_OFFSET(x) ((x) + 0xb2000000)
//#define AM33XX_L4_WK_IO_OFFSET(x) ((x) + 0xb5000000)


#define SZ_4M					0x00400000

#define L4_34XX_SIZE			SZ_4M
#define L4_34XX_BASE			0x48000000
#define OMAP2_L4_IO_OFFSET		0xb2000000
#define L4_34XX_PHYS			L4_34XX_BASE
#define L4_34XX_VIRT			(L4_34XX_PHYS + OMAP2_L4_IO_OFFSET)

#define L4_WK_AM33XX_SIZE		SZ_4M
#define L4_WK_AM33XX_BASE		0x44C00000
#define AM33XX_L4_WK_IO_OFFSET	0xb5000000
#define L4_WK_AM33XX_PHYS		L4_WK_AM33XX_BASE
#define L4_WK_AM33XX_VIRT		(L4_WK_AM33XX_PHYS + AM33XX_L4_WK_IO_OFFSET)

#define SZ_1M					0x00100000
#define SECTION_SIZE			SZ_1M

void board_init()	//Invoked by core/hypervisor/init.c:start_
{
	uint32_t section, offset;
	uint32_t start_pa = L4_34XX_PHYS;
	uint32_t end_pa = L4_34XX_PHYS + L4_34XX_SIZE;
	uint32_t start_va = L4_34XX_VIRT;
	uint32_t length = end_pa - start_pa;
	uint32_t number_of_sections = length / SECTION_SIZE;

	for (section = 0, offset = 0; section < number_of_sections; section++, offset += SECTION_SIZE) {
		uint32_t table2_pa = pt_create_coarse(flpt_va, start_va + offset, start_pa + offset, SECTION_SIZE, MLT_IO_RW_REG);
		if (table2_pa == 0) {
			printf("Hypervisor cannot map %x to %x\n", start_va, start_pa);
			while (1);
		}
	}

	start_pa = L4_WK_AM33XX_PHYS;
	end_pa = L4_WK_AM33XX_PHYS + L4_WK_AM33XX_SIZE;
	start_va = L4_WK_AM33XX_VIRT;
	length = end_pa - start_pa;
	number_of_sections = length / SECTION_SIZE;

	for (section = 0, offset = 0; section < number_of_sections; section++, offset += SECTION_SIZE) {
		uint32_t table2_pa = pt_create_coarse(flpt_va, start_va + offset, start_pa + offset, SECTION_SIZE, MLT_IO_RW_REG);
		if (table2_pa == 0) {
			printf("Hypervisor cannot map %x to %x\n", start_va, start_pa);
			while (1);
		}
	}

/*	pt_create_coarse(flpt_va, L3_34XX_VIRT			, __phys_to_pfn(L3_34XX_PHYS)			, L3_34XX_SIZE			, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_34XX_VIRT			, __phys_to_pfn(L4_34XX_PHYS)			, L4_34XX_SIZE			, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, OMAP34XX_GPMC_VIRT	, __phys_to_pfn(OMAP34XX_GPMC_PHYS)		, OMAP34XX_GPMC_SIZE	, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, OMAP343X_SMS_VIRT		, __phys_to_pfn(OMAP343X_SMS_PHYS)		, OMAP343X_SMS_SIZE		, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, OMAP343X_SDRC_VIRT	, __phys_to_pfn(OMAP343X_SDRC_PHYS)		, OMAP343X_SDRC_SIZE	, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_PER_34XX_VIRT		, __phys_to_pfn(L4_PER_34XX_PHYS)		, L4_PER_34XX_SIZE		, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_EMU_34XX_VIRT		, __phys_to_pfn(L4_EMU_34XX_PHYS)		, L4_EMU_34XX_SIZE		, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_WK_AM33XX_VIRT		, __phys_to_pfn(L4_WK_AM33XX_PHYS)		, L4_WK_AM33XX_SIZE		, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L3_44XX_VIRT			, __phys_to_pfn(L3_44XX_PHYS)			, L3_44XX_SIZE			, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_44XX_VIRT			, __phys_to_pfn(L4_44XX_PHYS)			, L4_44XX_SIZE			, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_PER_44XX_VIRT		, __phys_to_pfn(L4_PER_44XX_PHYS)		, L4_PER_44XX_SIZE		, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L3_54XX_VIRT			, __phys_to_pfn(L3_54XX_PHYS)			, L3_54XX_SIZE			, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_54XX_VIRT			, __phys_to_pfn(L4_54XX_PHYS)			, L4_54XX_SIZE			, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_WK_54XX_VIRT		, __phys_to_pfn(L4_WK_54XX_PHYS)		, L4_WK_54XX_SIZE		, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_PER_54XX_VIRT		, __phys_to_pfn(L4_PER_54XX_PHYS)		, L4_PER_54XX_SIZE		, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_CFG_MPU_DRA7XX_VIRT, __phys_to_pfn(L4_CFG_MPU_DRA7XX_PHYS)	, L4_CFG_MPU_DRA7XX_SIZE, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L3_MAIN_SN_DRA7XX_VIRT, __phys_to_pfn(L3_MAIN_SN_DRA7XX_PHYS)	, L3_MAIN_SN_DRA7XX_SIZE, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_PER1_DRA7XX_VIRT	, __phys_to_pfn(L4_PER1_DRA7XX_PHYS)	, L4_PER1_DRA7XX_SIZE	, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_PER2_DRA7XX_VIRT	, __phys_to_pfn(L4_PER2_DRA7XX_PHYS)	, L4_PER2_DRA7XX_SIZE	, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_PER3_DRA7XX_VIRT	, __phys_to_pfn(L4_PER3_DRA7XX_PHYS)	, L4_PER3_DRA7XX_SIZE	, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_CFG_DRA7XX_VIRT	, __phys_to_pfn(L4_CFG_DRA7XX_PHYS)		, L4_CFG_DRA7XX_SIZE	, MLT_IO_RW_REG);
	pt_create_coarse(flpt_va, L4_WKUP_DRA7XX_VIRT	, __phys_to_pfn(L4_WKUP_DRA7XX_PHYS)	, L4_WKUP_DRA7XX_SIZE	, MLT_IO_RW_REG);*/
#if 0
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48040000), 0x48040000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER2: Writes TIOCP_CFG to control parameters of OCP, and used for scheduling. No memory access.*/
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48042000), 0x48042000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER3: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48044000), 0x48044000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER4: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48046000), 0x48046000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER5: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48048000), 0x48048000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER6: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4804A000), 0x4804A000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER7: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4804C000), 0x4804C000, 0x1000, MLT_IO_RW_REG);	/* GPIO1: Writes GPIO_SYSCONFIG to control parameters of the L4 interconnect. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481AC000), 0x481AC000, 0x1000, MLT_IO_RW_REG);	/* GPIO2: Writes GPIO_SYSCONFIG to control parameters of the L4 interconnect. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481AE000), 0x481AE000, 0x1000, MLT_IO_RW_REG);	/* GPIO3: Writes GPIO_SYSCONFIG to control parameters of the L4 interconnect. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48200000), 0x48200000, 0x1000, MLT_IO_RW_REG);	/* Interrupt controller (INTCPS): Linux manages (acks, masks, unmasks) interrupts. No memory access */

	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E00000), 0x44E00000, 0x2000, MLT_IO_RW_REG);	/* CM & PRM: Writes PM_CEFUSE_PWRSTCTRL to control CEFUSE power state. No memory access. */
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E07000), 0x44E07000, 0x1000, MLT_IO_RW_REG);	/* GPIO0: Writes GPIO_FALLINGDETECT: enables/disables detection for interrupt. No memory acceess.*/
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E09000), 0x44E09000, 0x1000, MLT_IO_RW_REG);	/* UART0: Writes to print information to the terminal. No memory access. */
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E0B000), 0x44E0B000, 0x1000, MLT_IO_RW_REG);	/* I2C0: Writes to I2C System Configuration Register, including clock. No memory access. */
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E10000), 0x44E10000, 0x2000, MLT_IO_RW_REG);	/* Control Module: Reads device ID. No memory access. */
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E31000), 0x44E31000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER1_1MS: Writes TIOCP_CFG controlling parameters of the OCP interface. No memory access.*/
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E35000), 0x44E35000, 0x1000, MLT_IO_RW_REG);	/* WDT1: Writes Watchdog System Control Register to set parameters of L4 bus. No memory access.*/
#endif
#define CPSW_SS_VIRT 0xFA400000
#define CPSW_SS_PHYS 0x4A100000
#define CPSW_SS_SIZE 0x00004000
#define PRU_ICSS_VIRT (CPSW_SS_VIRT + CPSW_SS_SIZE)
#define PRU_ICSS_PHYS 0x4A300000
#define PRU_ICSS_SIZE 0x00027000
#define TPCC_VIRT (PRU_ICSS_VIRT + PRU_ICSS_SIZE)
#define TPCC_PHYS 0x49000000
#define TPCC_SIZE 0x00001000
#define TPTC0_VIRT (TPCC_VIRT + TPCC_SIZE)
#define TPTC0_PHYS 0x49800000
#define TPTC0_SIZE 0x00001000
#define TPTC1_VIRT (TPTC0_VIRT + TPTC0_SIZE)
#define TPTC1_PHYS 0x49900000
#define TPTC1_SIZE 0x00001000
#define TPTC2_VIRT (TPTC1_VIRT + TPTC1_SIZE)
#define TPTC2_PHYS 0x49A00000
#define TPTC2_SIZE 0x00001000
#define MMCHS2_VIRT (TPTC2_VIRT + TPTC2_SIZE)
#define MMCHS2_PHYS 0x47810000
#define MMCHS2_SIZE 0x00001000
#define USBSS_VIRT (MMCHS2_VIRT + MMCHS2_SIZE)
#define USBSS_PHYS 0x47400000
#define USBSS_SIZE 0x00008000
#define L3OCMC0_VIRT (USBSS_VIRT + USBSS_SIZE)
#define L3OCMC0_PHYS 0x40300000
#define L3OCMC0_SIZE 0x00010000
#define EMIF0_VIRT (L3OCMC0_VIRT + L3OCMC0_SIZE)
#define EMIF0_PHYS 0x4C000000
#define EMIF0_SIZE 0x00001000
#define GPMC_VIRT (EMIF0_VIRT + EMIF0_SIZE)
#define GPMC_PHYS 0x50000000
#define GPMC_SIZE 0x00001000
#define SHAM_VIRT (GPMC_VIRT + GPMC_SIZE)
#define SHAM_PHYS 0x53100000
#define SHAM_SIZE 0x00001000
#define AES_VIRT (SHAM_VIRT + SHAM_SIZE)
#define AES_PHYS 0x53500000
#define AES_SIZE 0x00001000
#define SGX530_VIRT (AES_VIRT + AES_SIZE)
#define SGX530_PHYS 0x56000000
#define SGX530_SIZE 0x00010000

	//See linux-5.15.13/arch/arm/mach-omap2/io.c.
	//CPSW_SS_VIRT starts on a MB aligned address and all of this space fits
	//within 1 MB. Hence, pt_create_coarse, which can manage a region spanning
	//at most 1 MB starting from a MB aligned address, can be used for these
	//mappings.
	pt_create_coarse(flpt_va, CPSW_SS_VIRT + 0x0000, CPSW_SS_PHYS, 0x1000, MLT_IO_RO_REG);	/* CPSW_SS except MDIO, CPSW_WR, CPPI_RAM 4KB */
	pt_create_coarse(flpt_va, CPSW_SS_VIRT + 0x1000, CPSW_SS_PHYS + 0x1000, 0x1000, MLT_IO_RW_REG);	/* MDIO, CPSW_WR which is 4KB. */
	pt_create_coarse(flpt_va, CPSW_SS_VIRT + 0x2000, CPSW_SS_PHYS + 0x2000, 0x2000, MLT_IO_RO_REG);	/* CPPI_RAM 8KB */
	pt_create_coarse(flpt_va, PRU_ICSS_VIRT, PRU_ICSS_PHYS, PRU_ICSS_SIZE, MLT_IO_RW_REG);	/* PRU_ICSS, 156kB. */
	pt_create_coarse(flpt_va, TPCC_VIRT, TPCC_PHYS, TPCC_SIZE, MLT_IO_RO_REG);	/* TPCC, 4kB. */
	pt_create_coarse(flpt_va, TPTC0_VIRT, TPTC0_PHYS, TPTC0_SIZE, MLT_IO_RO_REG);	/* TPTC0, 4kB. */
	pt_create_coarse(flpt_va, TPTC1_VIRT, TPTC1_PHYS, TPTC1_SIZE, MLT_IO_RO_REG);	/* TPTC2, 4kB. */
	pt_create_coarse(flpt_va, TPTC2_VIRT, TPTC2_PHYS, TPTC2_SIZE, MLT_IO_RO_REG);	/* TPTC3, 4kB. */
	pt_create_coarse(flpt_va, MMCHS2_VIRT, MMCHS2_PHYS, MMCHS2_SIZE, MLT_IO_RW_REG);	/* MMCHS2, 4kB. */
	pt_create_coarse(flpt_va, USBSS_VIRT, USBSS_PHYS, USBSS_SIZE, MLT_IO_RW_REG);	/* USBSS, 4kB. */
	pt_create_coarse(flpt_va, L3OCMC0_VIRT, L3OCMC0_PHYS, L3OCMC0_SIZE, MLT_USER_RAM);	/* L3 OCMC0, 64kB. */
	pt_create_coarse(flpt_va, EMIF0_VIRT, EMIF0_PHYS, EMIF0_SIZE, MLT_IO_RW_REG);	/* EMIF0, 4kB. */
	pt_create_coarse(flpt_va, GPMC_VIRT, GPMC_PHYS, GPMC_SIZE, MLT_IO_RW_REG);	/* GPMC, 4kB. */
	pt_create_coarse(flpt_va, SHAM_VIRT, SHAM_PHYS, SHAM_SIZE, MLT_IO_RW_REG);	/* SHAM, 4kB. */
	pt_create_coarse(flpt_va, AES_VIRT, AES_PHYS, AES_SIZE, MLT_IO_RW_REG);	/* AES, 4kB. */
	pt_create_coarse(flpt_va, SGX530_VIRT, SGX530_PHYS, SGX530_SIZE, MLT_IO_RW_REG);	/* SGX530, 64kB. */

	/*Flush the addresses as we are going to use it soon */
	mem_mmu_tlb_invalidate_all(TRUE, TRUE);
	mem_cache_invalidate(TRUE, TRUE, TRUE);	//instr, data, writeback
}

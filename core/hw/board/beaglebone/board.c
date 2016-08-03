#include <hw.h>
#include "board.h"

extern uint32_t *flpt_va;

#define OMAP2_L4_IO_OFFSET(x) ((x) + 0xb2000000)
#define AM33XX_L4_WK_IO_OFFSET(x) ((x) + 0xb5000000)

//////////// EVERYTHING HERE IS FOR exclusive access for LINUX!!!!!!!!!!!!!!!!!!
void board_init()
{

	//Initial static mappings in linux:
	//OMAP2_L4_IO_OFFSET[0x48000000, 0x48400000) --> [0xFA000000, 0xFA400000):
	//0x48000000 --> 0x48000000 + 0xb2000000 = 0xFA000000 of size 4 MB.
	//0x48000000 + 0x00400000 = 0x48400000 --> 0xFA400000
	//
	//The offset is 0xB2000000.
	//
	//OMAP2_L4_IO_OFFSET:
	//UART1 0x48022000 4KB
	//UART2 0x48024000 4KB
	//I2C1 0x4802A000 4KB
	//McSPI0 0x48030000 4KB
	//McASP0 CFG 0x48038000 8KB
	//McASP1 CFG 0x4803C000 8KB
	//DMTIMER2 0x48040000 4KB
	//DMTIMER3 0x48042000 4KB
	//DMTIMER4 0x48044000 4KB
	//DMTIMER5 0x48046000 4KB
	//DMTIMER6 0x48048000 4KB
	//DMTIMER7 0x4804A000 4KB
	//GPIO1 0x4804C000 4KB
	//MMCHS0 0x48060000 4KB
	//ELM 0x48080000 64KB
	//Mailbox 0 0x480C8000 4KB
	//Spinlock 0x480CA000 4KB
	//OCP Watchpoint 0x4818C000 4KB
	//I2C2 0x4819C000 4KB
	//McSPI1 0x481A0000 4KB
	//UART3 0x481A6000 4KB
	//UART4 0x481A8000 4KB
	//UART5 0x481AA000 4KB
	//GPIO2 0x481AC000 4KB
	//GPIO3 0x481AE000 4KB
	//DCAN0 0x481CC000 8KB
	//DCAN1 0x481D0000 8KB
	//MMC1 0x481D8000 4KB
	//Interrupt controller (INTCPS) 0x48200000 4KB
	//MPUSS config register 0x48240000 4KB
	//PWM Subsystem 0 0x48300000 4KB
	//PWM Subsystem 1 0x48302000 4KB
	//PWM Subsystem 2 0x48304000 4KB
	//LCD Controller 0x4830E000 8KB
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48022000), 0x48022000, 0x1000, MLT_IO_RW_REG);	/* UART1 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48024000), 0x48024000, 0x1000, MLT_IO_RW_REG);	/* UART2 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4802A000), 0x4802A000, 0x1000, MLT_IO_RW_REG);	/* I2C1 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48030000), 0x48030000, 0x1000, MLT_IO_RW_REG);	/* McSPI0 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48038000), 0x48038000, 0x2000, MLT_IO_RW_REG);	/* McASP0 CFG */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4803C000), 0x4803C000, 0x2000, MLT_IO_RW_REG);	/* McASP1 CFG */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48040000), 0x48040000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER2: Writes TIOCP_CFG to control parameters of OCP, and used for scheduling. No memory access.*/
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48042000), 0x48042000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER3: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48044000), 0x48044000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER4: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48046000), 0x48046000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER5: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48048000), 0x48048000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER6: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4804A000), 0x4804A000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER7: Writes TIOCP_CFG to control parameters of OCP. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4804C000), 0x4804C000, 0x1000, MLT_IO_RW_REG);	/* GPIO1: Writes GPIO_SYSCONFIG to control parameters of the L4 interconnect. No memory access. */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48060000), 0x48060000, 0x1000, MLT_IO_RW_REG);	/* MMCHS0 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48080000), 0x48080000, 0x10000, MLT_IO_RW_REG);	/* ELM */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x480C8000), 0x480C8000, 0x1000, MLT_IO_RW_REG);	/* Mailbox 0 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x480CA000), 0x480CA000, 0x1000, MLT_IO_RW_REG);	/* Spinlock */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4818C000), 0x4818C000, 0x1000, MLT_IO_RW_REG);	/* OCP Watchpoint */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4819C000), 0x4819C000, 0x1000, MLT_IO_RW_REG);	/* I2C2 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481A0000), 0x481A0000, 0x1000, MLT_IO_RW_REG);	/* McSPI1 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481A6000), 0x481A6000, 0x1000, MLT_IO_RW_REG);	/* UART3 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481A8000), 0x481A8000, 0x1000, MLT_IO_RW_REG);	/* UART4 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481AA000), 0x481AA000, 0x1000, MLT_IO_RW_REG);	/* UART5 */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481AC000), 0x481AC000, 0x1000, MLT_IO_RW_REG);	/* GPIO2: Writes GPIO_SYSCONFIG to control parameters of the L4 interconnect. No memory access. */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481AE000), 0x481AE000, 0x1000, MLT_IO_RW_REG);	/* GPIO3: Writes GPIO_SYSCONFIG to control parameters of the L4 interconnect. No memory access. */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481CC000), 0x481CC000, 0x2000, MLT_IO_RW_REG);	/* DCAN0 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481D0000), 0x481D0000, 0x2000, MLT_IO_RW_REG);	/* DCAN1 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x481D8000), 0x481D8000, 0x1000, MLT_IO_RW_REG);	/* MMC1 */
	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48200000), 0x48200000, 0x1000, MLT_IO_RW_REG);	/* Interrupt controller (INTCPS): Linux manages (acks, masks, unmasks) interrupts. No memory access */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48240000), 0x48240000, 0x1000, MLT_IO_RW_REG);	/* MPUSS config register */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48300000), 0x48300000, 0x1000, MLT_IO_RW_REG);	/* PWM Subsystem 0 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48302000), 0x48302000, 0x1000, MLT_IO_RW_REG);	/* PWM Subsystem 1 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x48304000), 0x48304000, 0x1000, MLT_IO_RW_REG);	/* PWM Subsystem 2 */
//	pt_create_coarse(flpt_va, OMAP2_L4_IO_OFFSET(0x4830E000), 0x4830E000, 0x2000, MLT_IO_RW_REG);	/* LCD Controller */





	//Initial static mappings in linux:
	//AM33XX_L4_WK_IO_OFFSET: [0x44C00000, 0x45000000) --> [0xF9C00000, 0xFA400000):
	//0x44C00000 --> 0x44C00000 + 0xb5000000 = 0xF9C00000 of size 4 MB.
	//0x44C00000 + 0x00400000 =   0x45000000 --> 0xFA400000
	//
	//The offset is 0xB5000000
	//
	//AM33XX_L4_WK_IO_OFFSET:
	//
	//L4_WKUP configuration 0x44C0_0000 8KB
	//
	//The following CM and PRM peripherals take 8KB starting at 0x44E00000
	//CM_PER 0x44E00000 1KB
	//CM_WKUP 0x44E00400 256 Bytes
	//CM_DPLL 0x44E00500 256 Bytes
	//CM_MPU 0x44E00600 256 Bytes
	//CM_DEVICE 0x44E00700 256 Bytes
	//CM_RTC 0x44E00800 256 Bytes
	//CM_GFX 0x44E00900 256 Bytes
	//CM_CEFUSE 0x44E00A00 256 Bytes
	//PRM_IRQ 0x44E00B00 256 Bytes
	//PRM_PER 0x44E00C00 256 Bytes
	//PRM_WKUP 0x44E00D00 256 Bytes
	//PRM_MPU 0x44E00E00 256 Bytes
	//PRM_DEV 0x44E00F00 256 Bytes
	//PRM_RTC 0x44E01000 256 Bytes 
	//PRM_GFX 0x44E01100 256 Bytes
	//PRM_CEFUSE 0x44E01200 256 Bytes
	//
	//DMTIMER0 0x44E05000 4KB
	//GPIO0 0x44E07000 4KB
	//UART0 0x44E09000 4KB
	//I2C0 0x44E0B000 4KB
	//ADC_TSC 0x44E0D000 8KB
	//Control Module 0x44E10000 8KB
	//DDR2/3/mDDR PHY 0x44E12000 4kB
	//DMTIMER1_1MS 0x44E31000 4KB
	//WDT1 0x44E35000 4KB
	//SmartReflex0 0x44E37000 4KB
	//SmartReflex1 0x44E39000 4KB
	//RTCSS 0x44E3E000 4KB
	//DebugSS Instrumentation HWMaster1 Port 0x44E40000 4KB

	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E00000), 0x44E00000, 0x2000, MLT_IO_RW_REG);	/* CM & PRM: Writes PM_CEFUSE_PWRSTCTRL to control CEFUSE power state. No memory access. */
//	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E05000), 0x44E05000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER0 */
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E07000), 0x44E07000, 0x1000, MLT_IO_RW_REG);	/* GPIO0: Writes GPIO_FALLINGDETECT: enables/disables detection for interrupt. No memory acceess.*/
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E09000), 0x44E09000, 0x1000, MLT_IO_RW_REG);	/* UART0: Writes to print information to the terminal. No memory access. */
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E0B000), 0x44E0B000, 0x1000, MLT_IO_RW_REG);	/* I2C0: Writes to I2C System Configuration Register, including clock. No memory access. */
//	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E0D000), 0x44E0D000, 0x2000, MLT_IO_RW_REG);	/* ADC_TSC */



	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E10000), 0x44E10000, 0x2000, MLT_IO_RO_REG);	/* Control Module: Reads device ID. No memory access. */
//	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E12000), 0x44E12000, 0x1000, MLT_IO_RW_REG);	/* DDR2/3/mDDR PHY */
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E31000), 0x44E31000, 0x1000, MLT_IO_RW_REG);	/* DMTIMER1_1MS: Writes TIOCP_CFG controlling parameters of the OCP interface. No memory access.*/
	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E35000), 0x44E35000, 0x1000, MLT_IO_RW_REG);	/* WDT1: Writes Watchdog System Control Register to set parameters of L4 bus. No memory access.*/
//	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E37000), 0x44E37000, 0x1000, MLT_IO_RW_REG);	/* SmartReflex0 */
//	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E39000), 0x44E39000, 0x1000, MLT_IO_RW_REG);	/* SmartReflex1 */
//	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E3E000), 0x44E3E000, 0x1000, MLT_IO_RW_REG);	/* RTCSS */
//	pt_create_coarse(flpt_va, AM33XX_L4_WK_IO_OFFSET(0x44E40000), 0x44E40000, 0x1000, MLT_IO_RW_REG);	/* DebugSS Instrumentation HWMaster1 Port */





	//ADDED STATIC MAPINGS IN arch/arm/mach-omap2/io.c
	//Initial static mappings in linux:
	//CPSW: [0x4A100000, 0x4A108000) --> [0xFA400000, 0xFA408000):
	/*
	 *  Maps the following memory regions that are related to the CPSW driver
	 *  (their names can be found in AM335x Sitara Processors Technical
	 *  Reference Manual):
	 *  *CPSW_SS: Starts at 0x4A10_0000. Size before CPSW_PORT: 0x100 = 256
	 *   Bytes. The region has read/write access in PL0 since the Linug guest
	 *   owns the physical memory area.
	 *
	 *  *CPSW_PORT: Starts at 0x4A10_0100. Size before CPSW_CPDMA:
	 *   0x700 = 4Ki - 256 Bytes. The region has read/write access in PL0 since
	 *   the Linux guest owns the physical memory area.
	 *
	 *  *CPSW_CPDMA: Starts at 0x4A10_0800. Size before CPSW_STATS:
	 *   0x100 = 256 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *CPSW_STATS: Starts at 0x4A10_0900. Size before CPSW_STATERAM:
	 *   0x100 = 256 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *CPSW_STATERAM: Starts at 0x4A10_0A00. Size before CPSW_CPTS:
	 *   0x200 = 512 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *CPSW_CPTS: Starts at 0x4A10_0C00. Size before CPSW_ALE:
	 *   0x100 = 256 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *CPSW_ALE: Starts at 0x4A10_0D00. Size before CPSW_SL1:
	 *   0x80 = 128 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *CPSW_SL1: Starts at 0x4A10_0D80. Size before CPSW_SL2:
	 *   0x40 = 64 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *CPSW_SL2: Starts at 0x4A10_0DC0. Size before RESERVED:
	 *   0x40 = 64 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *MDIO: Starts at 0x4A10_1000. Size before RESERVED:
	 *   0x100 = 256 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  *CPSW_WR: Starts at 0x4A10_1200. Size before CPPI_RAM:
	 *   0xE00 = 3584 Bytes. The region has read/write access in PL0 since the
	 *   Linux guest owns the physical memory area.
	 *
	 *  From, and including, CPSW_SW to, and including, CPSW_SL2 is a memory
	 *  region consisting of the following physical addresses:
	 *  [0x4A10_0000, 0x4A10_1000) which is a size of 0x1000 = 4KiB = 1 page.
	 */
	pt_create_coarse(flpt_va, 0xFA400000, 0x4A100000, 0x1000, MLT_IO_RO_REG);	/* CPSW_SS except MDIO, CPSW_WR, CPPI_RAM 4KB */
	/*
	 *  MDIO and CPSW_WR does not affect DMA transfers from/to memory and hence
	 *  they can be read/write. Size is 0x1000 = 4KiB which is one page.
	 */
	pt_create_coarse(flpt_va, 0xFA401000, 0x4A101000, 0x1000, MLT_IO_RW_REG);	/* MDIO, CPSW_WR which is 4KB. */
	/*
	 *  Maps the following memory region that is related to the CPSW driver
	 *  (their names can be found in AM335x Sitara Processors Technical
	 *  Reference Manual):
	 *  *CPPI_RAM: Starts at 0x4A10_2000. Size is: 0x2000 = 8KiB = 2 pages.
	 *   The region has read-only access since it controls DMA transfers.
	 */
	pt_create_coarse(flpt_va, 0xFA402000, 0x4A102000, 0x2000, MLT_IO_RO_REG);	/* CPPI_RAM 8KB */


	//Additional Linux mappings in the range [0xFA408000, 0xFA534000)
//	pt_create_coarse(flpt_va, 0xFA408000, 0x4C000000, 0x1000, MLT_IO_RW_REG);	/* EMIF0 */
//	pt_create_coarse(flpt_va, 0xFA409000, 0x47810000, 0x11000, MLT_IO_RW_REG);	/* MMCHS2 */
//	pt_create_coarse(flpt_va, 0xFA41A000, 0x50000000, 0x2000, MLT_IO_RW_REG);	/* GPMC */
//	pt_create_coarse(flpt_va, 0xFA41C000, 0x49800000, 0x2000, MLT_IO_RW_REG);	/* TPTC0 (EDMA3TC0) */
//	pt_create_coarse(flpt_va, 0xFA41E000, 0x49900000, 0x2000, MLT_IO_RW_REG);	/* TPTC1 (EDMA3TC1) */
//	pt_create_coarse(flpt_va, 0xFA420000, 0x49A00000, 0x2000, MLT_IO_RW_REG);	/* TPTC2 (EDMA3TC2) */
//	pt_create_coarse(flpt_va, 0xFA422000, 0x47400000, 0x1000, MLT_IO_RW_REG);	/* USBSS */
//	pt_create_coarse(flpt_va, 0xFA423000, 0x53100000, 0x1000, MLT_IO_RW_REG);	/* "sham" */
//	pt_create_coarse(flpt_va, 0xFA424000, 0x53500000, 0x100000, MLT_IO_RW_REG);	/* "aes" */
	pt_create_coarse(flpt_va, 0xFA524000, 0x40300000, 0x10000, MLT_USER_RAM);	/* L3 OCMC0: Comment in arch/arm/plat-omap/sram.c:omap_map_sram: "clock init needs SRAM early." */


	////
	/*Flush the addresses as we are going to use it soon */
	mem_mmu_tlb_invalidate_all(TRUE, TRUE);
	mem_cache_invalidate(TRUE, TRUE, TRUE);	//instr, data, writeback
}

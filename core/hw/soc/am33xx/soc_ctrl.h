
#ifndef _SOC_CTRL_H_

/* CTRL */
typedef struct {
	uint32_t control_revision;
	uint32_t control_hwinfo;
	uint32_t unused0[2];
	uint32_t control_sysconfig;
	uint32_t unused1[11];
	uint32_t control_status;
	uint32_t unused2[246];
	uint32_t cortex_vbbldo_ctrl;
	uint32_t unused3[2];
	uint32_t core_sldo_ctrl;
	uint32_t mpu_sldo_ctrl;
	uint32_t unused4[5];
	uint32_t clk32kdivratio_ctrl;
	uint32_t bandgap_ctrl;
	uint32_t bandgap_trim;
	uint32_t unused5[2];
	uint32_t pll_clkinpulow_ctrl;
	uint32_t unused6[3];
	uint32_t mosc_ctrl;
	uint32_t rcosc_ctrl;
	uint32_t deepsleep_ctrl;
	uint32_t unused7[99];
	uint32_t device_id;
	uint32_t dev_feature;
	uint32_t init_priority_0;
	uint32_t init_priority_1;
	uint32_t mmu_cfg;
	uint32_t tptc_cfg;
	uint32_t unused8[2];
	uint32_t usb_ctrl0;
	uint32_t usb_sts0;
	uint32_t usb_ctrl1;
	uint32_t usb_sts1;
	uint32_t mac_id0_lo;
	uint32_t mac_id0_hi;
	uint32_t mac_id1_lo;
	uint32_t mac_id1_hi;
	uint32_t unused9;
	uint32_t dcan_raminit;
	uint32_t usb_wkup_ctrl;
	uint32_t unused10;
	uint32_t gmii_sel;
	uint32_t unused11[4];
	uint32_t pwmss_ctrl;
	uint32_t unused12[2];
	uint32_t mreqprio_0;
	uint32_t mreqprio_1;
	uint32_t unused13[6];
	uint32_t hw_event_sel_grp1;
	uint32_t hw_event_sel_grp2;
	uint32_t hw_event_sel_grp3;
	uint32_t hw_event_sel_grp4;
	uint32_t smrt_ctrl;
	uint32_t mpuss_hw_debug_sel;
	uint32_t mpuss_hw_dbg_info;
	uint32_t unused14[49];
	uint32_t vdd_mpu_opp_050;
	uint32_t vdd_mpu_opp_100;
	uint32_t vdd_mpu_opp_120;
	uint32_t vdd_mpu_opp_turbo;
	uint32_t unused15[14];
	uint32_t vdd_core_opp_050;
	uint32_t vdd_core_opp_100;
	uint32_t unused16[4];
	uint32_t bb_scale;
	uint32_t unused17[8];
	uint32_t usb_vid_pid;
	uint32_t unused18[2];
	uint32_t conf_gpmc_ad0;
	uint32_t conf_gpmc_ad1;
	uint32_t conf_gpmc_ad2;
	uint32_t conf_gpmc_ad3;
	uint32_t conf_gpmc_ad4;
	uint32_t conf_gpmc_ad5;
	uint32_t conf_gpmc_ad6;
	uint32_t conf_gpmc_ad7;
	uint32_t conf_gpmc_ad8;
	uint32_t conf_gpmc_ad9;
	uint32_t conf_gpmc_ad10;
	uint32_t conf_gpmc_ad11;
	uint32_t conf_gpmc_ad12;
	uint32_t conf_gpmc_ad13;
	uint32_t conf_gpmc_ad14;
	uint32_t conf_gpmc_ad15;
	uint32_t conf_gpmc_a0;
	uint32_t conf_gpmc_a1;
	uint32_t conf_gpmc_a2;
	uint32_t conf_gpmc_a3;
	uint32_t conf_gpmc_a4;
	uint32_t conf_gpmc_a5;
	uint32_t conf_gpmc_a6;
	uint32_t conf_gpmc_a7;
	uint32_t conf_gpmc_a8;
	uint32_t conf_gpmc_a9;
	uint32_t conf_gpmc_a10;
	uint32_t conf_gpmc_a11;
	uint32_t conf_gpmc_wait0;
	uint32_t conf_gpmc_wpn;
	uint32_t conf_gpmc_be1n;
	uint32_t conf_gpmc_csn0;
	uint32_t conf_gpmc_csn1;
	uint32_t conf_gpmc_csn2;
	uint32_t conf_gpmc_csn3;
	uint32_t conf_gpmc_clk;
	uint32_t conf_gpmc_advn_ale;
	uint32_t conf_gpmc_oen_ren;
	uint32_t conf_gpmc_wen;
	uint32_t conf_gpmc_be0n_cle;
	uint32_t conf_lcd_data0;
	uint32_t conf_lcd_data1;
	uint32_t conf_lcd_data2;
	uint32_t conf_lcd_data3;
	uint32_t conf_lcd_data4;
	uint32_t conf_lcd_data5;
	uint32_t conf_lcd_data6;
	uint32_t conf_lcd_data7;
	uint32_t conf_lcd_data8;
	uint32_t conf_lcd_data9;
	uint32_t conf_lcd_data10;
	uint32_t conf_lcd_data11;
	uint32_t conf_lcd_data12;
	uint32_t conf_lcd_data13;
	uint32_t conf_lcd_data14;
	uint32_t conf_lcd_data15;
	uint32_t conf_lcd_vsync;
	uint32_t conf_lcd_hsync;
	uint32_t conf_lcd_pclk;
	uint32_t conf_lcd_ac_bias_en;
	uint32_t conf_mmc0_dat3;
	uint32_t conf_mmc0_dat2;
	uint32_t conf_mmc0_dat1;
	uint32_t conf_mmc0_dat0;
	uint32_t conf_mmc0_clk;
	uint32_t conf_mmc0_cmd;
	uint32_t conf_mii1_col;
	uint32_t conf_mii1_crs;
	uint32_t conf_mii1_rxerr;
	uint32_t conf_mii1_txen;
	uint32_t conf_mii1_rxdv;
	uint32_t conf_mii1_txd3;
	uint32_t conf_mii1_txd2;
	uint32_t conf_mii1_txd1;
	uint32_t conf_mii1_txd0;
	uint32_t conf_mii1_txclk;
	uint32_t conf_mii1_rxclk;
	uint32_t conf_mii1_rxd3;
	uint32_t conf_mii1_rxd2;
	uint32_t conf_mii1_rxd1;
	uint32_t conf_mii1_rxd0;
	uint32_t conf_rmii1_refclk;
	uint32_t conf_mdio_data;
	uint32_t conf_mdio_clk;
	uint32_t conf_spi0_sclk;
	uint32_t conf_spi0_d0;
	uint32_t conf_spi0_d1;
	uint32_t conf_spi0_cs0;
	uint32_t conf_spi0_cs1;
	uint32_t conf_ecap0_in_pwm0_out;
	uint32_t conf_uart0_ctsn;
	uint32_t conf_uart0_rtsn;
	uint32_t conf_uart0_rxd;
	uint32_t conf_uart0_txd;
	uint32_t conf_uart1_ctsn;
	uint32_t conf_uart1_rtsn;
	uint32_t conf_uart1_rxd;
	uint32_t conf_uart1_txd;
	uint32_t conf_i2c0_sda;
	uint32_t conf_i2c0_scl;
	uint32_t conf_mcasp0_aclkx;
	uint32_t conf_mcasp0_fsx;
	uint32_t conf_mcasp0_axr0;
	uint32_t conf_mcasp0_ahclkr;
	uint32_t conf_mcasp0_aclkr;
	uint32_t conf_mcasp0_fsr;
	uint32_t conf_mcasp0_axr1;
	uint32_t conf_mcasp0_ahclkx;
	uint32_t conf_xdma_event_intr0;
	uint32_t conf_xdma_event_intr1;
	uint32_t conf_nresetin_out;
	uint32_t conf_porz;
	uint32_t conf_nnmi;
	uint32_t conf_osc0_in;
	uint32_t conf_osc0_out;
	uint32_t conf_osc0_vss;
	uint32_t conf_tms;
	uint32_t conf_tdi;
	uint32_t conf_tdo;
	uint32_t conf_tck;
	uint32_t conf_ntrst;
	uint32_t conf_emu0;
	uint32_t conf_emu1;
	uint32_t conf_osc1_in;
	uint32_t conf_osc1_out;
	uint32_t conf_osc1_vss;
	uint32_t conf_rtc_porz;
	uint32_t conf_pmic_power_en;
	uint32_t conf_ext_wakeup;
	uint32_t conf_enz_kaldo_1p8v;
	uint32_t conf_usb0_dm;
	uint32_t conf_usb0_dp;
	uint32_t conf_usb0_ce;
	uint32_t conf_usb0_id;
	uint32_t conf_usb0_vbus;
	uint32_t conf_usb0_drvvbus;
	uint32_t conf_usb1_dm;
	uint32_t conf_usb1_dp;
	uint32_t conf_usb1_ce;
	uint32_t conf_usb1_id;
	uint32_t conf_usb1_vbus;
	uint32_t conf_usb1_drvvbus;
	uint32_t conf_ddr_resetn;
	uint32_t conf_ddr_csn0;
	uint32_t conf_ddr_cke;
	uint32_t unused19;
	uint32_t conf_ddr_nck;
	uint32_t conf_ddr_casn;
	uint32_t conf_ddr_rasn;
	uint32_t conf_ddr_wen;
	uint32_t conf_ddr_ba0;
	uint32_t conf_ddr_ba1;
	uint32_t conf_ddr_ba2;
	uint32_t conf_ddr_a0;
	uint32_t conf_ddr_a1;
	uint32_t conf_ddr_a2;
	uint32_t conf_ddr_a3;
	uint32_t conf_ddr_a4;
	uint32_t conf_ddr_a5;
	uint32_t conf_ddr_a6;
	uint32_t conf_ddr_a7;
	uint32_t conf_ddr_a8;
	uint32_t conf_ddr_a9;
	uint32_t conf_ddr_a10;
	uint32_t conf_ddr_a11;
	uint32_t conf_ddr_a12;
	uint32_t conf_ddr_a13;
	uint32_t conf_ddr_a14;
	uint32_t conf_ddr_a15;
	uint32_t conf_ddr_odt;
	uint32_t conf_ddr_d0;
	uint32_t conf_ddr_d1;
	uint32_t conf_ddr_d2;
	uint32_t conf_ddr_d3;
	uint32_t conf_ddr_d4;
	uint32_t conf_ddr_d5;
	uint32_t conf_ddr_d6;
	uint32_t conf_ddr_d7;
	uint32_t conf_ddr_d8;
	uint32_t conf_ddr_d9;
	uint32_t conf_ddr_d10;
	uint32_t conf_ddr_d11;
	uint32_t conf_ddr_d12;
	uint32_t conf_ddr_d13;
	uint32_t conf_ddr_d14;
	uint32_t conf_ddr_d15;
	uint32_t conf_ddr_dqm0;
	uint32_t conf_ddr_dqm1;
	uint32_t conf_ddr_dqs0;
	uint32_t conf_ddr_dqsn0;
	uint32_t conf_ddr_dqs1;
	uint32_t conf_ddr_dqsn1;
	uint32_t conf_ddr_vref;
	uint32_t conf_ddr_vtp;
	uint32_t conf_ddr_strben0;
	uint32_t conf_ddr_strben1;
	uint32_t conf_ain7;
	uint32_t conf_ain6;
	uint32_t conf_ain5;
	uint32_t conf_ain4;
	uint32_t conf_ain3;
	uint32_t conf_ain2;
	uint32_t conf_ain1;
	uint32_t conf_ain0;
	uint32_t conf_vrefp;
	uint32_t conf_vrefn;
	uint32_t conf_avdd;
	uint32_t conf_avss;
	uint32_t conf_iforce;
	uint32_t conf_vsense;
	uint32_t conf_testout;
	uint32_t unused20[173];
	uint32_t cqdetect_status;
	uint32_t ddr_io_ctrl;
	uint32_t unused21;
	uint32_t vtp_ctrl;
	uint32_t unused22;
	uint32_t vref_ctrl;
	uint32_t unused23[94];
	uint32_t tpcc_evt_mux[64 / 4];
	uint32_t timer_evt_capt;
	uint32_t ecap_evt_capt;
	uint32_t adc_evt_capt;
	uint32_t unused24[9];
	uint32_t reset_iso;
	uint32_t unused25[198];
	uint32_t ddr_cke_ctrl;
	uint32_t sma2;
	uint32_t m3_txev_eoi;
	uint32_t ipc_msg_reg0;
	uint32_t ipc_msg_reg1;
	uint32_t ipc_msg_reg2;
	uint32_t ipc_msg_reg3;
	uint32_t ipc_msg_reg4;
	uint32_t ipc_msg_reg5;
	uint32_t ipc_msg_reg6;
	uint32_t ipc_msg_reg7;
	uint32_t unused26[47];
	uint32_t ddr_cmd0_ioctrl;
	uint32_t ddr_cmd1_ioctrl;
	uint32_t ddr_cmd2_ioctrl;
	uint32_t unused27[12];
	uint32_t ddr_data0_ioctrl;
	uint32_t ddr_data1_ioctrl;
} volatile am355_ctrl;

typedef struct {
	uint32_t l4ls_clkstctrl;
	uint32_t l3s_clkstctrl;
	uint32_t unused0;
	uint32_t l3_clkstctrl;
	uint32_t unused1;
	uint32_t cpgmac0;
	uint32_t lcdc;
	uint32_t usb0;
	uint32_t unused2;
	uint32_t tptc0;
	uint32_t emif;
	uint32_t ocmcram;
	uint32_t gpmc;
	uint32_t mcasp0;
	uint32_t uart5;
	uint32_t mmc0;
	uint32_t elm;
	uint32_t i2c2;
	uint32_t i2c1;
	uint32_t spi0;
	uint32_t spi1;
	uint32_t unused3[3];
	uint32_t l4ls;
	uint32_t l4fw;
	uint32_t mcasp1;
	uint32_t uart1;
	uint32_t uart2;
	uint32_t uart3;
	uint32_t uart4;
	uint32_t timer7;
	uint32_t timer2;
	uint32_t timer3;
	uint32_t timer4;
	uint32_t unused4[8];
	uint32_t gpio1;
	uint32_t gpio2;
	uint32_t gpio3;
	uint32_t unused5;
	uint32_t tpcc;
	uint32_t dcan0;
	uint32_t dcan1;
	uint32_t unused6;
	uint32_t epwmss1;
	uint32_t unused7;
	uint32_t epwmss0;
	uint32_t epwmss2;
	uint32_t l3_instr;
	uint32_t l3;
	uint32_t ieee5000;
	uint32_t pruss;
	uint32_t timer5;
	uint32_t timer6;
	uint32_t mmc1;
	uint32_t mmc2;
	uint32_t tptc1;
	uint32_t tptc2;
	uint32_t unused8[2];
	uint32_t spinlock;
	uint32_t mailbox0;
	uint32_t unused9[2];
	uint32_t l4hs_clkstctrl;
	uint32_t l4hs;
	uint32_t unused10[2];
	uint32_t ocpwp_l3_clkstctrl;
	uint32_t ocpwp;
	uint32_t unused11[3];
	uint32_t pruss_clkstctrl;
	uint32_t cpsw_clkstctrl;
	uint32_t lcdc_clkstctrl;
	uint32_t clkdiv32k;
	uint32_t clk_24mhz_clkstctrl;
} volatile clock_per;

typedef struct {
	uint32_t clkstctrl;
	uint32_t control;
	uint32_t gpio0;
	uint32_t l4wkup;
	uint32_t timer0;
	uint32_t debugss;
	uint32_t l3_aon_clkstctrl;
	uint32_t autoidle_dpll_mpu;
	uint32_t idlest_dpll_mpu;
	uint32_t ssc_deltamstep_dpll_mpu;
	uint32_t ssc_modfreqdiv_dpll_mpu;
	uint32_t clksel_dpll_mpu;
	uint32_t autoidle_dpll_ddr;
	uint32_t idlest_dpll_ddr;
	uint32_t ssc_deltamstep_dpll_ddr;
	uint32_t ssc_modfreqdiv_dpll_ddr;
	uint32_t clksel_dpll_ddr;
	uint32_t autoidle_dpll_disp;
	uint32_t idlest_dpll_disp;
	uint32_t ssc_deltamstep_dpll_disp;
	uint32_t ssc_modfreqdiv_dpll_disp;
	uint32_t clksel_dpll_disp;
	uint32_t autoidle_dpll_core;
	uint32_t idlest_dpll_core;
	uint32_t ssc_deltamstep_dpll_core;
	uint32_t ssc_modfreqdiv_dpll_core;
	uint32_t clksel_dpll_core;
	uint32_t autoidle_dpll_per;
	uint32_t idlest_dpll_per;
	uint32_t ssc_deltamstep_dpll_per;
	uint32_t ssc_modfreqdiv_dpll_per;
	uint32_t clkdcoldo_dpll_per;
	uint32_t div_m4_dpll_core;
	uint32_t div_m5_dpll_core;
	uint32_t clkmode_dpll_mpu;
	uint32_t clkmode_dpll_per;
	uint32_t clkmode_dpll_core;
	uint32_t clkmode_dpll_ddr;
	uint32_t clkmode_dpll_disp;
	uint32_t clksel_dpll_periph;
	uint32_t div_m2_dpll_ddr;
	uint32_t div_m2_dpll_disp;
	uint32_t div_m2_dpll_mpu;
	uint32_t div_m2_dpll_per;
	uint32_t wkup_m3;
	uint32_t uart0;
	uint32_t i2c0;
	uint32_t adc_tsc;
	uint32_t smartreflex0;
	uint32_t timer1;
	uint32_t smartreflex1;
	uint32_t l4_wkup_aon_clkstctrl;
	uint32_t unused0;
	uint32_t wdt1;
	uint32_t div_m6_dpll_core;

} volatile clock_wakeup;

#endif				/* _SOC_CTRL_H_ */

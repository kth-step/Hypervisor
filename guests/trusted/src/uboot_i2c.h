#ifndef _UBOOT_I2C_H_
#define _UBOOT_I2C_H_

/*I2C registers offset*/
#define  I2C_REVNB_LO                   0x00000000
#define  I2C_REVNB_HI                   0x00000004
#define  I2C_SYSC                       0x00000010
#define  I2C_IRQSTATUS_RAW              0x00000024
#define  I2C_IRQSTATUS                  0x00000028
#define  I2C_IRQENABLE_SET              0x0000002C
#define  I2C_IRQENABLE_CLR              0x00000030
#define  I2C_WE                         0x00000034
#define  I2C_DMARXENABLE_SET            0x00000038
#define  I2C_DMATXENABLE_SET            0x0000003C
#define  I2C_DMARXENABLE_CLR            0x00000040
#define  I2C_DMATXENABLE_CLR            0x00000044
#define  I2C_DMARXWAKE_EN               0x00000048
#define  I2C_DMATXWAKE_EN               0x0000004C
#define  I2C_SYSS                       0x00000090
#define  I2C_BUF                        0x00000094
#define  I2C_CNT                        0x00000098
#define  I2C_DATA                       0x0000009C
#define  I2C_CON                        0x000000A4
#define  I2C_OA                         0x000000A8
#define  I2C_SA                         0x000000AC
#define  I2C_PSC                        0x000000B0
#define  I2C_SCLL                       0x000000B4
#define  I2C_SCLH                       0x000000B8
#define  I2C_SYSTEST                    0x000000BC
#define  I2C_BUFSTAT                    0x000000C0
#define  I2C_OA1                        0x000000C4
#define  I2C_OA2                        0x000000C8
#define  I2C_OA3                        0x000000CC
#define  I2C_ACTOA                      0x000000D0
#define  I2C_SBLOCK                     0x000000D4

// I2C_CON bits
#define CON_I2C_EN (1 << 15)
#define CON_MST (1 << 10)
#define CON_TRX (1 << 9)
#define CON_STP (1 << 1)
#define CON_STT (1 << 0)

// I2C_SYSC bits
#define SYSC_SRST (1 << 1)

// I2C_SYSS bits
#define SYSS_RDONE (1 << 0)

/* I2C_IRQSTATUS bits */
#define IRQSTATUS_XDR (1 << 14)
#define IRQSTATUS_RDR (1 << 13)
#define IRQSTATUS_BB (1 << 12)
#define IRQSTATUS_ROVR (1 << 11)
#define IRQSTATUS_XUDF (1 << 10)
#define IRQSTATUS_BF (1 << 8)
#define IRQSTATUS_XRDY (1 << 4)
#define IRQSTATUS_RRDY (1 << 3)
#define IRQSTATUS_ARDY (1 << 2)
#define IRQSTATUS_NACK (1 << 1)
#define IRQSTATUS_AL (1 << 0)

/* I2C_IRQSTATUS_RAW bits */
#define IRQSTATUS_AERR (1 << 7)

/* I2C_IRQENABLE_SET and I2C_IRQENABLE_SCLR bits */
#define IRQENABLE_XDR_IE (1 << 14)
#define IRQENABLE_RDR_IE (1 << 13)
#define IRQENABLE_ROVR_IE (1 << 11)
#define IRQENABLE_XUDF_IE (1 << 10)
#define IRQENABLE_BF_IE (1 << 8)
#define IRQENABLE_AERR_IE (1 << 7)
#define IRQENABLE_XRDY_IE (1 << 4)
#define IRQENABLE_RRDY_IE (1 << 3)
#define IRQENABLE_ARDY_IE (1 << 2)
#define IRQENABLE_NACK_IE (1 << 1)
#define IRQENABLE_AL_IE (1 << 0)

/* I2C System Test Register (I2C_SYSTEST): */

#define SYSTEST_ST_EN	(1 << 15) /* System test enable */
#define SYSTEST_FREE	(1 << 14) /* Free running mode, on brkpoint) */
#define SYSTEST_TMODE_MASK	(3 << 12) /* Test mode select */
#define SYSTEST_TMODE_SHIFT	(12)	  /* Test mode select */
#define SYSTEST_SCL_I	(1 << 3)  /* SCL line sense input value */
#define SYSTEST_SCL_O	(1 << 2)  /* SCL line drive output value */
#define SYSTEST_SDA_I	(1 << 1)  /* SDA line sense input value */
#define SYSTEST_SDA_O	(1 << 0)  /* SDA line drive output value */

/* I2C_WE bits */
#define XDR_WE (1 << 14)
#define RDR_WE (1 << 13)
#define DRDY_WE (1 << 3)
#define ARDY_WE (1 << 2)


/* I2C_BUF bits */
#define BUF_RXTRSH_SHIFT 8
#define BUF_RXFIFO_CLR (1 << 14)
#define BUF_TXFIFO_CLR (1 << 6)


/* I2C_BUFSTAT bits */
#define BUFSTAT_TXSTAT_MASK 0x003F
#define BUFSTAT_RXSTAT_MASK 0x3F00
#define BUFSTAT_RXSTAT_SHIFT 8
#define BUFSTAT_TRSH_MASK 0x3F

#define I2C_TIMEOUT 1000

#define OMAP_I2C_STANDARD	100000
#define OMAP_I2C_FAST_MODE	400000
#define OMAP_I2C_HIGH_SPEED	3400000

#define I2C_IP_CLK  96000000
#define I2C_INTERNAL_SAMPLING_CLK	19200000

#define I2C_PSC_MAX		0x0F
#define I2C_PSC_MIN		0x00
#define I2C_HIGHSPEED_PHASE_ONE_SCLL_TRIM  7
#define I2C_HIGHSPEED_PHASE_ONE_SCLH_TRIM  5
#define I2C_HIGHSPEED_PHASE_TWO_SCLL_TRIM  7
#define I2C_HIGHSPEED_PHASE_TWO_SCLH_TRIM  5
#define I2C_FASTSPEED_SCLL_TRIM		7
#define I2C_FASTSPEED_SCLH_TRIM		5

void ub_flush_fifo(void);
int wait_for_bb(int waitdelay);
uint16_t wait_for_event(int waitdelay);
void i2c_deblock(void);
void enable_i2c0_pin_mux (void);
void enable_i2c1_pin_mux (void);
void enable_i2c2_pin_mux (void);
void udelay(int count);
int i2c_findpsc(uint32_t *pscl, uint32_t *psch, int speed);
int i2c_setspeed(int speed, int *waitdelay);
void ub_i2c_init(int speed, int slaveaddr, int *waitdelay);
int i2c_read(int waitdelay,uint8_t slaveaddr, uint8_t offset, int alen, uint8_t *buffer, int len);
int i2c_write(int waitdelay, uint8_t slaveaddr, uint8_t offset, int alen, uint8_t *buffer, int len);
int i2c_probe(int waitdelay,uint8_t slaveaddr);
void i2c_eeprom_test(void);
void i2c2_cam_test(void);

#endif /* _UBOOT_I2C_H_ */

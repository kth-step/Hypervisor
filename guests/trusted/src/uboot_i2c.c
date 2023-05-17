#include <lib.h>
#include "uboot_i2c.h"

//#define I2C0_BASE 0x44E0B000
//#define I2C1_BASE 0x4802A000
#define I2C2_BASE 0x4819C000

//#define baseaddr   0x4819C000

#ifdef I2C0_BASE
#define OFFSET(x) ((x) + 0xB5000000)
#else
#define OFFSET(x) ((x) + 0xB2000000)
#endif

#define phys_to_virt(phys) (OFFSET(phys))

#ifdef I2C0_BASE
#define value_at_register(offset) (*((volatile  uint32_t *) phys_to_virt(I2C0_BASE + (offset))))
//#ifdef I2C1_BASE
//#define value_at_register(offset) (*((volatile  uint32_t *) phys_to_virt(I2C1_BASE + (offset))))
#else
#define value_at_register(offset) (*((volatile  uint32_t *) phys_to_virt(I2C2_BASE + (offset))))
#endif


void ub_flush_fifo(void)
{
	uint16_t stat;

	/*
	 * note: if you try and read data when its not there or ready
	 * you get a bus error
	 */
	while (1) {
		stat = value_at_register(I2C_IRQSTATUS);
		if (stat == IRQSTATUS_RRDY) {
			uint8_t data = value_at_register(I2C_DATA);
			value_at_register(I2C_IRQSTATUS) = IRQSTATUS_RRDY;
			udelay(1000);
		} else
			break;
	}
}

int wait_for_bb(int waitdelay)
{
	int timeout = I2C_TIMEOUT;
	int irq_stat_reg;
	uint16_t stat;

	/* clear current interrupts */
	value_at_register(I2C_IRQSTATUS) = 0xFFFF;

	while ((stat = value_at_register(I2C_IRQSTATUS_RAW) & IRQSTATUS_BB) && timeout--) {
		value_at_register(I2C_IRQSTATUS) = stat;
		udelay(waitdelay);
	}

	if (timeout <= 0) {
		printf("Timed out \n");
		return 1;
	}

	/* clear delayed stuff */
	value_at_register(I2C_IRQSTATUS) = 0xFFFF;
	return 0;
}

uint16_t wait_for_event(int waitdelay)
{
	uint16_t status;
	int timeout = I2C_TIMEOUT;
	
	do {
		udelay(waitdelay);
		status = value_at_register(I2C_IRQSTATUS_RAW);
	} while (!(status &
		   (IRQSTATUS_ROVR | IRQSTATUS_XUDF | IRQSTATUS_XRDY |
		    IRQSTATUS_RRDY | IRQSTATUS_ARDY | IRQSTATUS_NACK |
		    IRQSTATUS_AL)) && timeout--);

	if (timeout <= 0) {
		printf("Timed out in: status=%x\n", status);
		/*
		 * If status is still 0 here, probably the bus pads have
		 * not been configured for I2C, and/or pull-ups are missing.
		 */
		printf("Check if pads/pull-ups of bus are properly configured\n");
		value_at_register(I2C_IRQSTATUS) = 0xFFFF;
		status = 0;
	}

	return status;
}

void i2c_deblock(void)
{
	int i;
	uint16_t systest;
	uint16_t orgsystest;

	/* set test mode ST_EN = 1 */
	orgsystest = value_at_register(I2C_SYSTEST);
	systest = orgsystest;

	/* enable testmode */
	systest |= SYSTEST_ST_EN;
	value_at_register(I2C_SYSTEST) = systest;
	systest &= ~SYSTEST_TMODE_MASK;
	systest |= 3 << SYSTEST_TMODE_SHIFT;
	value_at_register(I2C_SYSTEST) = systest;

	/* set SCL, SDA  = 1 */
	systest |= SYSTEST_SCL_O | SYSTEST_SDA_O;
	value_at_register(I2C_SYSTEST) = systest;
	udelay(10);

	/* toggle scl 9 clocks */
	for (i = 0; i < 9; i++) {
		/* SCL = 0 */
		systest &= ~SYSTEST_SCL_O;
		value_at_register(I2C_SYSTEST) = systest;
		udelay(10);
		/* SCL = 1 */
		systest |= SYSTEST_SCL_O;
		value_at_register(I2C_SYSTEST) = systest;
		udelay(10);
	}

	/* send stop */
	systest &= ~SYSTEST_SDA_O;
	value_at_register(I2C_SYSTEST) = systest;
	udelay(10);
	systest |= SYSTEST_SCL_O | SYSTEST_SDA_O;
	value_at_register(I2C_SYSTEST) = systest;
	udelay(10);

	/* restore original mode */
	value_at_register(I2C_SYSTEST) = orgsystest;
}


void udelay(int count)
{
   while(count){
    count--;}
}

int i2c_findpsc(uint32_t *pscl, uint32_t *psch, int speed)
{
	uint32_t internal_clk = 0, fclk;
	int prescaler;

	/*
	 * This method is only called for Standard and Fast Mode speeds
	 *
	 * For some TI SoCs it is explicitly written in TRM (e,g, SPRUHZ6G,
	 * page 5685, Table 24-7)
	 * that the internal I2C clock (after prescaler) should be between
	 * 7-12 MHz (at least for Fast Mode (FS)).
	 *
	 * Such approach is used in v4.9 Linux kernel in:
	 * ./drivers/i2c/busses/i2c-omap.c (omap_i2c_init function).
	 */

	speed /= 1000; /* convert speed to kHz */

	if (speed > 100)
		internal_clk = 9600;
	else
		internal_clk = 4000;

	fclk = I2C_IP_CLK / 1000;
	prescaler = fclk / internal_clk;
	prescaler = prescaler - 1;

	if (speed > 100) {
		uint32_t scl;

		/* Fast mode */
		scl = internal_clk / speed;
		*pscl = scl - (scl / 3) - I2C_FASTSPEED_SCLL_TRIM;
		*psch = (scl / 3) - I2C_FASTSPEED_SCLH_TRIM;
	} else {
		/* Standard mode */
		*pscl = internal_clk / (speed * 2) - I2C_FASTSPEED_SCLL_TRIM;
		*psch = internal_clk / (speed * 2) - I2C_FASTSPEED_SCLH_TRIM;
	}

	//printf("speed [kHz]: %d psc: 0x%x sscl: 0x%x ssch: 0x%x\n", speed, prescaler, *pscl, *psch);

	if (*pscl <= 0 || *psch <= 0 || prescaler <= 0)
		return -1;

	return prescaler;
}

int i2c_setspeed(int speed, int *waitdelay)
{
	int psc, fsscll = 0, fssclh = 0;
	int hsscll = 0, hssclh = 0;
	uint32_t scll = 0, sclh = 0;

	if (speed >= OMAP_I2C_HIGH_SPEED) {
		/* High speed */
		psc = I2C_IP_CLK / I2C_INTERNAL_SAMPLING_CLK;
		psc -= 1;
		if (psc < I2C_PSC_MIN) {
			printf("Error : I2C unsupported prescaler %d\n", psc);
			return -1;
		}

		/* For first phase of HS mode */
		fsscll = I2C_INTERNAL_SAMPLING_CLK / (2 * speed);

		fssclh = fsscll;

		fsscll -= I2C_HIGHSPEED_PHASE_ONE_SCLL_TRIM;
		fssclh -= I2C_HIGHSPEED_PHASE_ONE_SCLH_TRIM;
		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing first phase clock\n");
			return -1;
		}

		/* For second phase of HS mode */
		hsscll = hssclh = I2C_INTERNAL_SAMPLING_CLK / (2 * speed);

		hsscll -= I2C_HIGHSPEED_PHASE_TWO_SCLL_TRIM;
		hssclh -= I2C_HIGHSPEED_PHASE_TWO_SCLH_TRIM;
		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing second phase clock\n");
			return -1;
		}

		scll = (unsigned int)hsscll << 8 | (unsigned int)fsscll;
		sclh = (unsigned int)hssclh << 8 | (unsigned int)fssclh;

	} else {
		/* Standard and fast speed */
		psc = i2c_findpsc(&scll, &sclh, speed);
		//printf("psc success\n");
		if (0 > psc) {
			printf("Error : I2C initializing clock\n");
			return -1;
		}
	}

	

	/* wait for 20 clkperiods */
	waitdelay = (10000000 / speed) * 2;

	value_at_register(I2C_CON) = 0;
	value_at_register(I2C_PSC) = psc;
	value_at_register(I2C_SCLL) = scll;
	value_at_register(I2C_SCLH) = sclh;
	value_at_register(I2C_CON) = CON_I2C_EN;

	/* clear all pending status */
	value_at_register(I2C_IRQSTATUS) = 0xFFFF;

	return 0;
}

void ub_i2c_init(int speed, int slaveaddr, int *waitdelay)
{
	int timeout = I2C_TIMEOUT;
	int deblock = 1;

retry:
	if (value_at_register(I2C_CON) & CON_I2C_EN) 
	{
		value_at_register(I2C_CON) = 0;
		udelay(50000);
	}

	/*  reset i2c */
	value_at_register(I2C_SYSC) = 0x2;
	udelay(1000);

	value_at_register(I2C_CON) = CON_I2C_EN;
	while (!(value_at_register(I2C_SYSS) & SYSS_RDONE) && timeout--) {
		if (timeout <= 0) {
			printf("ERROR: Timeout in soft-reset\n");
			return;
		}
		udelay(1000);
	}

    /* Set speed */
	if (i2c_setspeed(speed, waitdelay)) {
		printf("ERROR: failed to setup I2C bus speed!\n");
		return;
	}
	//printf("Set speed success!\n");

	/* Set own address */
	value_at_register(I2C_OA) = slaveaddr;

	udelay(1000);
	ub_flush_fifo();
	value_at_register(I2C_IRQSTATUS) = 0xFFFF;

	/* Handle possible failed I2C state */
	if (wait_for_bb(200))
		if (deblock == 1) {
			i2c_deblock();
			deblock = 0;
			goto retry;
		}
}


int i2c_read(int waitdelay,uint8_t slaveaddr, uint8_t offset, int alen, uint8_t *buffer, int len)
{
	int i2c_error = 0;
	uint16_t status;
	int pos = 0;

	if (alen < 0) {
		printf("I2C read: addr len < 0\n");
		return 1;
	}

	if (len < 0) {
		printf("I2C read: data len < 0\n");
		return 1;
	}

	if (buffer == NULL) {
		printf("I2C read: NULL pointer passed\n");
		return 1;
	}

	if (alen > 2) {
		printf("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (offset + len > (1 << 16)) {
		printf("I2C read: address out of range\n");
		return 1;
	}

	/* Wait until bus not busy */
	if (wait_for_bb(waitdelay))
		return 1;

	/* Zero, one or two bytes reg address (offset) */
	value_at_register(I2C_CNT) = alen;
	/* Set slave address */
	value_at_register(I2C_SA) = slaveaddr;

	if (alen) {

		/* Stop - Start (P-S) */
		value_at_register(I2C_CON) = CON_I2C_EN | CON_MST | CON_STT | CON_STP | CON_TRX;

		/* Send register offset */
		while (1) {
			status = wait_for_event(waitdelay);

			if (status == 0 || (status & IRQSTATUS_NACK)) {
				i2c_error = 1;
				printf("i2c_read: error waiting for addr ACK (status=0x%x)\n",
				       status);
				goto rd_exit;
			}
			if (alen) {
				if (status & IRQSTATUS_XRDY) {
					alen--;
					value_at_register(I2C_DATA) = offset;
					value_at_register(I2C_IRQSTATUS) = IRQSTATUS_XRDY;
				}
			}
			if (status & IRQSTATUS_ARDY) {
				value_at_register(I2C_IRQSTATUS) = IRQSTATUS_ARDY;
				break;
			}
		}
	}

	/* Set slave address */
	value_at_register(I2C_SA) = slaveaddr;
	/* Read len bytes from slave */
	value_at_register(I2C_CNT) = len;
	/* Need stop bit here */
	value_at_register(I2C_CON) = CON_I2C_EN | CON_MST | CON_STT | CON_STP;

	/* Receive data */
	while (1) {
		status = wait_for_event(waitdelay);
		/*
		 * Try to identify bus that is not padconf'd for I2C. This
		 * state could be left over from previous transactions if
		 * the address phase is skipped due to alen=0.
		 */
		if (status == IRQSTATUS_XRDY) {
			i2c_error = 2;
			printf("i2c_read (data phase): pads on bus probably not configured (status=0x%x)\n",
			       status);
			goto rd_exit;
		}
		if (status == 0 || (status & IRQSTATUS_NACK)) {
			i2c_error = 1;
			goto rd_exit;
		}
		if (status & IRQSTATUS_RRDY) {
			buffer[pos++] = value_at_register(I2C_DATA);
			value_at_register(I2C_IRQSTATUS) = IRQSTATUS_RRDY;
		}
		if (status & IRQSTATUS_ARDY) {
			value_at_register(I2C_IRQSTATUS) = IRQSTATUS_ARDY;
			break;
		}
	}

rd_exit:
	ub_flush_fifo();
	value_at_register(I2C_IRQSTATUS) = 0xFFFF;
	return i2c_error;
}

int i2c_write(int waitdelay, uint8_t slaveaddr, uint8_t offset, int alen, uint8_t *buffer, int len)
{
	int i = 0;
	uint16_t status;
	int i2c_error = 0;

	if (alen < 0) {
		printf("I2C write: addr len < 0\n");
		return 1;
	}

	if (len < 0) {
		printf("I2C write: data len < 0\n");
		return 1;
	}

	if (buffer == NULL) {
		printf("I2C write: NULL pointer passed\n");
		return 1;
	}

	if (alen > 2) {
		printf("I2C write: addr len %d not supported\n", alen);
		return 1;
	}

	if (offset + len > (1 << 16)) {
		printf("I2C write: address 0x%x + 0x%x out of range\n",
		       offset, len);
		return 1;
	}

	/* Wait until bus not busy */
	if (wait_for_bb(waitdelay))
		return 1;
        

	/* Start address phase - will write regoffset + len bytes data */
	value_at_register(I2C_CNT) = alen + len;
	/* Set slave address */
	value_at_register(I2C_SA) = slaveaddr;
        /* Set i2c_con */
	value_at_register(I2C_CON) = CON_I2C_EN | CON_MST | CON_STT | CON_STP | CON_TRX;

	while (alen) {
		/* Must write reg offset (one or two bytes) */
		status = wait_for_event(waitdelay);
		
		if (status == 0 || (status & IRQSTATUS_NACK)) {
			i2c_error = 1;
			printf("i2c_write: error waiting for addr ACK (status=0x%x)\n", status);
			goto wr_exit;
		}
		if (status & IRQSTATUS_XRDY) {
					alen--;
					value_at_register(I2C_DATA) = offset;
					value_at_register(I2C_IRQSTATUS) = IRQSTATUS_XRDY;
		}else {
			i2c_error = 1;
			printf("i2c_write: bus not ready for addr Tx (status=0x%x)\n", status);
			goto wr_exit;
		}
	}

	//printf("offset part finished\n");

	for (i = 0; i < len; i++) {
		status = wait_for_event(waitdelay);

		if (status == 0 || (status & IRQSTATUS_NACK)) {
			i2c_error = 1;
			printf("i2c_write: error waiting for addr ACK (status=0x%x)\n", status);
			goto wr_exit;
		}

		if (status & IRQSTATUS_XRDY) {
			value_at_register(I2C_DATA) = buffer[i];
			value_at_register(I2C_IRQSTATUS) = IRQSTATUS_XRDY;
		} else {
			i2c_error = 1;
			printf("i2c_write: bus not ready for data Tx (i=%d)\n", i);
			goto wr_exit;
		}
	}
       //value_at_register(I2C_CON) = 0;
    
        
        
wr_exit:
	ub_flush_fifo();
	value_at_register(I2C_IRQSTATUS) = 0xFFFF;
	return i2c_error;
}

int i2c_probe(int waitdelay,uint8_t slaveaddr)
{
	uint16_t status;
	int res = 1; /* default = fail */


	/* Wait until bus is free */
	if (wait_for_bb(waitdelay))
		return res;

	/* No data transfer, slave addr only */
	value_at_register(I2C_SA) = slaveaddr;

	/* Stop bit needed here */
	value_at_register(I2C_CON) = CON_I2C_EN | CON_MST | CON_STT | CON_STP | CON_TRX;

	status = wait_for_event(waitdelay);

	if ((status & ~IRQSTATUS_XRDY) == 0 || (status & IRQSTATUS_AL)) {
		/*
		 * With current high-level command implementation, notifying
		 * the user shall flood the console with 127 messages. If
		 * silent exit is desired upon unconfigured bus, remove the
		 * following 'if' section:
		 */

		goto pr_exit;
	}

	/* Check for ACK (!NAK) */
	if (!(status & IRQSTATUS_NACK)) {
		res = 0;				/* Device found */
		udelay(waitdelay);/* Required by AM335X in SPL */
		/* Abort transfer (force idle state) */
		value_at_register(I2C_CON) =  CON_MST | CON_TRX; /* Reset */
		udelay(1000);
		value_at_register(I2C_CON) = CON_I2C_EN | CON_MST | CON_STP | CON_TRX;	/* STP */
	}

pr_exit:
	ub_flush_fifo();
	value_at_register(I2C_IRQSTATUS) = 0xFFFF;
	return res;
}

/*For this test, enable I2C0_BASE and disable I2C1_BASE, I2C2_BASE. */
void i2c_eeprom_test(void)
{
	int * waitdelay = 200;
	//enable_i2c0_pin_mux();
	//printf("Step 1 Success!\n");
	udelay(1000);

	ub_i2c_init(100000, 1, waitdelay);
	printf("Init success!\n");
	udelay(1000);

	int probe_test = i2c_probe(200, 0x50);

	if (probe_test == 0)
	{
		printf("Probe success!\n");
	}else
	{
		printf("Probe failed!\n");
	}
	
	

	uint8_t buf[10];
    buf[0] = 0x00;
	buf[1] = 0x01;
	buf[2] = 0x02;
	buf[3] = 0x03;
	buf[4] = 0x04;
	buf[5] = 0x05;
	buf[6] = 0x06;
	buf[7] = 0x07;
	buf[8] = 0x08;
	buf[9] = 0x09;

	int w_result = i2c_write(200, 0x50, 0x00, 2, buf, 10);
    printf("Write Result is %d\n", w_result);

	uint8_t buff[10];
	int result = i2c_read(200, 0x50, 0x00, 2, buff, 10);
	printf("Read Result is %d\n", result);

	printf("Data: %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n",
	buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], buff[8], buff[9]);

}

void i2c2_cam_test(void)
{
	int * waitdelay = 200;
	//enable_i2c2_pin_mux();
	//printf("Step 1 Success!\n");
	udelay(1000);

	ub_i2c_init(100000, 1, waitdelay);
	printf("Init success!\n");
	udelay(1000);

	int probe_test = i2c_probe(200, 0x30);

	if (probe_test == 0)
	{
		printf("Probe success!\n");
	}else
	{
		printf("Probe failed!\n");}

	/* Camera ID number high */
	/*
	uint8_t buff_1[1];
	buff_1[0] = 0x00;
	int result_1 = i2c_write(200, 0x30, 0xFF, 2, buff_1, 1);
	printf("Write Result is %d\n", result_1);

	uint8_t buff_2[1];
	buff_2[0] = 0xFF;
	int result_2 = i2c_write(200, 0x30, 0x2C, 2, buff_2, 1);
	printf("Write Result is %d\n", result_2);

	uint8_t buff_3[1];
	buff_3[0] = 0xDF;
	int result_3 = i2c_write(200, 0x30, 0x2E, 2, buff_3, 1);
	printf("Write Result is %d\n", result_3);

	uint8_t buff_4[1];
	buff_4[0] = 0x01;
	int result_4 = i2c_write(200, 0x30, 0xFF, 2, buff_4, 1);
	printf("Write Result is %d\n", result_4);

	uint8_t buff_5[1];
	buff_5[0] = 0x32;
	int result_5 = i2c_write(200, 0x30, 0x3C, 2, buff_5, 1);
	printf("Write Result is %d\n", result_5);

	uint8_t buff_6[1];
	buff_6[0] = 0x00;
	int result_6 = i2c_write(200, 0x30, 0x11, 2, buff_6, 1);
	printf("Write Result is %d\n", result_6);

	uint8_t buff_7[1];
	buff_7[0] = 0x02;
	int result_7 = i2c_write(200, 0x30, 0x09, 2, buff_7, 1);
	printf("Write Result is %d\n", result_7);

	uint8_t buff_8[1];
	buff_8[0] = 0xA8;
	int result_8 = i2c_write(200, 0x30, 0x04, 2, buff_8, 1);
	printf("Write Result is %d\n", result_8);

	uint8_t buff_9[1];
	buff_9[0] = 0xE5;
	int result_9 = i2c_write(200, 0x30, 0x13, 2, buff_9, 1);
	printf("Write Result is %d\n", result_9);

	uint8_t buff_10[1];
	buff_10[0] = 0x48;
	int result_10 = i2c_write(200, 0x30, 0x14, 2, buff_10, 1);
	printf("Write Result is %d\n", result_10);

	
        uint8_t buff_11[1];
	buff_11[0] = 0x0C;
	int result_11 = i2c_write(200, 0x30, 0x2C, 2, buff_11, 1);
	printf("Write Result is %d\n", result_11);

	uint8_t buff_12[1];
	buff_12[0] = 0x78;
	int result_12 = i2c_write(200, 0x30, 0x33, 2, buff_12, 1);
	printf("Write Result is %d\n", result_12);

	uint8_t buff_13[1];
	buff_13[0] = 0x33;
	int result_13 = i2c_write(200, 0x30, 0x3A, 2, buff_13, 1);
	printf("Write Result is %d\n", result_13);

	uint8_t buff_14[1];
	buff_14[0] = 0xFB;
	int result_14 = i2c_write(200, 0x30, 0x3B, 2, buff_14, 1);
	printf("Write Result is %d\n", result_14); 

	
        uint8_t buff_15[1];
	buff_15[0] = 0x00;
	int result_15 = i2c_write(200, 0x30, 0x3E, 2, buff_15, 1);
	printf("Write Result for value0 is %d\n", result_15); 

        uint8_t buff_14[1];
	buff_14[0] = 0x80;
	int result_14 = i2c_write(200, 0x30, 0x12, 2, buff_14, 1);
	printf("Write Result is %d\n", result_14);*/
        uint8_t buff_1[1];
	buff_1[0] = 0x00;
	int result_1 = i2c_write(200, 0x30, 0xFF, 2, buff_1, 1);
	printf("Write Result is %d\n", result_1);
        
        uint8_t buff_15[1];
	buff_15[0] = NULL;
	int result_15 = i2c_read(200, 0x30, 0x3E, 2, buff_15, 1);
	printf("Read Result1 is %x\n", buff_15[0]);

        uint8_t buff_2[1];
	buff_2[0] = 0x01;
	int result_2 = i2c_write(200, 0x30, 0xFF, 2, buff_2, 1);

	
        uint8_t buff_16[1];
	buff_16[0] = 0x00;
	int result_16 = i2c_write(200, 0x30, 0x3E, 2, buff_16, 1);
	printf("Write Result for 0x3E is %d\n", result_16);

        uint8_t buff_3[1];
	buff_3[0] = 0x00;
	int result_3 = i2c_write(200, 0x30, 0xFF, 2, buff_3, 1);
	printf("Write Result is %d\n", result_3);	
        
	uint8_t buff_17[1];
	buff_17[0] = NULL;
	int result_17 = i2c_read(200, 0x30, 0x3E, 2, buff_17, 1);
	printf("Read Result2 is %x\n", buff_17[0]);
        
        /*
	uint8_t buff_18[1];
	buff_18[0] = 0x02;
	int result_18 = i2c_write(200, 0x30, 0x39, 2, buff_18, 1);
	printf("Write Result is %d\n", result_18);

	uint8_t buff_19[1];
	buff_19[0] = 0x88;
	int result_19 = i2c_write(200, 0x30, 0x35, 2, buff_19, 1);
	printf("Write Result is %d\n", result_19);*/

    /* Camera ID number low */
	//uint8_t buf[1];
	//buf[0] = NULL;
	//int result_1 = i2c_read(200, 0x30, 0xD3, 2, buf, 1);

	//printf("Camera Write_Read Test: %x %x\n",buff[0],buf[0]);

}

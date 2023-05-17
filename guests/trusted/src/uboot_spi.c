#include <lib.h>
#include "uboot_spi.h"

#define MCSPI0_BASE 0x48030100
//#define MCSPI1_BASE 0x481A0100

#define OFFSET(x) ((x) + 0xB2000000)

#define spi_phys_to_virt(phys) (OFFSET(phys))

#ifdef MCSPI0_BASE
#define spi_value_at_register(offset) (*((volatile  uint32_t *) spi_phys_to_virt(MCSPI0_BASE + (offset))))
#else
#define spi_value_at_register(offset) (*((volatile  uint32_t *) spi_phys_to_virt(MCSPI1_BASE + (offset))))
#endif


void spi_write_ch0conf(int val)
{
	spi_value_at_register(MCSPI_CH0CONF) = val;
	/* Flash post writes to make immediate effect */
	spi_value_at_register(MCSPI_CH0CONF);
}

void spi_set_ch0ctrl_enable(int enable)
{
	spi_value_at_register(MCSPI_CH0CTRL) = enable;
	/* Flash post writes to make immediate effect */
	spi_value_at_register(MCSPI_CH0CTRL);
}

int spi_write(unsigned int len, const void *txp, uint32_t flags)
{
	int timeout = SPI_WAIT_TIMEOUT;
	int i, chconf;

        /* Set up the channel configuration */    
	chconf = spi_value_at_register(MCSPI_CH0CONF);

	chconf &= ~(CHXCONF_TRM_MASK | CHXCONF_WL_MASK);
	chconf |= (0x8 - 1) << 7; //8 bits_per_word
	chconf |= CHXCONF_TRM_TX_ONLY;
	chconf |= CHXCONF_FORCE;
	spi_write_ch0conf(chconf);

        /* Enable the channel */
	spi_set_ch0ctrl_enable(CHXCTRL_EN);

	for (i = 0; i < len; i++) {
		/* wait till TX register is empty (TXS == 1) */
		while (!(spi_value_at_register(MCSPI_CH0STAT) & CHXSTAT_TXS) && timeout--)
        {
			if (timeout <= 0) {
				printf("SPI TXS timed out, status= %x\n",spi_value_at_register(MCSPI_CH0STAT));
				return -1;
			}
		}
		/* Write the data */
       spi_value_at_register(MCSPI_TX0) = ((uint8_t *) txp)[i];
	}

	/* wait to finish of transfer */
	while ((spi_value_at_register(MCSPI_CH0STAT) & (CHXSTAT_EOT | CHXSTAT_TXS)) != (CHXSTAT_EOT | CHXSTAT_TXS));

	/* Disable the channel otherwise the next immediate RX will get affected */
	spi_set_ch0ctrl_enable(CHXCTRL_DIS);

	if (flags & SPI_XFER_END) {

		chconf &= ~CHXCONF_FORCE;
		spi_write_ch0conf(chconf);
	}
	return 0;
}

int spi_read(unsigned int len, void *rxp, uint32_t flags)
{
	int i, chconf;
	int timeout = SPI_WAIT_TIMEOUT;

	chconf = spi_value_at_register(MCSPI_CH0CONF);

	/* Enable the channel */
        spi_set_ch0ctrl_enable(CHXCTRL_EN);

	chconf &= ~(CHXCONF_TRM_MASK | CHXCONF_WL_MASK);
	chconf |= (0x8 - 1) << 7;
	chconf |= CHXCONF_TRM_RX_ONLY;
	chconf |= CHXCONF_FORCE;
	spi_write_ch0conf(chconf);

	spi_value_at_register(MCSPI_TX0) = 0;


	for (i = 0; i < len; i++) {
		/* Wait till RX register contains data (RXS == 1) */
        while (!(spi_value_at_register(MCSPI_CH0STAT) & CHXSTAT_RXS) && timeout--)
        {
			if (timeout <= 0) {
				printf("SPI RXS timed out, status= %x\n",spi_value_at_register(MCSPI_CH0STAT));
				return -1;
			}
		}

		/* Disable the channel to prevent furher receiving */
		if (i == (len - 1))
			spi_set_ch0ctrl_enable(CHXCTRL_DIS);

		/* Read the data */
	    ((uint8_t *)rxp)[i] = (uint8_t)spi_value_at_register(MCSPI_RX0);
	}

	if (flags & SPI_XFER_END) {
		chconf &= ~CHXCONF_FORCE;
		spi_write_ch0conf(chconf);
	}

	return 0;
}

/*McSPI Transmit Receive Mode*/
int spi_txrx(unsigned int len, const void *txp, void *rxp, uint32_t flags)
{
	int timeout = SPI_WAIT_TIMEOUT;
	int chconf, i = 0;

	chconf = spi_value_at_register(MCSPI_CH0CONF);

	/*Enable SPI channel*/
    spi_set_ch0ctrl_enable(CHXCTRL_EN);

	/*set TRANSMIT-RECEIVE Mode*/
    chconf &= ~(CHXCONF_TRM_MASK | CHXCONF_WL_MASK);
	chconf |= (8 - 1) << 7;
	chconf |= CHXCONF_FORCE;
    spi_write_ch0conf(chconf);

	/*Shift in and out 1 byte at one time*/
	for (i = 0; i < len; i++){
		/* Write: wait for TX empty (TXS == 1)*/
        while (!(spi_value_at_register(MCSPI_CH0STAT) & CHXSTAT_TXS) && timeout--)
        {
			if (timeout <= 0) {
				printf("SPI TXS timed out, status= %x\n",spi_value_at_register(MCSPI_CH0STAT));
				return -1;
			}
		}
        
		/* Write the data */
		spi_value_at_register(MCSPI_TX0) = ((uint8_t *) txp)[i];
		//printf("Write finished\n");
		

		/*Read: wait for RX containing data (RXS == 1)*/
		timeout = SPI_WAIT_TIMEOUT;
        while (!(spi_value_at_register(MCSPI_CH0STAT) & CHXSTAT_RXS) && timeout--)
        {
			if (timeout <= 0) {
				printf("SPI RXS timed out, status= %x\n",spi_value_at_register(MCSPI_CH0STAT));
				return -1;
			}
		}

		/* Read the data */
        
            //printf("rx is %x\n", spi_value_at_register(MCSPI_RX0));
	    ((uint8_t *)rxp)[i] = (uint8_t) spi_value_at_register(MCSPI_RX0);
	}
	/* Disable the channel */
    spi_set_ch0ctrl_enable(CHXCTRL_DIS);

	/*if transfer must be terminated disable the channel*/
	if (flags & SPI_XFER_END) {

        chconf &= ~CHXCONF_FORCE;
		spi_write_ch0conf(chconf);
	}

	return 0;
}

int spi_xfer(unsigned int bitlen, const void *dout, void *din, uint32_t flags)
{
	unsigned int len;
	int ret = -1;

	if (bitlen % 8)
		return -1;

	len = bitlen / 8;

	if (bitlen == 0) {	 /* only change CS */
		int chconf = spi_value_at_register(MCSPI_CH0CONF);

		if (flags & SPI_XFER_BEGIN) {
			spi_set_ch0ctrl_enable(CHXCTRL_EN);
			chconf |= CHXCONF_FORCE;
			spi_write_ch0conf(chconf);
		}
		if (flags & SPI_XFER_END) {
			chconf &= ~CHXCONF_FORCE;
            spi_write_ch0conf(chconf);
            spi_set_ch0ctrl_enable(CHXCTRL_DIS);
		}
		ret = 0;
	} else {
		if (dout != NULL && din != NULL)
			ret = spi_txrx(len, dout, din, flags);
		else if (dout != NULL)
			ret = spi_write(len, dout, flags);
		else if (din != NULL)
			ret = spi_read(len, din, flags);
	}
	return ret;
}

void spi_set_speed(uint32_t speed_hz)
{
	uint32_t confr, div = 0;

	confr = spi_value_at_register(MCSPI_CH0CONF);

	/* Calculate clock divisor. Valid range: 0x0 - 0xC ( /1 - /4096 ) */
	if (speed_hz) {
		while (div <= 0xC && (MCSPI_MAX_FREQ / (1 << div))
					> speed_hz)
			div++;
	} else {
		 div = 0xC;
	}

	/* set clock divisor */
	confr &= ~CHXCONF_CLKD_MASK;
	confr |= div << 2;

	spi_write_ch0conf(confr);
}

void spi_set_mode(int mode)
{
	uint32_t confr;

	confr = spi_value_at_register(MCSPI_CH0CONF);

	/* standard 4-wire master mode:  SCK, MOSI/out, MISO/in, nCS
	 * REVISIT: this controller could support SPI_3WIRE mode.
	 */
	/* MCSPI_PINDIR_D0_IN_D1_OUT */
	confr &= ~(CHXCONF_IS| CHXCONF_DPE1);
	confr |= CHXCONF_DPE0;

	/* set SPI mode 0..3 */
	confr &= ~(CHXCONF_POL | CHXCONF_PHA);
	if (mode == 0) //POL = 0 and PHA = 0, confr not changed
		confr &= ~(CHXCONF_POL | CHXCONF_PHA);
	else if (mode == 1) //POL = 0 and PHA = 1
		confr |= CHXCONF_PHA;
	else if (mode == 2) //POL = 1 and PHA = 0
	    confr |= CHXCONF_POL;
	else if (mode == 3)  //POL = 1 and PHA = 1
            confr |= CHXCONF_POL | CHXCONF_PHA;
        else
            printf("Invaild mode!\n");
	
	/* set chipselect polarity; manage with FORCE */
	
	confr |= CHXCONF_EPOL; /* active-low; normal choose */
	

	/* Transmit & receive mode */
	confr &= ~CHXCONF_TRM_MASK;

	spi_write_ch0conf(confr);
}

void spi_set_wordlen(int wordlen)
{
	uint32_t confr;

	/* McSPI individual channel configuration */
	confr = spi_value_at_register(MCSPI_CH0CONF);

	/* wordlength */
	confr &= ~CHXCONF_WL_MASK;
	confr |= (wordlen - 1) << 7;

	spi_write_ch0conf(confr);
}

void spi_set_ch0conf(int mode, uint32_t speed_hz, int wordlen)
{
        uint32_t confr, div = 0;

	/* McSPI individual channel configuration */
	confr = spi_value_at_register(MCSPI_CH0CONF);
        
        /* Calculate clock divisor. Valid range: 0x0 - 0xC ( /1 - /4096 ) */
	if (speed_hz) {
		while (div <= 0xC && (MCSPI_MAX_FREQ / (1 << div))
					> speed_hz)
			div++;
	} else {
		 div = 0xC;
	}

	/* standard 4-wire master mode:  SCK, MOSI/out, MISO/in, nCS
	 * REVISIT: this controller could support SPI_3WIRE mode.
	 */
	/* MCSPI_PINDIR_D0_IN_D1_OUT */
	confr &= ~(CHXCONF_IS| CHXCONF_DPE1);
	confr |= CHXCONF_DPE0;

	/* set SPI mode 0..3 */
	confr &= ~(CHXCONF_POL | CHXCONF_PHA);
	if (mode == 0) //POL = 0 and PHA = 0, confr not changed
		confr &= ~(CHXCONF_POL | CHXCONF_PHA);
	else if (mode == 1) //POL = 0 and PHA = 1
		confr |= CHXCONF_PHA;
	else if (mode == 2) //POL = 1 and PHA = 0
	    confr |= CHXCONF_POL;
	else if (mode == 3)  //POL = 1 and PHA = 1
            confr |= CHXCONF_POL | CHXCONF_PHA;
        else
            printf("Invaild mode!\n");
	
	/* set chipselect polarity; manage with FORCE */
	
	confr |= CHXCONF_EPOL; /* active-low; normal choose */
	

	/* Transmit & receive mode */
	confr &= ~CHXCONF_TRM_MASK;

        /* wordlength */
	confr &= ~CHXCONF_WL_MASK;
	confr |= (wordlen - 1) << 7;

        confr &= ~CHXCONF_CLKD_MASK;
	confr |= div << 2;

	spi_write_ch0conf(confr);
}

void spi_reset(void)
{
	uint32_t tmp;

	spi_value_at_register(MCSPI_SYSCONFIG)= SYSCONFIG_SOFTRESET;
	
	do {
		tmp = spi_value_at_register(MCSPI_SYSSTATUS);
	} while (!(tmp & SYSSTATUS_RESETDONE));

	spi_value_at_register(MCSPI_SYSCONFIG) = SYSCONFIG_AUTOIDLE | SYSCONFIG_SMARTIDLE;
}

void spi_claim_bus(void)
{
	uint32_t modulctrl;

	/* setup when switching from (reset default) slave mode to single-channel master mode */
	modulctrl = spi_value_at_register(MCSPI_MODULCTRL);
	modulctrl &= ~(MODULCTRL_SYSTEM_TEST | MODULCTRL_MS);
	modulctrl |= MODULCTRL_SINGLE;

	spi_value_at_register(MCSPI_MODULCTRL) = modulctrl;
}

void spi_init (void)
{
	/* SPI0_CS0 based configuration*/
	uint32_t speed = 1000000; // 1 MHz
	int mode = 0;
	int wordlen = 8;

	spi_reset();
	printf("RESET is done!\n");

	spi_claim_bus();
	printf("Claim Bus finished!\n");

	//spi_set_wordlen(wordlen);
    //printf("Set up word length!\n");

	//spi_set_mode(mode);
    //printf("Set up mode!\n");

	//spi_set_speed(speed);
	//printf("Set up speed_hz!\n");

	printf("Init done!\n");
        spi_set_ch0conf(mode, speed, wordlen);
}

void spi_self_test (void)
{
	//enable_spi0_pin_mux();

	spi_init();

	unsigned int bitlength = 16;
	uint8_t din[2];
	din[0] = NULL;
    din[1] = NULL;

	uint8_t dout[2];
	dout[0] = 0x66;
	dout[1] = 0x25;
    
	printf("Self send data: %x %x\n",dout[0],dout[1]);
    spi_xfer(bitlength, dout, din, SPI_XFER_BEGIN | SPI_XFER_END);
	printf("Self receive data: %x %x\n",din[0],din[1]);
}

void spi_camera_test (void)
{
	//enable_spi0_pin_mux();

	spi_init();
        
        /* For the camera, the first bit[7] of the command phase is read/write byte, ‘0’ is for read and ‘1’ is for writte  */

	unsigned int bitlength = 16;
	uint8_t din[2];
	din[0] = 0;
    

	uint8_t dout[2];
	dout[0] = 0x80;
	dout[1] = 0x55;
    
	printf("Camera send address: %x (= addr|0x80), data: %x\n",dout[0],dout[1]);
    spi_xfer(bitlength, dout, din, SPI_XFER_BEGIN | SPI_XFER_END);
	printf("Write test register finished\n");
	
	uint8_t read_dout[2];
	read_dout[0] = 0x00;
	read_dout[1] = 0x00;
	
	uint8_t read_din[2];
	read_din[0] = 0;
	spi_xfer(bitlength, read_dout, read_din, SPI_XFER_BEGIN | SPI_XFER_END);
	printf("Camera read result: %x\n",read_din[1]);

 }

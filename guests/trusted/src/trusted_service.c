#include "hypercalls.h"	//For ISSUE_HYPERCALL_REG1 and HYPERCALL_END_RPC
#include <types.h>		//For uint32_t

#include <printing.h>	//For printing.

//For AES.
#define CBC 1
#define CTR 1
#define ECB 1
#include "aes.h"

//For SPI an I2C.
#include <types.h>
#include <lib.h>
#include "BBBCAM.h"
#include "uboot_i2c.h"
#include "uboot_spi.h"
#include "hypercalls.h"

#define OV2640_CHIPID_HIGH 	0x0A
#define OV2640_CHIPID_LOW 	0x0B

void camera_setup(void) 
{
	uint8_t vid,pid;
	uint8_t temp;

	ArduCAM(OV2640);
	printf("ArduCAM Start!\n");

	// Check the ArduCAM SPI bus
	write_reg(ARDUCHIP_TEST1, 0x55);
	temp = read_reg(ARDUCHIP_TEST1);
	if(temp != 0x55) {
		printf("SPI interface Error!\n");
		while(1);
	} else {
		printf("SPI bus works well!\n");
	}

	//Change MCU mode
	write_reg(ARDUCHIP_MODE, 0x00);

	//Check the I2C bus and camera module type
	rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
	printf("vid is : %x\n",vid);
	rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
	printf("pid is : %x\n",pid);
	if((vid != 0x26) || (pid != 0x42)) {
		printf("Can't find OV2640 module!\n");
		while (1);
	} else
		printf("OV2640 detected\n");

	set_format(BMP);

	InitCAM();
}

#define X	320
#define Y	240
#define BMP_HEADER_SIZE 66
#define BUFFER_SIZE (2*X*Y + BMP_HEADER_SIZE)
uint8_t buffer[BUFFER_SIZE];

const uint8_t bmp_header[BMP_HEADER_SIZE] = {
	0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
	0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
	0x00, 0x00
};

void camera_rgb(void) {
	BOOL isSavedFlag = FALSE;
	camera_setup();
	printf("Setup done!\n");

	uint8_t VH, VL;

	while(1) { 
		write_reg(ARDUCHIP_MODE, 0x00); //Resolution for BMP: 320*240
	
		//Flush the FIFO
		flush_fifo();
	
		//Clear the capture done flag
		clear_fifo_flag();
          
		//Start capture
		capture();

		while (isSavedFlag == FALSE) {
        	wrSensorReg8_8(0xff, 0x00);
			if(read_reg(ARDUCHIP_TRIG) & CAP_DONE_MASK) {
				int k, x, y;
				for (k = 0; k < BMP_HEADER_SIZE; k++)
					buffer[k] = bmp_header[k];

				//Read first dummy byte
				read_fifo();

				//print 256 pixels RGB888 value
				for(x = 0; x < X; x++)
					for(y = 0; y < Y; y++) {
						VH = read_fifo();
						VL = read_fifo();
						buffer[k++] = VL;
						buffer[k++] = VH;
//						printf("buffer[%d] = %x%x ", k/2, VH, VL);
//						if (k % 10 == 0)
//							printf("\n");

						//Use VH and VL for 1 pixel to count RGB888 value
						//blue = (((VH & 0x1F) * 527) + 23) >> 6;
						//green = (((((VH & 0xF0) >> 5 | ((VL & 0x0F) << 3)) & 0x3F) * 259) + 33) >> 6;
						//red = ((((VL >> 3) & 0x1F) * 527) + 23) >> 6;
						//k++;
					}
				printf("picture done\n");

				//Clear the capture done flag
				clear_fifo_flag();
				//Clear the start capture flag
				set_format(BMP);
				InitCAM();
				isSavedFlag = TRUE;
			}
		}

		isSavedFlag = FALSE;
	    break;
    }
}

void encrypt(void) {
    uint8_t key[16] = { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16,
						(uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6,
						(uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88,
						(uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c};
    uint8_t iv[16]  = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, buffer, BUFFER_SIZE);
}

void handler_rpc(void) {
	camera_rgb();
	encrypt();
	printf("Encryption done!\n");
	ISSUE_HYPERCALL_REG2(HYPERCALL_END_RPC, 0, buffer);
}

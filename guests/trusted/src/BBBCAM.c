 /*
 ============================================================================
 Name        : BBBCAM.c
 Author      : Lee
 Version     : V1.0
 Copyright   : ArduCAM demo (C)2015 Lee
 Description :

  Beaglebon Black library support for CMOS Image Sensor
  Copyright (C)2011-2015 ArduCAM.com. All right reserved

  Basic functionality of this library are based on the demo-code provided by
  ArduCAM.com. You can find the latest version of the library at
  http://www.ArduCAM.com

  Now supported controllers:
		-	OV7670
		-	MT9D111
		-	OV7675
		-	OV2640
		-	OV3640
		-	OV5642
		-	OV7660
		-	OV7725

	We will add support for many other sensors in next release.

  Supported MCU platform
 		-	Theoretically support all Arduino families
  	-	Arduino UNO R3				(Tested)
  	-	Arduino MEGA2560 R3		(Tested)
  	-	Arduino Leonardo R3		(Tested)
  	-	Arduino Nano					(Tested)
  	-	Arduino DUE						(Tested)
  	-	Arduino Yun						(Tested)  		
  	-	Beaglebon Black				(Tested)
  	-	Raspberry Pi					(Tested)
  	- ESP8266-12 						(Tested)

  If you make any modifications or improvements to the code, I would appreciate
  that you share the code with me so that I might include it in the next release.
  I can be contacted through http://www.ArduCAM.com

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <types.h>
#include <lib.h>
#include "BBBCAM.h"
#include "ov2640_regs.h"
#include "uboot_i2c.h"
#include "uboot_spi.h"




/*
Function: 		BBBCAM
Param: 				camera model
Description :	BeagleBone Black Camera Instantiation,
							Initialize the Camera structure,
							Initialize the SPI and I2C ports
*/
int ArduCAM(uint8_t model)
{
	int ret;
	myCAM.sensor_model = model;
	switch(myCAM.sensor_model)
	{
		case OV2640:
		case OV9655:
			myCAM.sensor_addr = 0x30;
			break;
		case MT9M112:
			myCAM.sensor_addr = 0x5d;
			break;
		default:
			myCAM.sensor_addr = 0x21;
			break;
	}
	//initialize spi0
	//enable_spi0_pin_mux();

	spi_init();
	printf("SPI0 initialized!\n");

	// initialize i2c2:
	int * waitdelay = 200;
	//enable_i2c2_pin_mux();
	ub_i2c_init(100000, 1, waitdelay);
	printf("I2C2 initialized!\n");
}

/*
Function: 		InitCAM
Param: 				None
Description :	Initialize the Camera Module
*/
void InitCAM()
{
	uint8_t rtn = 0;
	uint8_t reg_val;
	switch(myCAM.sensor_model)
	{
		
		case OV2640:
		{
			wrSensorReg8_8(0xff, 0x01);
			wrSensorReg8_8(0x12, 0x80);
			delayms(100);
			if(myCAM.m_fmt == JPEG)
			{
				wrSensorRegs8_8(OV2640_JPEG_INIT);
				wrSensorRegs8_8(OV2640_YUV422);
				wrSensorRegs8_8(OV2640_JPEG);
				wrSensorReg8_8(0xff, 0x01);
				wrSensorReg8_8(0x15, 0x00);
				wrSensorRegs8_8(OV2640_320x240_JPEG);
				//wrSensorReg8_8(0xff, 0x00);
				//wrSensorReg8_8(0x44, 0x32);
			}
			else
			{
				wrSensorRegs8_8(OV2640_QVGA);
			}
			break;
		}

		default:

			break;
	}
}

void flush_fifo(void)
{
	write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

void capture(void)
{
	write_reg(ARDUCHIP_FIFO, FIFO_START_MASK);
}

void clear_fifo_flag(void)
{
	write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

uint8_t read_fifo(void)
{
	uint8_t data;
	data = bus_read(0x3D);
	return data;
}

uint8_t read_reg(uint8_t addr)
{
	uint8_t data;
	data = bus_read(addr & 0x7F);
	return data;
}

void write_reg(uint8_t addr, uint8_t data)
{
	bus_write(addr | 0x80, data);
}

void bus_write(uint8_t address, uint8_t value)
{
        int ret;
	unsigned int bitlength = 16;
	
	uint8_t din[2] = {0,};
	uint8_t dout[2] = {address,value};
    
    ret = spi_xfer(bitlength, dout, din, SPI_XFER_BEGIN | SPI_XFER_END);
    //printf("address: %x,value: %x,input1:%x,input2:%x\n",dout[0],dout[1],din[0],din[1]);
    

}

uint8_t bus_read(uint8_t address)
{
	int ret;
        unsigned int bitlength = 16;

	uint8_t read_dout[2] = {address,0x00};
	uint8_t read_din[2] = {0,};

	ret = spi_xfer(bitlength, read_dout, read_din, SPI_XFER_BEGIN |SPI_XFER_END);
        
        //printf(" read address: %x,dout1: %x,input1:%x,value:%x\n",read_dout[0],read_dout[1],read_din[0],read_din[1]);
  	
	return read_din[1];
}

uint8_t wrSensorReg8_8(uint8_t regID, uint8_t regDat)
{
    uint8_t buf[1];
    buf[0] = regDat;

	i2c_write(200, 0x30, regID, 1, buf, 1);
	return 1;
}

uint8_t rdSensorReg8_8(uint8_t regID, uint8_t* regDat)
{
    
	uint8_t buff[1];
	buff[0] = NULL;
	i2c_read(200, 0x30, regID, 1, buff, 1);
	*regDat = buff[0];
	return 1;
}

int wrSensorRegs8_8(const struct sensor_reg reglist[])
{
	int err = 0;
	uint8_t reg_addr = 0;
	uint8_t reg_val = 0;
	const struct sensor_reg *next = reglist;

	while ((reg_addr != 0xff) | (reg_val != 0xff))
	{
		reg_addr = pgm_read_word(&next->reg);
		reg_val = pgm_read_word(&next->val);
		err = wrSensorReg8_8(reg_addr, reg_val);
                //printf("address is %x\n",reg_addr);
                
   	next++;
	}
	return 1;
}


void OV2640_set_JPEG_size(uint8_t size)
{
	
	switch(size)
	{
		case OV2640_160x120:
			wrSensorRegs8_8(OV2640_160x120_JPEG);
			break;
		case OV2640_176x144:
			wrSensorRegs8_8(OV2640_176x144_JPEG);
			break;
		case OV2640_320x240:
			wrSensorRegs8_8(OV2640_320x240_JPEG);
			break;
		case OV2640_352x288:
			wrSensorRegs8_8(OV2640_352x288_JPEG);
			break;
		case OV2640_640x480:
			wrSensorRegs8_8(OV2640_640x480_JPEG);
			break;
		case OV2640_800x600:
			wrSensorRegs8_8(OV2640_800x600_JPEG);
			break;
		case OV2640_1024x768:
			wrSensorRegs8_8(OV2640_1024x768_JPEG);
			break;
		case OV2640_1280x1024:
			wrSensorRegs8_8(OV2640_1280x1024_JPEG);
			break;
		case OV2640_1600x1200:
			wrSensorRegs8_8(OV2640_1600x1200_JPEG);
			break;
		default:
			wrSensorRegs8_8(OV2640_320x240_JPEG);
			break;
	}
}

void set_format(uint8_t fmt)
{
	if(fmt == BMP)
		myCAM.m_fmt = BMP;
	else
		myCAM.m_fmt = JPEG;
}

void delayms(int i)
{
	while(i)
	{i--;}
}

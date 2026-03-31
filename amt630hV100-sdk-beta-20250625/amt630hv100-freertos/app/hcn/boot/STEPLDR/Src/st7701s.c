/*
 * ST7701S LCD Display.
 *
 * LCD Type:		AMT TFT
 * Display Format:	320(RGB)*960
 * Input Data:		RGB 16bits(RGB565)
 */

#include "board.h"
#include "gpio.h"
#include "timer.h"


//3 lines mode spi.
#define ST7701S_GPIO_SPI_CS			23
#define ST7701S_GPIO_SPI_SCL		24
#define ST7701S_GPIO_SPI_SDA		25
#define ST7701S_GPIO_SPI_RST		26

#define ST7701S_DELAY				udelay(5)


static void SPI_SendData(unsigned char value)
{
	int i;

	for(i=0; i<8; i++)
	{
		if(value & 0x80)
			gpio_direction_output(ST7701S_GPIO_SPI_SDA, 1);	//SDA=1
		else
			gpio_direction_output(ST7701S_GPIO_SPI_SDA, 0);	//SDA=0

		value<<= 1;

		gpio_direction_output(ST7701S_GPIO_SPI_SCL, 0);	//SCL=0
		ST7701S_DELAY;
		gpio_direction_output(ST7701S_GPIO_SPI_SCL, 1);	//SCL=1
		ST7701S_DELAY;
	}
}

static void SPI_WriteComm(unsigned char value)
{
	gpio_direction_output(ST7701S_GPIO_SPI_CS, 0);	//CS=0

	//D/CX = 0
	gpio_direction_output(ST7701S_GPIO_SPI_SDA, 0);	//SDA=0
	gpio_direction_output(ST7701S_GPIO_SPI_SCL, 0);	//SCL=0
	ST7701S_DELAY;
	gpio_direction_output(ST7701S_GPIO_SPI_SCL, 1);	//SCL=1
	ST7701S_DELAY;

	//Command data.
	SPI_SendData(value);

	gpio_direction_output(ST7701S_GPIO_SPI_CS, 1);	//CS=1
	ST7701S_DELAY;
}

static void SPI_WriteData(unsigned char value)
{
	gpio_direction_output(ST7701S_GPIO_SPI_CS, 0);	//CS=0

	//D/CX = 1
	gpio_direction_output(ST7701S_GPIO_SPI_SDA, 1);	//SDA=1
	gpio_direction_output(ST7701S_GPIO_SPI_SCL, 0);	//SCL=0
	ST7701S_DELAY;
	gpio_direction_output(ST7701S_GPIO_SPI_SCL, 1);	//SCL=1
	ST7701S_DELAY;

	//Parameter data.
	SPI_SendData(value);

	gpio_direction_output(ST7701S_GPIO_SPI_CS, 1);	//CS=1
	ST7701S_DELAY;
}

static void st7701s_reg_init(void)
{
	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x13);

	SPI_WriteComm (0xEF);
	SPI_WriteData (0x08);

	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x10);

	SPI_WriteComm (0xC0);
	SPI_WriteData (0x77);
	SPI_WriteData (0x00);

	SPI_WriteComm (0xC1);	//proch control.
	SPI_WriteData (0x09);	//vbp
	SPI_WriteData (0x08);	//vfp

	SPI_WriteComm (0xC2);	//inv
#if 0
	SPI_WriteData (0x01);
#else
	SPI_WriteData (0x07);
#endif
	SPI_WriteData (0x02);

	//default:0x00, DE mode.
	SPI_WriteComm (0xC3);	//RGB control.
	SPI_WriteData (0x02);	//82 HVmode	 02 DEmode

#if 0	//default:0x00
	SPI_WriteComm (0xC7);	//add. Source direction control.
	SPI_WriteData (0x04);	//0x00:Source form 0 to 479, 0x04:from 479 to 0.
#endif

	SPI_WriteComm (0xCC);
	SPI_WriteData (0x10);

	SPI_WriteComm (0xB0);
	SPI_WriteData (0x40);
	SPI_WriteData (0x14);
	SPI_WriteData (0x59);
	SPI_WriteData (0x10);
	SPI_WriteData (0x12);
	SPI_WriteData (0x08);
	SPI_WriteData (0x03);
	SPI_WriteData (0x09);
	SPI_WriteData (0x05);
	SPI_WriteData (0x1E);
	SPI_WriteData (0x05);
	SPI_WriteData (0x14);
	SPI_WriteData (0x10);
	SPI_WriteData (0x68);
	SPI_WriteData (0x33);
	SPI_WriteData (0x15);

	SPI_WriteComm (0xB1);
	SPI_WriteData (0x40);
	SPI_WriteData (0x08);
	SPI_WriteData (0x53);
	SPI_WriteData (0x09);
	SPI_WriteData (0x11);
	SPI_WriteData (0x09);
	SPI_WriteData (0x02);
	SPI_WriteData (0x07);
	SPI_WriteData (0x09);
	SPI_WriteData (0x1A);
	SPI_WriteData (0x04);
	SPI_WriteData (0x12);
	SPI_WriteData (0x12);
	SPI_WriteData (0x64);
	SPI_WriteData (0x29);
	SPI_WriteData (0x29);

	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x11);

	SPI_WriteComm (0xB0);
	SPI_WriteData (0x6D);	//6D

	SPI_WriteComm (0xB1);	//vcom
	SPI_WriteData (0x1D);

	SPI_WriteComm (0xB2);
	SPI_WriteData (0x87);

	SPI_WriteComm (0xB3);
	SPI_WriteData (0x80);

	SPI_WriteComm (0xB5);
	SPI_WriteData (0x49);

	SPI_WriteComm (0xB7);
	SPI_WriteData (0x85);

	SPI_WriteComm (0xB8);
	SPI_WriteData (0x20);

	SPI_WriteComm (0xC1);
	SPI_WriteData (0x78);

	SPI_WriteComm (0xC2);
	SPI_WriteData (0x78);

	SPI_WriteComm (0xD0);
	SPI_WriteData (0x88);

	SPI_WriteComm (0xE0);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x02);

	SPI_WriteComm (0xE1);
	SPI_WriteData (0x02);
	SPI_WriteData (0x8C);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x03);
	SPI_WriteData (0x8C);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x33);
	SPI_WriteData (0x33);

	SPI_WriteComm (0xE2);
	SPI_WriteData (0x33);
	SPI_WriteData (0x33);
	SPI_WriteData (0x33);
	SPI_WriteData (0x33);
	SPI_WriteData (0xC9);
	SPI_WriteData (0x3C);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0xCA);
	SPI_WriteData (0x3C);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);

	SPI_WriteComm (0xE3);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x33);
	SPI_WriteData (0x33);

	SPI_WriteComm (0xE4);
	SPI_WriteData (0x44);
	SPI_WriteData (0x44);

	SPI_WriteComm (0xE5);
	SPI_WriteData (0x05);
	SPI_WriteData (0xCD);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);
	SPI_WriteData (0x01);
	SPI_WriteData (0xC9);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);
	SPI_WriteData (0x07);
	SPI_WriteData (0xCF);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);
	SPI_WriteData (0x03);
	SPI_WriteData (0xCB);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);

	SPI_WriteComm (0xE6);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x33);
	SPI_WriteData (0x33);

	SPI_WriteComm (0xE7);
	SPI_WriteData (0x44);
	SPI_WriteData (0x44);

	SPI_WriteComm (0xE8);
	SPI_WriteData (0x06);
	SPI_WriteData (0xCE);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);
	SPI_WriteData (0x02);
	SPI_WriteData (0xCA);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);
	SPI_WriteData (0x08);
	SPI_WriteData (0xD0);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);
	SPI_WriteData (0x04);
	SPI_WriteData (0xCC);
	SPI_WriteData (0x82);
	SPI_WriteData (0x82);

	SPI_WriteComm (0xEB);
	SPI_WriteData (0x08);
	SPI_WriteData (0x01);
	SPI_WriteData (0xE4);
	SPI_WriteData (0xE4);
	SPI_WriteData (0x88);
	SPI_WriteData (0x00);
	SPI_WriteData (0x40);

	SPI_WriteComm (0xEC);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);

	SPI_WriteComm (0xED);
	SPI_WriteData (0xFF);
	SPI_WriteData (0xF0);
	SPI_WriteData (0x07);
	SPI_WriteData (0x65);
	SPI_WriteData (0x4F);
	SPI_WriteData (0xFC);
	SPI_WriteData (0xC2);
	SPI_WriteData (0x2F);
	SPI_WriteData (0xF2);
	SPI_WriteData (0x2C);
	SPI_WriteData (0xCF);
	SPI_WriteData (0xF4);
	SPI_WriteData (0x56);
	SPI_WriteData (0x70);
	SPI_WriteData (0x0F);
	SPI_WriteData (0xFF);

	SPI_WriteComm (0xEF);
	SPI_WriteData (0x10);
	SPI_WriteData (0x0D);
	SPI_WriteData (0x04);
	SPI_WriteData (0x08);
	SPI_WriteData (0x3F);
	SPI_WriteData (0x1F);

	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);

	mdelay(114);			//Sleep out cmd cannot be send after hw-reset for 120ms.
	SPI_WriteComm (0x11);	//sleep out.

	SPI_WriteComm (0x35);
	SPI_WriteData (0x00);

#if 0
	SPI_WriteComm (0x36);	//
	SPI_WriteData (0x10);   //bit4(0:normal scan, 1:inverse scan); bit3(0:RGB, 1:BGR), default:0x00
#endif

	SPI_WriteComm (0x3A);	//RGB Interface pixel format.
	SPI_WriteData (0x77);	//bit[4-6](101:16-bit/pixel; 110:18-bit/pixel;111:24-bit/pixel), default:0x70

	//SPI_WriteComm (0x11);
	//Delay(120);

	SPI_WriteComm (0x29);	//display on.
}

static void st7701s_hw_reset(void)
{
	gpio_direction_output(ST7701S_GPIO_SPI_RST, 1);
	mdelay(10);
	gpio_direction_output(ST7701S_GPIO_SPI_RST, 0);
	mdelay(1);	//>=9us
	gpio_direction_output(ST7701S_GPIO_SPI_RST, 1);
	mdelay(6);	//>=5ms
}

int st7701s_init(void)
{
	gpio_direction_output(ST7701S_GPIO_SPI_CS, 1);	//CS=1
	gpio_direction_output(ST7701S_GPIO_SPI_SCL, 1);	//SCL=1
	gpio_direction_output(ST7701S_GPIO_SPI_SDA, 0);	//SDA=0

	st7701s_hw_reset();

	st7701s_reg_init();

	return 0;
}

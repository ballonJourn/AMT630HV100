#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"


#if LCD_INTERFACE_TYPE == LCD_INTERFACE_MIPI
/* Global macro defination */
#define MIPI_WTIRE_DATA				0x70
#define MIPI_WTIRE_COMMAND			0x6c

#define ARK_MIPI_WRITEL(reg, val)	writel(val, REGS_MIPI_BASE + reg); udelay(200)
#define ARK_MIPI_READL(reg)			readl(REGS_MIPI_BASE + reg)

/* Mipi panel parameters */
#define LPDT_LPK					0x39	//1cmd + >=2data
#define LPDT_SPK					0x15	//1cmd + 1data.
#define NORM_LPK					0x05	//1cmd

#define MIPI_WIDTH					LCD_WIDTH
#define MIPI_HEIGHT					LCD_HEIGHT
#define MIPI_VBP					LCD_TIMING_VBP
#define MIPI_VFP					LCD_TIMING_VFP
#define MIPI_VSW					LCD_TIMING_VSW
#define MIPI_HBP					LCD_TIMING_HBP
#define MIPI_HFP					LCD_TIMING_HFP
#define MIPI_HSW					LCD_TIMING_HSW
#define MIPI_SPEED					460000000

#define MIPI_LANE					4	//1-4LANE
#define DISPLAY_MODE				0	//0: nornal mode;	1: test colour bar mode.
#define MIPI_NP_POLARITY_INVERSE	0	//0: clk and data NP polarity normal; 1:clk and data NP polarity inverse.
//#define MIPI_RESET_GPIO				73
/* End of mipi panel parameters */

#define MIPI_SEND_DATA(arg...)		do { \
										uint8_t buf[] = {arg}; \
										mipi_write_data(buf, sizeof(buf)); \
										mdelay(2);	\
									} while (0)


static void mipi_set_speed(uint32_t speed)
{
	speed = speed / 1000000;

	if ((speed >= 200) && (speed <= 219))
	{
		ARK_MIPI_WRITEL(0xb8, 0x06);
	}
	else  if((speed >= 220) && (speed <= 239))
	{
		ARK_MIPI_WRITEL(0xb8, 0x26);
	}
	else  if((speed >= 240) && (speed <= 249))
	{
		ARK_MIPI_WRITEL(0xb8, 0x46);
	}
	else  if((speed >= 250) && (speed <= 269))
	{
		ARK_MIPI_WRITEL(0xb8, 0x08);
	}
	else  if((speed >= 270) && (speed <= 299))
	{
		ARK_MIPI_WRITEL(0xb8, 0x28);
	}
	else  if((speed >= 300) && (speed <= 329))
	{
		ARK_MIPI_WRITEL(0xb8, 0x0a);
	}
	else  if((speed >= 330) && (speed <= 359))
	{
		ARK_MIPI_WRITEL(0xb8, 0x2a);
	}
	else  if((speed >= 260) && (speed <= 399))
	{
		ARK_MIPI_WRITEL(0xb8, 0x4a);
	}
	else  if((speed >= 400) && (speed <= 449))
	{
		ARK_MIPI_WRITEL(0xb8, 0x0c);
	}
	else  if((speed >= 450) && (speed <= 499))
	{
		ARK_MIPI_WRITEL(0xb8, 0x2c);
	}
	else  if((speed >= 500) && (speed <= 549))
	{
		ARK_MIPI_WRITEL(0xb8, 0x0e);
	}
	else  if((speed >= 550) && (speed <= 599))
	{
		ARK_MIPI_WRITEL(0xb8, 0x2e);
	}
	else  if((speed >= 600) && (speed <= 649))
	{
		ARK_MIPI_WRITEL(0xb8, 0x10);
	}
	else  if((speed >= 650) && (speed <= 699))
	{
		ARK_MIPI_WRITEL(0xb8, 0x30);
	}
	else  if((speed >= 700) && (speed <= 749))
	{
		ARK_MIPI_WRITEL(0xb8, 0x12);
	}
	else  if((speed >= 750) && (speed <= 799))
	{
		ARK_MIPI_WRITEL(0xb8, 0x32);
	}
	else  if((speed >= 800) && (speed <= 849))
	{
		ARK_MIPI_WRITEL(0xb8, 0x52);
	}
	else  if((speed >= 850) && (speed <= 899))
	{
		ARK_MIPI_WRITEL(0xb8, 0x72);
	}
	else  if((speed >= 900) && (speed <= 949))
	{
		ARK_MIPI_WRITEL(0xb8, 0x14);
	}
	else  if((speed >= 950) && (speed <= 1000))
	{
		ARK_MIPI_WRITEL(0xb8, 0x34);
	}
	else
	{
		ARK_MIPI_WRITEL(0xb8, 0x2c);
	}
}

static int mipi_write_data(const uint8_t *buffer, int length)
{
	uint32_t value = 0;
	int mult = length / 4;
	int left = length % 4;
	int i = 0;

	if (!buffer || (length <= 0))
	{
		printf("%s: Invalid buffer:%p or length:%d\n", __func__, buffer, length);
		return -1;
	}

	if (length == 1)		//only 1cmd.
	{
		ARK_MIPI_WRITEL(MIPI_WTIRE_COMMAND, (buffer[0]<<8) | NORM_LPK);
	}
	else if (length == 2)	//1cmd + 1data.
	{
		ARK_MIPI_WRITEL(MIPI_WTIRE_DATA, (buffer[1]<<8) | buffer[0]);
		ARK_MIPI_WRITEL(MIPI_WTIRE_COMMAND, (length<<8) | LPDT_SPK);
	}
	else	//1cmd + >=2data.
	{
		for (i=0; i<mult; i++) {
			value = (buffer[4*i+3]<<24) | (buffer[4*i+2]<<16) | (buffer[4*i+1]<<8) | buffer[4*i+0];
			ARK_MIPI_WRITEL(MIPI_WTIRE_DATA, value);
		}
		if (left) {
			for (value=0,i=0; i<left; i++) {
				value |= (buffer[4*mult+i]<<(8*i));
			}
			ARK_MIPI_WRITEL(MIPI_WTIRE_DATA, value);
		}
		ARK_MIPI_WRITEL(MIPI_WTIRE_COMMAND, LPDT_LPK | (length<<8));
	}

	return 0;
}

static int mipi_panel_init(void)
{
	printf("%s start\n", __func__);

#ifdef MIPI_RESET_GPIO
	gpio_direction_output(MIPI_RESET_GPIO, 1);
	mdelay(10);
	gpio_direction_output(MIPI_RESET_GPIO, 0);
	mdelay(220);
	gpio_direction_output(MIPI_RESET_GPIO, 1);
	mdelay(120);
#endif

	MIPI_SEND_DATA(0xB9,0xFF,0x83,0x94);
	MIPI_SEND_DATA(0xB1,0x48,0x12,0x72,0x09,0x32,0x24,0x71,0x51,0x2F,0x43);
	MIPI_SEND_DATA(0xBA,0x63,0x03,0x68,0x6B,0xB2,0xC0);
	MIPI_SEND_DATA(0xB2,0x40,0xA0,0x64,0x0E,0x0A,0x2F);
	MIPI_SEND_DATA(	0xB4,0x1C,0x78,0x1C,0x78,0x1C,0x78,0x01,0x0C,0x86,0x55,0x00,0x3F,0x1C,0x78,0x1C,
					0x78,0x1C,0x78,0x01,0x0C,0x86);
	MIPI_SEND_DATA(	0xD3,0x00,0x00,0x00,0x00,0x64,0x07,0x08,0x08,0x32,0x10,0x07,0x00,0x07,0x32,0x15,
					0x15,0x05,0x15,0x00,0x32,0x10,0x08,0x00,0x35,0x33,0x09,0x09,0x37,0x0D,0x07,0x37,
					0x0E,0x08);
	MIPI_SEND_DATA(	0xD5,0x18,0x18,0x24,0x24,0x1A,0x1A,0x1B,0x1B,0x04,0x05,0x06,0x07,0x00,0x01,0x02,
					0x03,0x18,0x18,0x19,0x19,0x20,0x21,0x22,0x23,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
					0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18);
	MIPI_SEND_DATA(	0xD6,0x19,0x19,0x24,0x24,0x1A,0x1A,0x1B,0x1B,0x03,0x02,0x01,0x00,0x07,0x06,0x05,
					0x04,0x18,0x18,0x18,0x18,0x23,0x22,0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
					0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18);
	MIPI_SEND_DATA(0xB6,0x48,0x48);
	MIPI_SEND_DATA(	0xE0,0x00,0x1E,0x28,0x2F,0x30,0x33,0x37,0x34,0x6B,0x7A,0x88,0x84,0x8B,0x9B,0x9E,
					0x9F,0xAA,0xAA,0xA4,0xB1,0xC0,0x5F,0x5D,0x61,0x65,0x68,0x70,0x7E,0x7F,0x00,0x1E,
					0x28,0x2F,0x30,0x33,0x37,0x34,0x6B,0x7A,0x88,0x84,0x8B,0x9B,0x9E,0x9F,0xAA,0xAA,
					0xA4,0xB1,0xC0,0x5F,0x5D,0x61,0x65,0x68,0x70,0x7E,0x7F);
	MIPI_SEND_DATA(0xCC,0x03);
	MIPI_SEND_DATA(0xC0,0x1F,0x31);
	MIPI_SEND_DATA(0xD4,0x02);
	MIPI_SEND_DATA(0xBD,0x01);
	MIPI_SEND_DATA(0xB1,0x00);
	MIPI_SEND_DATA(0xBD,0x00);
	MIPI_SEND_DATA(0xC6,0xED);
	MIPI_SEND_DATA(0x11);
	mdelay(200);
	MIPI_SEND_DATA(0x29);
	mdelay(120);

	printf("%s finish\n", __func__);
	
	return 0;
}

#if MIPI_NP_POLARITY_INVERSE
/* mipi clk and 4channel data NP polarity inverse. */
static void mipi_np_polarity_inv(void)
{
	ARK_MIPI_WRITEL(0xb8, 0x35);
	ARK_MIPI_WRITEL(0xb8, 0x10035);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x1);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x45);
	ARK_MIPI_WRITEL(0xb8, 0x10045);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x1);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x55);
	ARK_MIPI_WRITEL(0xb8, 0x10055);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x1);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x85);
	ARK_MIPI_WRITEL(0xb8, 0x10085);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);
	ARK_MIPI_WRITEL(0xb8, 0x1);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x95);
	ARK_MIPI_WRITEL(0xb8, 0x10095);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	ARK_MIPI_WRITEL(0xb8, 0x1);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);
}
#endif

void mipi_init(void)
{
	unsigned int val;

	val = readl(REGS_SYSCTL_BASE + SYS_ANA1_CFG);
	val |= (0x3 <<30)|(0x7 <<22);
	writel(val, REGS_SYSCTL_BASE + SYS_ANA1_CFG);

	val = readl(REGS_SYSCTL_BASE + SYS_PERCTL_CFG);
	val |= (0x1<<8);
	writel(val, REGS_SYSCTL_BASE + SYS_PERCTL_CFG);

	ARK_MIPI_WRITEL(0x04, 0x0);
	ARK_MIPI_WRITEL(0xa0, 0x0);
	ARK_MIPI_WRITEL(0x08, 0x17);
	ARK_MIPI_WRITEL(0x2c, 0x1c);
	ARK_MIPI_WRITEL(0x9c, 0x09000c);
	ARK_MIPI_WRITEL(0x94, 0x0);
	ARK_MIPI_WRITEL(0x98, 0x180014);
	ARK_MIPI_WRITEL(0xa4, 0x2800+(MIPI_LANE-1));
	ARK_MIPI_WRITEL(0x38, 0xbf02);
	ARK_MIPI_WRITEL(0x0c, 0x0);
	ARK_MIPI_WRITEL(0x68, 0x000b4700);
	ARK_MIPI_WRITEL(0x10, 0x5);
	ARK_MIPI_WRITEL(0x14, 0x00);
	ARK_MIPI_WRITEL(0x3c, MIPI_WIDTH);
	ARK_MIPI_WRITEL(0x48, MIPI_HSW);
	ARK_MIPI_WRITEL(0x4c, MIPI_HBP);
	ARK_MIPI_WRITEL(0x50, MIPI_WIDTH+MIPI_HSW+MIPI_HBP+MIPI_HFP);
	ARK_MIPI_WRITEL(0x54, MIPI_VSW);
	ARK_MIPI_WRITEL(0x58, MIPI_VBP);
	ARK_MIPI_WRITEL(0x5c, MIPI_VFP);
	ARK_MIPI_WRITEL(0x60, MIPI_HEIGHT);
	ARK_MIPI_WRITEL(0x34, 0x1);
	ARK_MIPI_WRITEL(0x18, 0xa000a);
	ARK_MIPI_WRITEL(0xc4, 0xffffffff);
	ARK_MIPI_WRITEL(0xc8, 0xffffffff);
	ARK_MIPI_WRITEL(0x04, 0x1);
	ARK_MIPI_WRITEL(0xa0, 0xf);
#if MIPI_NP_POLARITY_INVERSE
	mipi_np_polarity_inv();
#endif
	ARK_MIPI_WRITEL(0xb8, 0x44);
	ARK_MIPI_WRITEL(0xb8, 0x10044);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);
	//ARK_MIPI_WRITEL(0xb8, 0x0C);	//MIPI CLK
	mipi_set_speed(MIPI_SPEED);
	ARK_MIPI_WRITEL(0xb4, 0x2);
	ARK_MIPI_WRITEL(0xb4, 0x0);

	while(!(ARK_MIPI_READL(0xb0) & 0x5));

	ARK_MIPI_WRITEL(0x94, 0x1);

	ARK_MIPI_WRITEL(0x100, 0x1000000);
	mdelay(200);

	/* mipi panel init */
	mipi_panel_init();

#if DISPLAY_MODE
	ARK_MIPI_WRITEL(0x38, 0x1bf02);	//test mode  test mode colour bar
#else
	ARK_MIPI_WRITEL(0x38, 0xbf02);	//normal mode
#endif

	ARK_MIPI_WRITEL(0x34, 0x0);
}
#endif

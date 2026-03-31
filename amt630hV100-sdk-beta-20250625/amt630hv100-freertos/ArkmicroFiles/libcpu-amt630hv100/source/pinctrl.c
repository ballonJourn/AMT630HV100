#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"

#define PINCTL_REG_BASE		REGS_SYSCTL_BASE

#define MAX_PINS_PER_GROUP	32

typedef struct {
	short muxio;
	short pinval;
	int drive;
} xPin_t;

typedef struct {
	int	groupid;
	uint32_t mux_reg;
	uint32_t mux_offset;
	uint32_t mux_mask;
	uint32_t mux_val;
	short pins_num;
	xPin_t pins[MAX_PINS_PER_GROUP];
} xPinGroup_t;

/* typedef struct {

} xPinFunction_t; */

typedef struct {
	int reg;
	int offset;
	int mask;
} xPinmap_t;

static xPinmap_t amt630h_pin_map[] = {
	{0xc0, 0, 0x3},
	{0xc0, 2, 0x3},
	{0xc0, 4, 0x3},
	{0xc0, 6, 0x3},
	{0xc0, 8, 0x3},
	{0xc0, 10, 0x3},
	{0xc0, 12, 0x3},
	{0xc0, 14, 0x3},
	{0xc0, 16, 0x3},
	{0xc0, 18, 0x3},
	{0xc0, 20, 0x3},
	{0xc0, 22, 0x3},
	{0xc0, 24, 0x3},
	{0xc0, 26, 0x3},
	{0xc0, 28, 0x3},
	{0xc0, 30, 0x3},
	{0xc4, 0, 0x3},
	{0xc4, 2, 0x3},
	{0xc4, 4, 0x3},
	{0xc4, 6, 0x3},
	{0xc4, 8, 0x3},
	{0xc4, 10, 0x3},
	{0xc4, 12, 0x3},
	{0xc4, 14, 0x3},
	{0xc4, 16, 0x3},
	{0xc4, 18, 0x3},
	{0xc4, 20, 0x3},
	{0xc4, 22, 0x3},
	{0xc4, 24, 0x3},
	{0xc4, 26, 0x3},
	{0xc4, 28, 0x3},
	{0xc4, 30, 0x3},
	{0xc8, 0, 0x3},
	{0xc8, 2, 0x3},
	{0xc8, 4, 0x3},
	{0xc8, 6, 0x3},
	{0xc8, 8, 0x3},
	{0xc8, 10, 0x3},
	{0xc8, 12, 0x3},
	{0xc8, 14, 0x3},
	{0xc8, 16, 0x3},
	{0xc8, 18, 0x3},
	{0xc8, 20, 0x3},
	{0xc8, 22, 0x3},
	{0xc8, 24, 0x3},
	{0xc8, 26, 0x3},
	{0xc8, 28, 0x3},
	{0xc8, 30, 0x3},
	{0xcc, 0, 0x3},
	{0xcc, 2, 0x3},
	{0xcc, 4, 0x3},
	{0xcc, 6, 0x3},
	{0xcc, 8, 0x3},
	{0xcc, 10, 0x3},
	{0xcc, 12, 0x3},
	{0xcc, 14, 0x3},
	{0xcc, 16, 0x3},
	{0xcc, 18, 0x3},
	{0xcc, 20, 0x3},
	{0xcc, 22, 0x3},
	{0xcc, 24, 0x3},
	{0xcc, 26, 0x3},
	{0xcc, 28, 0x3},
	{0xcc, 30, 0x3},
	{0xd0, 0, 0x3},
	{0xd0, 2, 0x3},
	{0xd0, 4, 0x3},
	{0xd0, 6, 0x3},
	{0xd0, 8, 0x3},
	{0xd0, 10, 0x3},
	{0xd0, 12, 0x3},
	{0xd0, 14, 0x3},
	{0xd0, 16, 0x3},
	{0xd0, 18, 0x3},
	{0xd0, 20, 0x3},
	{0xd0, 22, 0x3},
	{0xd0, 24, 0x3},
	{0xd0, 26, 0x3},
	{0xd0, 28, 0x3},
	{0xd0, 30, 0x3},
	{0xd4, 0, 0x3},
	{0xd4, 2, 0x3},
	{0xd4, 4, 0x3},
	{0xd4, 6, 0x3},
	{0xd4, 8, 0x3},
	{0xd4, 10, 0x3},
	{0xd4, 12, 0x3},
	{0xd4, 14, 0x3},
	{0xd4, 16, 0x3},
	{0xd4, 18, 0x3},
	{0xd4, 20, 0x3},
	{0xd4, 22, 0x3},
	{0xd4, 24, 0x3},
	{0xd4, 26, 0x3},
	{0xd4, 28, 0x3},
	{0xd4, 30, 0x3},
	{0xd8, 4, 0x3},
	{0xd8, 0, 0x3},
	{0xd8, 6, 0x3},
	{0xd8, 2, 0x3},
	{0xd8, 8, 0x3},
	{0xd8, 10, 0x3},	
	/* pad not mux with gpio */

};
#define PIN_NUM		ARRAY_SIZE(amt630h_pin_map)

static xPinGroup_t pin_groups[] = {
													/* scl      sda */
	{.groupid = PGRP_I2C0, .pins_num = 2, .pins = {{48, 1}, {49, 1}}},
													/* scl      sda */
	{.groupid = PGRP_I2C1, .pins_num = 2, .pins = {{50, 1}, {51, 1}}},
	{.groupid = PGRP_LCD_TTL_CH0, .pins_num = 28,
				/* de       clk      vynsc    hsync */
		.pins = {{88, 1}, {89, 1, PAD_DRIVE_2MA}, {90, 1}, {91, 1},
				 {64, 1}, {65, 1}, {66, 1}, {67, 1}, {68, 1}, {69, 1}, {70, 1}, {71, 1},	/* B0-B7 */
				 {72, 1}, {73, 1}, {74, 1}, {75, 1}, {76, 1}, {77, 1}, {78, 1}, {79, 1},	/* G0-G7 */
				 {80, 1}, {81, 1}, {82, 1}, {83, 1}, {84, 1}, {85, 1}, {86, 1}, {87, 1},}},	/* R0-R7 */
	{.groupid = PGRP_LCD_TTL_CH1, .pins_num = 28,
				/* de       clk      vynsc    hsync */
		.pins = {{12, 1}, {13, 1}, {14, 1}, {15, 1},
				 {64, 1}, {65, 1}, {66, 1}, {67, 1}, {68, 1}, {69, 1}, {70, 1}, {71, 1},	/* r0-r7 */
				 {72, 1}, {73, 1}, {74, 1}, {75, 1}, {84, 3}, {85, 3}, {86, 3}, {87, 3},	/* g0-g7 */
				 {88, 3}, {89, 3}, {90, 3}, {91, 3}, {8, 1}, {9, 1}, {10, 1}, {11, 1},}},	/* b0-b7 */
	{.groupid = PGRP_LCD_LVDS, .pins_num = 10, 
				 /* dp      dn       clkp     clkn     cp       cn       bp       bn */
		.pins = {{64, 2}, {65, 2}, {66, 2}, {67, 2}, {68, 2}, {69, 2}, {70, 2}, {71, 2},
				 /* ap      an */
				{72, 2}, {73, 2}, }},
	{.groupid = PGRP_LCD_SRGB, .pins_num = 8,
		.pins = {{74, 1}, {75, 1}, {76, 1}, {77, 1}, {78, 1}, {79, 1}, {80, 1}, {81, 1},}},	/* d0-d7 */
	/* 由于uart0有些平台rx没有接上拉电阻，需要将rx脚配置成gpio，防止收到随机数据导致异常 */
	{.groupid = PGRP_UART0, .pins_num = 2, .pins = {{38, 1}, {39, 1}}},
#ifdef UART1_FLOW_CTL
													/* rx	   tx		cts		rts */
	{.groupid = PGRP_UART1, .pins_num = 4, .pins = {{40, 1}, {41, 1}, {100, 1}, {101, 1}}},
#else
													/* rx	   tx */
	{.groupid = PGRP_UART1, .pins_num = 2, .pins = {{40, 1}, {41, 1}}},
#endif
													/* rx      tx */
	{.groupid = PGRP_UART2, .pins_num = 2, .pins = {{42, 1}, {43, 1}}},
#ifdef UART3_FLOW_CTL
													/* rx	   tx		cts		rts */
	{.groupid = PGRP_UART3, .pins_num = 4, .pins = {{44, 1}, {45, 1}, {46, 1}, {47, 1}}},
#else
													/* rx      tx */
	{.groupid = PGRP_UART3, .pins_num = 2, .pins = {{44, 1}, {45, 1}}},
#endif
													/* cs        clk	d0		d1		d2		d3 */      
	{.groupid = PGRP_SPI0, .pins_num = 5, .pins = {/*{32, 1},*/ {33, 1}, {34, 1}, {35, 1}, {36, 1}, {37, 1}}},
													/* cs		clk		txd		rxd */
	{.groupid = PGRP_SPI1, .pins_num = 3, .pins = {/*{23, 1},*/ {24, 1}, {25, 1}, {26, 1}}},
	{.groupid = PGRP_SDMMC0, .pins_num = 7, .pins = {{16, 1}, {17, 1}, {18, 1}, {19, 1},
													{20, 1}, {21, 1}, {22, 1}}},
#ifdef HCN_BL_PWM_ENABLE
	{.groupid = PGRP_PWM0, .pins_num = 1, .pins = {{0, 1}}},
#else
	{.groupid = PGRP_PWM1, .pins_num = 1, .pins = {{1, 1}}},
	{.groupid = PGRP_PWM2, .pins_num = 1, .pins = {{2, 1}}},
	{.groupid = PGRP_PWM3, .pins_num = 1, .pins = {{3, 1}}},
#endif
	{.groupid = PGRP_PWM0_IN, .pins_num = 1, .pins = {{4, 0}},//pin18   GPIO4
        .mux_reg = 0x60000120, .mux_offset = 12, .mux_mask = 0x3, .mux_val = 0},
	{.groupid = PGRP_PWM1_IN, .pins_num = 1, .pins = {{5, 0}},//pin19   GPIO5
        .mux_reg = 0x60000120, .mux_offset = 14, .mux_mask = 0x3, .mux_val = 0},
	{.groupid = PGRP_PWM2_IN, .pins_num = 1, .pins = {{6, 0}},//pin47   GPIO6
        .mux_reg = 0x60000120, .mux_offset = 28, .mux_mask = 0x3, .mux_val = 0},
	{.groupid = PGRP_PWM3_IN, .pins_num = 1, .pins = {{7, 0}},//pin48   GPIO7
        .mux_reg = 0x60000120, .mux_offset = 30, .mux_mask = 0x3, .mux_val = 0},
	{.groupid = PGRP_ITU_CH0, .pins_num = 11,
		.pins = {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {96, 0}, {97, 0}, {98, 0}, {99, 0}, /* d0-d7 */
				 {100, 0}, {101, 0}, {0, 0}},	/* hs, vs, clk */
		.mux_reg = 0x600000dc, .mux_offset = 16, .mux_mask = 0x7, .mux_val = 2},
	{.groupid = PGRP_ITU_CH0_INV, .pins_num = 11,
		.pins = {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {96, 0}, {97, 0}, {98, 0}, {99, 0}, /* d0-d7 */
				 {100, 0}, {101, 0}, {0, 0}},	/* hs, vs, clk */
		.mux_reg = 0x600000dc, .mux_offset = 16, .mux_mask = 0x7, .mux_val = 6},
	{.groupid = PGRP_ITU_CH1, .pins_num = 11,
		.pins = {{8, 0}, {9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0}, {15, 0}, /* d0-d7 */
				 {59, 0}, {60, 0}, {61, 0}},	/* hs, vs, clk */
		.mux_reg = 0x600000dc, .mux_offset = 16, .mux_mask = 0x7, .mux_val = 3},
	{.groupid = PGRP_ITU_CH1_INV, .pins_num = 11,
		.pins = {{8, 0}, {9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0}, {15, 0}, /* d0-d7 */
				 {59, 0}, {60, 0}, {61, 0}},	/* hs, vs, clk */
		.mux_reg = 0x600000dc, .mux_offset = 16, .mux_mask = 0x7, .mux_val = 7},
												  /* tx      rx */
	{.groupid = PGRP_CAN0_CH0, .pins_num = 2, .pins = {{0, 2}, {1, 2}},
		.mux_reg = 0x600000dc, .mux_offset = 19, .mux_mask = 0x3, .mux_val = 1},
	{.groupid = PGRP_CAN0_CH1, .pins_num = 2, .pins = {{12, 2}, {13, 2}},
		.mux_reg = 0x600000dc, .mux_offset = 19, .mux_mask = 0x3, .mux_val = 2},
	{.groupid = PGRP_CAN0_CH2, .pins_num = 2, .pins = {{96, 2}, {97, 2}},
		.mux_reg = 0x600000dc, .mux_offset = 19, .mux_mask = 0x3, .mux_val = 3},
	{.groupid = PGRP_CAN1_CH0, .pins_num = 2, .pins = {{2, 2}, {3, 2}},
		.mux_reg = 0x600000dc, .mux_offset = 21, .mux_mask = 0x3, .mux_val = 1},
	{.groupid = PGRP_CAN1_CH1, .pins_num = 2, .pins = {{14, 2}, {15, 2}},
		.mux_reg = 0x600000dc, .mux_offset = 21, .mux_mask = 0x3, .mux_val = 2},
	{.groupid = PGRP_CAN1_CH2, .pins_num = 2, .pins = {{98, 2}, {99, 2}},
		.mux_reg = 0x600000dc, .mux_offset = 21, .mux_mask = 0x3, .mux_val = 3},
	{.groupid = PGRP_I2S0_PLAY, .pins_num = 4, .pins = {{52, 1, PAD_DRIVE_2MA}, {54, 1, PAD_DRIVE_2MA}, {55, 1, PAD_DRIVE_2MA}, {56, 1, PAD_DRIVE_2MA}}},
	{.groupid = PGRP_I2S0_RECORD, .pins_num = 4, .pins = {{52, 1, PAD_DRIVE_2MA}, {53, 1, PAD_DRIVE_2MA}, {55, 1, PAD_DRIVE_2MA}, {56, 1, PAD_DRIVE_2MA}}},
	{.groupid = PGRP_I2S1_PLAY, .pins_num = 4, .pins = {{23, 1, PAD_DRIVE_2MA}, {24, 1, PAD_DRIVE_2MA}, {25, 1, PAD_DRIVE_2MA}, {26, 1, PAD_DRIVE_2MA}},
			//bit2: select i2s1 function(bit[2] 0:ssp1_interface; 1:i2s1_interface).
			//bit3: i2s1 data pin output(bit[3] 0:input, 1:output).
		.mux_reg = REGS_SYSCTL_BASE + SYS_PAD_CTRL08, .mux_offset = 2, .mux_mask = 0x3, .mux_val = 3},
	{.groupid = PGRP_I2S1_RECORD, .pins_num = 4, .pins = {{23, 1, PAD_DRIVE_2MA}, {24, 1, PAD_DRIVE_2MA}, {25, 1, PAD_DRIVE_2MA}, {26, 1, PAD_DRIVE_2MA}},
			//bit2: select i2s1 function(bit[2] 0:ssp1_interface; 1:i2s1_interface).
			//bit3: i2s1 data pin  input(bit[3] 0:input, 1:output).
		.mux_reg = REGS_SYSCTL_BASE + SYS_PAD_CTRL08, .mux_offset = 2, .mux_mask = 0x3, .mux_val = 1},
	{.groupid = PGRP_RCRT, .pins_num = 1, .pins = {{57, 1}}},
};
#define GROUP_NUM	ARRAY_SIZE(pin_groups)	

static __INLINE void pinctrl_set_pin(int npin, int val, int drive)
{
	xPinmap_t *pctrl;
	uint32_t reg;

	if (npin >= PIN_NUM)
		return;

	pctrl = &amt630h_pin_map[npin];

	reg = readl(PINCTL_REG_BASE + pctrl->reg);
	reg &= ~(pctrl->mask << pctrl->offset);
	reg |= val << pctrl->offset;
	writel(reg, PINCTL_REG_BASE + pctrl->reg);

	if (drive != PAD_DRIVE_DEFAULT) {
		uint32_t drv_reg = SYS_IO_DRIVER00 + npin / 16 * 4;
		uint32_t offset = (npin % 16) * 2;
		uint32_t drv_val = drive - 1;
		vSysctlConfigure(drv_reg, offset, 3, drv_val);
	}
}

void pinctrl_gpio_request(int gpio)
{
	pinctrl_set_pin(gpio, 0, PAD_DRIVE_DEFAULT);
}

void pinctrl_set_group(int groupid)
{
	int i, j;
	xPinGroup_t *pgrp;
	uint32_t reg;
	
	for (i = 0; i < GROUP_NUM; i++) {
		pgrp = &pin_groups[i];
		if (pgrp->groupid == groupid) {
			configASSERT(pgrp->pins_num <= MAX_PINS_PER_GROUP);
			for (j = 0; j < pgrp->pins_num; j++)
				pinctrl_set_pin(pgrp->pins[j].muxio, pgrp->pins[j].pinval,
								pgrp->pins[j].drive);
			if (pgrp->mux_reg) {
				reg = readl(pgrp->mux_reg);
				reg &= ~(pgrp->mux_mask << pgrp->mux_offset);
				reg |= pgrp->mux_val << pgrp->mux_offset;
				writel(reg, pgrp->mux_reg);
			}
			break;
		}
	}
}

void vPinctrlSetup(void)
{
#ifdef HW_I2C0_SUPPORT
	pinctrl_set_group(PGRP_I2C0);
#endif
#ifdef HW_I2C1_SUPPORT
	pinctrl_set_group(PGRP_I2C1);
#endif
	pinctrl_set_group(PGRP_UART0);
	pinctrl_set_group(PGRP_UART2);
#ifdef PWM_CAP_SUPPORT
	pinctrl_set_group(PGRP_PWM2_IN);
#endif
#if 0
	/* 高速串口先初始化再配置pad脚(放在vUartInit()函数中初始化)，否则在初始化过程中收到数据会导致串口出错 */
	pinctrl_set_group(PGRP_UART1);
	pinctrl_set_group(PGRP_UART3);
#endif
	pinctrl_set_group(PGRP_SPI0);
#ifdef SPI1_SUPPORT
	pinctrl_set_group(PGRP_SPI1);
#endif
	pinctrl_set_group(PGRP_SDMMC0);
#ifdef REMOTE_SUPPORT
	pinctrl_set_group(PGRP_RCRT);
#endif
#if LCD_INTERFACE_TYPE == LCD_INTERFACE_TTL
	pinctrl_set_group(PGRP_LCD_TTL_CH0);
#elif LCD_INTERFACE_TYPE == LCD_INTERFACE_LVDS
	pinctrl_set_group(PGRP_LCD_LVDS);
#endif

#ifdef HCN_CARBACK_SUPPORT_ENABLE
	pinctrl_set_group(PGRP_ITU_CH1_INV);
#endif

#ifdef CAN_MODULE_ENABLE
	pinctrl_set_group(PGRP_CAN1_CH0);
#endif

#ifdef AUDIO_REPLAY
#if	(AUDIO_REPLAY_I2S == I2S_ID1)
	pinctrl_set_group(PGRP_I2S1_PLAY);
#else
	pinctrl_set_group(PGRP_I2S0_PLAY);
#endif
#endif

#ifdef AUDIO_RECORD
#if	(AUDIO_RECORD_I2S == I2S_ID1)
	pinctrl_set_group(PGRP_I2S1_RECORD);
#else
	pinctrl_set_group(PGRP_I2S0_RECORD);
#endif
#endif

#ifdef HCN_BL_PWM_ENABLE
	pinctrl_set_group(PGRP_PWM0);
#endif
}

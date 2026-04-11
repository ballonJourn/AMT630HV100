#ifndef _PINCTRL_H
#define _PINCTRL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PGRP_I2C0,
	PGRP_I2C1,
	PGRP_LCD_TTL_CH0,
	PGRP_LCD_TTL_CH1,
	PGRP_LCD_LVDS,
	PGRP_LCD_SRGB,
	PGRP_LCD_ITU601,
	PGRP_UART0,
	PGRP_UART1,
	PGRP_UART2,
	PGRP_UART3,
	PGRP_SPI0,
	PGRP_SPI1,
	PGRP_PWM0,
	PGRP_PWM1,
	PGRP_PWM2,
	PGRP_PWM3,
	PGRP_PWM0_IN,
	PGRP_PWM1_IN,
	PGRP_PWM2_IN,
	PGRP_PWM3_IN,
	PGRP_SDMMC0,
	PGRP_ITU_CH0,
	PGRP_ITU_CH0_INV,
	PGRP_ITU_CH1,
	PGRP_ITU_CH1_INV,
	PGRP_CAN0_CH0,
	PGRP_CAN0_CH1,
	PGRP_CAN0_CH2,
	PGRP_CAN1_CH0,
	PGRP_CAN1_CH1,
	PGRP_CAN1_CH2,
	PGRP_I2S0_PLAY,
	PGRP_I2S0_RECORD,
	PGRP_I2S1_PLAY,
	PGRP_I2S1_RECORD,
	PGRP_RCRT,
}ePingroupID;

typedef enum {
	PAD_DRIVE_DEFAULT,
	PAD_DRIVE_2MA,
	PAD_DRIVE_4MA,
	PAD_DRIVE_8MA,
	PAD_DRIVE_12MA,
}ePadDrive;

void vPinctrlSetup(void);
void pinctrl_gpio_request(int gpio);
void pinctrl_set_group(int groupid);


#ifdef __cplusplus
}
#endif

#endif

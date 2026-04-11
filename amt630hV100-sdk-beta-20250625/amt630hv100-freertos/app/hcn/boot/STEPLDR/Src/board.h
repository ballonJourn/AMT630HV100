#ifndef _BOARD_H
#define _BOARD_H

///< add hcn config 2025/9/17
#include "hcn_config.h"

/********** display configuration **********/
#define LCD_INTERFACE_TTL		0
#define LCD_INTERFACE_LVDS		1
#define LCD_INTERFACE_CPU		2
#define LCD_INTERFACE_MIPI		3

#define LCD_WIRING_MODE_RGB		0
#define LCD_WIRING_MODE_RBG		1
#define LCD_WIRING_MODE_GRB		2
#define LCD_WIRING_MODE_GBR		3
#define LCD_WIRING_MODE_BRG		4
#define LCD_WIRING_MODE_BGR		5

#define LCD_WIRING_BIT_ORDER_MSB	0
#define LCD_WIRING_BIT_ORDER_LSB	1

#define LVDS_PANEL_FORMAT_TI	0
#define LVDS_PANEL_FORMAT_NS	1

#define LVDS_PANEL_DATA_8BIT	0
#define LVDS_PANEL_DATA_6BIT	1

#define CPU_PANEL_18BIT_MODE     0
#define CPU_PANEL_16BIT_MODE     1
#define CPU_PANEL_9BIT_MODE      2
#define CPU_PANEL_8BIT_MODE      3


#define LCD_ROTATE_ANGLE_0		0
#define LCD_ROTATE_ANGLE_90		1
#define LCD_ROTATE_ANGLE_180	2
#define LCD_ROTATE_ANGLE_270	3


#ifdef DOUBLE_POINTER_HALO
#define LCD_WIDTH	1280
#define LCD_HEIGHT	480
#define LCD_BPP		16
#define LCD_INTERFACE_TYPE		LCD_INTERFACE_LVDS
#else
#define LCD_WIDTH	HCN_LCD_WIDTH
#define LCD_HEIGHT	HCN_LCD_HEIGHT
#define LCD_BPP		16
#define LCD_INTERFACE_TYPE		HCN_LCD_INTERFACE_TTL//LCD_INTERFACE_TTL
#endif

#define LCD_H_FLIP				0//水平镜像
#define LCD_V_FLIP				0//垂直镜像
#define LCD_ROTATE_ANGLE        LCD_ROTATE_ANGLE_0


#if LCD_INTERFACE_TYPE == LCD_INTERFACE_TTL
#define LCD_WIRING_MODE		LCD_WIRING_MODE_BGR
#define LCD_WIRING_BIT_ORDER	LCD_WIRING_BIT_ORDER_MSB
#elif LCD_INTERFACE_TYPE == LCD_INTERFACE_LVDS
#define LCD_WIRING_MODE			LCD_WIRING_MODE_BGR
#define LCD_WIRING_BIT_ORDER	LCD_WIRING_BIT_ORDER_MSB
#define LVDS_SCREEN_RST_GPIO	HCN_LVDS_SCREEN_RST_GPIO
#define LVDS_PANEL_FORMAT		LVDS_PANEL_FORMAT_TI
#define LVDS_PANEL_DATA			LVDS_PANEL_DATA_8BIT
#elif LCD_INTERFACE_TYPE == LCD_INTERFACE_CPU
#define CPU_PANEL_DATA			CPU_PANEL_8BIT_MODE
#elif LCD_INTERFACE_TYPE == LCD_INTERFACE_MIPI
#define LCD_WIRING_MODE			LCD_WIRING_MODE_RGB
#define LCD_WIRING_BIT_ORDER	LCD_WIRING_BIT_ORDER_MSB
#endif

#define FB_SIZE		(LCD_WIDTH * LCD_HEIGHT * LCD_BPP / 8)

#define VIDEO_DISPLAY_WIDTH			LCD_WIDTH
#define VIDEO_DISPLAY_HEIGHT		LCD_HEIGHT
#define VIDEO_DISPLAY_BUF_NUM		2

#ifndef HCN_SCREEN_ENABLE

#if (LCD_WIDTH == 1024 && LCD_HEIGHT == 600)
#define LCD_TIMING_VBP		1
#define LCD_TIMING_VFP		1
#define LCD_TIMING_VSW		30
#define LCD_TIMING_HBP		100
#define LCD_TIMING_HFP		100
#define LCD_TIMING_HSW		370
#define LCD_CLK_FREQ		50000000
#elif (LCD_WIDTH == 800 && LCD_HEIGHT == 480)
#define LCD_TIMING_VBP		1
#define LCD_TIMING_VFP		1
#define LCD_TIMING_VSW		30
#define LCD_TIMING_HBP		50
#define LCD_TIMING_HFP		50
#define LCD_TIMING_HSW		180
#define LCD_CLK_FREQ		35000000
#elif (LCD_WIDTH == 1280 && LCD_HEIGHT == 720)
#define LCD_TIMING_VBP		5
#define LCD_TIMING_VFP		65
#define LCD_TIMING_VSW		2
#define LCD_TIMING_HBP		16
#define LCD_TIMING_HFP		42
#define LCD_TIMING_HSW		2
#define LCD_CLK_FREQ		60000000
#elif (LCD_WIDTH == 1280 && LCD_HEIGHT == 480)
#define LCD_TIMING_VBP		5
#define LCD_TIMING_VFP		8
#define LCD_TIMING_VSW		3
#define LCD_TIMING_HBP		16
#define LCD_TIMING_HFP		26
#define LCD_TIMING_HSW		12
#define LCD_CLK_FREQ		40000000
#elif (LCD_WIDTH == 480 && LCD_HEIGHT == 1280)
#define LCD_TIMING_VBP		6// 14
#define LCD_TIMING_VFP		6// 16
#define LCD_TIMING_VSW		16// 2
#define LCD_TIMING_HBP		16
#define LCD_TIMING_HFP		36
#define LCD_TIMING_HSW		26
#define LCD_CLK_FREQ		50000000
#elif (LCD_WIDTH == 480 && LCD_HEIGHT == 960)
#define LCD_TIMING_VBP		20
#define LCD_TIMING_VFP		20
#define LCD_TIMING_VSW		10
#define LCD_TIMING_HBP		30
#define LCD_TIMING_HFP		40
#define LCD_TIMING_HSW		10
#define LCD_CLK_FREQ		33330000
#else
#error "no lcd timing configuraion."
#endif
#else

#if defined (HCN_LCD_TIMING_VBP) && defined(HCN_LCD_TIMING_VFP) && defined(HCN_LCD_TIMING_VSW) && defined(HCN_LCD_TIMING_HBP) && defined(HCN_LCD_TIMING_HFP) && defined(HCN_LCD_TIMING_HSW) && defined(HCN_LCD_CLK_FREQ)
#define LCD_TIMING_VBP		HCN_LCD_TIMING_VBP 
#define LCD_TIMING_VFP		HCN_LCD_TIMING_VFP 
#define LCD_TIMING_VSW	    HCN_LCD_TIMING_VSW 
#define LCD_TIMING_HBP		HCN_LCD_TIMING_HBP 
#define LCD_TIMING_HFP		HCN_LCD_TIMING_HFP 
#define LCD_TIMING_HSW	    HCN_LCD_TIMING_HSW 
#define LCD_CLK_FREQ		HCN_LCD_CLK_FREQ
#else
#error "no lcd timing configuraion."
#endif

#endif
/*******************************************/

/*********** uart configuration ************/
#define UART_MCU_PORT		3
/*******************************************/

/************ usb configuration ************/
#define USB_SUPPORT
#define CONFIG_USB_DWC2_HOST		1
#define CONFIG_USB_NEW_DWC2_HOST	1
/*******************************************/

/************ spi configuration ************/
#define SPI0_QSPI_MODE
/*******************************************/

/************ watch dog io ************/

/************ i2c configuration ************/
//#define I2C_SUPPORT

#ifdef I2C_SUPPORT
#define ANALOG_I2C_SUPPORT		//Analog i2c support
#ifdef ANALOG_I2C_SUPPORT
#define I2C_GPIO0_SDA_PIN		49//51
#define I2C_GPIO0_SCL_PIN		48//50
#endif
#endif
/*******************************************/

#endif

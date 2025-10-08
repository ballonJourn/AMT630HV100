/**
*
* @file hcn_config.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/27 14:57
* @author och
*
*/
#ifndef __HCN_CONFIG_H__
#define __HCN_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HCN_SCREEN_ENABLE

/**
 * @brief 屏幕参数配置
 */

 ///< 屏幕类型
#define HCN_LCD_INTERFACE_TTL		(0)
#define HCN_LCD_INTERFACE_LVDS		(1)
#define HCN_LCD_INTERFACE_CPU		(2)
#define HCN_LCD_INTERFACE_MIPI		(3)

///< 屏幕分辨率
#define HCN_LCD_WIDTH	            800
#define HCN_LCD_HEIGHT	            480
#define HCN_LCD_EC_WIDTH            800
#define HCN_LCD_EC_HEIGHT           480

///< 屏幕色深
#define HCN_LCD_BPP		            32
#define HCN_LCD_INTERFACE_TYPE		HCN_LCD_INTERFACE_TTL

///< 屏幕参数配置
#define HCN_LCD_TIMING_VBP          16
#define HCN_LCD_TIMING_VFP		    16
#define HCN_LCD_TIMING_VSW	        4
#define HCN_LCD_TIMING_HBP		    8
#define HCN_LCD_TIMING_HFP		    8
#define HCN_LCD_TIMING_HSW		    4
#define HCN_LCD_CLK_FREQ            26000000

///< 屏幕GPIO配置
#if HCN_LCD_INTERFACE_TYPE == HCN_LCD_INTERFACE_LVDS
#define HCN_LVDS_SCREEN_RST_GPIO    74
#endif

///< 背光使能GPIO
#define HCN_LCD_BL_EN_GPIO          25

///< 背光亮度控制使能
#define HCN_BL_PWM_ENABLE
#ifdef  HCN_BL_PWM_ENABLE
#define HCN_LCD_PWM_CH              0
#endif

///< OSD显示设置, UI显示大小
#define OSD_WIDTH      800 
#define OSD_HEIGHT     480            

/**
 * @brief memory config 以FreeRTos + AWTK为例
 * ROM[5M] + RAM  的配置:amt630hv100-freertos\proj\amt630hv100_awtk.icf
 * Memory Regions-
 * define symbol __ICFEDIT_region_ROM_start__ = 0x20000080;
 * define symbol __ICFEDIT_region_ROM_end__   = 0x2063ffff;
 * define symbol __ICFEDIT_region_RAM_start__ = 0x20640000;
 * define symbol __ICFEDIT_region_RAM_end__   = 0x23ffffff;
 */
#define HCN_configTOTAL_HEAP_SIZE (( ( size_t ) ( (25.5) * 1024 * 1024) ) )
#define HCN_VG_HEAP_SIZE  ( (12) * 1024 * 1024) 
#define HCN_AWTK_HEAP_SIZE ((17) * 1024 * 1024)

///< 背光亮度控制使能
#define HCN_BL_PWM_ENABLE

///< 定义MCU串口使能
//#define HCN_MCU_UART_ENABLE

///< 定义32MB spi nor flash使能
//#define HCN_SPI_NOR_FLASH_32MB_ENABLE

///< 胎压相关信息
#define HCN_TPMS_NONE        (0)
#define HCN_F433_TPMS_ENABLE (1)
#define HCN_BLE_TPMS_ENABLE  (2)
#define HCN_TPMS_TYPE        HCN_TPMS_NONE

///< wifi相关信息
#define HCN_WIFI_SUPPORT

///< 手机互联使能
#define HCN_CARLINK_ENABLE

///< OTA功能
#ifdef HCN_SPI_NOR_FLASH_32MB_ENABLE
#define HCN_OTA_UPDATE_ENABLE
#endif

///< CAN功能
#define CAN_MODULE_ENABLE
#define CAN_STB_GPIO   (58)

///< BT_WIFI模块类型
#define FSC_BW121       (1)
#define GK_GOCRS440     (2)
#define BT_WIFI_MODULE_TYPE  GK_GOCRS440

///< KEY使能
#define HCN_IO_KEY_ENABLE

#ifndef HCN_IO_KEY_ENABLE
#define HCN_ADC_KEY_ENABLE   ///< ADC KEY使能
#endif  

///< 手机互联名称信息配置
#define HCN_WIFI_NAME_FORMAT_ENABLE                 ///< hcn wifi名称格式化使能,否则使用默认名称
#define HCN_DEFAULT_AP_NAME             "ap63011"   ///< 默认wifi ap名称
#define HCN_CUSTOMER_NAME               "HCN"       ///< 客户名称前缀
#define HCN_CUSTOMER_AP_PASSWD          "88888888"  ///< wifi ap/p2p密码
#undef HCN_STRING_LOWER_ENABLE                      ///< 字符名称默认是大写

///< 串口通信使能,与MCU通信
//#define HCN_UART_COMM_ENABLE
#ifdef HCN_UART_COMM_ENABLE
#define HCN_UART_MCU_PORT    (2)
#define HCN_UART_MCU_BAUDRATE (115200)
#endif

///< 里程保养使能
#define HCN_MILEAGE_MAINTENCE_ENABLE

///< 倒车使能
//#define HCN_CARBACK_SUPPORT_ENABLE

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CONFIG_H__
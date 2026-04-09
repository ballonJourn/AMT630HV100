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
#define HCN_LCD_WIDTH	            1024
#define HCN_LCD_HEIGHT	            600

///< 屏幕色深
#define HCN_LCD_BPP		            32
#define HCN_LCD_INTERFACE_TYPE		HCN_LCD_INTERFACE_LVDS

///< 屏幕参数配置
#define HCN_LCD_TIMING_VBP          23
#define HCN_LCD_TIMING_VFP		    12
#define HCN_LCD_TIMING_VSW	        2
#define HCN_LCD_TIMING_HBP		    136
#define HCN_LCD_TIMING_HFP		    160
#define HCN_LCD_TIMING_HSW		    24
#define HCN_LCD_CLK_FREQ            52000000

///< 屏幕GPIO配置
#if HCN_LCD_INTERFACE_TYPE == HCN_LCD_INTERFACE_LVDS
#define HCN_LVDS_SCREEN_RST_GPIO    74
#define HCN_AVDD_PWM_CH             (2)
#define HCN_AVDD_EN_GPIO            (8)
#endif

///< 背光使能GPIO
#define HCN_LCD_BL_EN_GPIO          (25)

///< lcd屏幕供电,可控制屏幕显示
#define HCN_LCD_DISPLAY_EN_GPIO     (26)

///< 背光亮度控制使能
#define HCN_BL_PWM_ENABLE
#ifdef  HCN_BL_PWM_ENABLE
#define HCN_LCD_PWM_CH              (0)
#endif

///< OSD显示设置, UI显示大小
#define OSD_WIDTH      1024 
#define OSD_HEIGHT     600           

/**
 * @brief memory config 以FreeRTos + AWTK为例
 * ROM[5M] + RAM  的配置:amt630hv100-freertos\proj\amt630hv100_awtk.icf
 * Memory Regions-
 * define symbol __ICFEDIT_region_ROM_start__ = 0x20000080;
 * define symbol __ICFEDIT_region_ROM_end__   = 0x2063ffff;
 * define symbol __ICFEDIT_region_RAM_start__ = 0x20640000;
 * define symbol __ICFEDIT_region_RAM_end__   = 0x23ffffff;
 */
#define HCN_configTOTAL_HEAP_SIZE (( ( size_t ) ( (23.5) * 1024 * 1024) ) )
#define HCN_VG_HEAP_SIZE  ( (12) * 1024 * 1024) 
#define HCN_AWTK_HEAP_SIZE ((19) * 1024 * 1024)

///< 背光亮度控制使能
#define HCN_BL_PWM_ENABLE

///< 定义32MB spi nor flash使能
#define HCN_SPI_NOR_FLASH_32MB_ENABLE

///< 胎压相关信息
#define HCN_TPMS_NONE        (0)
#define HCN_F433_TPMS_ENABLE (1)
#define HCN_BLE_TPMS_ENABLE  (2)
#define HCN_TPMS_TYPE        HCN_TPMS_NONE

///< wifi相关信息
#define HCN_WIFI_SUPPORT
#define HCN_WIFI_INIT_DELAY_ENABLE  ///< wifi初始化延时
#define WIFI_RESET_IO		(13)
#define WIFI_BT_PWR_GPIO    (12)

///< 蓝牙reset io
#define BT_RESET_IO        (46) 
#define BT_UART_PORT       (1)  ///< bt通信串口号

///<用于解决蓝牙wifi 初始化不成功时 复位 
#define WIFI_BT_SDO_CMD_GPIO   (20)
#define WIFI_BT_SDO_CLK_GPIO   (22)
#define WIFI_BT_SDO_D3_GPIO    (19)
#define WIFI_BT_SDO_D2_GPIO    (18)
#define WIFI_BT_SDO_D1_GPIO    (17)
#define WIFI_BT_SDO_D0_GPIO    (16)
#define WIFI_BT_UART_TX_GPIO   (41)
#define WIFI_BT_UART_RX_GPIO   (40)
#define WIFI_BT_UART1_CTS_GPIO (100)
#define WIFI_BT_UART1_RTS_GPIO (101)

///< 手机互联使能
#define HCN_CARLINK_ENABLE
#define HCN_LCD_EC_WIDTH        (1024)
#define HCN_LCD_EC_HEIGHT       (600)

///< 天气功能使能
//#define HCN_CARLINK_WEATHER_ENABLE

///< 简易导航中的路口放大和车道引导图片使能
//#define HCN_CARLINK_ROAD_PIC_ENABLE

///< OTA功能
#ifdef HCN_SPI_NOR_FLASH_32MB_ENABLE
#define HCN_OTA_UPDATE_ENABLE
#endif

///< AW功放供电
#define AW_88028_PWR_EN_GPIO   (23)

///< CAN功能
#define CAN_MODULE_ENABLE
#define CAN_STB_GPIO        (58)
#define CAN_PWR_EN_GPIO     (24)

//#define DEBUG_CAN_INFO_ENABLE

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
#define HCN_CUSTOMER_NAME               "HCN"       ///< 客户名称前缀，会改变
#define HCN_MANUFACTURER_NAME           "HCN"       ///< 制造商名称，固定不改变
#define HCN_CUSTOMER_AP_PASSWD          "88888888"  ///< wifi ap/p2p密码
#undef HCN_STRING_LOWER_ENABLE                      ///< 字符名称默认是大写

#define HCN_CARLINK_PROTOTYPE_MODE
#ifdef HCN_CARLINK_PROTOTYPE_MODE
#define HCN_CHINESE_UUID    "CARBITDC0D30226452"
#endif

///< 串口DVR使能
// #define HCN_UART_DVR_ENABLE
#ifdef HCN_UART_DVR_ENABLE
#define HCN_UART_DVR_PORT    (2)
#define HCN_UART_DVR_BAUDRATE (115200)
#endif

///< 串口通信使能,与MCU通信
#define HCN_UART_COMM_ENABLE
#ifdef HCN_UART_COMM_ENABLE
#define HCN_UART_MCU_PORT    (2)
#define HCN_UART_MCU_BAUDRATE (115200)
#endif

///< mcu升级
#ifdef HCN_UART_COMM_ENABLE
#define HCN_UART_MCU_UPDATE_ENABLE
#define HCN_MCU_PROTOCOL_UPDATE_ENABLE  ///< 恒晨自身升级协议
//#define MCU_UPDATE_SIMULATE_ENABLE      ///< 模拟MCU升级
#endif

///< 里程保养使能
#define HCN_MILEAGE_MAINTENCE_ENABLE

///< 倒车使能
//#define HCN_CARBACK_SUPPORT_ENABLE

///< 光感使能
#define HCN_ADC_LIGHT_SENSOR_ENABLE

///< SOC IGN 使能
//#define HCN_SOC_IGN_ENABLE

///< 关机动画使能
//#define HCN_SHUTDOWN_ANIM_ENABLE

///< nor flash参数配置
#define HCN_NOR_FLASH_PARAM_ENABLE

#ifdef HCN_NOR_FLASH_PARAM_ENABLE
#define PARAM_WEAR_LEVEL_ENABLE     ///< 磨损均衡使能
#endif

///< use flash db 
//#define HCN_USE_FLASH_DB_ENABLE

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CONFIG_H__
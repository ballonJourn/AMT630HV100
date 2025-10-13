#ifndef _BOARD_H
#define _BOARD_H

#include "config/hcn_config.h"

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

#define LCD_H_FLIP				0	//水平镜像
#define LCD_V_FLIP				0	//垂直镜像
//#define LCD_CLK_INVERSE			//LCD时钟极性反向
#define LCD_ROTATE_ANGLE		LCD_ROTATE_ANGLE_0

#if defined(LVGL_VG_GPU) || defined(VG_ONLY) || defined(REVERSE_TRACK)
#ifndef VG_DRIVER
#error "Error! Must define VG_DRIVER."
#endif
#endif

#if defined(DOUBLE_POINTER_HALO) || defined(SINGLE_POINTER_HALO)
#ifndef VG_ONLY
#error "Error! Must define VG_ONLY."
#endif
#endif

#ifdef VG_ONLY
#if defined(DOUBLE_POINTER_HALO)
#define LCD_WIDTH	1280
#define LCD_HEIGHT	480
#define LCD_BPP		16
#define LCD_INTERFACE_TYPE		LCD_INTERFACE_LVDS
#elif defined(SINGLE_POINTER_HALO)
#define LCD_WIDTH	1024
#define LCD_HEIGHT	600
#define LCD_BPP		16
#define LCD_INTERFACE_TYPE		LCD_INTERFACE_TTL
#endif
#elif defined(AWTK)
#define LCD_WIDTH	HCN_LCD_WIDTH 
#define LCD_HEIGHT	HCN_LCD_HEIGHT 
#define LCD_BPP		HCN_LCD_BPP 
#define LCD_INTERFACE_TYPE	HCN_LCD_INTERFACE_TYPE
#else
#define LCD_WIDTH	1024
#define LCD_HEIGHT	600
#ifdef REVERSE_UI
#define LCD_BPP		32
#else
#define LCD_BPP		16
#endif
#define LCD_INTERFACE_TYPE		LCD_INTERFACE_TTL
#endif

#if LCD_INTERFACE_TYPE == LCD_INTERFACE_TTL
#define LCD_WIRING_MODE			LCD_WIRING_MODE_BGR
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

#define VIDEO_DISPLAY_WIDTH			((LCD_WIDTH + 0xF) & (~0xF))
#define VIDEO_DISPLAY_HEIGHT		((LCD_HEIGHT + 0xF) & (~0xF))
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
#define LCD_TIMING_HFP		36// 26
#define LCD_TIMING_HSW		26
#define LCD_CLK_FREQ		50000000
#elif (LCD_WIDTH == 480 && LCD_HEIGHT == 960)
#define LCD_TIMING_VBP		20//10
#define LCD_TIMING_VFP		20//30
#define LCD_TIMING_VSW		10
#define LCD_TIMING_HBP		30//5
#define LCD_TIMING_HFP		40//10
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

/************ adc configuration ************/
//#define ADC_TOUCH
#ifdef HCN_ADC_KEY_ENABLE
#define ADC_KEY
#endif
/*******************************************/

/*******************************************/
#if BT_WIFI_MODULE_TYPE == GK_GOCRS440
#define wifi_RS440
#endif

/******* video in configuration *******/
#define VIDEO_DECODER_RN6752
//#define VIDEO_DECODER_ARK7116
#define VIN_CVBS_PAL		0
#define VIN_CVBS_NTSC		1
#define VIN_AHD_720P_25		2
#define VIN_AHD_720P_30    	3
#define VIDEO_IN_FORMAT		VIN_AHD_720P_25
#if VIDEO_IN_FORMAT == VIN_CVBS_PAL
#define VIN_WIDTH		720
#define VIN_HEIGHT		288
#elif VIDEO_IN_FORMAT == VIN_CVBS_NTSC
#define VIN_WIDTH		720
#define VIN_HEIGHT		240
#elif VIDEO_IN_FORMAT == VIN_AHD_720P_25 || VIDEO_IN_FORMAT == VIN_AHD_720P_30
#define VIN_WIDTH		1280
#define VIN_HEIGHT		720
#endif
#if VIN_WIDTH * VIN_HEIGHT > 0x96000
#define VIN_SMALL_MEM
#endif
/*******************************************/

/************ carback configuration ********/
#define CARBACK_DETECT
/*******************************************/

/********* touchscreen configuration *******/
//#define TP_SUPPORT

#ifdef TP_SUPPORT
/* Select one tp IC */
//#define TP_USE_GT9XX
//#define TP_USE_GA657X
//#define TP_USE_FT6336U

/* Config tp parameters */
#define TP_GPIO_INT			4
#define TP_GPIO_RST			5
#define TP_INV_X			0
#define TP_INV_Y			0
#define TP_INV_XY_AXIS		0	//翻转XY轴
#define TP_MT_TOUCH			0	//支持多点触摸
#endif
/*******************************************/

/*********** carlink enable ***********/
#ifdef HCN_CARLINK_ENABLE
#define CARLINK_ENABLE
#endif

/*********** sdmmc configuration ***********/
#define SDMMC_SUPPORT
/*******************************************/

/*********** uart configuration ************/
#define UART_MCU_PORT		3
/*******************************************/

/************ rtc configuration ************/
#define RTC_SUPPORT
/*******************************************/

/************ remote configuration ************/
//#define REMOTE_SUPPORT
/*******************************************/

/************ pwm capture configuration ************/
//#define PWM_CAP_SUPPORT
/*******************************************/

/************ spi configuration ************/
//#define SPI1_SUPPORT
//#define SPI1_DMA_SUPPORT
/*******************************************/

/************ dma configuration ************/
/* the smaller channel has higher priority */
#define I2S_DMA_RXCH		0
#define I2S_DMA_TXCH		1
#define SPI0_RX_DMA_CH		2
#define SDMMC_DMA_CH		3
#define SPI1_RX_DMA_CH		0
#define SPI1_TX_DMA_CH		3
/*******************************************/

/************ usb configuration ************/
#define USB_SUPPORT
#define USB_MODE_ID			-1
//#define USB_DMA

//#define USB_UVC_SUPPORT
#define CONFIG_USB_DWC2_HOST		1
#define CONFIG_USB_NEW_DWC2_HOST	1
#define CONFIG_USB_DWC2_PERIPHERAL	1
#define CONFIG_USB_NEW_DWC2_GADGET	1
#if	USB_MODE_ID && USB_MODE_ID != -1
#define CONFIG_USB_DEVICE_CDC_NCM   1
#endif
/*******************************************/

/************ carlink configuration ************/
#ifdef HCN_WIFI_SUPPORT	/* define in iar options */
#define WIFI_SUPPORT
#endif

#ifdef WIFI_SUPPORT
#ifdef CARLINK_ENABLE
#define CARLINK_EY			0
//如果要将亿联,carplay和android auto都使能，有可能苹果手机和支持android auto的安卓手机都安装了亿联的应用,
//v100端应用程序必须加上设置界面来配置优先选择连亿联还是手机自带的互联(android auto/carplay)
#define CARLINK_EC			1
#define CARLINK_CP          0
#define CARLINK_AA          0
#endif

#else
#define CARLINK_EY			0
#define CARLINK_EC			0
#define CARLINK_CP          0
#define CARLINK_AA          0
#endif

#if CARLINK_EY && CARLINK_EC
#error "Do not choose two car links"
#endif

#if CARLINK_EC == 1 || CARLINK_CP == 1 || CARLINK_AA == 1
#define USE_LWIP			1
#else
#define USE_LWIP			0
#endif

//#define RELTECK_WIFI_AP_MODE
/*******************************************/

/************ i2c configuration ************/
// #define HW_I2C0_SUPPORT			//Hardware i2c0 support
#define HW_I2C1_SUPPORT			//Hardware i2c1 support
//#define ANALOG_I2C_SUPPORT		//Analog i2c support

#ifdef ANALOG_I2C_SUPPORT
#define I2C_GPIO0_SDA_PIN		102
#define I2C_GPIO0_SCL_PIN		101
#endif
/*******************************************/

/************ audio configuration ************/
#define I2S_ID0					0
#define I2S_ID1					1
#define I2S_NUMS				2

#define AUDIO_FLAG_REPLAY			1
#define AUDIO_FLAG_RECORD			2
#define AUDIO_FLAG_REPLAY_RECORD	(AUDIO_FLAG_REPLAY | AUDIO_FLAG_RECORD)

/* add your adc type */
#define AUDIO_CODEC_ADC_NONE 	0	//Not use codec adc ic or no need driver.
#define AUDIO_CODEC_ADC_ES7243E	1	//Use codec adc ic es7243e.

/* add your dac type */
#define AUDIO_CODEC_DAC_NONE	0	//Not use codec adc ic or no need driver.
#define AUDIO_CODEC_DAC_ES8156	1 	//Use codec dac ic es8156.

/* choose your audio use type */
#define AUDIO_REPLAY
//#define AUDIO_RECORD

#ifdef AUDIO_REPLAY
#define AUDIO_REPLAY_I2S		I2S_ID0					/* Select i2s id */
#define AUDIO_CODEC_DAC_IC		AUDIO_CODEC_DAC_NONE	/* Select your codec dac type */
#endif

#ifdef AUDIO_RECORD
#define AUDIO_RECORD_I2S		I2S_ID1					/* Select i2s id */
#define AUDIO_CODEC_ADC_IC		AUDIO_CODEC_ADC_ES7243E	/* Select your codec adc type */
#endif

#ifndef AUDIO_CODEC_DAC_IC
#define AUDIO_CODEC_DAC_IC		AUDIO_CODEC_DAC_NONE	/* Do not use codec dac by default */
#endif

#ifndef AUDIO_CODEC_ADC_IC
#define AUDIO_CODEC_ADC_IC		AUDIO_CODEC_ADC_NONE	/* Do not use codec adc by default */
#endif
/*******************************************/


/********** romfile configuration **********/
/* 没有定义ROMFILE_USE_SMALL_MEM，romfile内容会全部加载到ddr
 * 内存不足时可以定义ROMFILE_USE_SMALL_MEM节省内存使用 */
#if !defined(VG_ONLY)
#define ROMFILE_USE_SMALL_MEM
#endif
#ifndef ROMFILE_USE_SMALL_MEM
/* 定义READ_ROMFILE_ONCE，romfile内容会整个一次加载到ddr，如果
 * 文件过大会导致启动时间慢 */
//#define READ_ROMFILE_ONCE
#else
/* 缓存在DDR里的文件数，未缓存的文件需要重新从flash里读取 */
#define ROMFILE_CACHE_DEF_SIZE	0
#endif
/*******************************************/

/********** animation configuration **********/
#define ANIMATION_NONE				0
#define ANIMATION_USE_SMALL_MEM		1
#define ANIMATION_NORMAL			2
#define ANIMATION_POLICY	ANIMATION_NORMAL
/*********************************************/

/************ flash type configuration ************/
#define SPI_NOR_FLASH				0
#define SPI_NAND_FLASH				1
#define EMMC_FLASH					2
#define DEVICE_TYPE_SELECT			SPI_NOR_FLASH
/*******************************************/

/********** update address configuration **********/
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
#define LOADER_OFFSET				0x0
#define STEPLDRA_OFFSET				0x20000
#define STEPLDRB_OFFSET				0x40000
#define STEPLDR_MAX_SIZE			0x14000
#define SYSINFOA_MEDIA_OFFSET		0x60000
#define SYSINFOB_MEDIA_OFFSET		0x80000
#define UPDATEFILE_MEDIA_OFFSET		0xa0000
#define UPDATEFILE_MEDIA_B_OFFSET	0xc00000
#define UPDATEFILE_MAX_SIZE			0xb00000
#define OTA_MEDIA_OFFSET			0x1800000
#define OTA_MEDIA_SIZE				0x800000
#define SPI0_QSPI_MODE
#elif DEVICE_TYPE_SELECT == SPI_NOR_FLASH
#define LOADER_OFFSET				0x0
#define STEPLDRA_OFFSET				0x4000
#define STEPLDRB_OFFSET				0x20000
#define STEPLDR_MAX_SIZE			0x14000
#define SYSINFOA_MEDIA_OFFSET		0x3c000
#define SYSINFOB_MEDIA_OFFSET		0x3e000

#define UPDATEFILE_MEDIA_OFFSET		0x40000
#define UPDATEFILE_MEDIA_B_OFFSET	0x1000000   //0xc00000
#define UPDATEFILE_MAX_SIZE			0xf00000   //0xb00000 15M

#if !(1 == CARLINK_CP)
#define OTA_MEDIA_OFFSET			0x1f00000 //0x1600000
#define OTA_MEDIA_SIZE				0x100000 //0xa00000
#else
#define OTA_MEDIA_OFFSET			0xa00000 //0xa00000
#define OTA_MEDIA_SIZE				0x500000 //0x500000
#endif
#define SPI0_QSPI_MODE
#elif DEVICE_TYPE_SELECT == EMMC_FLASH
#define LOADER_OFFSET				0x0
#define LOADERB_OFFSET				0x100000
#define STEPLDRA_OFFSET				0x200000
#define STEPLDRB_OFFSET				0x300000
#define STEPLDR_MAX_SIZE			0x14000
#define SYSINFOA_MEDIA_OFFSET		0x400000
#define SYSINFOB_MEDIA_OFFSET		0x500000
#define UPDATEFILE_MEDIA_OFFSET		0xA00000
#define UPDATEFILE_MEDIA_B_OFFSET	0x40000000
#define UPDATEFILE_MAX_SIZE			0x1000000
#define OTA_MEDIA_OFFSET			0x80000000
#define OTA_MEDIA_SIZE				0x10000000
//#define SPI0_QSPI_MODE
#endif

/*********************************************/

/********** ulog configuration **********/
//#define USE_ULOG
#define ULOG_NAME_MAX	8
#define ULOG_LINE_BUF_SIZE	1024
#define ULOG_OUTPUT_LEVEL
#define ULOG_OUTPUT_TIME
//#define ULOG_TIME_USING_TIMESTAMP
//#define ULOG_OUTPUT_TAG
//#define ULOG_USING_COLOR
#define ULOG_USING_ISR_LOG
#define ULOG_BACKEND_USING_CONSOLE
#define ULOG_EASYFLASH_BACKEND_ENABLE
#ifdef ULOG_EASYFLASH_BACKEND_ENABLE
#define EASYFLASH_LOG
#if DEVICE_TYPE_SELECT != EMMC_FLASH
#define EASYFLASH_LOG_AREA_SIZE		0x100000
#define EASYFLASH_ERASE_GRAN		0x1000
#define EASYFLASH_WRITE_GRAN		0x100
#define EASYFLASH_START_ADDR		0x1700000
#else
#define EASYFLASH_LOG_AREA_SIZE		0x100000
#define EASYFLASH_ERASE_GRAN		0x10000
#define EASYFLASH_WRITE_GRAN		0x200
#define EASYFLASH_START_ADDR		0x60000000
#endif
#endif
//#define ULOG_FILE_BACKEND_ENABLE
#ifdef ULOG_FILE_BACKEND_ENABLE
#if DEVICE_TYPE_SELECT != EMMC_FLASH
#define ULOG_FILE_ROOT_PATH			"/sf/logs"
#else
#define ULOG_FILE_ROOT_PATH			"/sd/logs"
#endif
#define ULOG_FILE_NAME_BASE  		"ulog.log"
#define ULOG_FILE_MAX_NUM    		10
#define ULOG_FILE_MAX_SIZE   		(1024 * 32)
#endif
//#define ULOG_USING_ASYNC_OUTPUT
#define ULOG_ASYNC_OUTPUT_BUF_SIZE	1024
#define ULOG_ASYNC_OUTPUT_THREAD_STACK	2048
#define ULOG_ASYNC_OUTPUT_THREAD_PRIORITY	10
/*********************************************/

/********** update address configuration **********/
#ifdef HCN_OTA_UPDATE_ENABLE
#define OTA_UPDATE_SUPPORT
#endif

#ifdef OTA_UPDATE_SUPPORT
//#define DELTA_UPDATE_SUPPORT

//#define WIFI_UPDATE_SUPPORT
#if defined(WIFI_UPDATE_SUPPORT) && !defined(USB_SUPPORT)
#error "Error! Should define USB_SUPPORT to support wifi simu update"
#endif
//#define NCM_UPDATE_SUPPORT
#if defined(NCM_UPDATE_SUPPORT) && !CONFIG_USB_DEVICE_CDC_NCM
#error "Error! Should define CONFIG_USB_DEVICE_CDC_NCM=1 to support ncm update"
#endif
//#define NCM_LOG_SUPPORT
#if defined(NCM_LOG_SUPPORT) && !CONFIG_USB_DEVICE_CDC_NCM
#error "Error! Should define CONFIG_USB_DEVICE_CDC_NCM=1 to support ncm log"
#endif
#endif
/*********************************************/

void VideoDisplayBufInit(void);
int xVideoDisplayBufTake(uint32_t xTicksToWait);
void vVideoDisplayBufGive(void);
uint32_t ulVideoDisplayBufGet(void);
uint32_t ulVideoDisplayBufGetSize(void);
void vVideoDisplayBufRender(uint32_t buf_addr);
void vVideoDisplayBufFree(uint32_t buf_addr);
unsigned int ulVideoDisplayGetBufferAddr(int index);

#ifdef VIDEO_DECODER_RN6752
int rn6752_init(void);
#endif
#ifdef VIDEO_DECODER_ARK7116
int ark7116_init();
#endif

#ifdef SDMMC_SUPPORT
int mmcsd_core_init(void);
int mmc_init(void);
#endif

int carback_init(void);
void notify_enter_carback(void);
void notify_exit_carback(void);
int get_carback_status(void);


 void aw88082_init(void);
 int aw_pa_start(void);
 void aw_pa_stop(void);

#endif

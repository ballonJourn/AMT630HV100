#ifndef __AMT630HV100__H
#define __AMT630HV100__H

#ifdef __cplusplus
 extern "C" {
#endif

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#include <stdint.h>
#ifdef __cplusplus
  #define __I  volatile       /**< Defines 'read-only'  permissions */
#else
  #define __I  volatile const /**< Defines 'read-only'  permissions */
#endif
#define   __O  volatile       /**< Defines 'write-only' permissions */
#define   __IO volatile       /**< Defines 'read/write' permissions */
#endif

#include "os_adapt.h"

typedef enum IRQn
{
  LCD_IRQn			  =  0, /**<  0 AMT630HV100 LCD Interrupt ID */
  JPG_IRQn			  =  1, /**<  1 AMT630HV100 Jpeg Decoder Interrupt (JPG) */
  GPU_IRQn			  =  2, /**<  2 AMT630HV100 GPU Interrupt (GPU) */
  USB_IRQn			  =  3, /**<  3 AMT630HV100 USB Controller Interrupt (USB) */
  PXP_IRQn			  =  4, /**<  4 AMT630HV100 PXP Interrupt (PXP) */
  DMA_IRQn			  =  5, /**<  5 AMT630HV100 DMA Controller Interrupt (DMAC) */
  SDMMC0_IRQn		  =  6, /**<  6 AMT630HV100 SDMMC 0 Controller Interrupt (SDMMC0) */
  SPI0_IRQn			  =  7, /**<  7 AMT630HV100 SPI 0 Controller Interrupt (SPI0) */
  SPI1_IRQn			  =  8, /**<  8 AMT630HV100 SPI 1 Controller Interrupt (SPI1) */  
  I2C0_IRQn			  =  9, /**<  9 AMT630HV100 I2C0 Controller Interrupt (I2C0) */
  I2C1_IRQn			  = 10, /**< 10 AMT630HV100 I2C1 Controller Interrupt (I2C1) */
  UART0_IRQn 		  = 11, /**< 11 AMT630HV100 UART 0 Controller Interrupt (UART0) */
  UART1_IRQn 		  = 12, /**< 12 AMT630HV100 UART 1 Controller Interrupt (UART1) */
  UART2_IRQn 		  = 13, /**< 13 AMT630HV100 UART 2 Controller Interrupt (UART2) */
  UART3_IRQn 		  = 14, /**< 14 AMT630HV100 UART 3 Controller Interrupt (UART3) */
  GPIOA_IRQn 		  = 15, /**< 15 AMT630HV100 GPIO0~31 Controller Interrupt (GPIOA) */
  GPIOB_IRQn 		  = 16, /**< 16 AMT630HV100 GPIO32~63 Controller Interrupt (GPIOB) */
  GPIOC_IRQn 		  = 17, /**< 17 AMT630HV100 GPIO64~95 Controller Interrupt (GPIOC) */
  GPIOD_IRQn 		  = 18, /**< 18 AMT630HV100 GPIO96~127 Controller Interrupt (GPIOD) */
  TIMER0_IRQn		  = 19, /**< 19 AMT630HV100 Timer 0 Interrupt (TIMER0) */
  TIMER1_IRQn		  = 20, /**< 20 AMT630HV100 Timer 1 Interrupt (TIMER1) */
  TIMER2_IRQn		  = 21, /**< 21 AMT630HV100 Timer 2/3 Interrupt (TIMER2/TIMER3) */
  I2S1_IRQn			  = 22, /**< 22 AMT630HV100 I2S1 Interrupt (I2S1) */
  ITU_IRQn			  = 23, /**< 23 AMT630HV100 ITU Controller Interrupt (ITU) */
  WDT_IRQn			  = 24, /**< 24 AMT630HV100 Watchdog timer Interrupt (WDT) */
  I2S_IRQn		      = 25, /**< 25 AMT630HV100 I2S Interrupt (I2S) */
  BLEND2D_IRQn		  = 26, /**< 26 AMT630HV100 BLEND2D Interrupt (BLEND2D) */
  RTC_PRD_IRQn		  = 27, /**< 27 AMT630HV100 RTC Controller Period Interrupt (RTCP) */
  ADC_IRQn			  = 28, /**< 28 AMT630HV100 ADC controller Interrupt (ADC) */
  RCRT_IRQn			  = 29, /**< 29 */
  CAN0_IRQn			  = 30, /**< 30 AMT630HV100 CAN0 Controller Interrupt (CAN0) */
  CAN1_IRQn			  = 31, /**< 31 AMT630HV100 CAN1 Controller Interrupt (CAN1) */
  MAX_IRQ_NUM		  = 32  /**< Number of peripheral IDs */
} IRQn_Type;

/*@}*/

/* ************************************************************************** */
/*   BASE ADDRESS DEFINITIONS FOR AMT630HV100 */
/* ************************************************************************** */
/** \addtogroup AMT630HV100_base Peripheral Base Address Definitions */
/*@{*/

#define REGS_SYSCTL_BASE	(0x60000000U)
#define REGS_SPI0_BASE		(0x60100000U)
#define REGS_SPI1_BASE		(0x60200000U)
#define REGS_IIC0_BASE		(0x60300000U)
#define REGS_IIC1_BASE		(0x60400000U)
#define REGS_UART0_BASE		(0x60500000U)
#define REGS_UART1_BASE		(0x60600000U)
#define REGS_UART2_BASE		(0x60700000U)
#define REGS_UART3_BASE		(0x60800000U)
#define REGS_GPIO_BASE		(0x60900000U)
#define REGS_TIMER_BASE		(0x60a00000U)
#define REGS_PWM_BASE		(0x60b00000U)
#define REGS_WDT_BASE		(0x60c00000U)
#define REGS_I2S_BASE		(0x60d00000U)
#define REGS_I2S1_BASE		(0x60f00000U)
#define REGS_MIPI_BASE		(0x60e00000U)
#define REGS_RTC_BASE		(0x61000000U)
#define REGS_ADC_BASE		(0x61100000U)
#define REGS_RCRT_BASE		(0x61200000U)
#define REGS_AES_BASE		(0x61300000U)
#define REGS_AIC_BASE		(0x61400000U)
#define REGS_CAN0_BASE		(0x61500000U)
#define REGS_CAN1_BASE		(0x61600000U)

#define REGS_DMAC_BASE		(0x70100000U)
#define REGS_GPU_BASE		(0x70200000U)
#define REGS_USB_BASE		(0x70300000U)
#define REGS_SDMMC0_BASE	(0X70400000U)
#define REGS_ITU_BASE		(0X70600000U)
#define REGS_BLEND2D_BASE	(0x70700000U)
#define REGS_LCD_BASE		(0X71000000U)
#define REGS_PXP_BASE		(0X71100000U)
#define REGS_ROTATE_BASE	(0X71207000U)
#define REGS_JPG_BASE		(0X71208000U)
#define REGS_DDRC_BASE		(0X71300000U)


/* ************************************************************************** */
/*   ELECTRICAL DEFINITIONS FOR AMT630HV100 */
/* ************************************************************************** */

#ifdef __cplusplus
}
#endif

/*@}*/

#endif /* __AMT630HV100__H */

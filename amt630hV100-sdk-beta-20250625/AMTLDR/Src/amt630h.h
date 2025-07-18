/***********************************************************************
Copyright (c)2020 Arkmicro Technologies Inc.  All Rights Reserved
Filename: amt630h.h
Version : 1.0
Date    : 2020.04.08
Author  : Sim. Huang
History :
************************************************************************/

#ifndef _AMT630H_H_
#define _AMT630H_H_

#define VECTOR_ENABLE									0	//if you want to change this value, please
															//change the Assemble logic valuable VECTOR_ENABLE														//in file Boot.s
#define CLK_24MHZ	24000000
#define CLK_12MHZ	12000000


#define SPINOR_BOOTER_SD			    0
#define BOOT_FROM_USB					1
#define SPINAND_BOOTER_SD				2		
#define BOOT_FROM_SPI					3

#define SPI_NOR_FLASH					0
#define SPI_NAND_FLASH					1
#define EMMC_FLASH						2
#define DEVICE_TYPE_SELECT				SPI_NOR_FLASH

/* register base address */

#define SDHC0_BASE										0x70400000
#define USB_BASE										0x70300000//0x700C0000
#define SSI1_BASE										0x60200000//0x48002000
#define SSI0_BASE										0x60100000
#define GPIO_BASE										0x60900000//0x40409000
#define SYS_BASE										0x60000000//0x40408000
#define TIMER_BASE										0x60a00000//0x40405000
#define WDT_BASE										0x60c00000//0x40404000
#define RTC_BASE										0x61000000//0x40406000
#define UART0_BASE										0x60500000//0x4040B000

//uart
#define	UART_BASE		UART0_BASE
#define rUART_DR		*((volatile unsigned int *)(UART_BASE + 0x00))
#define rUART_RSR		*((volatile unsigned int *)(UART_BASE + 0x04))
#define rUART_FR		*((volatile unsigned int *)(UART_BASE + 0x18))
#define rUART_ILPR		*((volatile unsigned int *)(UART_BASE + 0x20))
#define rUART_IBRD		*((volatile unsigned int *)(UART_BASE + 0x24))
#define rUART_FBRD		*((volatile unsigned int *)(UART_BASE + 0x28))
#define rUART_LCR_H		*((volatile unsigned int *)(UART_BASE + 0x2C))
#define rUART_CR		*((volatile unsigned int *)(UART_BASE + 0x30))
#define rUART_IFLS		*((volatile unsigned int *)(UART_BASE + 0x34))
#define rUART_IMSC		*((volatile unsigned int *)(UART_BASE + 0x38))
#define rUART_RIS		*((volatile unsigned int *)(UART_BASE + 0x3C))
#define rUART_MIS		*((volatile unsigned int *)(UART_BASE + 0x40))
#define rUART_ICR		*((volatile unsigned int *)(UART_BASE + 0x44))
#define rUART_DMACR		*((volatile unsigned int *)(UART_BASE + 0x48))


/***************************************************************
      AHB slave interface registers definition
****************************************************************/
/* AHB system */
#define rSYS_BOOT_SAMPLE						*((volatile unsigned int *)(SYS_BASE+0x0))
#define rSYS_BUS_CLK_SEL						*((volatile unsigned int *)(SYS_BASE+0x40))
#define rSYS_PLLRFCK_CTL						*((volatile unsigned int *)(SYS_BASE+0x44))
#define rSYS_SDMMC_CLK_CFG						*((volatile unsigned int *)(SYS_BASE+0x48))
#define rSYS_VOU_CLK_CFG						*((volatile unsigned int *)(SYS_BASE+0x4c))
#define rSYS_PER_CLK_EN							*((volatile unsigned int *)(SYS_BASE+0x50))
#define rSYS_LCD_CLK_CFG						*((volatile unsigned int *)(SYS_BASE+0x54))
#define rSYS_SD_CLK_CFG							*((volatile unsigned int *)(SYS_BASE+0x58))
#define rSYS_SOFT_RST							*((volatile unsigned int *)(SYS_BASE+0x5c))
#define rSYS_SOFT1_RST							*((volatile unsigned int *)(SYS_BASE+0x60))
#define rSYS_SSP_CLK_CFG						*((volatile unsigned int *)(SYS_BASE+0x64))
#define rSYS_TIMER_CLK_CFG						*((volatile unsigned int *)(SYS_BASE+0x68))
#define rSYS_I2S_NCO_CFG						*((volatile unsigned int *)(SYS_BASE+0x6c))

#define rSYS_BUS_CLK1_SEL						*((volatile unsigned int *)(SYS_BASE+0x140))
#define rSYS_PER_CLK1_EN					    *((volatile unsigned int *)(SYS_BASE+0x144))
#define rSYS_I2S1_NCO_CFG						*((volatile unsigned int *)(SYS_BASE+0x148))

#define rSYS_DDRCTL_CFG							*((volatile unsigned int *)(SYS_BASE+0x70))
#define rSYS_DDRCTL1_CFG						*((volatile unsigned int *)(SYS_BASE+0x74))
#define rSYS_DDRCTL2_CFG						*((volatile unsigned int *)(SYS_BASE+0xb0))
#define rSYS_DDRCTL3_CFG						*((volatile unsigned int *)(SYS_BASE+0xb4))
#define rSYS_PERCTL_CFG							*((volatile unsigned int *)(SYS_BASE+0x78))
#define rSYS_TIMER1_CLK_CFG						*((volatile unsigned int *)(SYS_BASE+0x7c))
#define rSYS_ANA_CFG							*((volatile unsigned int *)(SYS_BASE+0x80))
#define rSYS_ANA1_CFG 							*((volatile unsigned int *)(SYS_BASE+0x84))
#define rSYS_CPUPLL_CFG							*((volatile unsigned int *)(SYS_BASE+0x88))
#define rSYS_SYSPLL_CFG							*((volatile unsigned int *)(SYS_BASE+0x8c))
#define rSYS_DDRPLL_CFG							*((volatile unsigned int *)(SYS_BASE+0x90))
#define rSYS_VPUPLL_CFG							*((volatile unsigned int *)(SYS_BASE+0x94))
#define rSYS_ANA2_CFG							*((volatile unsigned int *)(SYS_BASE+0x98))
#define rSYS_ANA3_CFG							*((volatile unsigned int *)(SYS_BASE+0x9c))
#define rSYS_ANA4_CFG							*((volatile unsigned int *)(SYS_BASE+0xa0))



#define rSYS_PAD_CTRL00							*((volatile unsigned int *)(SYS_BASE+0x30*4))
#define rSYS_PAD_CTRL01							*((volatile unsigned int *)(SYS_BASE+0x31*4))
#define rSYS_PAD_CTRL02							*((volatile unsigned int *)(SYS_BASE+0x32*4))
#define rSYS_PAD_CTRL03							*((volatile unsigned int *)(SYS_BASE+0x33*4))
#define rSYS_PAD_CTRL04							*((volatile unsigned int *)(SYS_BASE+0x34*4))
#define rSYS_PAD_CTRL05							*((volatile unsigned int *)(SYS_BASE+0xD4))
#define rSYS_PAD_CTRL06							*((volatile unsigned int *)(SYS_BASE+0x36*4))
#define rSYS_PAD_CTRL07							*((volatile unsigned int *)(SYS_BASE+0x37*4))
#define rSYS_PAD_CTRL08							*((volatile unsigned int *)(SYS_BASE+0x120))



#define rSYS_IO_DRIVER00						*((volatile unsigned int *)(SYS_BASE+0x38*4))
#define rSYS_IO_DRIVER01						*((volatile unsigned int *)(SYS_BASE+0x39*4))
#define rSYS_IO_DRIVER02						*((volatile unsigned int *)(SYS_BASE+0x3A*4))
#define rSYS_IO_DRIVER03                        *((volatile unsigned int *)(SYS_BASE+0x3B*4))
#define rSYS_IO_DRIVER04                        *((volatile unsigned int *)(SYS_BASE+0x3C*4))
#define rSYS_IO_DRIVER05                        *((volatile unsigned int *)(SYS_BASE+0x3D*4))
#define rSYS_IO_DRIVER06                        *((volatile unsigned int *)(SYS_BASE+0x3E*4))
#define rSYS_IO_DRIVER07                        *((volatile unsigned int *)(SYS_BASE+0x3F*4))

#define rSYS_IO_UP00						    *((volatile unsigned int *)(SYS_BASE+0x40*4))
#define rSYS_IO_UP01						    *((volatile unsigned int *)(SYS_BASE+0x41*4))
#define rSYS_IO_UP02						    *((volatile unsigned int *)(SYS_BASE+0x42*4))
#define rSYS_IO_UP03                            *((volatile unsigned int *)(SYS_BASE+0x43*4))
#define rSYS_IO_UP04                            *((volatile unsigned int *)(SYS_BASE+0x44*4))
#define rSYS_IO_UP05                            *((volatile unsigned int *)(SYS_BASE+0x45*4))
#define rSYS_IO_UP06                            *((volatile unsigned int *)(SYS_BASE+0x46*4))
#define rSYS_IO_UP07                            *((volatile unsigned int *)(SYS_BASE+0x47*4))

/* Timer */
#define rTIMER0_LOAD_COUNT						(*(volatile unsigned int *)(TIMER_BASE + 0x00))
#define rTIMER0_CURRENT_VALUE					(*(volatile unsigned int *)(TIMER_BASE + 0x04))
#define rTIMER0_CONTROL							(*(volatile unsigned int *)(TIMER_BASE + 0x08))
#define rTIMER0_EOI								(*(volatile unsigned int *)(TIMER_BASE + 0x0C))
#define rTIMER0_INT_STATUS						(*(volatile unsigned int *)(TIMER_BASE + 0x10))

/* WDT */
#define rWDT_CR									(*(volatile unsigned int *)(WDT_BASE + 0x00))
#define rWDT_PSR								(*(volatile unsigned int *)(WDT_BASE + 0x04))
#define rWDT_LDR								(*(volatile unsigned int *)(WDT_BASE + 0x08))
#define rWDT_VLR								(*(volatile unsigned int *)(WDT_BASE + 0x0C))
#define rWDT_ISR								(*(volatile unsigned int *)(WDT_BASE + 0x10))
#define rWDT_RCR								(*(volatile unsigned int *)(WDT_BASE + 0x14))
#define rWDT_TMR								(*(volatile unsigned int *)(WDT_BASE + 0x18))
#define rWDT_TCR								(*(volatile unsigned int *)(WDT_BASE + 0x1C))

/* RTC */
#define rRTC_CTL	   							(*(volatile unsigned int *)(RTC_BASE + 0x00)) /*control register*/
#define rRTC_ANAWEN	   							(*(volatile unsigned int *)(RTC_BASE + 0x04)) /*analog block write enable register*/
#define rRTC_ANACTL	   							(*(volatile unsigned int *)(RTC_BASE + 0x08)) /*analog block control register*/
#define rRTC_IM	   								(*(volatile unsigned int *)(RTC_BASE + 0x0C)) /*interrupt mode register*/
#define rRTC_STA	   							(*(volatile unsigned int *)(RTC_BASE + 0x10)) /*rtc status register*/
#define rRTC_ALMDAT	   							(*(volatile unsigned int *)(RTC_BASE + 0x14)) /*alarm data register*/
#define rRTC_DONT	   							(*(volatile unsigned int *)(RTC_BASE + 0x18)) /*delay on timer register*/
#define rRTC_RAM	    						(*(volatile unsigned int *)(RTC_BASE + 0x1C)) /*ram bit register*/
#define rRTC_CNTL     							(*(volatile unsigned int *)(RTC_BASE + 0x20)) /*rtc counter register*/
#define rRTC_CNTH 	  							(*(volatile unsigned int *)(RTC_BASE + 0x24)) /*rtc sec counter register*/

/* UART0 */
#define rUART0_DR								(*(volatile unsigned int *)(UART0_BASE + 0x00))
#define rUART0_RSR								(*(volatile unsigned int *)(UART0_BASE + 0x04))
#define rUART0_FR								(*(volatile unsigned int *)(UART0_BASE + 0x18))
#define rUART0_ILPR								(*(volatile unsigned int *)(UART0_BASE + 0x20))
#define rUART0_IBRD								(*(volatile unsigned int *)(UART0_BASE + 0x24))
#define rUART0_FBRD								(*(volatile unsigned int *)(UART0_BASE + 0x28))
#define rUART0_LCR_H							(*(volatile unsigned int *)(UART0_BASE + 0x2C))
#define rUART0_CR								(*(volatile unsigned int *)(UART0_BASE + 0x30))
#define rUART0_IFLS								(*(volatile unsigned int *)(UART0_BASE + 0x34))
#define rUART0_IMSC								(*(volatile unsigned int *)(UART0_BASE + 0x38))
#define rUART0_RIS								(*(volatile unsigned int *)(UART0_BASE + 0x3C))
#define rUART0_MIS								(*(volatile unsigned int *)(UART0_BASE + 0x40))
#define rUART0_ICR								(*(volatile unsigned int *)(UART0_BASE + 0x44))
#define rUART0_DMACR							(*(volatile unsigned int *)(UART0_BASE + 0x48))


/* SSI */
#define rSPI_CONTROLREG             			(*(volatile unsigned int *)(SSI1_BASE + 0x08))
#define rSPI_CONFIGREG              			(*(volatile unsigned int *)(SSI1_BASE + 0x0C))
#define rSPI_INTREG                 			(*(volatile unsigned int *)(SSI1_BASE + 0x10))
#define rSPI_DMAREG                 			(*(volatile unsigned int *)(SSI1_BASE + 0x14))
#define rSPI_STATUSREG              			(*(volatile unsigned int *)(SSI1_BASE + 0x18))
#define rSPI_PERIODREG              			(*(volatile unsigned int *)(SSI1_BASE + 0x1C))
#define rSPI_TESTREG                			(*(volatile unsigned int *)(SSI1_BASE + 0x20))
#define rSPI_MSGREG                 			(*(volatile unsigned int *)(SSI1_BASE + 0x40))
#define rSPI_RXDATA                 			(*(volatile unsigned int *)(SSI1_BASE + 0x50))
#define rSPI_TXDATA                 			(*(volatile unsigned int *)(SSI1_BASE + 0x460))
#define rSPI_TXFIFO                   			(SSI_BASE + 0x460)
#define rSPI_RXFIFO                   			(SSI_BASE + 0x50) 

/* SSI0 */
#define rSPI_CTLR0             					(*(volatile unsigned int *)(SSI0_BASE + 0x00))
#define rSPI_CTLR1             					(*(volatile unsigned int *)(SSI0_BASE + 0x04))
#define rSPI_SSIENR             				(*(volatile unsigned int *)(SSI0_BASE + 0x08))
#define rSPI_MWCR             					(*(volatile unsigned int *)(SSI0_BASE + 0x0c))
#define rSPI_SER             					(*(volatile unsigned int *)(SSI0_BASE + 0x10))
#define rSPI_BAUDR             					(*(volatile unsigned int *)(SSI0_BASE + 0x14))
#define rSPI_TXFTLR             				(*(volatile unsigned int *)(SSI0_BASE + 0x18))
#define rSPI_RXFTLR             				(*(volatile unsigned int *)(SSI0_BASE + 0x1C))
#define rSPI_TXFLR            					(*(volatile unsigned int *)(SSI0_BASE + 0x20))
#define rSPI_RXFLR             					(*(volatile unsigned int *)(SSI0_BASE + 0x24))
#define rSPI_SR             					(*(volatile unsigned int *)(SSI0_BASE + 0x28))
#define rSPI_IMR             					(*(volatile unsigned int *)(SSI0_BASE + 0x2C))
#define rSPI_ISR								(*(volatile unsigned int *)(SSI0_BASE + 0x30))
#define rSPI_RISR            					(*(volatile unsigned int *)(SSI0_BASE + 0x34))
#define rSPI_TXOICR             				(*(volatile unsigned int *)(SSI0_BASE + 0x38))
#define rSPI_RXOICR            					(*(volatile unsigned int *)(SSI0_BASE + 0x3C))
#define rSPI_RXUICR								(*(volatile unsigned int *)(SSI0_BASE + 0x40))
#define rSPI_MSTICR            					(*(volatile unsigned int *)(SSI0_BASE + 0x44))
#define rSPI_ICR             					(*(volatile unsigned int *)(SSI0_BASE + 0x48))
#define rSPI_DMACR            					(*(volatile unsigned int *)(SSI0_BASE + 0x4C))
#define rSPI_DMATDLR							(*(volatile unsigned int *)(SSI0_BASE + 0x50))
#define rSPI_DMARDLR           					(*(volatile unsigned int *)(SSI0_BASE + 0x54))
#define rSPI_IDR             					(*(volatile unsigned int *)(SSI0_BASE + 0x58))
#define rSPI_SSI_COMP_VERSION          	 		(*(volatile unsigned int *)(SSI0_BASE + 0x5C))
#define rSPI_DR									(*(volatile unsigned int *)(SSI0_BASE + 0x60))
#define SPI_DR	                        		(SSI0_BASE + 0x60)

#define rSPI_RX_SAMPLE_DLY						(*(volatile unsigned int *)(SSI0_BASE + 0xf0))
#define rSPI_SPI_CTRLR0           				(*(volatile unsigned int *)(SSI0_BASE + 0xf4))
//#define rSPI_RSVD_1            				(*(volatile unsigned int *)(SSI0_BASE + 0xf8))
//#define rSPI_RSVD_1           				(*(volatile unsigned int *)(SSI0_BASE + 0xfC))



/* GPIO */
#define rGPIO_PA_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x00))
#define rGPIO_PA_RDATA						    (*(volatile unsigned int *)(GPIO_BASE + 0x04))
#define rGPIO_PA_INTEN							(*(volatile unsigned int *)(GPIO_BASE + 0x08))
#define rGPIO_PA_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x0C))
#define rGPIO_PA_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x10))

#define rGPIO_PB_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x20))
#define rGPIO_PB_RDATA							(*(volatile unsigned int *)(GPIO_BASE + 0x24))
#define rGPIO_PB_INTEN							(*(volatile unsigned int *)(GPIO_BASE + 0x28))
#define rGPIO_PB_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x2C))
#define rGPIO_PB_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x30))

#define rGPIO_PC_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x40))
#define rGPIO_PC_RDATA							(*(volatile unsigned int *)(GPIO_BASE + 0x44))
#define rGPIO_PC_INTEN							(*(volatile unsigned int *)(GPIO_BASE + 0x48))
#define rGPIO_PC_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x4C))
#define rGPIO_PC_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x50))

#define rGPIO_PD_MOD							(*(volatile unsigned int *)(GPIO_BASE + 0x60))
#define rGPIO_PD_RDATA							(*(volatile unsigned int *)(GPIO_BASE + 0x64))
#define rGPIO_PD_INTEN							(*(volatile unsigned int *)(GPIO_BASE + 0x68))
#define rGPIO_PD_LEVEL							(*(volatile unsigned int *)(GPIO_BASE + 0x6C))
#define rGPIO_PD_PEND							(*(volatile unsigned int *)(GPIO_BASE + 0x70))


//DDR Reg


#define DDR2_BASE  0x71300000

#define MEM_STA_REG        (*(volatile unsigned int *)(DDR2_BASE  + 0x00))
#define MEM_CMD_REG        (*(volatile unsigned int *)(DDR2_BASE  + 0x04))
#define DIR_CMD_REG        (*(volatile unsigned int *)(DDR2_BASE  + 0x08))
#define MEM_CFG_REG        (*(volatile unsigned int *)(DDR2_BASE  + 0x0C))
#define REF_PRD_REG        (*(volatile unsigned int *)(DDR2_BASE  + 0x10))
#define TCAS_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x14))
#define TWL_REG            (*(volatile unsigned int *)(DDR2_BASE  + 0x18))
#define TMRD_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x1C))
#define TRAS_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x20))
#define TRC_REG            (*(volatile unsigned int *)(DDR2_BASE  + 0x24))
#define TRCD_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x28))
#define TRFC_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x2C))
#define TRP_REG            (*(volatile unsigned int *)(DDR2_BASE  + 0x30))
#define TRRD_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x34))
#define TWR_REG            (*(volatile unsigned int *)(DDR2_BASE  + 0x38))
#define TWTR_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x3C))
#define TXP_REG            (*(volatile unsigned int *)(DDR2_BASE  + 0x40))
#define TXSR_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x44))
#define TESR_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x48))
#define MEM_CFG2_REG       (*(volatile unsigned int *)(DDR2_BASE  + 0x4C))
#define MEM_CFG3_REG       (*(volatile unsigned int *)(DDR2_BASE  + 0x50))
#define TFAW_REG           (*(volatile unsigned int *)(DDR2_BASE  + 0x54))
#define READ_DELAY_REG     (*(volatile unsigned int *)(DDR2_BASE  + 0x58))
#define CHIP_CFG_REG       (*(volatile unsigned int *)(DDR2_BASE  + 0x200)) 
#define FEA_CTL_REG        (*(volatile unsigned int *)(DDR2_BASE  + 0x30C))  


//#define MMU_ENABLE

#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
#define LOADER_OFFSET									0x0
#define LOADER_MAX_SIZE									0x4000

#define STEPLDRA_OFFSET									0x20000
#define STEPLDRB_OFFSET									0x40000
#define STEPLDR_MAX_SIZE								0x14000

#define SYSINFOA_OFFSET									0x60000
#define SYSINFOB_OFFSET									0x80000
#define SYSINFO_MAX_SIZE								0x1000

#define IMAGE_OFFSET									0xa0000
#define IMAGE_READ_SIZE									0x20000

#define LOADER_FILE_NAME								"spildr.bin"
#define STEPLDR_FILE_NAME								"stepldr.bin"
#define APP_FILE_NAME									"update.bin"

#elif DEVICE_TYPE_SELECT == SPI_NOR_FLASH
#define LOADER_OFFSET									0x0
#define LOADER_MAX_SIZE									0x4000

#define STEPLDRA_OFFSET									0x4000
#define STEPLDRB_OFFSET									0x20000
#define STEPLDR_MAX_SIZE								0x14000

#define SYSINFOA_OFFSET									0x3c000
#define SYSINFOB_OFFSET									0x3e000
#define SYSINFO_MAX_SIZE								0x1000

#define IMAGE_OFFSET									0x40000
#define IMAGE_MAX_SIZE									0xf00000

#define LOADER_FILE_NAME								"spildr.bin"
#define STEPLDR_FILE_NAME								"stepldr.bin"
#define APP_FILE_NAME									"update.bin"

#elif DEVICE_TYPE_SELECT == EMMC_FLASH
/**********************************************************************************************
emmc·ÖÇøÇé¿ö
(0-------0x200000-------0x400000------0x500000---0xA00000-----0x40000000-----0x80000000-----end
|------------|--------------|--------------|---------|-------------|----------- --|-----------|
|            |              |              |         |             |              |           |
|  EMMCLDR   |   stepldr    |   SYSINFO    |    log  |     app1    |      app2    |    ota    |
|____________|______________|______________|_________|_____________|______________|___________|

**********************************************************************************************/
#define LOADER_OFFSET									0x0
#define LOADERB_OFFSET									0x100000
#define LOADER_MAX_SIZE									0x4000

#define STEPLDRA_OFFSET									0x200000
#define STEPLDRB_OFFSET									0x300000
#define STEPLDR_MAX_SIZE								0x14000

#define SYSINFOA_OFFSET									0x400000
#define SYSINFOB_OFFSET									0x500000
#define SYSINFO_MAX_SIZE								0x1000

#define IMAGE_OFFSET									0xA00000
#define IMAGEB_OFFSET									0x40000000
#define IMAGE_MAX_SIZE									0xf00000
#define IMAGE_READ_SIZE									0x80000

#define LOADER_FILE_NAME								"emmcldr.bin"
#define STEPLDR_FILE_NAME								"stepldr.bin"
#define APP_FILE_NAME									"update.bin"

#endif


#define IMAGE_ENTRY										0x30c000
#define STEPLDR_ENTRY									0x20f00000
#define IMAGE_FLAG 										0x424b5244
#define CheckImageValid									(*((volatile unsigned int *)(IMAGE_ENTRY+4)) == IMAGE_FLAG)

typedef struct {
	unsigned int magic;
	unsigned int offset;
	unsigned int size;
} UpFileInfo;

typedef struct {
	unsigned int magic;
	unsigned int filenum;
	unsigned int size;
	unsigned int checksum;
	unsigned int reserved1;
	unsigned int reserved2;
	UpFileInfo files[];
} UpFileHeader;

#define ENOENT      2  /* No such file or directory */
#define EIO      	5  /* I/O error */
#define ENXIO        6  /* No such device or address */
#define ENOMEM      12  /* Out of memory */
#define ENODEV      19  /* No such device */
#define EINVAL      22  /* Invalid argument */
#define EFBIG       27  /* File too large */
#define ENOSPC      28  /* No space left on device */
#define ENOTCONN    107 /* Transport endpoint is not connected */


#define min(x,y) ((x) < (y) ? x : y)
#define max(x,y) ((x) > (y) ? x : y)


#define min3(x, y, z) min(min(x, y), z)
#define max3(x, y, z) max(max(x, y), z)

#define ROUND(a,b)      (((a) + (b) - 1) & ~((b) - 1))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define ALIGN(x,a)      __ALIGN_MASK((x),(uintptr_t)(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


#define ARCH_DMA_MINALIGN	32//64
#define USB_DMA_MINALIGN	ARCH_DMA_MINALIGN
//typedef unsigned long       uintptr_t;

#define PAD_COUNT(s, pad) (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad) (PAD_COUNT(s, pad) * pad)
#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)		\
	char __##name[ROUND(PAD_SIZE((size) * sizeof(type), pad), align)  \
		      + (align - 1)];					\
									\
	type *name = (type *)ALIGN((uintptr_t)__##name, align)
#define ALLOC_ALIGN_BUFFER(type, name, size, align)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)			\
	ALLOC_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)


static inline int ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}
/*
static void writeb(unsigned char val, void *ptr)
{
	*(volatile unsigned char*)ptr = val;
}

static unsigned char readb(void *ptr)
{
	return *(volatile unsigned char*)ptr;
}

static void writew(unsigned short val, void *ptr)
{
	*(volatile unsigned short*)ptr = val;
}

static unsigned short readw(void *ptr)
{
	return *(volatile unsigned short*)ptr;
}
*/
#define reg32_read(addr)      *((volatile unsigned int *)(addr))
#define reg32_write(addr,val) *((volatile unsigned int *)(addr)) = (val)
#define readl(a)        reg32_read(a)
#define writel(v, a)    reg32_write(a, v)

#endif // _AMT630H_H_

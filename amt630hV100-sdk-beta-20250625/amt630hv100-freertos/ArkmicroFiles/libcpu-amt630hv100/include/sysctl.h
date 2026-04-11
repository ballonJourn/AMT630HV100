#ifndef _SYSCTL_H
#define _SYSCTL_H

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_BOOT_SAMPLE							0x0
#define SYS_BUS_CLK_CFG							0x40
#define SYS_BUS_CLK1_CFG						0x140
#define SYS_PER_CLK_CFG							0x44
#define SYS_SDMMC_CLK_CFG						0x48
#define SYS_VOU_CLK_CFG							0x4c
#define SYS_BUS_CLK_EN							0x50
#define SYS_BUS1_CLK_EN							0x54
#define SYS_PER_CLK_EN							0x58
#define SYS_SOFT_RST							0x5c
#define SYS_SOFT1_RST							0x60
#define SYS_SSP_CLK_CFG							0x64
#define SYS_TIMER_CLK_CFG						0x68 
#define SYS_I2S_NCO_CFG							0x6c 
#define SYS_DDRCTL_CFG							0x70
#define SYS_PERCTL_CFG							0x78
#define SYS_TIMER1_CLK_CFG						0x7c
#define SYS_ANA_CFG								0x80
#define SYS_ANA1_CFG							0x84
#define SYS_CPUPLL_CFG							0x88
#define SYS_SYSPLL_CFG							0x8c
#define SYS_ANA2_CFG							0x98
#define SYS_ANA3_CFG							0x9c
#define SYS_ANA4_CFG							0xa0
#define SYS_ANA5_CFG							0xa4
#define SYS_ANA6_CFG							0xa8

#define SYS_PAD_CTRL00							0xc0
#define SYS_PAD_CTRL01							0xc4
#define SYS_PAD_CTRL02							0xc8
#define SYS_PAD_CTRL03							0xcc
#define SYS_PAD_CTRL04							0xd0
#define SYS_PAD_CTRL05							0xd4
#define SYS_PAD_CTRL06							0xd8
#define SYS_PAD_CTRL07							0xdc
#define SYS_PAD_CTRL08							0x120


#define SYS_IO_DRIVER00							0xe0
#define SYS_IO_DRIVER01							0xe4
#define SYS_IO_DRIVER02							0xe8
#define SYS_IO_DRIVER03							0xec
#define SYS_IO_DRIVER04							0xf0
#define SYS_IO_DRIVER05							0xf4
#define SYS_IO_DRIVER06							0xf8
#define SYS_IO_DRIVER07							0xfc

enum sys_soft_reset{
	//sys_soft0_reset
	softreset_lcd=0,
	softreset_dma,
	softreset_jpeg,
	softreset_usb,
	softreset_card,
	softreset_itu,
	softreset_gpu,
	softreset_pxp,
	softreset_ssp,
	softreset_ssp1,
	softreset_i2c,
	softreset_i2c1,
	softreset_uart0,
	softreset_uart1,
	softreset_uart2,
	softreset_uart3,
	softreset_gpio,
	softreset_timer0,
	softreset_timer1,
	softreset_timer2,
	softreset_timer3,
	softreset_pwm,
	softreset_wdt,
	softreset_i2s,
	softreset_rtc,
	softreset_adc,
	softreset_rcrt,
	softreset_aes,
	softreset_icu,
	softreset_ddr,
	softreset_usbphy,
	softreset_imc,		//31
	//sys_soft1_reset
	softreset_can0,		//0,
	softreset_can1,		//1
	softreset_h2xdma,	//2
	softreset_h2xusb,	//3
	softreset_mipi,		//4
	softreset_usb_utmi,	//5
	softreset_vpu,		//6,
	softreset_i2s1,		//8,
};


extern void vSysctlConfigure(uint32_t regoffset, uint32_t bitoffset, uint32_t mask, uint32_t val);

extern void sys_soft_reset (int reset_dev);

extern void sys_soft_reset_from_isr (int reset_dev);

#ifdef __cplusplus
}
#endif

#endif


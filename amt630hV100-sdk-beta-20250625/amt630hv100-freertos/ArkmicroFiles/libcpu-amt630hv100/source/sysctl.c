#include "chip.h"

void vSysctlConfigure(uint32_t regoffset, uint32_t bitoffset, uint32_t mask, uint32_t val)
{
	uint32_t tmp = readl(REGS_SYSCTL_BASE + regoffset);

	tmp &= ~(mask << bitoffset);
	tmp |= val << bitoffset;
	writel(tmp, REGS_SYSCTL_BASE + regoffset);
}

void sys_soft_reset (int reset_dev)
{
	unsigned int mask;
	volatile unsigned int* reg;
	switch (reset_dev)
	{
		case softreset_imc:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 31;
			break;
		case softreset_usbphy:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 30;
			break;
		case softreset_ddr:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 29;
			break;
		case softreset_icu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 28;
			break;
		case softreset_aes:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 27;
			break;
		case softreset_rcrt:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 26;
			break;
		case softreset_adc:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 25;
			break;
		case softreset_rtc:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 24;
			break;
		case softreset_i2s:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 23;
			break;
		case softreset_wdt:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 22;
			break;
		case softreset_pwm:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 21;
			break;
		case softreset_timer3:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 20;
			break;
		case softreset_timer2:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 19;
			break;
		case softreset_timer1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 18;
			break;
		case softreset_timer0:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 17;
			break;
		case softreset_gpio:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 16;
			break;
		case softreset_uart3:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 15;
			break;
		case softreset_uart2:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 14;
			break;
		case softreset_uart1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 13;
			break;
		case softreset_uart0:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 12;
			break;
		case softreset_i2c1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 11;
			break;
	   case softreset_i2c:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 10;
			break;
		case softreset_ssp1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 9;
			break;
		case softreset_ssp:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 8;
			break;
		case softreset_pxp:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 7;
			break;
		case softreset_gpu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 6;
			break;
		case softreset_itu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 5;
			break;
		case softreset_card:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 4;
			break;
		case softreset_usb:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 3;
			break;
		case softreset_jpeg:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 2;
			break;
		case softreset_dma:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 1;
			break;
		case softreset_lcd:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 0;
			break;
		case softreset_can0:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 0;
			break;
		case softreset_can1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 1;
			break;
		case softreset_h2xdma:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 2;
			break;
		case softreset_h2xusb:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 3;
			break;
		case softreset_mipi:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 4;
			break;
		case softreset_usb_utmi:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 5;
			break;
		case softreset_vpu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 6;
			break;
		case softreset_i2s1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 8;
			break;

		default:
			printf ("illegal reset device (%d)\n", reset_dev);
			return;
	}

	portENTER_CRITICAL();
	*reg &= ~(1 << mask);
	udelay (100);
	*reg |=  (1 << mask);
	portEXIT_CRITICAL();
}

void sys_soft_reset_from_isr (int reset_dev)
{
	unsigned int mask;
	volatile unsigned int* reg;
	switch (reset_dev)
	{
		case softreset_imc:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 31;
			break;
		case softreset_usbphy:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 30;
			break;
		case softreset_ddr:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 29;
			break;
		case softreset_icu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 28;
			break;
		case softreset_aes:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 27;
			break;
		case softreset_rcrt:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 26;
			break;
		case softreset_adc:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 25;
			break;
		case softreset_rtc:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 24;
			break;
		case softreset_i2s:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 23;
			break;
		case softreset_wdt:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 22;
			break;
		case softreset_pwm:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 21;
			break;
		case softreset_timer3:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 20;
			break;
		case softreset_timer2:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 19;
			break;
		case softreset_timer1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 18;
			break;
		case softreset_timer0:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 17;
			break;
		case softreset_gpio:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 16;
			break;
		case softreset_uart3:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 15;
			break;
		case softreset_uart2:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 14;
			break;
		case softreset_uart1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 13;
			break;
		case softreset_uart0:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 12;
			break;
		case softreset_i2c1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 11;
			break;
	   case softreset_i2c:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 10;
			break;
		case softreset_ssp1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 9;
			break;
		case softreset_ssp:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 8;
			break;
		case softreset_pxp:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 7;
			break;
		case softreset_gpu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 6;
			break;
		case softreset_itu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 5;
			break;
		case softreset_card:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 4;
			break;
		case softreset_usb:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 3;
			break;
		case softreset_jpeg:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 2;
			break;
		case softreset_dma:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 1;
			break;
		case softreset_lcd:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x5c));
			mask = 0;
			break;
		case softreset_can0:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 0;
			break;
		case softreset_can1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 1;
			break;
		case softreset_h2xdma:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 2;
			break;
		case softreset_h2xusb:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 3;
			break;
		case softreset_mipi:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 4;
			break;
		case softreset_usb_utmi:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 5;
			break;
		case softreset_vpu:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 6;
			break;
		case softreset_i2s1:
			reg = ((volatile unsigned int *)(REGS_SYSCTL_BASE+0x60));
			mask = 8;
			break;
		default:
			printf ("illegal reset device (%d)\n", reset_dev);
			return;
	}

	*reg &= ~(1 << mask);
	udelay (100);
	*reg |=  (1 << mask);
}


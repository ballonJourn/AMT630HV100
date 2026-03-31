#include <stdio.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "errno.h"

#define WTCON		0x00
#define WTPSR		0x04
#define WTCNT		0x08
#define WTCLRINT	0x10
#define WTRCR		0x14

#define WTCNT_MAXCNT	0xffff
#define WTCON_MAXDIV	0x80

#define WTCON_ENABLE	(1 << 0)
#define WTCON_RSTEN		(1 << 1)
#define WTCON_INTEN		(1 << 2)

#define WTCON_DIV16		(0 << 4)
#define WTCON_DIV32		(1 << 4)
#define WTCON_DIV64		(2 << 4)
#define WTCON_DIV128	(3 << 4)
#define WTCON_DIVMASK	(0x3 << 4)

#define WTCON_PRESCALE_MAX	0x10000

#define WATCHDOG_DEFAULT_TIME	(15)

static int wdt_timeout = WATCHDOG_DEFAULT_TIME;
static int soft_noboot = 1;		/* 1: not reboot when watchdog timer expired */
static int wdt_count;

static __INLINE unsigned int wdt_max_timeout()
{
	unsigned long freq = ulClkGetRate(CLK_APB);

	return WTCNT_MAXCNT / (freq / WTCON_PRESCALE_MAX
				       / WTCON_MAXDIV);
}

int wdt_set_heartbeat(unsigned int timeout)
{
	unsigned long freq = ulClkGetRate(CLK_APB);
	unsigned int count;
	unsigned int divisor = 1;

	if (timeout < 1 || timeout > wdt_max_timeout())
		return -EINVAL;

	freq = DIV_ROUND_UP(freq, 128);
	count = timeout * freq;

	TRACE_DEBUG("Heartbeat: count=%d, timeout=%d, freq=%lu\n",
		count, timeout, freq);

	/* if the count is bigger than the watchdog register,
	   then work out what we need to do (and if) we can
	   actually make this value
	*/

	if (count >= 0x10000) {
		divisor = DIV_ROUND_UP(count, 0xffff);

		if (divisor > WTCON_PRESCALE_MAX) {
			TRACE_ERROR("timeout %d too big\n", timeout);
			return -EINVAL;
		}
	}

	TRACE_DEBUG("Heartbeat: timeout=%d, divisor=%d, count=%d (%08x)\n",
		timeout, divisor, count, DIV_ROUND_UP(count, divisor));

	count = DIV_ROUND_UP(count, divisor);
	wdt_count = count;

	/* update the pre-scaler */
	writel(divisor - 1, REGS_WDT_BASE + WTPSR);

	writel(count, REGS_WDT_BASE + WTCNT);

	wdt_timeout = timeout;

	return 0;
}

void wdt_stop(void)
{
	unsigned long wtcon;

	wtcon = readl(REGS_WDT_BASE + WTCON);
	wtcon &= ~(WTCON_ENABLE | WTCON_RSTEN);
	writel(wtcon, REGS_WDT_BASE + WTCON);
}

void wdt_start(void)
{
	unsigned long wtcon;

	wdt_stop();

	wtcon = readl(REGS_WDT_BASE + WTCON);
	wtcon &= ~WTCON_DIVMASK;
	wtcon |= WTCON_ENABLE | WTCON_DIV128;
	if (soft_noboot) {
		wtcon |= WTCON_INTEN;
		wtcon &= ~WTCON_RSTEN;
	} else {
		wtcon &= ~WTCON_INTEN;
		wtcon |= WTCON_RSTEN;
	}

	TRACE_DEBUG("Starting watchdog: count=0x%08x, wtcon=%08lx\n",
		wdt_count, wtcon);

	writel(wdt_count, REGS_WDT_BASE + WTCNT);
	writel(wtcon, REGS_WDT_BASE + WTCON);
}

void ark_wdt_keepalive(void)
{
	writel(wdt_count, REGS_WDT_BASE + WTCNT);
}

void wdt_isr(void *para)
{
	TRACE_DEBUG("watchdog timer expired (irq)\n");
	ark_wdt_keepalive();
	writel(0x1, REGS_WDT_BASE + WTCLRINT);
}

int wdt_init(void)
{
	int ret;
	
	ret = wdt_set_heartbeat(wdt_timeout);
	if (ret) {
		TRACE_ERROR("failed to set timeout value\n");
	}

	request_irq(WDT_IRQn, 0, wdt_isr, NULL);

	wdt_start();

	return 0;
}

void wdt_cpu_reboot(void)
{
	printf("cpu reboot...\n");
	soft_noboot = 0;
	wdt_count = 300;
	wdt_start();
	while(1);
}

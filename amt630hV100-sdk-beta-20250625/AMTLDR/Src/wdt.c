#include "amt630h.h"
#include "UartPrint.h"

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

#define WATCHDOG_DEFAULT_TIME	(3)

static int wdt_timeout = WATCHDOG_DEFAULT_TIME;
static int soft_noboot = 0;		/* 1: not reboot when watchdog timer expired */
static int wdt_count;

static inline unsigned int wdt_max_timeout(unsigned int clk_freq)
{
	return WTCNT_MAXCNT / (clk_freq / WTCON_PRESCALE_MAX
				       / WTCON_MAXDIV);
}

int wdt_set_heartbeat(unsigned int timeout, unsigned int clk_freq)
{
	unsigned int count;
	unsigned int divisor = 1;

	if (timeout < 1 || timeout > wdt_max_timeout(clk_freq))
		return -EINVAL;

	clk_freq = DIV_ROUND_UP(clk_freq, 128);
	count = timeout * clk_freq;

	/* TRACE_DEBUG("Heartbeat: count=%d, timeout=%d, freq=%lu\n",
		count, timeout, freq); */

	/* if the count is bigger than the watchdog register,
	   then work out what we need to do (and if) we can
	   actually make this value
	*/

	if (count >= 0x10000) {
		divisor = DIV_ROUND_UP(count, 0xffff);

		if (divisor > WTCON_PRESCALE_MAX) {
			SendUartString("timeout too big\n");
			return -EINVAL;
		}
	}

	/* TRACE_DEBUG("Heartbeat: timeout=%d, divisor=%d, count=%d (%08x)\n",
		timeout, divisor, count, DIV_ROUND_UP(count, divisor)); */

	count = DIV_ROUND_UP(count, divisor);
	wdt_count = count;

	/* update the pre-scaler */
	writel(divisor - 1, WDT_BASE + WTPSR);

	writel(count, WDT_BASE + WTCNT);

	wdt_timeout = timeout;

	return 0;
}

void wdt_stop(void)
{
	unsigned long wtcon;

	wtcon = readl(WDT_BASE + WTCON);
	wtcon &= ~(WTCON_ENABLE | WTCON_RSTEN);
	writel(wtcon, WDT_BASE + WTCON);
}

void wdt_start(void)
{
	unsigned long wtcon;

	wdt_stop();

	wtcon = readl(WDT_BASE + WTCON);
	wtcon &= ~WTCON_DIVMASK;
	wtcon |= WTCON_ENABLE | WTCON_DIV128;
	if (soft_noboot) {
		wtcon |= WTCON_INTEN;
		wtcon &= ~WTCON_RSTEN;
	} else {
		wtcon &= ~WTCON_INTEN;
		wtcon |= WTCON_RSTEN;
	}

	/* TRACE_DEBUG("Starting watchdog: count=0x%08x, wtcon=%08lx\n",
		wdt_count, wtcon); */

	writel(wdt_count, WDT_BASE + WTCNT);
	writel(wtcon, WDT_BASE + WTCON);
}

int wdt_init(unsigned int clk_freq)
{
	int ret;

	wdt_stop();
	
	ret = wdt_set_heartbeat(wdt_timeout, clk_freq);
	if (ret) {
		SendUartString("failed to set timeout value\n");
	}

	wdt_start();

	return 0;
}

void wdt_cpu_reboot(void)
{
	SendUartString("cpu reboot...\n");
	soft_noboot = 0;
	wdt_count = 300;
	wdt_start();
	while(1);
}

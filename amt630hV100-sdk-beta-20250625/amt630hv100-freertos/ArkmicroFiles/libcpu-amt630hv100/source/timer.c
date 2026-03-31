
#include <stdint.h>
#include "amt630hv100.h"
#include "clock.h"
#include "timer.h"
#include "chip.h"

#define TIMER_PERIOD	1000000
#define TIMER_LOAD_VAL	0xFFFFFFFF

/* For convenience the high frequency timer increments a variable that is then
used as the time base for the run time stats. */
volatile uint32_t ulHighFrequencyTimerCounts = 0;

void vTimerInit(uint32_t id, int32_t inten, int32_t periodic, uint32_t rate)
{
	uint32_t regbase = REGS_TIMER_BASE;
	uint32_t ctrl = TIMER_CTRL_INT_MASK;

	if (inten) ctrl &= ~TIMER_CTRL_INT_MASK;
	if (periodic) ctrl |= TIMER_CTRL_PERIODIC;
	
	writel(0, regbase + TIMER_CTRL(id));
	writel(ulClkGetRate(CLK_TIMER) / rate, regbase + TIMER_LOAD_COUNT(id));
	writel(ctrl, regbase + TIMER_CTRL(id));
}

void vTimerEnable(uint32_t id)
{
	uint32_t regbase = REGS_TIMER_BASE;

	writel(readl(regbase + TIMER_CTRL(id)) | TIMER_CTRL_ENABLE, regbase + TIMER_CTRL(id));
}

void vTimerDisable(uint32_t id)
{
	uint32_t regbase = REGS_TIMER_BASE;
	
	writel(readl(regbase + TIMER_CTRL(id)) & ~TIMER_CTRL_ENABLE, regbase + TIMER_CTRL(id));
}

void vTimerClrInt(uint32_t id)
{
	uint32_t regbase = REGS_TIMER_BASE;
	volatile uint32_t val;

	val = readl(regbase + TIMER_EOI(id));
}

/*-----------------------------------------------------------*/

static void prvRunTimer_Handler( void *para )
{
	vTimerClrInt((uint32_t)para);
	
	ulHighFrequencyTimerCounts++;
}

void vInitialiseTimerForRunTimeState(void) 
{
	vTimerInit(TIMER_ID1, 1, 1, configTICK_RATE_HZ * 20);
	request_irq(TIMER1_IRQn, 1, prvRunTimer_Handler, (void*)TIMER_ID1);
	vTimerEnable(TIMER_ID1);
}

uint32_t ulGetRunTimeCountValue(void)
{
	uint32_t regbase = REGS_TIMER_BASE;

	return TIMER_LOAD_VAL - readl(regbase + TIMER_CURRENT_VAL(TIMER_ID1));
}

void vInitialiseTimerForDelay(void)
{
	uint32_t regbase = REGS_TIMER_BASE;

	writel(0, regbase + TIMER_CTRL(TIMER_ID2));
	writel(TIMER_LOAD_VAL, regbase + TIMER_LOAD_COUNT(TIMER_ID2));
	writel(TIMER_CTRL_INT_MASK | TIMER_CTRL_PERIODIC | TIMER_CTRL_ENABLE,
		regbase +  TIMER_CTRL(TIMER_ID2));
}

void udelay(uint32_t usec)
{
	uint32_t regbase = REGS_TIMER_BASE;
	long tmo = usec * (ulClkGetRate(CLK_TIMER) / TIMER_PERIOD);
	unsigned long last = readl(regbase + TIMER_CURRENT_VAL(TIMER_ID2));
	unsigned long now;

	while (tmo > 0) {
		now = readl(regbase + TIMER_CURRENT_VAL(TIMER_ID2));
		if (last >= now)
			tmo -= last - now;
		else
			tmo -= last + (TIMER_LOAD_VAL - now);
		last = now;
	}	
}

void mdelay(uint32_t msec)
{
	udelay(msec * 1000);
}

void vTimerUdelay(uint32_t usec)
{
	udelay(usec);
}

void vTimerMdelay(uint32_t msec)
{
	vTimerUdelay(msec * 1000);
}

static unsigned long long timestamp;
static unsigned long lastdec;

uint32_t get_timer_masked(void)
{
	uint32_t regbase = REGS_TIMER_BASE;
	uint32_t now;
	uint32_t timeus;

	portENTER_CRITICAL();
	now = readl(regbase + TIMER_CURRENT_VAL(TIMER_ID2));

	if (lastdec >= now) {			 /* normal mode (non roll) */
		/* normal mode */
		timestamp += lastdec - now; /* move stamp fordward with absoulte diff ticks */
	} else {						/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += lastdec + (TIMER_LOAD_VAL - now);
	}
	lastdec = now;

	timeus = timestamp / (ulClkGetRate(CLK_TIMER) / TIMER_PERIOD);
	portEXIT_CRITICAL();

	return timeus;
}

uint32_t get_timer(uint32_t base)
{
	uint32_t now = get_timer_masked();

	if (now >= base) {
		return now - base;
	} else {
		return 0xffffffff - base + now;
	}
}

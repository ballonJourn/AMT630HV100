#include "amt630h.h"
#include "timer.h"

#define APBTMR_CONTROL_ENABLE			(1 << 0)
/* 1: periodic, 0:free running. */
#define APBTMR_CONTROL_MODE_PERIODIC	(1 << 1)
#define APBTMR_CONTROL_INT_DISABLE		(1 << 2)

#define TIMER_CLK		CLK_24MHZ
#define TIMER_PERIOD	1000000
#define TIMER_LOAD_VAL	0xffffffff

static unsigned long timestamp;
static unsigned long lastdec;

void timer_init(void)
{
	rTIMER0_CONTROL = 0;
	rTIMER0_LOAD_COUNT = TIMER_LOAD_VAL;
	/* No timer interrupt */
	rTIMER0_CONTROL =  APBTMR_CONTROL_INT_DISABLE |
					   APBTMR_CONTROL_MODE_PERIODIC | APBTMR_CONTROL_ENABLE;
}

void udelay(unsigned long usec)
{
	long tmo = usec * (TIMER_CLK / TIMER_PERIOD);
	unsigned long last = rTIMER0_CURRENT_VALUE;
	unsigned long now;

	while (tmo > 0) {
		now = rTIMER0_CURRENT_VALUE;
		if (last >= now)
			tmo -= last - now;
		else
			tmo -= last + (TIMER_LOAD_VAL - now);
		last = now;
	}
}

void mdelay(unsigned long msec)
{
	udelay(1000*msec);
}

#if 0
void reset_timer_masked (void)
{
	/* reset time */
	lastdec = rTIMER0_CURRENT_VALUE;  /* capure current decrementer value time */
	timestamp = 0;         /* start "advancing" time stamp from 0 */
}
#endif

ULONG get_timer_masked (void)
{
	unsigned long now = rTIMER0_CURRENT_VALUE;            /* current tick value */

	if (lastdec >= now) {            /* normal mode (non roll) */
		/* normal mode */
		timestamp += lastdec - now; /* move stamp fordward with absoulte diff ticks */
	} else {                        /* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += lastdec + (TIMER_LOAD_VAL - now);
	}
	lastdec = now;

	return timestamp / DIV_ROUND_UP(TIMER_CLK, TIMER_PERIOD) / 1000;
}

ULONG get_timer(ULONG base)
{
	return get_timer_masked () - base;
}
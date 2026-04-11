#include <stdio.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "errno.h"

/* RTC registers */
#define RTC_CTL			0x00 /*control register*/
#define RTC_ANAWEN		0x04 /*analog block write enable register*/
#define RTC_ANACTL		0x08 /*analog block control register*/
#define RTC_IM			0x0c /*interrupt mode register*/
#define RTC_STA			0x10 /*rtc status register*/
#define RTC_ALMDAT		0x14 /*alarm data register*/
#define RTC_DONT		0x18 /*delay on timer register*/
#define RTC_RAM			0x1c /*ram bit register*/
#define RTC_CNTL		0x20 /*rtc counter register*/
#define RTC_CNTH		0x24 /*rtc sec counter register*/

//RTC_CTL register fields defination
#define CTL_CTL3_VALUE(x)				(x<<23)
#define CTL_CTL2_VALUE(x)				(x<<22)
#define CTL_BIAS_TRM_VALUE(x)			(x<<21)
#define CTL_SOFT_SEL_VALUE(x)			(x<<20)
#define CTL_CTL1_VALUE(x)				(x<<19)
#define CTL_CTL0_VALUE(x)				(x<<18)
#define CTL_SOFT_STR_VALUE(x)			(x<<17)
#define CTL_OSC_EN_VALUE(x)				(x<<16)
#define CTL_CTL3_SET					(1<<15)
#define CTL_CTL2_SET					(1<<14)
#define CTL_BIAS_TRM_SET				(1<<13)
#define CTL_SOFT_SEL_SET				(1<<12)
#define CTL_CTL1_SET					(1<<11)
#define CTL_CTL0_SET					(1<<10)
#define CTL_SOFT_STR_SET				(1<<9)
#define CTL_OSC_EN_SET					(1<<8)
#define CTL_ALM_DATA_WEN				(1<<3)
#define CTL_PERIOD_INT_EN				(1<<2)
#define CTL_ALARM_INT_EN				(1<<1)
#define CTL_RESET						(1<<0)

//RTC_ANAWEN register fields defination
#define ANA_CNT_WEN						(1<<7)
#define ANA_RAM_WEN						(1<<6)
#define ANA_DELAY_TIMER_WEN				(1<<5)
#define ANA_CLR_PWR_DET_WEN				(1<<4)
#define ANA_DELAY_POWER_ON_WEN			(1<<3)
#define ANA_FORCE_POWER_OFF_WEN			(1<<2)
#define ANA_FORCE_POWER_ON_WEN			(1<<1)
#define ANA_RTC_WEN						(1<<0)

//RTC_ANACTL register fields defination
#define ANACTL_CLR_PWR					(1<<4)
#define ANACTL_DELAY_POWER_ON			(1<<3)
#define ANACTL_FORCE_POWER_OFF			(1<<2)
#define ANACTL_FORCE_POWER_ON			(1<<1)
#define ANACTL_COUNTER_EN				(1<<0)

/* STATUS_REG */
#define STA_PWR_DET			(1<<6)
#define STA_DELAY_ON		(1<<5)
#define STA_FORCE_OFF		(1<<4)
#define STA_FORCE_ON		(1<<3)
#define STA_RCT_BUSY		(1<<2)
#define STA_PERIOD_INT		(1<<1)
#define STA_ALARM_INT		(1<<0)

/* 2020-01-01 Wednesday */
static struct rtc_time default_tm = {
	.tm_sec = 0,
	.tm_min = 0,
	.tm_hour = 0,
	.tm_mday = 1,
	.tm_mon = 0,
	.tm_year = 120,
	.tm_wday = 3,
	.tm_yday = 1,
};

static const unsigned char rtc_days_in_month[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const unsigned short rtc_ydays[2][13] = {
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

/*
 * The number of days in the month.
 */
int rtc_month_days(unsigned int month, unsigned int year)
{
	return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
}

/*
 * The number of days since January 1. (0 to 365)
 */
int rtc_year_days(unsigned int day, unsigned int month, unsigned int year)
{
	return rtc_ydays[is_leap_year(year)][month] + day-1;
}

/*
 * rtc_time_to_tm - Converts time to rtc_time.
 * Convert seconds since 01-01-1970 00:00:00 to Gregorian date.
 */
void rtc_time_to_tm(uint32_t time, struct rtc_time *tm)
{
	unsigned int month, year;
	unsigned long secs;
	int days;

	/* time must be positive */
	days = time / 86400;
	secs = time - (unsigned int) days * 86400;

	/* day of the week, 1970-01-01 was a Thursday */
	tm->tm_wday = (days + 4) % 7;

	year = 1970 + days / 365;
	days -= (year - 1970) * 365
		+ LEAPS_THRU_END_OF(year - 1)
		- LEAPS_THRU_END_OF(1970 - 1);
	if (days < 0) {
		year -= 1;
		days += 365 + is_leap_year(year);
	}
	tm->tm_year = year - 1900;
	tm->tm_yday = days + 1;

	for (month = 0; month < 11; month++) {
		int newdays;

		newdays = days - rtc_month_days(month, year);
		if (newdays < 0)
			break;
		days = newdays;
	}
	tm->tm_mon = month;
	tm->tm_mday = days + 1;

	tm->tm_hour = secs / 3600;
	secs -= tm->tm_hour * 3600;
	tm->tm_min = secs / 60;
	tm->tm_sec = secs - tm->tm_min * 60;
}

/*
 * Does the rtc_time represent a valid date/time?
 */
int rtc_valid_tm(struct rtc_time *tm)
{
	if (tm->tm_year < 70
		|| ((unsigned)tm->tm_mon) >= 12
		|| tm->tm_mday < 1
		|| tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year + 1900)
		|| ((unsigned)tm->tm_hour) >= 24
		|| ((unsigned)tm->tm_min) >= 60
		|| ((unsigned)tm->tm_sec) >= 60)
		return -EINVAL;

	return 0;
}

/*
 * mktime - Converts date to seconds.
 * Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * This algorithm was first published by Gauss (I think).
 *
 * A leap second can be indicated by calling this function with sec as
 * 60 (allowable under ISO 8601).  The leap second is treated the same
 * as the following second since they don't exist in UNIX time.
 *
 * An encoding of midnight at the end of the day as 24:00:00 - ie. midnight
 * tomorrow - (allowable under ISO 8601) is supported.
 */
uint32_t mktime(const unsigned int year0, const unsigned int mon0,
		const unsigned int day, const unsigned int hour,
		const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int) (mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ((((uint32_t)
		  (year/4 - year/100 + year/400 + 367*mon/12 + day) +
		  year*365 - 719499
	    )*24 + hour /* now have hours - midnight tomorrow handled here */
	  )*60 + min /* now have minutes */
	)*60 + sec; /* finally seconds */
}


/*
 * rtc_tm_to_time - Converts rtc_time to time.
 * Convert Gregorian date to seconds since 01-01-1970 00:00:00.
 */
uint32_t rtc_tm_to_time(struct rtc_time *tm)
{
	return mktime(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static __INLINE void rtc_clear_interrupt(void)
{
	unsigned int val;

	val = readl(REGS_RTC_BASE + RTC_STA);
	val &= ~CTL_ALARM_INT_EN;
	writel(val, REGS_RTC_BASE + RTC_STA);
}

static __INLINE void rtc_enable_interrupt(void)
{
	unsigned int val;

	val = readl(REGS_RTC_BASE + RTC_CTL);
	if (!(val & CTL_ALARM_INT_EN)) {
		rtc_clear_interrupt();
		val |= CTL_ALARM_INT_EN;
		writel(val, REGS_RTC_BASE + RTC_CTL);
	}
}

static __INLINE void rtc_disable_interrupt(void)
{
	unsigned int val;

	val = readl(REGS_RTC_BASE + RTC_CTL);
	if (val & CTL_ALARM_INT_EN) {
		val &= ~CTL_ALARM_INT_EN;
		writel(val, REGS_RTC_BASE + RTC_CTL);
	}
}

static void rtc_wait_not_busy(void)
{
	int status, count = 0;

	/* Assuming BUSY may stay active for 80 msec) */
	for (count = 0; count < 0x1000; count++) {
		status = readl(REGS_RTC_BASE + RTC_STA);
		if ((status & STA_RCT_BUSY) == 0)
			break;
		/* check status busy, after each msec */
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

static void rtc_isr(void *para)
{
	unsigned int irq_data;

	irq_data = readl(REGS_RTC_BASE + RTC_STA);

	if ((irq_data & CTL_ALARM_INT_EN)) {
		rtc_clear_interrupt();
		return;
	} else
		return;	
}

static void rtc_update_time(unsigned int time)
{
	unsigned int val;
	int timeout = 100000;

	val = readl(REGS_RTC_BASE + RTC_ANAWEN);
	writel(val | ANA_RTC_WEN, REGS_RTC_BASE + RTC_ANAWEN);
	val = readl(REGS_RTC_BASE + RTC_ANACTL);
	writel(val | ANACTL_COUNTER_EN, REGS_RTC_BASE + RTC_ANACTL);

	//wait rtc_busy;
	rtc_wait_not_busy();

	val = readl(REGS_RTC_BASE + RTC_ANAWEN);
	writel(val | ANA_CNT_WEN, REGS_RTC_BASE + RTC_ANAWEN);
	writel(time, REGS_RTC_BASE + RTC_CNTH);

	//wait rtc_busy;
	rtc_wait_not_busy();
	while(readl(REGS_RTC_BASE + RTC_CNTH) != time) {
		if (timeout-- == 0)
			break;
		taskYIELD();
	}
}

/*
 * rtc_read_time - set the time
 * @tm: holds date and time
 *
 * This function read time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int rtc_read_time(struct rtc_time *tm)
{
	unsigned int time;

	/* we don't report wday/yday/isdst ... */
	rtc_wait_not_busy();

	time = readl(REGS_RTC_BASE + RTC_CNTH);

	rtc_time_to_tm(time, tm);

	return 0;
}

/*
 * rtc_set_time - set the time
 * @tm: holds date and time
 *
 * This function set time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int rtc_set_time(struct rtc_time *tm)
{
	long unsigned int time;

	if (rtc_valid_tm(tm) < 0)
		return -EINVAL;

	/* convert tm to seconds. */
	time = rtc_tm_to_time(tm);

	rtc_update_time(time);

	return 0;
}

#if 0
static void rtc_update_alarm_time(unsigned int time)
{
	unsigned int val;
	int timeout = 100000;

	val = readl(REGS_RTC_BASE + RTC_CTL);
	writel(val | CTL_ALM_DATA_WEN, REGS_RTC_BASE + RTC_CTL);

	writel(time, REGS_RTC_BASE + RTC_ALMDAT);

	//wait rtc_busy;
	rtc_wait_not_busy();
	while(readl(REGS_RTC_BASE + RTC_ALMDAT) != time) {
		if (timeout-- == 0)
			break;
		taskYIELD();
	}
}

/*
 * rtc_read_alarm - read the alarm time
 * @alm: holds alarm date and time
 *
 * This function read alarm time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int rtc_read_alarm(struct rtc_wkalrm *alm)
{
	unsigned int time;

	rtc_wait_not_busy();

	time = readl(REGS_RTC_BASE + RTC_ALMDAT);

	rtc_time_to_tm(time, &alm->time);

	alm->enabled = readl(REGS_RTC_BASE + RTC_CTL) & CTL_ALARM_INT_EN;

	return 0;
}

/*
 * rtc_set_alarm - set the alarm time
 * @alm: holds alarm date and time
 *
 * This function set alarm time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int rtc_set_alarm(struct rtc_wkalrm *alm)
{
	long unsigned int time;

	if (rtc_valid_tm(&alm->time) < 0)
		return -EINVAL;

	/* convert tm to seconds. */
	time = rtc_tm_to_time(&alm->time);

	rtc_update_alarm_time(time);

	if (alm->enabled)
		rtc_enable_interrupt();
	else
		rtc_disable_interrupt();

	return 0;
}
#endif

static int alarm_irq_enable(unsigned int enabled)
{
	int ret = 0;

	rtc_clear_interrupt();

	switch (enabled) {
	case 0:
		/* alarm off */
		rtc_disable_interrupt();
		break;
	case 1:
		/* alarm on */
		rtc_enable_interrupt();
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


int rtc_init(void)
{
	struct rtc_time tm;

	writel(0, REGS_RTC_BASE + RTC_CTL);
	writel(CTL_SOFT_STR_SET|CTL_SOFT_STR_VALUE(1), REGS_RTC_BASE + RTC_CTL);
	writel(CTL_OSC_EN_SET|CTL_OSC_EN_VALUE(0), REGS_RTC_BASE + RTC_CTL);
	writel(CTL_CTL0_SET|CTL_CTL0_VALUE(0), REGS_RTC_BASE + RTC_CTL);
	writel(CTL_CTL1_SET|CTL_CTL1_VALUE(0), REGS_RTC_BASE + RTC_CTL);
	writel(CTL_CTL2_SET|CTL_CTL2_VALUE(0), REGS_RTC_BASE + RTC_CTL);
	writel(CTL_CTL3_SET|CTL_CTL3_VALUE(0), REGS_RTC_BASE + RTC_CTL);
	writel(CTL_BIAS_TRM_SET|CTL_BIAS_TRM_VALUE(1)|CTL_CTL3_VALUE(0), REGS_RTC_BASE + RTC_CTL);
	udelay(1000);
	writel(CTL_OSC_EN_SET|CTL_OSC_EN_VALUE(1), REGS_RTC_BASE + RTC_CTL);
	udelay(1000);

	request_irq(RTC_PRD_IRQn, 0, rtc_isr, NULL);
	alarm_irq_enable(0);

	rtc_read_time(&tm);
	if (tm.tm_year == 70) {
		rtc_set_time(&default_tm);
	}

	return 0;
}

int iGetLocalTime(SystemTime_t *tm)
{
	rtc_read_time(tm);
	tm->tm_year += 1900;
	tm->tm_mon += 1;
	
	return 0;
}

void vSetLocalTime(SystemTime_t *tm)
{
	tm->tm_year -= 1900;
	tm->tm_mon -= 1;
	rtc_set_time(tm);
}

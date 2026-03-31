#ifndef _RTC_H
#define _RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The struct used to pass data via the following ioctl. Similar to the
 * struct tm in <time.h>, but it needs to be here so that the kernel
 * source is self contained, allowing cross-compiles, etc. etc.
 */

typedef struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
} SystemTime_t;

/*
 * This data structure is inspired by the EFI (v0.92) wakeup
 * alarm API.
 */
struct rtc_wkalrm {
	unsigned char enabled;	/* 0 = alarm disabled, 1 = alarm enabled */
	unsigned char pending;  /* 0 = alarm not pending, 1 = alarm pending */
	struct rtc_time time;	/* time the alarm is set to */
};

static inline int is_leap_year(unsigned int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}

int rtc_init(void);

int iGetLocalTime(SystemTime_t *tm);

void vSetLocalTime(SystemTime_t *tm);

#ifdef __cplusplus
}
#endif

#endif

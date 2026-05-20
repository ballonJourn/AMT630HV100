#ifndef __TIMER_H__
#define __TIMER_H__

#include "typedef.h"

extern void timer_init(void);
//extern void reset_timer_masked (void);
extern void udelay(unsigned long usec);
extern void mdelay(unsigned long msec);
extern ULONG get_timer(ULONG base);

#endif

#ifndef _TIMER_H
#define _TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TIMER_ID0 = 0,
	TIMER_ID1,
	TIMER_ID2,
	TIMER_ID3,
} eTimerID;

#define TIMER_LOAD_COUNT(x)		((x) * 0x14 + 0x00)
#define TIMER_CURRENT_VAL(x)	((x) * 0x14 + 0x04)
#define TIMER_CTRL(x)			((x) * 0x14 + 0x08)
#define TIMER_EOI(x)			((x) * 0x14 + 0x0c)
#define TIMER_INT_STATUS(x)		((x) * 0x14 + 0x10)


#define TIMER_CTRL_INT_MASK	(1ul << 2)
#define TIMER_CTRL_PERIODIC	(1ul << 1)
#define TIMER_CTRL_ENABLE	(1ul << 0)

void vTimerInit(uint32_t id, int32_t inten, int32_t periodic, uint32_t rate);

void vTimerEnable(uint32_t id);

void vTimerDisable(uint32_t id);

void vTimerClrInt(uint32_t id);

void vInitialiseTimerForRunTimeState(void);
	
uint32_t ulGetRunTimeCountValue(void);

void vInitialiseTimerForDelay(void);

void vTimerUdelay(uint32_t usec);

void vTimerMdelay(uint32_t msec);

void udelay(uint32_t usec);

void mdelay(uint32_t msec);

uint32_t get_timer(uint32_t base);	/* us */

#ifdef __cplusplus
}
#endif

#endif

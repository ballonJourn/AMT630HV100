#include <stdint.h> 
#include "timer.h"

static inline void sys_ctl_delay(unsigned long ulCount)
{
	while(ulCount--);
}

/**
  * Customer provide us level delay
  * FreeRTOS does not provide us level delay. Use busy wait
  * It is CPU platform dependent
  */

void WLAN_BSP_UsLoop(int us)
{

	//unsigned long nloops = us * (configCPU_CLOCK_HZ / 3000000);
	//sys_ctl_delay(nloops);
	udelay(us);

}

/* Customer function to control HW specific wlan gpios */
void Set_WLAN_Power_On(void)
{

}

/* Customer function to control HW specific wlan gpios */
void Set_WLAN_Power_Off(void)
{

}

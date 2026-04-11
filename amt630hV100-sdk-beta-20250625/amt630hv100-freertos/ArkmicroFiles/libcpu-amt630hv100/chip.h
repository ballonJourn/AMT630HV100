#ifndef _LIB_CHIP_AMT630HV100_
#define _LIB_CHIP_AMT630HV100_

/*
 * Peripherals registers definitions
 */
#if defined AMT630HV100
    #include "include/amt630hv100.h"
#else
    #warning Library does not support the specified chip, specifying AMT630HV100
    #define AMT630HV100
    #include "include/amt630hv100.h"
#endif

/* Define attribute */
#if defined   ( __CC_ARM   ) /* Keil 礦ision 4 */
    #define WEAK __attribute__ ((weak))
#elif defined ( __ICCARM__ ) /* IAR Ewarm 5.41+ */
    #define WEAK __weak
#elif defined (  __GNUC__  ) /* GCC CS3 2009q3-68 */
    #define WEAK __attribute__ ((weak))
#endif

/* Define NO_INIT attribute and compiler specific symbols */
#if defined   ( __CC_ARM   )
    #define NO_INIT
    #define __ASM            __asm                                    /*!< asm keyword for ARM Compiler          */
    #define __INLINE         __inline                                 /*!< inline keyword for ARM Compiler       */
#elif defined ( __ICCARM__ )
    #define NO_INIT __no_init
    #define __ASM           __asm                                     /*!< asm keyword for IAR Compiler           */
    #define __INLINE        inline                                    /*!< inline keyword for IAR Compiler. Only avaiable in High optimization mode! */
#elif defined (  __GNUC__  )
    #define __ASM            asm                                      /*!< asm keyword for GNU Compiler          */
    #define __INLINE         inline                                   /*!< inline keyword for GNU Compiler       */
    #define NO_INIT
#endif

#define CP15_PRESENT

struct device {
	const char		*init_name;
};

#include <stdbool.h>

/*
 * Peripherals
 */
#include "errno.h"

#include "include/mmu.h"
#include "cp15/cp15.h"
#include "include/trace.h"
#include "include/sysctl.h"
#include "include/aic.h"
#include "include/timer.h"
#include "include/uart.h"
#include "include/clock.h"
#include "include/pinctrl.h"
#include "include/gpio.h"
#include "include/i2c.h"
#include "include/i2c-gpio.h"
#include "include/i2c-dw.h"
#include "include/spi.h"
#include "include/wdt.h"
#include "include/rtc.h"
#include "include/dma.h"
#include "include/lcd.h"
#include "include/pwm.h"
#include "include/itu.h"
#include "include/pxp.h"
#include "include/can.h"
#include "include/adc.h"
#include "include/sdmmc.h"
#include "include/vdec.h"
#include "include/blend2d.h"
#include "include/remote.h"

#endif /* _LIB_CHIP_AMT630HV100_ */


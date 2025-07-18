#ifndef _CLOCK_H
#define _CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CLK_SOURCE_NUM		4
#define MAX_CLK_ENABLE_BITS		4

typedef enum {
	CLK_XTAL32K = 0,
	CLK_XTAL24M,
	CLK_240MHZ,
	CLK_12MHZ,
	CLK_6MHZ,
	CLK_CPUPLL,
	CLK_SYSPLL,
	CLK_VPUPLL,
	CLK_DDRPLL,//Ôö¼ÓDDR PLL
	CLK_DDR,
	CLK_CPU,
	CLK_H2X,
	CLK_AHB,
	CLK_APB,
	CLK_RTC,
	CLK_SPI0,
	CLK_SPI1,
	CLK_SDMMC0,
	CLK_LCD,
	CLK_UART1,
	CLK_UART2,
	CLK_UART3,
	CLK_TIMER,
	CLK_MFC,
	CLK_PWM,
	CLK_CAN0,
	CLK_CAN1,
	CLK_ADC,
	CLK_I2S,
	CLK_I2S1,
}eClockID;

typedef enum {
	FIXED_CLOCK = 0,
	FIXED_FACTOR_CLOCK,
	PLL_CLOCK,
	SYS_CLOCK,
}eClockType;	

typedef enum {
	DIVMODE_NOZERO = 0,		/* div = div ? div : 1 */
	DIVMODE_PLUSONE,		/* div = div + 1 */
	DIVMODE_DOUBLE,			/* div = div * 2 */
	DIVMODE_EXPONENT,		/* div = 1 << div */
	DIVMODE_PONEDOUBLE,		/* div = (div + 1) * 2 */
}eDivMode;

void vClkInit(void);
uint32_t ulClkGetRate(uint32_t clkid);
void vClkSetRate(uint32_t clkid, uint32_t freq);
void vClkEnable(uint32_t clkid);
void vClkDisable(uint32_t clkid);

#ifdef __cplusplus
}
#endif

#endif

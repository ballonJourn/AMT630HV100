#ifndef _ADC_H
#define _ADC_H

typedef enum {
	ADC_CH_BAT = 1,
	ADC_CH_TP = 2,
	ADC_CH_AUX0 = 3,
	ADC_CH_AUX1 = 4,
	ADC_CH_AUX2 = 5,
	ADC_CH_AUX3 = 6,
	ADC_CH_AUX4 = 7,
	ADC_CH_AUX5 = 8,
	ADC_CH_AUX6 = 9,
	ADC_CH_AUX7 = 10,
} eAdcChannel;

#define  AUX0_START_INT       		(1<<0)
#define  AUX0_STOP_INT        		(1<<1)
#define  AUX0_VALUE_INT       		(1<<2)

#define  AUX1_START_INT       		(1<<3)
#define  AUX1_STOP_INT        		(1<<4)
#define  AUX1_VALUE_INT       		(1<<5)

#define  AUX2_START_INT       		(1<<6)
#define  AUX2_STOP_INT        		(1<<7)
#define  AUX2_VALUE_INT       		(1<<8)

#define  AUX3_START_INT       		(1<<9)
#define  AUX3_STOP_INT        		(1<<10)
#define  AUX3_VALUE_INT       		(1<<11)

#define  AUX4_START_INT       		(1<<16)
#define  AUX4_STOP_INT        		(1<<17)
#define  AUX4_VALUE_INT       		(1<<18)

#define  AUX5_START_INT       		(1<<19)
#define  AUX5_STOP_INT        		(1<<20)
#define  AUX5_VALUE_INT       		(1<<21)

#define  AUX6_START_INT       		(1<<22)
#define  AUX6_STOP_INT        		(1<<23)
#define  AUX6_VALUE_INT       		(1<<24)

#define  AUX7_START_INT       		(1<<25)
#define  AUX7_STOP_INT        		(1<<26)
#define  AUX7_VALUE_INT       		(1<<27)

#define  TP_START_INT   		(1<<12)
#define  TP_STOP_INT    		(1<<13)
#define  TP_VALUE_INT   		(1<<14)

#define  BAT_INT     				(1<<15)

void adc_init(void);
void adc_channel_enable(eAdcChannel ch);
void adc_channel_disable(eAdcChannel ch);
unsigned int adc_get_channel_value(int ch);

#endif

#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"

#ifdef ADC_TOUCH
#include "touch.h"
#endif

#ifdef ADC_KEY
#include "keypad.h"
#endif

#include "config/hcn_config.h"

#define ADC_CTR			0x00
#define ADC_CFG			0x04
#define ADC_IMR			0x08
#define ADC_STA			0x0C
#define ADC_BAT			0x10
#define ADC_AUX0		0x14
#define ADC_AUX1		0x18
#define ADC_AUX2		0x1C
#define ADC_AUX3		0x20
#define ADC_AUX4		0x38
#define ADC_AUX5		0x40
#define ADC_AUX6		0x44
#define ADC_AUX7		0x4C
#define ADC_PANXZ1		0x24
#define ADC_PANXZ2		0x28
#define ADC_DBNCNT		0x2C
#define ADC_DETINTER	0x30
#define ADC_SCTR		0x34

#ifndef HCN_ADC_KEY_ENABLE
#define ADC_CLK_FREQ		1000000
#else
#define ADC_CLK_FREQ		37000
#endif

#define ADC_DEBOUNCE_CNT	0x10000

static void adc_set_deinter(uint32_t count)
{
	/* dbncnt * freq_adc / (2 * freq_apb) */
	int mincnt = ADC_DEBOUNCE_CNT * ulClkGetRate(CLK_ADC) / 2 / ulClkGetRate(CLK_APB);

	writel(configMAX(mincnt, count), REGS_ADC_BASE + ADC_DETINTER);
}

void adc_channel_enable(eAdcChannel ch)
{
	uint32_t ctr, imr;
	
	configASSERT(ch >= ADC_CH_BAT && ch <= ADC_CH_AUX7);

	ctr = readl(REGS_ADC_BASE + ADC_CTR);
	imr = readl(REGS_ADC_BASE + ADC_IMR);
	
	switch(ch) {
	case ADC_CH_BAT:
		ctr |= 1 << ADC_CH_BAT;
		imr &= ~BAT_INT;
		break;

	case ADC_CH_TP:
		ctr |= 1 << ADC_CH_TP;
		imr &= ~(TP_START_INT | TP_STOP_INT | TP_VALUE_INT);
		break;

	case ADC_CH_AUX0:
		ctr |= (1 << 8) | (1 << ADC_CH_AUX0);
		imr &= ~(AUX0_START_INT | AUX0_STOP_INT | AUX0_VALUE_INT);
		break;

	case ADC_CH_AUX1:
		ctr |= (1 << 9) | (1 << ADC_CH_AUX1);
		imr &= ~(AUX1_START_INT | AUX1_STOP_INT | AUX1_VALUE_INT);
		break;

	case ADC_CH_AUX2:
		ctr |= (1 << 10) | (1 << ADC_CH_AUX2);
		imr &= ~(AUX2_START_INT | AUX2_STOP_INT | AUX2_VALUE_INT);
		break;

	case ADC_CH_AUX3:
		ctr |= (1 << 11) | (1 << ADC_CH_AUX3);
		imr &= ~(AUX3_START_INT | AUX3_STOP_INT | AUX3_VALUE_INT);
		break;
	case ADC_CH_AUX4:
		ctr |= (1 << 24) | (1 << 20);
		imr &= ~(AUX4_START_INT | AUX4_STOP_INT | AUX4_VALUE_INT);
		break;

	case ADC_CH_AUX5:
		ctr |= (1 << 25) | (1 << 21);
		imr &= ~(AUX5_START_INT | AUX5_STOP_INT | AUX5_VALUE_INT);
		break;

	case ADC_CH_AUX6:
		ctr |= (1 << 26) | (1 << 22);
		imr &= ~(AUX6_START_INT | AUX6_STOP_INT | AUX6_VALUE_INT);
		break;

	case ADC_CH_AUX7:
		ctr |= (1 << 27) | (1 << 23);
		imr &= ~(AUX7_START_INT | AUX7_STOP_INT | AUX7_VALUE_INT);
		break;
	}

	writel(ctr, REGS_ADC_BASE + ADC_CTR);
	writel(imr, REGS_ADC_BASE + ADC_IMR);
}

void adc_channel_disable(eAdcChannel ch)
{
	uint32_t ctr, imr;
	
	configASSERT(ch >= ADC_CH_BAT && ch <= ADC_CH_AUX7);

	ctr = readl(REGS_ADC_BASE + ADC_CTR);
	imr = readl(REGS_ADC_BASE + ADC_IMR);
	
	switch(ch) {
	case ADC_CH_BAT:
		ctr &= ~(1 << ADC_CH_BAT);
		imr |= BAT_INT;
		break;

	case ADC_CH_TP:
		ctr &= ~(1 << ADC_CH_TP);
		imr |= (TP_START_INT | TP_STOP_INT | TP_VALUE_INT);
		break;

	case ADC_CH_AUX0:
		ctr &= ~((1 << 8) | (1 << ADC_CH_AUX0));
		imr |= (AUX0_START_INT | AUX0_STOP_INT | AUX0_VALUE_INT);
		break;

	case ADC_CH_AUX1:
		ctr &= ~((1 << 9) | (1 << ADC_CH_AUX1));
		imr |= (AUX1_START_INT | AUX1_STOP_INT | AUX1_VALUE_INT);
		break;

	case ADC_CH_AUX2:
		ctr &= ~((1 << 10) | (1 << ADC_CH_AUX2));
		imr |= (AUX2_START_INT | AUX2_STOP_INT | AUX2_VALUE_INT);
		break;

	case ADC_CH_AUX3:
		ctr &= ~((1 << 11) | (1 << ADC_CH_AUX3));
		imr |= (AUX3_START_INT | AUX3_STOP_INT | AUX3_VALUE_INT);
		break;

	case ADC_CH_AUX4:
		ctr &= ~((1 << 24) | (1 << 20));
		imr |= (AUX4_START_INT | AUX4_STOP_INT | AUX4_VALUE_INT);
		break;

	case ADC_CH_AUX5:
		ctr &= ~((1 << 25) | (1 << 21));
		imr |= (AUX5_START_INT | AUX5_STOP_INT | AUX5_VALUE_INT);
		break;

	case ADC_CH_AUX6:
		ctr &= ~((1 << 26) | (1 << 22));
		imr |= (AUX6_START_INT | AUX6_STOP_INT | AUX6_VALUE_INT);
		break;

	case ADC_CH_AUX7:
		ctr &= ~((1 << 27) | (1 << 23));
		imr |= (AUX7_START_INT | AUX7_STOP_INT | AUX7_VALUE_INT);
		break;
	}

	writel(ctr, REGS_ADC_BASE + ADC_CTR);
	writel(imr, REGS_ADC_BASE + ADC_IMR);

}

static void adc_int_handler(void *para)
{
	uint32_t status;
#ifdef ADC_TOUCH
	uint32_t xpos, ypos;
#endif
	//uint32_t value;

	status = readl(REGS_ADC_BASE + ADC_STA);
	writel(0, REGS_ADC_BASE + ADC_STA);

	//printf("adc_int_handler status=0x%x.\n", status);

	if (status & TP_START_INT) {
#ifdef ADC_TOUCH
		TouchEventHandler(TOUCH_START_EVENT, 0, 0);
#endif
	}

	if (status & TP_STOP_INT) {
#ifdef ADC_TOUCH
		TouchEventHandler(TOUCH_STOP_EVENT, 0, 0);
#endif
	}

	if (status & TP_VALUE_INT) {
#ifdef ADC_TOUCH
		xpos = readl(REGS_ADC_BASE + ADC_PANXZ1);
		ypos = readl(REGS_ADC_BASE + ADC_PANXZ2);
		//printf("tp press %d, %d.\n", xpos, ypos);
		TouchEventHandler(TOUCH_SAMPLE_EVENT, xpos, ypos);
#endif
	}

	if (status & AUX0_START_INT) {
#ifdef ADC_KEY
		KeyEventHandler(KEY_START_EVENT, 0, 0);
#endif
	}

	if (status & AUX0_STOP_INT) {
#ifdef ADC_KEY
		KeyEventHandler(KEY_STOP_EVENT, 0, 0);
#endif
	}

	if (status & AUX0_VALUE_INT) {
#ifdef ADC_KEY
		uint32_t value = readl(REGS_ADC_BASE + ADC_AUX0);
		KeyEventHandler(KEY_SAMPLE_EVENT, value, 0);
#endif
	}

#if 0
	if (status & AUX1_START_INT) {

	}

	if (status & AUX1_STOP_INT) {

	}

	if (status & AUX1_VALUE_INT) {
		value = readl(REGS_ADC_BASE + ADC_AUX1);
	}

	if (status & AUX2_START_INT) {

	}

	if (status & AUX2_STOP_INT) {

	}

	if (status & AUX2_VALUE_INT) {
		value = readl(REGS_ADC_BASE + ADC_AUX2);
	}

	if (status & AUX3_START_INT) {

	}

	if (status & AUX3_STOP_INT) {

	}

	if (status & AUX3_VALUE_INT) {
		value = readl(REGS_ADC_BASE + ADC_AUX3);
	}

	if (status & AUX4_START_INT) {

	}

	if (status & AUX4_STOP_INT) {

	}

	if (status & AUX4_VALUE_INT) {
		value = readl(REGS_ADC_BASE + ADC_AUX4);
	}

    if (status & AUX5_START_INT) {

	}

	if (status & AUX5_STOP_INT) {

	}

	if (status & AUX5_VALUE_INT) {
		value = readl(REGS_ADC_BASE + ADC_AUX5);
	}

	if (status & AUX6_START_INT) {

	}

	if (status & AUX6_STOP_INT) {

	}

	if (status & AUX6_VALUE_INT) {
		value = readl(REGS_ADC_BASE + ADC_AUX6);
	}

	if (status & AUX7_START_INT) {

	}

	if (status & AUX7_STOP_INT) {

	}

	if (status & AUX7_VALUE_INT) {
		value = readl(REGS_ADC_BASE + ADC_AUX7);
	}

	if (status & BAT_INT) {

	}
#endif
}

void adc_init(void)
{
	vSysctlConfigure(SYS_ANA1_CFG, 7, 1, 0);	// ref : 3.3v

	vClkSetRate(CLK_ADC, ADC_CLK_FREQ);

	/* reset adc modulex */
	writel(readl(REGS_ADC_BASE + ADC_CTR) | 1, REGS_ADC_BASE + ADC_CTR);

	/* disable all adc channel */
	writel(readl(REGS_ADC_BASE + ADC_CTR) & ~0x7e, REGS_ADC_BASE + ADC_CTR);
	
	/* disable and clear irq */
	writel(0xffffffff, REGS_ADC_BASE + ADC_IMR);
	writel(0, REGS_ADC_BASE + ADC_STA);
	
	/* set debounce count */
	writel(ADC_DEBOUNCE_CNT, REGS_ADC_BASE + ADC_DBNCNT);

	adc_set_deinter(50);

	request_irq(ADC_IRQn, 0, adc_int_handler, NULL);
}

unsigned int adc_get_channel_value(int ch)
{
	int i;

	configASSERT(ch >= ADC_CH_AUX0 && ch <= ADC_CH_AUX7);

	adc_channel_disable(ADC_CH_TP);
	for (i = ADC_CH_AUX0; i <= ADC_CH_AUX7; i++) {
		if (ch == i)
			adc_channel_enable(i);
		else
			adc_channel_disable(i);
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	if(ch<=ADC_CH_AUX3)
	{
		return readl(REGS_ADC_BASE + ADC_AUX0 + 4 * (ch - ADC_CH_AUX0));
	}
	else
	{
		return readl(REGS_ADC_BASE + ADC_AUX4 + 4 * (ch - ADC_CH_AUX4));
	}
}

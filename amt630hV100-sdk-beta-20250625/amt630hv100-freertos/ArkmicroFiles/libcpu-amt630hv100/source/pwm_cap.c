#include "FreeRTOS.h"
#include "board.h"

#ifdef PWM_CAP_SUPPORT
#include "chip.h"
#include "pwm_cap.h"

#define PWM_CAP_SYS_FRQ			(0x00)
#define PWM_CAP_SETTING			(0x04)
#define PWM_CAP_CYCLE_CAP		(0x08)
#define PWM_CAP_FRE_CAP			(0x0c)

#define PWM_CAP_INT_CLEAR       (0x40)
#define PWM_CAP_INT_EN          (0x80)
#define PWM_CAP_INT_STA         (0x84)

#define PWM_CAP_CLK				(ulClkGetRate(CLK_APB))

#define PWM_CAP_REG(x)			(REGS_PWM_BASE + 0x100 + 0x10 * (x))

static uint32_t pwmCapVal[PWM_CAP_CH_NUM];

static void pwm_cap_clk_config(PWM_CAP_CH id, uint32_t clk)
{
	writel(clk, PWM_CAP_REG(id) + PWM_CAP_SYS_FRQ);
}

static void pwm_cap_en(PWM_CAP_CH id, int enable)
{
	unsigned int reg;

	reg = readl(PWM_CAP_REG(id) + PWM_CAP_SETTING);
	if(enable)
		reg |= (1UL << 31);
	else
		reg &= ~(1UL << 31);
	writel(reg, PWM_CAP_REG(id) + PWM_CAP_SETTING);
}

static void pwm_cap_int_method(PWM_CAP_CH id, uint8_t int_method)
{
	unsigned int reg;

	reg = readl(PWM_CAP_REG(id) + PWM_CAP_SETTING);
	reg &= ~(0x3 << 28);
	reg |= (int_method << 28);
	writel(reg,PWM_CAP_REG(id) + PWM_CAP_SETTING);
}

static void pwm_cap_set_glitch(PWM_CAP_CH id, uint8_t glitch)
{
	unsigned int reg;

	reg = readl(PWM_CAP_REG(id) + PWM_CAP_SETTING);
	reg &= ~(0xF << 24);
	reg |= (glitch << 24);
	writel(reg, PWM_CAP_REG(id) + PWM_CAP_SETTING);
}

static void pwm_cap_method(PWM_CAP_CH id, uint8_t cap_method)
{
	unsigned int reg;

	reg = readl(PWM_CAP_REG(id) + PWM_CAP_SETTING);
	reg &= ~(0x1 << 30);
	reg |= (cap_method << 30);
	writel(reg,PWM_CAP_REG(id) + PWM_CAP_SETTING);
}

static void pwm_cap_times(PWM_CAP_CH id, uint8_t cat_times)
{
	unsigned int reg;

	reg = readl(PWM_CAP_REG(id) + PWM_CAP_SETTING);
	reg &= ~(0xFF << 16);
	reg |= (cat_times << 16);
	writel(reg,PWM_CAP_REG(id) + PWM_CAP_SETTING);
}

static void pwm_cap_based_unit(PWM_CAP_CH id, uint8_t cap_based_unit)
{
	unsigned int reg;

	reg = readl(PWM_CAP_REG(id) + PWM_CAP_SETTING);
	reg &= ~(0x7 << 12);
	reg |= (cap_based_unit << 12);
	writel(reg,PWM_CAP_REG(id) + PWM_CAP_SETTING);
}

static void pwm_cap_interval(PWM_CAP_CH id, uint8_t cap_interval)
{
	unsigned int reg;
	reg = readl(PWM_CAP_REG(id) + PWM_CAP_SETTING);
	reg &= ~(0xFF << 0);
	reg |= (cap_interval << 0);
	writel(reg,PWM_CAP_REG(id) + PWM_CAP_SETTING);
}

//低4位为小数部分
static uint32_t pwm_cap_get_freq(PWM_CAP_CH id)
{
	return readl(PWM_CAP_REG(id) + PWM_CAP_FRE_CAP);
}

static void pwm_cap_int_Handler(void *para)
{
	unsigned int val;
	PWM_CAP_CH id;

	val = readl(PWM_CAP_REG(0) + PWM_CAP_INT_STA);

	for (id = PWM_CAP_CH0; id < PWM_CAP_CH_NUM; id++) {
		if (val & (1 << id)) {
			writel(1 << id, PWM_CAP_REG(0) + PWM_CAP_INT_CLEAR);
			pwmCapVal[id] = pwm_cap_get_freq(id);
			//printf("pwmCapVal[%d]=0x%x.\n", id, pwmCapVal[id]);
			writel(0, PWM_CAP_REG(0) + PWM_CAP_INT_CLEAR);
			pwm_cap_en(id, 1);
		}
	}
}

void pwm_cap_initial(PWM_CAP_CH id)
{
	pwm_cap_clk_config(id, PWM_CAP_CLK);
	pwm_cap_int_method(id, PWM_CAP_ONCE_FINISH_INT);
	pwm_cap_set_glitch(id, PWM_CAP_GLITCH);
	pwm_cap_method(id, PWM_CAP_NUM);
	pwm_cap_times(id, PWM_CAP_TIMES);
	pwm_cap_based_unit(id, PWM_CAP_UINT_1000MS);
	pwm_cap_interval(id, PWM_CAP_INTERVAL);
}

void pwm_cap_enable_irq(PWM_CAP_CH id, int en)
{
	unsigned int reg;

	reg = readl(PWM_CAP_REG(0) + PWM_CAP_INT_EN);
	if (en)
		reg |= 1 << id;
	else
		reg &= ~(1 << id);
	writel(reg, PWM_CAP_REG(0) + PWM_CAP_INT_EN);
}

void vPWMCapInit(void)
{
	PWM_CAP_CH id;

	request_irq(RCRT_IRQn, 0, pwm_cap_int_Handler, NULL);
	for (id = PWM_CAP_CH0; id < PWM_CAP_CH_NUM; id++)
		pwm_cap_initial(id);
}

void vPWMCapChEnable(PWM_CAP_CH id)
{
	pwm_cap_enable_irq(id, 1);
	pwm_cap_en(id, 1);
}

void vPWMCapChDisable(PWM_CAP_CH id)
{
	pwm_cap_enable_irq(id, 0);
	pwm_cap_en(id, 0);
}

uint32_t ulPWMCapGetFreq(PWM_CAP_CH id)
{
	return pwmCapVal[id];
}

#endif

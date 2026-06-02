#include "amt630h.h"

#define PWM_EN		0x0
#define PWM_DUTY	0x4
#define PWM_CNTR	0x8

#define PWM_REG(x)	(REGS_PWM_BASE + 0x10 * (x)) 

int pwm_config(int id, uint32_t duty_ns, uint32_t period_ns)
{
	uint32_t clk_mhz = 24;//ulClkGetRate(CLK_PWM) / 1000000;
	uint32_t duty = (unsigned long long)duty_ns * clk_mhz / 1000;
	uint32_t period = (unsigned long long)period_ns * clk_mhz / 1000;
	
	writel(0, PWM_REG(id) + PWM_EN);
	writel(duty, PWM_REG(id) + PWM_DUTY);
	writel(period, PWM_REG(id) + PWM_CNTR);

	return 0;
}

void pwm_enable(int id)
{
	writel(1, PWM_REG(id) + PWM_EN);
}

void pwm_disable(int id)
{
	writel(0, PWM_REG(id) + PWM_EN);
}

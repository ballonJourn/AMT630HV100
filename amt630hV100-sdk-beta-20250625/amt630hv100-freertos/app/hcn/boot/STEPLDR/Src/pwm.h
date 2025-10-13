#ifndef _PWM_H
#define _PWM_H

typedef enum {
	PWM_ID0 = 0,
	PWM_ID1,
	PWM_ID2,
	PWM_ID3
} PWM_ID;

int pwm_config(int id, uint32_t duty_ns, uint32_t period_ns);
void pwm_enable(int id);
void pwm_disable(int id);

#endif

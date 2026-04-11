#ifndef PWM_CAP_H_
#define PWM_CAP_H_

#define PWM_CAP_TIMES     1
#define PWM_CAP_INTERVAL  1//64
#define PWM_CAP_GLITCH    0x7//0xF

typedef enum {
	PWM_CAP_CH0 = 0,
	PWM_CAP_CH1,
	PWM_CAP_CH2,
	PWM_CAP_CH3,
	PWM_CAP_CH_NUM,
} PWM_CAP_CH;

typedef enum {
	PWM_CAP_NUM = 0,
	PWM_CAP_EXIT,
} PWM_CAP_METHOD;

typedef enum {
	PWM_CAP_UINT_1MS = 0,
	PWM_CAP_UINT_10MS,
	PWM_CAP_UINT_100MS,
	PWM_CAP_UINT_1000MS = 4,
} PWM_CAP_BASED_UINT;

typedef enum {
	PWM_CAP_NO_INT = 0,
	PWM_CAP_ONCE_INT,
	PWM_CAP_ONCE_FINISH_INT,
	PWM_CAP_FINISH_ALL,
} PWM_CAP_INT_METHOD;

void vPWMCapInit(void);
void vPWMCapChEnable(PWM_CAP_CH id);
void vPWMCapChDisable(PWM_CAP_CH id);

//返回频率值(单位Hz)，最大只能采样24Mhz，低4位表示小数部分
//如果只需要整数值将返回值右移4位
uint32_t ulPWMCapGetFreq(PWM_CAP_CH id);

#endif

/**
*
* @file hal_pwm.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/03 14:11
* @author och
*
*/
#ifndef __HAL_PWM_H__
#define __HAL_PWM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int hal_pwm_config(int id, uint32_t duty_ns, uint32_t period_ns);
void hal_pwm_enable(int id);
void hal_pwm_disable(int id);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HAL_PWM_H__
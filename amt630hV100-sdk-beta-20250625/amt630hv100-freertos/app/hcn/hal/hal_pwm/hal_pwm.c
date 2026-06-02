/**
*
* @file hal_pwm.c
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

#include <math.h>
#include "hal_pwm/hal_pwm.h"
#include "pwm.h"

int hal_pwm_config(int id, uint32_t duty_ns, uint32_t period_ns) {
    return pwm_config(id, duty_ns, period_ns);
}

void hal_pwm_enable(int id) {
    pwm_enable(id);
}

void hal_pwm_disable(int id) {
    pwm_disable(id);
}

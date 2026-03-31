/**
*
* @file hcn_ign.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/14 14:45
* @author och
*
*/

#include <FreeRTOS.h>
#include "timers.h"
#include "semphr.h"
#include "shutdown_manage/hcn_ign.h"
#include "shutdown_manage/hcn_shutdown_anim.h"
#include "dashboard_state/hcn_dev_state.h"
#include "hal_gpio/hal_gpio.h"
#include "log/hcn_log.h"

#ifdef HCN_SOC_IGN_ENABLE

#define IGN_DET_GPIO    (58)
#define IGN_EN_GPIO     (5)

///< ign det io检测周期为100ms
#define IGN_CHECK_THRED_PERIOD (100)  

#ifdef HCN_SHUTDOWN_ANIM_ENABLE

///< ign det引脚探测到io断电超过300ms认为断电
#define IGN_OFF_TIME_DIVIDE (300)  
#define IGN_OFF_TIME_DIVIDE_INDEX IGN_OFF_TIME_DIVIDE / IGN_CHECK_THRED_PERIOD
#define SHUTDOWN_ANIM_PERIOD (pdMS_TO_TICKS(3000))

static TimerHandle_t power_off_timer = NULL;
static bool is_delay_play_shutanim = false;

#endif

static ign_state_e ign_state = IGN_OFF_NONE;

ign_state_e get_cur_ign_state(void) {
    return ign_state;
}

void ign_check_scan(void) {
     static bool is_start_acc = false;
    if (hal_gpio_get_input_value(IGN_DET_GPIO) && !is_start_acc) {
        is_start_acc = true;
        ign_state = IGN_OFF_PREPARE;

#ifdef HCN_SHUTDOWN_ANIM_ENABLE
        if (get_check_self_state() >= CHECK_SELF_STATE_START) {
            if (power_off_timer) {
                xTimerReset(power_off_timer, 0);
            }

            hcn_ign_off();
        } else {
            is_delay_play_shutanim = true;
        }
#endif
    } else if (!hal_gpio_get_input_value(IGN_DET_GPIO)) {
        if (is_start_acc) {
            is_start_acc = false;
            ign_state = IGN_OFF_NONE;

#ifdef HCN_SHUTDOWN_ANIM_ENABLE
            if (get_check_self_state() >= CHECK_SELF_STATE_START) {
                if (power_off_timer) {
                    xTimerStop(power_off_timer, 0);
                }

                hcn_ign_on();
            } else {
                is_delay_play_shutanim = false;
            }
#endif
        }
    }

#ifdef HCN_SHUTDOWN_ANIM_ENABLE

    ///< 如果在播放开机动画，延迟播放关机动画
    if (is_delay_play_shutanim) {
        if ((get_check_self_state() >= CHECK_SELF_STATE_START) &&
            !get_boot_animation_status()) {
            hcn_log_info("Delay play shutdown animation!\n");

            is_delay_play_shutanim = false;

            if (power_off_timer) {
                xTimerReset(power_off_timer, 0);
            }

            hcn_ign_off();
        }
    }
#endif
}

void ign_gpio_init(void) {
    ///< 拉住仪表的电源，使仪表不断电
    hal_gpio_set_output(IGN_EN_GPIO, 1);
    hal_gpio_set_input(IGN_EN_GPIO);

#ifdef HCN_SHUT_DOWN_ANIM_ENABLE
    power_off_timer = xTimerCreate("power_timer", SHUTDOWN_ANIM_PERIOD, pdFALSE,
                                   NULL, power_off_timer_task);

#endif
}

#endif
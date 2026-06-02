/**
*
* @file hcn_dev_state.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/25 09:06
* @author och
*
*/

#include "hcn_dev_state.h"
#include "animation.h"
#include <FreeRTOS.h>
#include "timers.h"
#include "semphr.h"
#include "log/hcn_log.h"

//#define DEBUG_HCN_CHECK_SELF

#ifdef DEBUG_HCN_CHECK_SELF

#define USR_TIMER_INTERVAL_PERIOD pdMS_TO_TICKS(1000)

static TimerHandle_t usr_stauts_timer = NULL;
static int timeout_cnt = 0;

#endif

static check_self_state_e current_state = CHECK_SELF_STATE_INIT;

void set_check_self_state(check_self_state_e state) {
    current_state = state;
}

check_self_state_e get_check_self_state(void) {
    return current_state;
}

animation_state_e get_boot_animation_status(void) {
    return (animation_state_e)get_animation_status();
}

#ifdef DEBUG_HCN_CHECK_SELF
static void usr_timer_entry(TimerHandle_t xTimer) {
    timeout_cnt++;
    if (timeout_cnt == 1) {
        hcn_log_info("Test check self start.\r\n");
        set_check_self_state(CHECK_SELF_STATE_START);
    }

    if (timeout_cnt >= 6) {
        hcn_log_info("Test check self success.\r\n");
        set_check_self_state(CHECK_SELF_STATE_SUCCESS);
        xTimerStop(usr_stauts_timer, 0);
    } else {
        xTimerStart(usr_stauts_timer, 0);
    }
}

#endif

void dev_state_init(void) {
#ifdef DEBUG_HCN_CHECK_SELF
     if (!usr_stauts_timer) {
        usr_stauts_timer = xTimerCreate("usr timer", USR_TIMER_INTERVAL_PERIOD,
                                        pdFALSE, NULL, usr_timer_entry);
        if (!usr_stauts_timer) {
            hcn_log_error("Create usr timer failed!\n");
            return;
        }

        xTimerStart(usr_stauts_timer, 0);

        hcn_log_info("Create usr timer success!\n");
    }
#endif
}
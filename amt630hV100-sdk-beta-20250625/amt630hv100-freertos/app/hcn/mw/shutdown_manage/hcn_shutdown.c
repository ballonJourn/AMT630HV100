/**
*
* @file hcn_shutdown.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/14 17:59
* @author och
*
*/

#include <FreeRTOS.h>
#include "timers.h"
#include "shutdown_manage/hcn_shutdown.h"
#include "hal_gpio/hal_gpio.h"
#include "log/hcn_log.h"
#include "uart_communicate/hcn_uart_send_cmd.h"

#ifdef HCN_SHUTDOWN_ANIM_ENABLE
#define POWER_OFF_TIME  (4000)

static TimerHandle_t power_off_timer = NULL;

static uint8_t save_usr_param_before_ign(void) {
    hcn_log_info("save usr param!\n");

    return 0;
}

static void power_off_timer_task(TimerHandle_t xTimer) {
    hcn_log_info("Power off timer!\n");

    if (save_usr_param_before_ign() == 0) {
        hal_gpio_set_output(HCN_LCD_BL_EN_GPIO, 0);

        hcn_log_info("Acc off success!\n");
    }

    send_mcu_shut_down();
}

void hcn_shutdown_init(void) {
    power_off_timer = xTimerCreate("power_timer", pdMS_TO_TICKS(POWER_OFF_TIME), 
                                   pdFALSE, NULL, power_off_timer_task);
    if (!power_off_timer) {
        hcn_log_error("Create power off timer failed!\n");
    }
}

void start_power_off_timer(void) {
    if (power_off_timer) {
        xTimerReset(power_off_timer, 0);
    }
}

void stop_power_off_timer(void) {
    if (power_off_timer) {
        xTimerStop(power_off_timer, 0);
    }      
}   
#endif

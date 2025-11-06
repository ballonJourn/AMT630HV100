/**
*
* @file hcn_shutdown_anim.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/14 17:58
* @author och
*
*/

#include <FreeRTOS.h>
#include <stdbool.h>
#include "task.h"
#include "animation.h"
#include "shutdown_manage/hcn_shutdown_anim.h"
#include "dashboard_state/hcn_dev_state.h"
#include "storage_param1/hcn_usr_param.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#include "hal_gpio/hal_gpio.h"
#include "log/hcn_log.h"

static bool is_ign_on = true;

bool get_ign_is_on(void) {
    return is_ign_on;
}

#ifdef HCN_SHUTDOWN_ANIM_ENABLE
static TaskHandle_t mirror_ctrl_task = NULL;
static bool is_play_shutanim = false;

static void start_mirror_thread(void *param) {
    do {
        vTaskDelay(pdMS_TO_TICKS(100));
    } while (get_boot_animation_status());

    if (is_ign_on) {
        ///< 开启镜像处理
        hcn_log_info("Start mirror task!\n");
    }

    mirror_ctrl_task = NULL;

    vTaskDelete(NULL);
}

#endif

void hcn_ign_on(void) {
#ifdef HCN_SHUTDOWN_ANIM_ENABLE
    animation_stop();

    if (mirror_ctrl_task == NULL) {
        if (xTaskCreate(start_mirror_thread, "start_mirror",
                        512, NULL, 3, &mirror_ctrl_task) != pdPASS) {
            hcn_log_error("Create start mirror thread failed!\n");
            return;
        }
    }
#endif

    ///< 开背光
    hal_gpio_set_output(HCN_LCD_BL_EN_GPIO, 1);
    is_ign_on = true;

    hcn_log_info("hcn ign on.\n");
}

void hcn_ign_off(void) {
    is_ign_on = false;

#ifdef HCN_SHUTDOWN_ANIM_ENABLE
    if (mirror_ctrl_task) {
        vTaskDelete(mirror_ctrl_task);
        mirror_ctrl_task = NULL;
    }

    ///< 停止镜像操作

    animation_init();

    if (is_play_shutanim) {
        animation_restart();
    } else {
        is_play_shutanim = true;
        animation_start();
    }

#else
    ///< 关背光   
    hal_gpio_set_output(HCN_LCD_BL_EN_GPIO, 0);

    if (save_hcn_usr_param() == 0) {
        hcn_log_info("Before acc save usr param success!\r\n");
    } 
    send_mcu_shut_down();

#endif
}

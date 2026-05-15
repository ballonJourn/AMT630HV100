/**
*
* @file hcn_mw_common.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 12:20
* @author och
*
*/

#include <FreeRTOS.h>
#include <string.h>
#include "task.h"
#include "config/hcn_config.h"
#include "mw_init/hcn_mw_common.h"
#include "mw_init/hcn_mw_init.h"    
#include "log/hcn_log.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#include "dashboard_state/hcn_dev_state.h"
#include "backlight/hcn_backlight.h"
#include "storage_param1/hcn_usr_param.h"
#include "ota_manage/hcn_ota_start_sta.h"

#define VEHICLE_THREAD_PERIOD  pdMS_TO_TICKS(100)

#define SYS_PADCTL1_ADDR    (0x000000C4U)
#define SYS_PADCTL0_ADDR    (0x000000C0U)

#if 0
static void read_reg(void) {
    uint32_t reg_val = *((volatile uint32_t *)(REGS_SYSCTL_BASE + SYS_PADCTL1_ADDR));
    hcn_log_info("pad ctrl value = 0x%08X\n", reg_val);
}
#endif

void set_os_date_time(SystemTime_t date_time) {
#ifdef HCN_UART_COMM_ENABLE
    send_mcu_set_time(date_time);
#else
    vSetLocalTime(&date_time);
#endif
}

SystemTime_t get_os_date_time(void) {
#ifdef HCN_UART_COMM_ENABLE
    return get_mcu_time();
#else
    SystemTime_t sys_time;
    memset(sys_time, 0, sizeof(SystemTime_t));

    iGetLocalTime(&sys_time);

    return sys_time;
#endif
}

#if 0
static void test_backlight(void) {
    if (get_check_self_state() >= CHECK_SELF_STATE_SUCCESS) {
        static uint8_t backlight_level = 5;
        static bool is_end = false;
        if (backlight_level >= 2 && (!is_end)) {
            backlight_level--;
            if (backlight_level == 1) {
                is_end = true;
            }
            set_backlight_level(backlight_level);
            return;
        }

        if (backlight_level <= 4 && is_end) { 
            backlight_level++;
            if (backlight_level == 5) {
                is_end = false;
            }
        }
        set_backlight_level(backlight_level);
    }
}

static void test_display_mode(void) {
    if (get_check_self_state() >= CHECK_SELF_STATE_SUCCESS) {
        static bool is_first_set = true;
        uint8_t display_mode = 2;
        if (is_first_set) {
            is_first_set = false;
            set_hcn_usr_param(HCN_PARAM_THEME, &display_mode);
        }
    }
}

static void test_shake_hand(void) {
    send_mcu_heartbeat();
}

static void test_odo_data(void) {
    static uint32_t time_cnt = 0;
    if (get_check_self_state() >= CHECK_SELF_STATE_SUCCESS) {
        if ((time_cnt % 10) == 0) {
            send_mcu_request_odo();
            //send_mcu_request_trip(0);
        } 
        time_cnt++;
    }
}

#endif

extern void ota_mcu_process(void);
extern void amp_off_timer_callback(void);
static void common_io_thread(void *param) {
    light_gpio_init(VEHICLE_THREAD_PERIOD);

    for (;;) {
        vTaskDelay(VEHICLE_THREAD_PERIOD);
        scan_frame_light();
        wifi_mode_switching();
        ota_mcu_process();
        amp_off_timer_callback();
    }
}

void mw_common_init(void) {
    if (xTaskCreate(common_io_thread, "common_io_thread", configMINIMAL_STACK_SIZE*5,
                    NULL, configMAX_PRIORITIES / 4, NULL) != pdPASS) {
        hcn_log_error("create common io task fail.\n");
        return;
    }
}

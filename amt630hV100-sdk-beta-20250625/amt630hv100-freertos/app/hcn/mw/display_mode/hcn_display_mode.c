/**
*
* @file hcn_display_mode.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/29 12:27
* @author och
*
*/

#include <FreeRTOS.h>
#include "task.h"
#include "display_mode/hcn_display_mode.h"
#include "storage_param1/hcn_usr_param.h"
#include "vehicle_param/vehicle_param.h"
#include "light_sensor/hcn_light_sensor.h"
#include "dashboard_state/hcn_dev_state.h"
#include "backlight/hcn_backlight.h"
#include "log/hcn_log.h"

#ifdef HCN_ADC_LIGHT_SENSOR_ENABLE

#define DISPLAY_MODE_PERIOD (50)
#define LIGHT_SENSOR_GET_DATA_INTERVAL (3)

typedef struct {
    int arr_index;
    int sample_interval;
    uint8_t is_first_display;
    uint8_t sensor_level;
    uint8_t cur_display_mode;
} display_param_t;

static display_param_t display = {0, 0, true, 0, 0xFF};

static uint16_t lg_ref[LIGHT_SENSOR_ARR_LEVEL];
static uint16_t lg_value[LIGHT_SENSOR_ARR_SIZE];

static void light_sensor_ref_init(void) {
    lg_ref[0] = 4060;
    lg_ref[1] = 2400;
    lg_ref[2] = 1500;
    lg_ref[3] = 700;
    lg_ref[4] = 100;
    lg_ref[5] = 0;
}

static void check_auto_backlight_level(void) {
    static uint8_t cur_level = 0;
    
    if (!get_recovery_usr_param() || 
        get_check_self_state() < CHECK_SELF_STATE_SUCCESS) {
        return;
    }

    if (!get_hcn_usr_param(HCN_PARAM_BRIGHTNESS_LEVEL, &cur_level)) {
        hcn_log_error("Get usr param backlight failed!\r\n");
        return;
    }

    if (cur_level == BACKLIGHT_LEVEL_AUTO) {
        if (display.sensor_level > 1 && cur_level != BACKLIGHT_LEVEL_4) {
            set_backlight_level(BACKLIGHT_LEVEL_4);
        } else if (display.sensor_level <= 1 && cur_level != BACKLIGHT_LEVEL_1) {
            set_backlight_level(BACKLIGHT_LEVEL_1);
        }
    }
}

static void check_auto_healight(void) {
    static uint8_t last_headlight = 0;
    if (display.sensor_level <= 1 && last_headlight != 1){
        vehicle_set_data(VEH_AUTO_HEADLIGH, 1);
        last_headlight = 1;
    } else if (display.sensor_level > 1 && last_headlight != 2) {
        vehicle_set_data(VEH_AUTO_HEADLIGH, 0);
        last_headlight = 2;
    }       
}

static void check_display_mode(void) {
    uint8_t display_mode = 0;

    if (!get_recovery_usr_param()) {
        return;
    }

    if (!get_hcn_usr_param(HCN_PARAM_THEME, &display_mode)) {
        hcn_log_error("Get usr param display mode failed!\r\n");
        return;
    }

    if (display_mode == AUTO_MODE) {
        int target_display = display.sensor_level > 1 ? DAY_MODE : NIGHT_MODE;
        if (display.cur_display_mode != target_display) {
            display.cur_display_mode = target_display;
            vehicle_set_data(VEH_CUR_DISPALY_MODE, display.cur_display_mode);
            hcn_log_info("Current display mode: %s %d\n",
                   display.cur_display_mode == DAY_MODE ? "Day" : "Night",
                   display.cur_display_mode);
        }
    } else {
        if (display.cur_display_mode != display_mode) {
            display.cur_display_mode = display_mode;
            vehicle_set_data(VEH_CUR_DISPALY_MODE, display.cur_display_mode);
        }
    }
}

static void check_sensor_level(void) {
    if (is_acc_start() && display.is_first_display) {
        display.is_first_display = false;
        if (lg_value[0] <= lg_ref[1]) {
            display.sensor_level = 2;
        } else {
            display.sensor_level = 1;
        }
    } else {
        for (int i = 0; i < LIGHT_SENSOR_ARR_LEVEL - 1; i++) {
            int success_count = 0;

            for (int j = 0; j < LIGHT_SENSOR_ARR_SIZE; j++) {
                if (lg_value[j] > lg_ref[i + 1] && lg_value[j] <= lg_ref[i]) {
                    success_count++;
                } else {
                    break;
                }

                if (success_count == LIGHT_SENSOR_ARR_LEVEL - 1) {
                    display.sensor_level = i + 1;
                }
            }
        }
    }
    
    check_display_mode();
    check_auto_backlight_level();
    check_auto_healight();
}

static void check_light_sensor(void) {
    if ((display.sample_interval % LIGHT_SENSOR_GET_DATA_INTERVAL) == 0) {
        if (display.arr_index < LIGHT_SENSOR_ARR_SIZE) {
            lg_value[display.arr_index] = (uint16_t)get_light_sensor_value();
            display.arr_index++;
        }

        if (display.arr_index >= LIGHT_SENSOR_ARR_SIZE) {
            display.arr_index = 0;
        }

        check_sensor_level();
    }
    display.sample_interval++;
}

static void display_mode_thread(void *param) {
    light_sensor_ref_init();
    //auto_backlight_init();

    for (;;) {
        set_light_sensor_value();
        check_light_sensor();
        //check_auto_backlight_level();
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_MODE_PERIOD));
    }
}

void display_mode_init(void) {
    if (xTaskCreate(display_mode_thread, "display_mode",
                    configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES / 5,
                    NULL) != pdPASS) {
        hcn_log_error("Create display thread failed!\n");
    }

    return;
}

#endif
/**
*
* @file hcn_gpio_light.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/09 15:50
* @author och
*
*/

#include "io_module/hcn_gpio_light.h"
#include "dashboard_state/hcn_dev_state.h"
#include "vehicle_param/vehicle_param.h"
#include "log/hcn_log.h"
#include "gpio.h"

///< gpio刷新周期为100ms,需要防止在100ms任务中
#define VEH_IO_CHECK_PERIOD (500)

///< gpio探测脚
#define VEH_LEFT_TURN_DET_GPIO (95)
#define VEH_RIGHT_TURN_DET_GPIO (94)
#define VEH_HIGH_BEAM_DET_GPIO (93)
#define VEH_LOW_BEAM_DET_GPIO   (92)

///< 控制灯光的gpio
#define VEH_LEFT_TURN_GPIO (54)
#define VEH_RIGHT_TURN_GPIO (25)
#define VEH_HIGH_BEAM_GPIO (56)
#define VEH_POSITION_GPIO (55)
#define VEH_OIL_PRESSURE_GPIO (24)
#define VEH_ABS_GPIO (23)
#define VEH_OBD_GPIO (52)
#define VEH_N_GEAR_GPIO (26)
#define VEH_WATER_TEMP_GPIO (53)

#ifdef GPIO_FILTER_ENABLE

///< gpio过滤时间值
#define GPIO_FILTER_TIME (300)

///< gpio过滤计数值
#define GPIO_FILTER_CNT(period) (GPIO_FILTER_TIME / period)

static uint8_t gpio_last_value[10] = {0};
static uint8_t gpio_count[10] = {0};
#endif

///< 灯光io刷新周期值
static int thread_period = 0;

static void set_frame_light(int gpio_id, int value) {

    switch (gpio_id) {
        case VEH_LEFT_TURN_DET_GPIO:
            vehicle_set_data(VEH_INDICATOR_TURN_LEFT, !value);
            break;

        case VEH_RIGHT_TURN_DET_GPIO:
            vehicle_set_data(VEH_INDICATOR_TURN_RIGHT, !value);
            break;

        case VEH_HIGH_BEAM_DET_GPIO:
            vehicle_set_data(VEH_LIGHT_HIGH_BEAM, !value);
            break;

        case VEH_LOW_BEAM_DET_GPIO:
            vehicle_set_data(VEH_LIGHT_LOW_BEAM, !value);
            break;

        default:
            break;
    }
}

/**
 * @brief  检测外框灯
 * @param  gpio_id IO编号
 * @param  io_index 过滤io序号
 * @param  is_filter 是否需要进行io过滤
 * @return none
 */
static void check_frame_light(int gpio_id, int io_index, bool is_filter) {
    int value = gpio_get_value(gpio_id);

    if (is_filter) {
        if (gpio_last_value[io_index] == value) {
            if (gpio_count[io_index] >= GPIO_FILTER_CNT(thread_period)) {
                if (get_check_self_state() == CHECK_SELF_STATE_SUCCESS) {
                    set_frame_light(gpio_id, value);
                }
            } else {
                gpio_count[io_index] += 1;
            }
        } else {
            gpio_last_value[io_index] = value;
            gpio_count[io_index] = 1;
        }
    } else {
        if (get_check_self_state() != CHECK_SELF_STATE_SUCCESS) {
            return;
        }

        set_frame_light(gpio_id, value);
    }
}

void scan_frame_light(void) {
    static int time_interval = 0;
    int base_value = VEH_IO_CHECK_PERIOD / thread_period;
    if ((time_interval % base_value) == 0) {
        check_frame_light(VEH_LEFT_TURN_DET_GPIO, 0, false);
        check_frame_light(VEH_RIGHT_TURN_DET_GPIO, 1, false);
        check_frame_light(VEH_HIGH_BEAM_DET_GPIO, 2, false);
        check_frame_light(VEH_LOW_BEAM_DET_GPIO, 3, false);
    }
    time_interval++;
}

void ctrl_frame_light_start_state(led_state_e state) {
    gpio_direction_output(VEH_LEFT_TURN_GPIO, state);
    gpio_direction_output(VEH_RIGHT_TURN_GPIO, state);
    gpio_direction_output(VEH_HIGH_BEAM_GPIO, state);
    gpio_direction_output(VEH_POSITION_GPIO, state);
    gpio_direction_output(VEH_OIL_PRESSURE_GPIO, state);
    gpio_direction_output(VEH_ABS_GPIO, state);
    gpio_direction_output(VEH_OBD_GPIO, state);
    gpio_direction_output(VEH_N_GEAR_GPIO, state);
    gpio_direction_output(VEH_WATER_TEMP_GPIO, state);
}

void ctrl_frame_light(void) {
    float temp = 0;
    if (vehicle_get_data(VEH_INDICATOR_TURN_LEFT)) {
        gpio_direction_output(VEH_LEFT_TURN_GPIO, 1);
    } else {
        gpio_direction_output(VEH_LEFT_TURN_GPIO, 0);
    }

    if (vehicle_get_data(VEH_INDICATOR_TURN_RIGHT)) {
        gpio_direction_output(VEH_RIGHT_TURN_GPIO, 1);
    } else {
        gpio_direction_output(VEH_RIGHT_TURN_GPIO, 0);
    }

    if (vehicle_get_data(VEH_LIGHT_HIGH_BEAM)) {
        gpio_direction_output(VEH_HIGH_BEAM_GPIO, 1);
    } else {
        gpio_direction_output(VEH_HIGH_BEAM_GPIO, 0);
    }

    if (vehicle_get_data(VEH_LIGHT_LOCATION)) {
        gpio_direction_output(VEH_POSITION_GPIO, 1);
    } else {
        gpio_direction_output(VEH_POSITION_GPIO, 0);
    }

    if (vehicle_get_data(VEH_LIGHT_OIL_PRESSURE)) {
        gpio_direction_output(VEH_OIL_PRESSURE_GPIO, 1);
    } else {
        gpio_direction_output(VEH_OIL_PRESSURE_GPIO, 0);
    }

    if (vehicle_get_data(VEH_LIGHT_ABS)) {
        gpio_direction_output(VEH_ABS_GPIO, 1);
    } else {
        gpio_direction_output(VEH_ABS_GPIO, 0);
    }

    if (vehicle_get_data(VEH_LIGHT_ENGINE_FAULT)) {
        gpio_direction_output(VEH_OBD_GPIO, 1);
    } else {
        gpio_direction_output(VEH_OBD_GPIO, 0);
    }

    if (vehicle_get_data(VEH_GEAR_POSITION) == 0) {
        gpio_direction_output(VEH_N_GEAR_GPIO, 1);
    } else {
        gpio_direction_output(VEH_N_GEAR_GPIO, 0);
    }

    temp = vehicle_get_data(VEH_TEMP_WATER) / 10.0;
    if (temp >= 120.0) {
        gpio_direction_output(VEH_WATER_TEMP_GPIO, 1);
    } else if (temp < 118.0) {
        gpio_direction_output(VEH_WATER_TEMP_GPIO, 0);
    }
}

void light_gpio_init(int period) {
    gpio_direction_input(VEH_LEFT_TURN_DET_GPIO);
    gpio_direction_input(VEH_RIGHT_TURN_DET_GPIO);
    gpio_direction_input(VEH_HIGH_BEAM_DET_GPIO);
    gpio_direction_input(VEH_LOW_BEAM_DET_GPIO);

    if (period > VEH_IO_CHECK_PERIOD) {
        hcn_log_error("Thread cycle too slow!\n");
        return;
    }

    thread_period = period;
}

/**
*
* @file hcn_backlight.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/03 14:12
* @author och
*
*/

#include <stdlib.h>
#include "backlight/hcn_backlight.h"
#include "hal_pwm/hal_pwm.h"
#include "config/hcn_config.h"
#include "log/hcn_log.h"

#if 0
#define LEVEL_1_DUTY_VALUE (500000)
#define LEVEL_2_DUTY_VALUE (400000)
#define LEVEL_3_DUTY_VALUE (600000)
#define LEVEL_4_DUTY_VALUE (200000)
#define LEVEL_5_DUTY_VALUE (100000)
#else
#define LEVEL_1_DUTY_VALUE (200000)
#define LEVEL_2_DUTY_VALUE (150000)
#define LEVEL_3_DUTY_VALUE (100000)
#define LEVEL_4_DUTY_VALUE (50000)
#define LEVEL_5_DUTY_VALUE (1)
#endif
#define BREATH_BACKLIGHT_PERIOD (80)

typedef struct {
    uint32_t pre_led_value;
    uint32_t cur_led_value;
    uint32_t target_led_value;
    uint32_t inc_led_value;
} breath_bl_param_t;

static uint8_t led_level = 0;
static uint8_t auto_led_level = 0;
static breath_bl_param_t auto_backlight;

static uint32_t get_dutu_cycle_value(uint8_t level) {
    uint32_t value = LEVEL_1_DUTY_VALUE;

    switch (level) {
        case 1:
            value = LEVEL_1_DUTY_VALUE;
            break;

        case 2:
        case 3:
            value = LEVEL_3_DUTY_VALUE;
            break;

        case 4:
        case 5:
            value = LEVEL_5_DUTY_VALUE;
            break;

        default:
            break;
    }

    return value;
}

void set_backlight_level(uint8_t level) {
    if (level != led_level) {
        if (level >= BACKLIGHT_LEVEL_1 && level <= BACKLIGHT_LEVEL_5) {
            uint32_t level_value = LEVEL_3_DUTY_VALUE;
            switch (level) {
                case 1:
                    level_value = LEVEL_1_DUTY_VALUE;
                    break;

                case 2:
                    level_value = LEVEL_2_DUTY_VALUE;
                    break;

                case 3:
                    level_value = LEVEL_3_DUTY_VALUE;
                    break;

                case 4:
                    level_value = LEVEL_4_DUTY_VALUE;
                    break;

                case 5:
                    level_value = LEVEL_5_DUTY_VALUE;
                    break;

                default:
                    break;
            }

            hal_pwm_config(HCN_LCD_PWM_CH, level_value, PWM_BACKLIGHT_PERION);
            hal_pwm_enable(HCN_LCD_PWM_CH);

            auto_backlight.target_led_value = level_value;
            auto_backlight.cur_led_value = level_value;
            hcn_log_info("Set pwm value:%d\n", level_value);
            led_level = level;
        }
    }
}

void init_auto_backlight(uint8_t level) {
    if (auto_led_level != level) {
        uint32_t level_value = get_dutu_cycle_value(level);
        hal_pwm_config(HCN_LCD_PWM_CH, level_value, PWM_BACKLIGHT_PERION);
        hal_pwm_enable(HCN_LCD_PWM_CH);

        auto_backlight.target_led_value = level_value;
        auto_backlight.cur_led_value = level_value;
        auto_led_level = level;
    }
}

void init_backlight_pwm(void) {
    hal_pwm_config(HCN_LCD_PWM_CH, PWM_BACKLIGHT_PERION, PWM_BACKLIGHT_PERION);
    hal_pwm_enable(HCN_LCD_PWM_CH);
    hcn_log_info("init pwm value:1000000\r\n");
}

void update_breath_backlight_level(uint8_t level) {
    auto_backlight.pre_led_value = auto_backlight.target_led_value;
    auto_backlight.target_led_value = get_dutu_cycle_value(level);
    if (auto_backlight.target_led_value > LEVEL_1_DUTY_VALUE) {
        auto_backlight.target_led_value = LEVEL_1_DUTY_VALUE;
    }

    if (auto_backlight.target_led_value < LEVEL_5_DUTY_VALUE) {
        auto_backlight.target_led_value = LEVEL_5_DUTY_VALUE;
    }

    if (auto_backlight.cur_led_value > LEVEL_1_DUTY_VALUE) {
        auto_backlight.cur_led_value = LEVEL_1_DUTY_VALUE;
    }

    if (auto_backlight.cur_led_value < LEVEL_5_DUTY_VALUE) {
        auto_backlight.cur_led_value = LEVEL_5_DUTY_VALUE;
    }

    if (auto_backlight.pre_led_value != auto_backlight.target_led_value) {
        auto_backlight.inc_led_value =
            (uint32_t)abs(auto_backlight.target_led_value -
                          auto_backlight.cur_led_value) /
            BREATH_BACKLIGHT_PERIOD;
        if (auto_backlight.inc_led_value == 0) {
            auto_backlight.inc_led_value = 1;    
        }
    }

    if (auto_backlight.cur_led_value <= auto_backlight.target_led_value) {
        auto_backlight.cur_led_value += auto_backlight.inc_led_value;
        if (auto_backlight.cur_led_value > auto_backlight.target_led_value) {
            auto_backlight.cur_led_value = auto_backlight.target_led_value;
        }

        hal_pwm_config(HCN_LCD_PWM_CH, auto_backlight.cur_led_value, PWM_BACKLIGHT_PERION);
        hal_pwm_enable(HCN_LCD_PWM_CH);
    } else if (auto_backlight.cur_led_value > auto_backlight.target_led_value) {
        auto_backlight.cur_led_value -= auto_backlight.inc_led_value;
        if (auto_backlight.cur_led_value < auto_backlight.target_led_value) {
            auto_backlight.cur_led_value = auto_backlight.target_led_value;
        }

        hal_pwm_config(HCN_LCD_PWM_CH, auto_backlight.cur_led_value, PWM_BACKLIGHT_PERION);
        hal_pwm_enable(HCN_LCD_PWM_CH);
    }
}

bool is_change_backlight_success(uint8_t level) {
    uint32_t temp = get_dutu_cycle_value(level);
    return (temp == auto_backlight.cur_led_value);
}



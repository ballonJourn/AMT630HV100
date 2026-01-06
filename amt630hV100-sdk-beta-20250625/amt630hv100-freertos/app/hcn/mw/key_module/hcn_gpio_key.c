/**
*
* @file hcn_gpio_key.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/27 15:24
* @author och
*
*/

#include <FreeRTOS.h>
#include <stdbool.h>
#include "key_module/hcn_gpio_key.h"
#include "key_module/hcn_key_common.h"
#include "log/hcn_log.h"
#include "task.h"
#include "gpio.h"

#ifdef HCN_IO_KEY_ENABLE

#define SCAN_KEY_THREAD_INTERVAL_PERIOD (20)

///< defirate_time 60ms
#define SCAN_KEY_DEFIBRATE_TIME (60)

///< more than 2500 ms 
#define GPIO_KEY_LONG_PRESS_TIME (2500)

///< super long press time 10000ms(10s)
#define GPIO_KEY_SUPER_LONG_PRESS_TIME (10000)

#define SCAN_KEY_DEFIBRATE_INDEX \
    (SCAN_KEY_DEFIBRATE_TIME / SCAN_KEY_THREAD_INTERVAL_PERIOD)
#define GPIO_KEY_LONG_PRESS_INDEX \
    (GPIO_KEY_LONG_PRESS_TIME / SCAN_KEY_THREAD_INTERVAL_PERIOD)

#define GPIO_KEY_SUPER_LONG_PRESS_INDEX \
    (GPIO_KEY_SUPER_LONG_PRESS_TIME / SCAN_KEY_THREAD_INTERVAL_PERIOD)

#define SYS_TIME_GET() xTaskGetTickCount()

typedef struct {
    uint32_t defibrate_time;
    uint32_t scan_key_time;
    bool is_real_key_press;
    bool is_long_press;
} scan_key_time_t;

static scan_key_time_t scan_key = {0, 0, false, false};
static uint8_t key_up_flag = 1;
static uint8_t key_status = NO_KEY_PRESS;

static void check_key_status(uint8_t key_status, uint8_t mode) {
    uint8_t key_value = 0;

    switch (key_status) {
        case UP_KEY_PRESS:
            key_value = mode ? UP_KEY_LONG_PR : UP_KEY_SHORT_PR;
            printf("%s\n", mode ? "up key long press" : "up key short press");
            send_key_event(key_value);
            break;

        case DOWN_KEY_PRESS:
            key_value = mode ? MODE_KEY_LONG_PR : MODE_KEY_SHORT_PR;
            printf("%s\n",
                   mode ? "down key long press" : "down key short press");
            send_key_event(key_value);
            break;

        case ENTER_KEY_PRESS:
            if (mode == SHORT_PRESS) {
                key_value = SET_KEY_SHORT_PR;
                printf("enter key short press\n");
            } else if (mode == LONG_PRESS) {
                key_value = SET_KEY_LONG_PR;
                printf("enter key long press\n");
            } else if (mode == SUPER_LONG_PRESS) {
                key_value = SET_KEY_SUPER_LONG_PR;
                printf("enter key super long press\n");
            }
            send_key_event(key_value);
            break;

        case BACK_KEY_PRESS:
            key_value = mode ? BACK_KEY_LONG_PR : BACK_KEY_SHORT_PR;
            printf("%s\n",
                   mode ? "back key long press" : "back key short press");
            send_key_event(key_value);
            break;

        default:
            break;
    }
}

static void scan_gpio_key(void) {
    if (key_up_flag && (!UP_KEY || !DOWN_KEY || !ENTER_KEY || !BACK_KEY)) {
        if (scan_key.defibrate_time <= SCAN_KEY_DEFIBRATE_INDEX - 1) {
            scan_key.defibrate_time++;
        }

        if (scan_key.defibrate_time >= SCAN_KEY_DEFIBRATE_INDEX) {
            key_up_flag = 0;
            scan_key.is_real_key_press = true;
        }
    }

    if (scan_key.is_real_key_press &&
        (!UP_KEY || !DOWN_KEY || !ENTER_KEY || !BACK_KEY)) {
        scan_key.scan_key_time++;
        if (!UP_KEY) {
            key_status = UP_KEY_PRESS;
        } else if (!DOWN_KEY) {
            key_status = DOWN_KEY_PRESS;
        } else if (!ENTER_KEY) {
            key_status = ENTER_KEY_PRESS;
        } else if (!BACK_KEY) {
            key_status = BACK_KEY_PRESS;
        }

        if ((scan_key.scan_key_time >= GPIO_KEY_LONG_PRESS_INDEX) &&
            (!scan_key.is_long_press)) {
            scan_key.is_long_press = true;

            ///< 如果是enter键，特殊处理,不需要置标志
            if (key_status != ENTER_KEY_PRESS) {
                scan_key.is_real_key_press = false;
            } 
            check_key_status(key_status, LONG_PRESS);
            key_status = NO_KEY_PRESS;
        }

         ///< enter键的超长按
        if (key_status == ENTER_KEY_PRESS && 
            (scan_key.scan_key_time >= GPIO_KEY_SUPER_LONG_PRESS_INDEX)
            && (scan_key.is_long_press)) {
            scan_key.is_long_press = false;
            scan_key.is_real_key_press = false;
            check_key_status(key_status, SUPER_LONG_PRESS); 
            key_status = NO_KEY_PRESS;
        }
    }

    if (UP_KEY && DOWN_KEY && ENTER_KEY && BACK_KEY) {
        key_up_flag = 1;
        scan_key.defibrate_time = 0;
        scan_key.is_long_press = false;
        scan_key.is_real_key_press = false;

        ///< 处理enter键的释放逻辑
        if (key_status == ENTER_KEY_PRESS) {
             if (scan_key.scan_key_time < GPIO_KEY_LONG_PRESS_INDEX) {
                check_key_status(key_status, SHORT_PRESS);
             }
        }

        if (key_status != ENTER_KEY_PRESS) {
            if (scan_key.scan_key_time < GPIO_KEY_LONG_PRESS_INDEX) {
                if (key_status > NO_KEY_PRESS && key_status <= BACK_KEY_PRESS) {
                    check_key_status(key_status, SHORT_PRESS);
                }
            }
        }
     
        scan_key.scan_key_time = 0;
        key_status = NO_KEY_PRESS;
    }
}

#if 0
static void reset_io_key_status(void) {
    key_up_flag = 1;
    memset(&scan_key, 0, sizeof(scan_key_time_t));
    key_status = NO_KEY_PRESS;
}
#endif

static void gpio_key_thread(void *param) {
    hal_gpio_set_input(KEY_MODE_GPIO);
    hal_gpio_set_input(KEY_SET_GPIO);
    hal_gpio_set_input(KEY_UP_GPIO);
    hal_gpio_set_input(KEY_BACK_GPIO);
    for (;;) {
        scan_gpio_key();
        vTaskDelay(pdMS_TO_TICKS(SCAN_KEY_THREAD_INTERVAL_PERIOD));
    }
}

int gpio_key_init(void) {
    if (xTaskCreate(gpio_key_thread, "gpio_key_thread",
                    configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES / 2,
                    NULL) != pdPASS) {
        hcn_log_error("Create gpio_key_thread failed.\n");
        return -1;
    }

    hcn_log_info("gpio key init ok!\n");

    return 0;
}

#endif //HCN_IO_KEY_ENABLE
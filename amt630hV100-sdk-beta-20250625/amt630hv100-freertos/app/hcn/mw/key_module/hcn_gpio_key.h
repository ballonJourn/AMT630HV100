/**
*
* @file hcn_gpio_key.h
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
#ifndef __HCN_GPIO_KEY_H__
#define __HCN_GPIO_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal_gpio/hal_gpio.h"

#define KEY_UP    (0)
#define KEY_DOWN  (1)  

#define KEY_MODE_GPIO           (15)
#define KEY_UP_GPIO             (14)
#define KEY_SET_GPIO            (13)
#define KEY_BACK_GPIO           (12)

#define  UP_KEY    hal_gpio_get_input_value(KEY_UP_GPIO)
#define  DOWN_KEY  hal_gpio_get_input_value(KEY_MODE_GPIO)
#define  ENTER_KEY hal_gpio_get_input_value(KEY_SET_GPIO)
#define  BACK_KEY  hal_gpio_get_input_value(KEY_BACK_GPIO)

/**
 * @brief 按键按下状态枚举
 */
typedef enum {
    SHORT_PRESS,
    LONG_PRESSS,
} key_press_mode_e;

/**
 * @brief 按键种类枚举
 */
typedef enum {
   NO_KEY_PRESS = 0,
   UP_KEY_PRESS = 1,
   DOWN_KEY_PRESS = 2,
   ENTER_KEY_PRESS = 3,
   BACK_KEY_PRESS = 4,
} key_press_e;

/**
 * @brief 按键事件枚举
 */
typedef enum {
    NO_KEY_EVENT = 0x0,
    MODE_KEY_SHORT_PR = 0x01,
    MODE_KEY_LONG_PR = 0x02,
    SET_KEY_SHORT_PR = 0x03,
    SET_KEY_LONG_PR = 0x04,
    COM_KEY_SHORT_PR = 0x05,
    COM_KEY_LONG_PR = 0x06,
    UP_KEY_SHORT_PR = 0x07,
    UP_KEY_LONG_PR = 0x08,
    DOWN_KEY_SHORT_PR = 0x09,
    DOWN_KEY_LONG_PR = 0x0A,
    ENTER_KEY_SHORT_PR = 0x10,
    ENTER_KEY_LONG_PR = 0x11,
    BACK_KEY_SHORT_PR = 0x12,
    BACK_KEY_LONG_PR = 0x13,
    COM_KEY_SHORT_PR1 = 0x14,
    COM_KEY_LONG_PR1 = 0x15,
} gpio_key_event_e;

typedef uint8_t key_event_t_;
typedef void (*key_event_cb_t)(key_event_t_);

/**
 * @brief  gpio 按键初始化
 * @param  none
 * @return 0:成功 -1:失败 
 */
int gpio_key_init(void);

/**
 * @brief  设置按键事件回调
 * @param  event_cb 按键事件回调函数
 * @return 0:成功 -1:失败  1:已经初始过
 */
int set_key_event_cb(key_event_cb_t event_cb);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_GPIO_KEY_H__
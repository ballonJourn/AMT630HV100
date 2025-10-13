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
#include "config/hcn_config.h"

#ifdef HCN_IO_KEY_ENABLE

#define KEY_UP    (0)
#define KEY_DOWN  (1)  

#define KEY_MODE_GPIO           (5)
#define KEY_UP_GPIO             (1)
#define KEY_SET_GPIO            (49)
#define KEY_BACK_GPIO           (4)

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
 * @brief  gpio 按键初始化
 * @param  none
 * @return 0:成功 -1:失败 
 */
int gpio_key_init(void);


#endif //HCN_IO_KEY_ENABLE

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_GPIO_KEY_H__
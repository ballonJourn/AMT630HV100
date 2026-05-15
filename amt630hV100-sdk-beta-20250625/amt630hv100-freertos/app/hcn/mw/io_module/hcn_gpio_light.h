/**
*
* @file hcn_gpio_light.h
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

#ifndef __HCN_GPIO_LIGHT_H__
#define __HCN_GPIO_LIGHT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define GPIO_FILTER_ENABLE

typedef enum {
    FRAME_LIGHT_OFF,
    FRAME_LIGHT_ON,
} led_state_e;

void scan_frame_light(void);

/**
 * @brief  灯光gpio初始化
 * @param  [in] id: 灯光刷新周期
 * @return no
 */
void light_gpio_init(int period);

/**
 * @brief  控制外框灯状态
 * @param  [in] state: FRAME_LIGHT_OFF:off FRAME_LIGHT_ON:on
 * @return no
 */
void ctrl_frame_light_start_state(led_state_e state);

void ctrl_frame_light(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_GPIO_LIGHT_H__
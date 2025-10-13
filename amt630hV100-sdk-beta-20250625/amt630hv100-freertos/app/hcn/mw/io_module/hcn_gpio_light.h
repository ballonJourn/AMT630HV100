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

typedef union {
    uint32_t flag;
    struct {
        uint32_t left_turn : 1;   ///< 左转信号
        uint32_t right_turn : 1;  ///< 右转信号
        uint32_t high_beam : 1;   ///< 远光信号
        uint32_t position : 1;    ///< 位置灯信号
        uint32_t n_gear : 1;      ///< 空挡信号
        uint32_t oil_pressure :1; ///< 机油压力
        uint32_t abs :1;          ///< ABS灯
        uint32_t obd :1;          ///< OBD故障灯
    };
} light_info_t;

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
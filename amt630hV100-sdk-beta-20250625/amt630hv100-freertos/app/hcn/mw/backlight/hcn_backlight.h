/**
*
* @file hcn_backlight.h
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
#ifndef __HCN_BACKLIGHT_H__
#define __HCN_BACKLIGHT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define PWM_BACKLIGHT_PERION (1000000)  ///< pwm背光周期1MHZ

typedef enum {
   BACKLIGHT_LEVEL_AUTO = 0x00,  ///< 等级自动
   BACKLIGHT_LEVEL_1, 
   BACKLIGHT_LEVEL_2,
   BACKLIGHT_LEVEL_3,
   BACKLIGHT_LEVEL_4,
   BACKLIGHT_LEVEL_5
} led_level_e;

void set_backlight_level(uint8_t level);

void init_backlight_pwm(void);

/**
 * @brief  init auto backlight
 * @param  level: backlight level
 * @return none
 */
void init_auto_backlight(uint8_t level);

/**
 * @brief  update auto backlight level
 * @param  level: backlight level
 * @return none
 */
void update_breath_backlight_level(uint8_t level);

/**
 * @brief  judge auto backlight level is change success
 * @param  level: backlight level
 * @return true:change success false: is changing backlight 
 */
bool is_change_backlight_success(uint8_t level);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_BACKLIGHT_H__
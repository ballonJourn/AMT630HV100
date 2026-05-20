/**
*
* @file hal_gpio.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/27 16:30
* @author och
*
*/
#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    GPIO_INPUT_E,
    GPIO_OUTPUT_E,
} hal_gpio_dir_e;

int hal_gpio_get_input_value(uint32_t gpio_pin);
void hal_gpio_set_output(uint32_t gpio_pin, int value);
void hal_gpio_set_input(uint32_t gpio_pin);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HAL_GPIO_H__
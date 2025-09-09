/**
*
* @file hal_gpio.c
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

#include "gpio.h"
#include "hal_gpio/hal_gpio.h"

int hal_gpio_get_input_value(uint32_t gpio_pin) {
    return gpio_get_value(gpio_pin);
}

void hal_gpio_set_output(uint32_t gpio_pin, int value) {
    gpio_direction_output(gpio_pin, value);
}

void hal_gpio_set_input(uint32_t gpio_pin) {
    gpio_direction_input(gpio_pin);
}

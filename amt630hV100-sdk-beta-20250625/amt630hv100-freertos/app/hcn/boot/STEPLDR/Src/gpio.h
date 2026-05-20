#ifndef _GPIO_H
#define _GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

void gpio_request(unsigned gpio);

void gpio_direction_output(unsigned gpio, int value);

void gpio_direction_input(unsigned gpio);

void gpio_set_value(unsigned gpio, int value);

int gpio_get_value(unsigned gpio);


#ifdef __cplusplus
}
#endif

#endif

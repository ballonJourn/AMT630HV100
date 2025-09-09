#ifndef _GPIO_H
#define _GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	GPIOIRQ_TYPE_EDGE_BOTH,
	GPIOIRQ_TYPE_EDGE_RISING,
	GPIOIRQ_TYPE_EDGE_FALLING,
	GPIOIRQ_TYPE_LEVEL_HIGH,
	GPIOIRQ_TYPE_LEVEL_LOW,	
} eGpioIrqType;

typedef void (*ISRFunction_t)( void *param );

void gpio_request(unsigned gpio);

void gpio_direction_output(unsigned gpio, int value);

void gpio_direction_input(unsigned gpio);

void gpio_set_value(unsigned gpio, int value);

int gpio_get_value(unsigned gpio);

int gpio_irq_request(unsigned gpio, int irq_type, ISRFunction_t irq_handler, void *param);

int gpio_irq_free(unsigned gpio);


#ifdef __cplusplus
}
#endif

#endif

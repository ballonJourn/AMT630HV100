#ifndef _I2C_GPIO_H
#define _I2C_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

struct i2c_gpio_platform_data {
	int		devid;
	unsigned int	sda_pin;
	unsigned int	scl_pin;
	int 	udelay;
	int 	timeout;
	unsigned int	sda_is_open_drain:1;
	unsigned int	scl_is_open_drain:1;
	unsigned int	scl_is_output_only:1;
};

struct i2c_algo_bit_data {
	void *data; 	/* private data for lowlevel routines */
	void (*setsda) (void *data, int state);
	void (*setscl) (void *data, int state);
	int  (*getsda) (void *data);
	int  (*getscl) (void *data);

	/* local settings */
	int udelay; 	/* half clock cycle time in us,
				   minimum 2 us for fast-mode I2C,
				   minimum 5 us for standard-mode I2C and SMBus,
				   maximum 50 us for SMBus */
	int timeout;		/* in jiffies */
};

void i2c_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif

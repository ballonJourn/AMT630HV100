#include <string.h>
#include "FreeRTOS.h"
#include "amt630h.h"
#include "board.h"
#include "timer.h"
#include "i2c.h"
#include "i2c-gpio.h"


#ifdef I2C_SUPPORT
#define MAX_I2C_DEVICE_NUM		4

static struct i2c_adapter *i2c_devs[MAX_I2C_DEVICE_NUM] = {NULL};
static int i2c_devices_count = 0;

int i2c_add_adapter(struct i2c_adapter *adap)
{
	if (i2c_devices_count >= MAX_I2C_DEVICE_NUM)
		return -1;

	/* Set default timeout to 1 second if not already set */
	if (adap->timeout == 0)
		adap->timeout = configTICK_RATE_HZ;

	i2c_devs[i2c_devices_count++] = adap;

	return 0;
}

struct i2c_adapter *i2c_open(const char *i2cdev)
{
	struct i2c_adapter *adap;
	int i;

	for (i = 0; i < i2c_devices_count; i++) {
		adap = i2c_devs[i];
		if (!strcmp(adap->name, i2cdev)) {
			adap->open_count++;
			return adap;
		}
	}

	return NULL;
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	unsigned long orig_jiffies;
	int ret, try;

	configASSERT(adap && msgs);

	/* Retry automatically on arbitration loss */
	orig_jiffies = get_timer(0);
	for (ret = 0, try = 0; try <= adap->retries; try++) {
		ret = adap->algo->master_xfer(adap, msgs, num);
		if (ret != -EAGAIN)
			break;
		if (get_timer(orig_jiffies) > adap->timeout)
			break;
	}

	return ret;
}

void i2c_init(void)
{
#ifdef ANALOG_I2C_SUPPORT
	i2c_gpio_init();
#endif
}
#endif

#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "errno.h"

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
			if (adap->open_count == 1)
				adap->xMutex = xSemaphoreCreateMutex();
			return adap;
		}
	}

	return NULL;
}

void i2c_close(struct i2c_adapter *adap)
{
 	if (adap && --adap->open_count == 0)
		vSemaphoreDelete(adap->xMutex);
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	unsigned long orig_jiffies;
	int ret, try;

	configASSERT(adap && msgs);

	xSemaphoreTake(adap->xMutex, portMAX_DELAY);

	/* Retry automatically on arbitration loss */
	orig_jiffies = xTaskGetTickCount();
	for (ret = 0, try = 0; try <= adap->retries; try++) {
		ret = adap->algo->master_xfer(adap, msgs, num);
		if (ret != -EAGAIN)
			break;
		if (xTaskGetTickCount() > orig_jiffies + adap->timeout)
			break;
	}

	xSemaphoreGive(adap->xMutex);

	return ret;
}

int i2c_slave_register(struct i2c_adapter *adap, u8 addr, i2c_slave_cb_t slave_cb)
{
	int ret;

	if (!adap || !slave_cb)
		return -EINVAL;

	if (!(adap->flags & I2C_CLIENT_SLAVE))
		TRACE_WARNING("%s: client slave flag not set.\n", __func__);

	if (!adap->algo->reg_slave) {
		printf("%s: not supported by adapter\n", __func__);
		return -ENOTSUP;
	}

	adap->slave_cb = slave_cb;
	adap->addr = addr;

	ret = adap->algo->reg_slave(adap);

	if (ret) {
		adap->slave_cb = NULL;
		printf("%s: adapter returned error %d\n", __func__, ret);
	}

	return ret;
}

int i2c_slave_unregister(struct i2c_adapter *adap)
{
	int ret;

	if (!adap)
		return -EINVAL;

	if (!adap->algo->unreg_slave) {
		printf("%s: not supported by adapter\n", __func__);
		return -ENOTSUP;
	}

	ret = adap->algo->unreg_slave(adap);

	if (ret == 0)
		adap->slave_cb = NULL;
	else
		printf("%s: adapter returned error %d\n", __func__, ret);

	return ret;
}

void i2c_init(void)
{
#ifdef HW_I2C0_SUPPORT
	i2c_dw_init(0);
#endif
#ifdef HW_I2C1_SUPPORT
	i2c_dw_init(1);
#endif
#ifdef ANALOG_I2C_SUPPORT
	i2c_gpio_init();
#endif
}

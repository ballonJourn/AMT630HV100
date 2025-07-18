#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"

#ifdef I2C_EEPROM_SLAVE_DEMO
#if !defined(I2C0_SLAVE_MODE) && !defined(I2C1_SLAVE_MODE)
#error "i2c eeprom slave demo needs a slave mode i2c dev"
#endif

#define I2C_EEPROM_24C02_ADDR	0x50
#define I2C_EEPROM_24C02_SIZE	256

struct eeprom_data {
	bool first_write;
	u8 buffer_idx;
	u8 buffer[];
};

static struct eeprom_data *g_eeprom = NULL;
static struct i2c_adapter *g_adap = NULL;

static int i2c_slave_eeprom_slave_cb(struct i2c_adapter *adap,
				     enum i2c_slave_event event, u8 *val)
{
	struct eeprom_data *eeprom = g_eeprom;

	switch (event) {
	case I2C_SLAVE_WRITE_RECEIVED:
		if (eeprom->first_write) {
			eeprom->buffer_idx = *val;
			eeprom->first_write = false;
		} else {
			eeprom->buffer[eeprom->buffer_idx++] = *val;
		}
		break;

	case I2C_SLAVE_READ_REQUESTED:
		*val = eeprom->buffer[eeprom->buffer_idx++];
		/*
		 * Do not increment buffer_idx here, because we don't know if
		 * this byte will be actually used. Read Linux I2C slave docs
		 * for details.
		 */
		break;

	case I2C_SLAVE_STOP:
	case I2C_SLAVE_WRITE_REQUESTED:
		eeprom->first_write = true;
		break;

	default:
		break;
	}

	return 0;
}

int i2c_slave_eeprom_init(void)
{
	struct eeprom_data *eeprom;
	int ret;
	unsigned size = I2C_EEPROM_24C02_SIZE;
	struct i2c_adapter *adap = NULL;

	eeprom = pvPortMalloc(sizeof(struct eeprom_data) + size);
	if (!eeprom)
		return -ENOMEM;
	g_eeprom = eeprom;

	eeprom->first_write = true;
#ifdef I2C0_SLAVE_MODE
	adap = i2c_open("i2c0");
#else
	adap = i2c_open("i2c1");
#endif
	if (!adap) {
		printf("%s open i2c fail.\n", __func__);
		vPortFree(eeprom);
		return -1;
	}
	g_adap = adap;

	ret = i2c_slave_register(adap, I2C_EEPROM_24C02_ADDR, i2c_slave_eeprom_slave_cb);
	if (ret) {
		return ret;
	}

	return 0;
};

int i2c_slave_eeprom_uninit(void)
{
	if (g_adap)
		i2c_slave_unregister(g_adap);

	if (g_eeprom)
		vPortFree(g_eeprom);

	return 0;
}

#endif

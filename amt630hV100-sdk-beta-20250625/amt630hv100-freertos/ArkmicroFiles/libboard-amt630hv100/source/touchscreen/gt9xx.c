#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"


#ifdef TP_USE_GT9XX
#if !defined(VG_ONLY) && !defined(AWTK)
#include "lvgl/lvgl.h"
#else
typedef int16_t lv_coord_t;
typedef uint8_t lv_indev_state_t;
typedef struct {
    lv_coord_t x;
    lv_coord_t y;
} lv_point_t;

enum { LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR };

enum {
    LV_KEY_UP        = 17,  /*0x11*/
    LV_KEY_DOWN      = 18,  /*0x12*/
    LV_KEY_RIGHT     = 19,  /*0x13*/
    LV_KEY_LEFT      = 20,  /*0x14*/
    LV_KEY_ESC       = 27,  /*0x1B*/
    LV_KEY_DEL       = 127, /*0x7F*/
    LV_KEY_BACKSPACE = 8,   /*0x08*/
    LV_KEY_ENTER     = 10,  /*0x0A, '\n'*/
    LV_KEY_NEXT      = 9,   /*0x09, '\t'*/
    LV_KEY_PREV      = 11,  /*0x0B, '*/
    LV_KEY_HOME      = 2,   /*0x02, STX*/
    LV_KEY_END       = 3,   /*0x03, ETX*/
};

typedef struct {
    lv_point_t point; /**< For LV_INDEV_TYPE_POINTER the currently pressed point*/
    uint32_t key;     /**< For LV_INDEV_TYPE_KEYPAD the currently pressed key*/
    uint32_t btn_id;  /**< For LV_INDEV_TYPE_BUTTON the currently pressed button*/
    int16_t enc_diff; /**< For LV_INDEV_TYPE_ENCODER number of steps since the previous read*/

    lv_indev_state_t state; /**< LV_INDEV_STATE_REL or LV_INDEV_STATE_PR*/
} lv_indev_data_t;
#endif

#define GOODIX_GPIO_INT_NAME		"irq"
#define GOODIX_GPIO_RST_NAME		"reset"

#define GOODIX_MAX_HEIGHT		4096
#define GOODIX_MAX_WIDTH		4096
#define GOODIX_INT_TRIGGER		1
#define GOODIX_CONTACT_SIZE		8
#define GOODIX_MAX_CONTACTS		10

#define GOODIX_CONFIG_MAX_LENGTH	240
#define GOODIX_CONFIG_911_LENGTH	186
#define GOODIX_CONFIG_967_LENGTH	228

/* Register defines */
#define GOODIX_REG_COMMAND		0x8040
#define GOODIX_CMD_SCREEN_OFF		0x05

#define GOODIX_READ_COOR_ADDR		0x814E
#define GOODIX_GT1X_REG_CONFIG_DATA	0x8050
#define GOODIX_GT9X_REG_CONFIG_DATA	0x8047
#define GOODIX_REG_ID			0x8140

#define GOODIX_BUFFER_STATUS_READY	BIT(7)
#define GOODIX_BUFFER_STATUS_TIMEOUT	20

#define RESOLUTION_LOC		1
#define MAX_CONTACTS_LOC	5
#define TRIGGER_LOC			6

#define GT9XX_SLAVE_ADDR		0x5d
#define GT9XX_GPIO_INT			TP_GPIO_INT
#define GT9XX_GPIO_RST			TP_GPIO_RST
#define GT9XX_INVERTED_X		TP_INV_X
#define GT9XX_INVERTED_Y		TP_INV_Y
#define GT9XX_MT_TOUCH			TP_MT_TOUCH//支持多点触摸

struct goodix_ts_data;

struct goodix_chip_data {
	u16 config_addr;
	int config_len;
};

struct goodix_ts_data {
	struct i2c_adapter *adap;
	const struct goodix_chip_data *chip;
	unsigned int max_touch_num;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int int_trigger_type;
	int gpio_int;
	int gpio_rst;
	u16 id;
	u16 version;
	const char cfg_name[32];
	unsigned long irq_flags;
	TaskHandle_t irq_task;
};

static const struct goodix_chip_data gt1x_chip_data = {
	.config_addr		= GOODIX_GT1X_REG_CONFIG_DATA,
	.config_len		= GOODIX_CONFIG_MAX_LENGTH,
};

static const struct goodix_chip_data gt911_chip_data = {
	.config_addr		= GOODIX_GT9X_REG_CONFIG_DATA,
	.config_len		= GOODIX_CONFIG_911_LENGTH,
};

static const struct goodix_chip_data gt967_chip_data = {
	.config_addr		= GOODIX_GT9X_REG_CONFIG_DATA,
	.config_len		= GOODIX_CONFIG_967_LENGTH,
};

static const struct goodix_chip_data gt9x_chip_data = {
	.config_addr		= GOODIX_GT9X_REG_CONFIG_DATA,
	.config_len		= GOODIX_CONFIG_MAX_LENGTH,
};

static const unsigned long goodix_irq_flags[] = {
	IRQ_TYPE_EDGE_RISING,
	IRQ_TYPE_EDGE_FALLING,
	IRQ_TYPE_LEVEL_LOW,
	IRQ_TYPE_LEVEL_HIGH,
};

static int last_input_x, last_input_y;

extern void SendTouchInputEvent(lv_indev_data_t *indata);

#ifdef AWTK
#include <xm_base.h>
#include <xm_event.h>
#endif

static inline int get_unaligned_le16(void *p)
{
	u8 *tmp = (u8*)p;

	return (tmp[1] << 8) | tmp[0];
}

/**
 * goodix_i2c_read - read data from a register of the i2c slave device.
 *
 * @adap: i2c device.
 * @reg: the register to read from.
 * @buf: raw write data buffer.
 * @len: length of the buffer to write
 */
static int goodix_i2c_read(struct i2c_adapter *adap,
			   u16 reg, u8 *buf, int len)
{
	struct i2c_msg msgs[2];
	u8 wbuf[2];
	int ret;

	wbuf[0] = reg >> 8;
	wbuf[1] = reg & 0xFF;

	msgs[0].flags = 0;
	msgs[0].addr  = GT9XX_SLAVE_ADDR;
	msgs[0].len   = 2;
	msgs[0].buf   = (u8 *)&wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = GT9XX_SLAVE_ADDR;
	msgs[1].len   = len;
	msgs[1].buf   = buf;

	ret = i2c_transfer(adap, msgs, 2);
	return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

/**
 * goodix_i2c_write - write data to a register of the i2c slave device.
 *
 * @adap: i2c device.
 * @reg: the register to write to.
 * @buf: raw data buffer to write.
 * @len: length of the buffer to write
 */
static int goodix_i2c_write(struct i2c_adapter *adap, u16 reg, const u8 *buf,
			    unsigned len)
{
	u8 *addr_buf;
	struct i2c_msg msg;
	int ret;

	addr_buf = pvPortMalloc(len + 2);
	if (!addr_buf)
		return -ENOMEM;

	addr_buf[0] = reg >> 8;
	addr_buf[1] = reg & 0xFF;
	memcpy(&addr_buf[2], buf, len);

	msg.flags = 0;
	msg.addr = GT9XX_SLAVE_ADDR;
	msg.buf = addr_buf;
	msg.len = len + 2;

	ret = i2c_transfer(adap, &msg, 1);
	vPortFree(addr_buf);
	return ret < 0 ? ret : (ret != 1 ? -EIO : 0);
}

static int goodix_i2c_write_u8(struct i2c_adapter *adap, u16 reg, u8 value)
{
	return goodix_i2c_write(adap, reg, &value, sizeof(value));
}

static const struct goodix_chip_data *goodix_get_chip_data(u16 id)
{
	switch (id) {
	case 1151:
		return &gt1x_chip_data;

	case 911:
	case 9271:
	case 9110:
	case 927:
	case 928:
		return &gt911_chip_data;

	case 912:
	case 967:
		return &gt967_chip_data;

	default:
		return &gt9x_chip_data;
	}
}

static int goodix_ts_read_input_report(struct goodix_ts_data *ts, u8 *data)
{
	unsigned long max_timeout;
	int touch_num;
	int error;

	/*
	 * The 'buffer status' bit, which indicates that the data is valid, is
	 * not set as soon as the interrupt is raised, but slightly after.
	 * This takes around 10 ms to happen, so we poll for 20 ms.
	 */
	max_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(GOODIX_BUFFER_STATUS_TIMEOUT);
	do {
		error = goodix_i2c_read(ts->adap, GOODIX_READ_COOR_ADDR,
					data, GOODIX_CONTACT_SIZE + 1);
		if (error) {
			TRACE_ERROR("I2C transfer error: %d\n", error);
			return error;
		}

		if (data[0] & GOODIX_BUFFER_STATUS_READY) {
			touch_num = data[0] & 0x0f;
			if (touch_num > ts->max_touch_num)
				return -EPROTO;
#if GT9XX_MT_TOUCH
			if (touch_num > 1) {
				data += 1 + GOODIX_CONTACT_SIZE;
				error = goodix_i2c_read(ts->adap,
						GOODIX_READ_COOR_ADDR +
							1 + GOODIX_CONTACT_SIZE,
						data,
						GOODIX_CONTACT_SIZE *
							(touch_num - 1));
				if (error)
					return error;
			}

			return touch_num;
#else
			return touch_num > 0 ? 1 : 0;
#endif

		}

		vTaskDelay(pdMS_TO_TICKS(1)); /* Poll every 1 - 2 ms */
	} while (xTaskGetTickCount() < max_timeout);

	/*
	 * The Goodix panel will send spurious interrupts after a
	 * 'finger up' event, which will always cause a timeout.
	 */
	return 0;
}

static void goodix_ts_report_touch(struct goodix_ts_data *ts, u8 *coor_data)
{
	//int id = coor_data[0] & 0x0F;
	int input_x = get_unaligned_le16(&coor_data[1]);
	int input_y = get_unaligned_le16(&coor_data[3]);
	int input_w = get_unaligned_le16(&coor_data[5]);

	/* printf("id=0x%x, input_x=%d, input_y=%d, input_w=%d.\n", id, input_x,
		input_y, input_w); */

#if GT9XX_INVERTED_X
	input_x = ts->x_max - input_x;
#endif

#if GT9XX_INVERTED_Y
	input_y = ts->y_max - input_y;
#endif

	//send press event
#ifdef AWTK
	XM_TpEventProc (XM_EVENT_TOUCHDOWN, input_x, input_y, 0);
#else
	lv_indev_data_t indata = {0};
	indata.point.x = input_x;
	indata.point.y = input_y;
	indata.state = LV_INDEV_STATE_PR;
	SendTouchInputEvent(&indata);
#endif

	last_input_x = input_x;
	last_input_y = input_y;
}

/**
 * goodix_process_events - Process incoming events
 *
 * @ts: our goodix_ts_data pointer
 *
 * Called when the IRQ is triggered. Read the current device state, and push
 * the input events to the user space.
 */
static void goodix_process_events(struct goodix_ts_data *ts)
{
	u8  point_data[1 + GOODIX_CONTACT_SIZE * GOODIX_MAX_CONTACTS];
	int touch_num;
	int i;

	touch_num = goodix_ts_read_input_report(ts, point_data);
	if (touch_num < 0)
		return;

	/*
	 * Bit 4 of the first byte reports the status of the capacitive
	 * Windows/Home button.
	 */
	//input_report_key(ts->input_dev, KEY_LEFTMETA, point_data[0] & BIT(4));

	for (i = 0; i < touch_num; i++)
		goodix_ts_report_touch(ts,
				&point_data[1 + GOODIX_CONTACT_SIZE * i]);

	if (touch_num == 0) {
		//send release event
#ifdef AWTK
		XM_TpEventProc (XM_EVENT_TOUCHUP, last_input_x, last_input_y, 0);
#else
		lv_indev_data_t indata = {0};
		indata.point.x = last_input_x;
		indata.point.y = last_input_y;
		indata.state = LV_INDEV_STATE_REL;
		SendTouchInputEvent(&indata);
#endif
	}

	/* input_mt_sync_frame(ts->input_dev);
	input_sync(ts->input_dev); */
}

static void goodix_ts_irq_task(void *param)
{
	uint32_t ulNotifiedValue;
	struct goodix_ts_data *ts;

	for (;;) {
		xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
					0xffffffff, /* Reset the notification value to 0 on exit. */
					&ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
					portMAX_DELAY);

		ts = (struct goodix_ts_data *)ulNotifiedValue;

		goodix_process_events(ts);

		if (goodix_i2c_write_u8(ts->adap, GOODIX_READ_COOR_ADDR, 0) < 0)
			TRACE_ERROR("I2C write end_cmd error\n");
	}
}

/**
 * goodix_ts_irq_handler - The IRQ handler
 *
 * @param: private data pointer.
 */
static void goodix_ts_irq_handler(void *param)
{
	struct goodix_ts_data *ts = param;

	xTaskNotifyFromISR(ts->irq_task, (uint32_t)ts, eSetValueWithOverwrite, 0);

	return;
}

static int goodix_request_irq(struct goodix_ts_data *ts)
{
	if (xTaskCreate(goodix_ts_irq_task, "gt9xx", 1024, ts,
			10, &ts->irq_task) != pdPASS) {
		printf("create gt9xx irq task fail.\n");
	}

	return gpio_irq_request(ts->gpio_int, ts->irq_flags, goodix_ts_irq_handler, ts);
}

static int goodix_int_sync(struct goodix_ts_data *ts)
{
	gpio_direction_output(ts->gpio_int, 0);

	vTaskDelay(pdMS_TO_TICKS(50));				/* T5: 50ms */

	gpio_direction_input(ts->gpio_int);

	return 0;
}

/**
 * goodix_reset - Reset device during power on
 *
 * @ts: goodix_ts_data pointer
 */
static int goodix_reset(struct goodix_ts_data *ts)
{
	int error;

	/* begin select I2C slave addr */
	gpio_direction_output(ts->gpio_rst, 0);

	vTaskDelay(pdMS_TO_TICKS(20));				/* T2: > 10ms */

	/* HIGH: 0x28/0x29, LOW: 0xBA/0xBB */
	gpio_direction_output(ts->gpio_int, GT9XX_SLAVE_ADDR == 0x14);

	vTaskDelay(pdMS_TO_TICKS(1));		/* T3: > 100us */

	gpio_direction_output(ts->gpio_rst, 1);

	vTaskDelay(pdMS_TO_TICKS(6));		/* T4: > 5ms */

	/* end select I2C slave addr */
	gpio_direction_input(ts->gpio_rst);

	error = goodix_int_sync(ts);
	if (error)
		return error;

	return 0;
}

/**
 * goodix_read_config - Read the embedded configuration of the panel
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called during probe
 */
static void goodix_read_config(struct goodix_ts_data *ts)
{
	u8 config[GOODIX_CONFIG_MAX_LENGTH];
	int error;

	error = goodix_i2c_read(ts->adap, ts->chip->config_addr,
				config, ts->chip->config_len);
	if (error) {
		TRACE_WARNING("Error reading config: %d\n", error);
		ts->int_trigger_type = GOODIX_INT_TRIGGER;
		ts->max_touch_num = GOODIX_MAX_CONTACTS;
		return;
	}

	ts->int_trigger_type = config[TRIGGER_LOC] & 0x03;
	ts->max_touch_num = config[MAX_CONTACTS_LOC] & 0x0f;

	ts->x_max = get_unaligned_le16(&config[RESOLUTION_LOC]);
	ts->y_max = get_unaligned_le16(&config[RESOLUTION_LOC + 2]);
}

/**
 * goodix_read_version - Read goodix touchscreen version
 *
 * @ts: our goodix_ts_data pointer
 */
static int goodix_read_version(struct goodix_ts_data *ts)
{
	int error;
	u8 buf[6];
	char id_str[5];

	error = goodix_i2c_read(ts->adap, GOODIX_REG_ID, buf, sizeof(buf));
	if (error) {
		TRACE_ERROR("read version failed: %d\n", error);
		return error;
	}

	memcpy(id_str, buf, 4);
	id_str[4] = 0;
	char *ptr;
	ts->id = strtoul(id_str, &ptr, 10);
	if (ts->id == 0xffff)
		ts->id = 0x1001;

	ts->version = get_unaligned_le16(&buf[4]);

	TRACE_INFO("ID %d, version: %04x\n", ts->id, ts->version);

	return 0;
}

/**
 * goodix_i2c_test - I2C test function to check if the device answers.
 *
 * @adap: the i2c adapter
 */
static int goodix_i2c_test(struct i2c_adapter *adap)
{
	int retry = 0;
	int error;
	u8 test;

	while (retry++ < 2) {
		error = goodix_i2c_read(adap, GOODIX_REG_ID,
					&test, 1);
		if (!error)
			return 0;

		TRACE_ERROR("i2c test failed attempt %d: %d\n",
			retry, error);
		vTaskDelay(pdMS_TO_TICKS(20));
	}

	return error;
}

/**
 * goodix_configure_dev - Finish device initialization
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called from probe to finish initialization of the device.
 * Contains the common initialization code for both devices that
 * declare gpio pins and devices that do not. It is either called
 * directly from probe or from request_firmware_wait callback.
 */
static int goodix_configure_dev(struct goodix_ts_data *ts)
{
	int error;

	ts->int_trigger_type = GOODIX_INT_TRIGGER;
	ts->max_touch_num = GOODIX_MAX_CONTACTS;

	/* Read configuration and apply touchscreen parameters */
	goodix_read_config(ts);

	ts->irq_flags = goodix_irq_flags[ts->int_trigger_type];
	error = goodix_request_irq(ts);
	if (error) {
		TRACE_ERROR("request IRQ failed: %d\n", error);
		return error;
	}

	return 0;
}

int goodix_ts_probe(struct i2c_adapter *adap)
{
	struct goodix_ts_data *ts;
	int error;

	TRACE_DEBUG("I2C Address: 0x%02x\n", GT9XX_SLAVE_ADDR);

	ts = pvPortMalloc(sizeof(*ts));
	if (!ts)
		return -ENOMEM;
	memset(ts, 0, sizeof(*ts));

	ts->adap = adap;
	ts->gpio_int = GT9XX_GPIO_INT;
	ts->gpio_rst = GT9XX_GPIO_RST;

	if (ts->gpio_int && ts->gpio_rst) {
		/* reset the controller */
		error = goodix_reset(ts);
		if (error) {
			TRACE_ERROR("Controller reset failed.\n");
			return error;
		}
	}

	error = goodix_i2c_test(adap);
	if (error) {
		TRACE_ERROR("I2C communication failure: %d\n", error);
		return error;
	}

	error = goodix_read_version(ts);
	if (error) {
		TRACE_ERROR("Read version failed.\n");
		return error;
	}

	ts->chip = goodix_get_chip_data(ts->id);

	error = goodix_configure_dev(ts);
	if (error)
		return error;

	return 0;
}

int goodix_init(void)
{
	struct i2c_adapter *adap = NULL;

	if (!(adap = i2c_open("i2c0"))) {
		printf("open i2c0 fail.\n");
		return -1;
	}

	goodix_ts_probe(adap);

	return 0;
}
#endif

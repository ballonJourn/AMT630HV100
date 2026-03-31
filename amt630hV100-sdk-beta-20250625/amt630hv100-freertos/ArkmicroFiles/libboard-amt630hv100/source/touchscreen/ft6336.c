#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "timers.h"


#ifdef TP_USE_FT6336U
#define FT6336U_STAT_ADDR 				0x02
#define FT6336U_TOUCH1_ADDR				0x03
#define FT6336U_TOUCH2_ADDR 			0x09
#define FT6336U_TOUCH3_ADDR				0x0F
#define FT6336U_TOUCH4_ADDR 			0x15
#define FT6336U_TOUCH5_ADDR 			0x1B
#define FT6336U_CHIP_ID		 			0xA3

#define FT6336U_CONFIG_X_MAX			960
#define FT6336U_CONFIG_Y_MAX			320

#define FT6336U_SLAVE_ADDR				0x38
#define FT6336U_GPIO_INT				TP_GPIO_INT
#define FT6336U_GPIO_RST				TP_GPIO_RST
#define FT6336U_INVERTED_X				TP_INV_X
#define FT6336U_INVERTED_Y				TP_INV_Y
#if TP_MT_TOUCH
#define FT6336U_MAX_CONTACTS			5
#else
#define FT6336U_MAX_CONTACTS			1
#endif
//#define FT6336U_USE_RELEASE_TIMER
//#define FT6336U_REPEAT_FILTER

#define GPIO_IS_VALID(n)				((n >= 0) && (n < 128) ? 1 : 0)
#define FT6336U_ERR(fmt, args...)		printf("###ERR:[%s]"fmt, __func__, ##args)
#define FT6336U_DBG(fmt, args...)		//printf("DBG:[%s]"fmt, __func__, ##args)

enum {
	FT6336U_INDEV_STATE_DOWN,
	FT6336U_INDEV_STATE_UP,
	FT6336U_INDEV_STATE_CONTACT
};

struct ft6336u_ts_data {
	struct i2c_adapter *adap;
	unsigned int max_touch_num;
	unsigned int x_max;
	unsigned int y_max;
	int last_input_x;
	int last_input_y;
	int last_input_state;
	int gpio_int;
	int gpio_rst;
	uint8_t id;
	unsigned long irq_flags;
	TaskHandle_t irq_task;
#ifdef FT6336U_USE_RELEASE_TIMER
	TimerHandle_t release_timer;
#endif
};

typedef struct _ft6336u_point{
	int16_t x;
	int16_t y;
	uint8_t state;
} ft6336u_point_t;

#if !defined(VG_ONLY) && !defined(AWTK)
#include "lvgl/lvgl.h"
#else
enum { LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR };

typedef int16_t lv_coord_t;
typedef uint8_t lv_indev_state_t;

typedef struct {
	lv_coord_t x;
	lv_coord_t y;
} lv_point_t;

typedef struct {
	lv_point_t point;	/**< For LV_INDEV_TYPE_POINTER the currently pressed point*/
	uint32_t key;		/**< For LV_INDEV_TYPE_KEYPAD the currently pressed key*/
	uint32_t btn_id;	/**< For LV_INDEV_TYPE_BUTTON the currently pressed button*/
	int16_t enc_diff;	/**< For LV_INDEV_TYPE_ENCODER number of steps since the previous read*/
	lv_indev_state_t state;	/**< LV_INDEV_STATE_REL or LV_INDEV_STATE_PR*/
} lv_indev_data_t;
#endif

extern void SendTouchInputEvent(lv_indev_data_t *indata);
extern void SendTouchInputEventFromISR(lv_indev_data_t *indata);

static int ft6336u_i2c_read(struct i2c_adapter *adap, uint8_t reg, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr = adap->addr;
	msgs[0].len = 1;
	msgs[0].buf = &reg;
	msgs[1].flags = I2C_M_RD;
	msgs[1].addr = adap->addr;
	msgs[1].len = len;
	msgs[1].buf = buf;
	ret = i2c_transfer(adap, msgs, 2);

	return ((ret != 2) ? -EIO : 0);
}

#if 0
static int ft6336u_i2c_write(struct i2c_adapter *adap, const uint8_t *buf, int len)
{
	struct i2c_msg msg;
	int ret;

	msg.flags = !I2C_M_RD;
	msg.addr = adap->addr;
	msg.buf = buf;
	msg.len = len + 1;
	ret = i2c_transfer(adap, &msg, 1);

	return ((ret != 1) ? -EIO : 0);
}
#endif

static int ft6336u_ts_read_input_report(struct ft6336u_ts_data *ts)
{
	uint8_t counter = 0;
	uint8_t lvalue;
	uint8_t ts_num;

	do {
		//make sure data in buffer is valid
		ft6336u_i2c_read(ts->adap, FT6336U_STAT_ADDR, &lvalue, 1);
		if(counter++ == 0x30)
			return 0;
	} while(lvalue & 0x08);

	ts_num = (uint8_t)(lvalue & 0x0F);

	if(ts_num > FT6336U_MAX_CONTACTS)
		ts_num = FT6336U_MAX_CONTACTS;

//	FT6336U_DBG("points num:%d\n", ts_num);

	return ts_num;
}

static int ft6336u_ts_report_touch(struct ft6336u_ts_data *ts, int id)
{
	ft6336u_point_t point;
	uint8_t values[4];
	const uint8_t x_base[] = {
		FT6336U_TOUCH1_ADDR,
		FT6336U_TOUCH2_ADDR,
		FT6336U_TOUCH3_ADDR,
		FT6336U_TOUCH4_ADDR,
		FT6336U_TOUCH5_ADDR
	};

	if(ft6336u_i2c_read(ts->adap, x_base[id], values, 4) != 0) {
		FT6336U_ERR("i2c read coordinate failed\n");
		return -1;
	}

	point.state = (values[0]>>6) & 0x3;
	point.x = (((uint16_t)(values[0]&0x0f)) << 8) | values[1];
	point.y = (((uint16_t)(values[2]&0x0f)) << 8) | values[3];

	/*invalid point val*/
	if((point.x > ts->x_max) || (point.y > ts->y_max) || (point.state > FT6336U_INDEV_STATE_CONTACT)) {
		FT6336U_ERR("Coordinate out of range: x:%d, y:%d\n", point.x, point.y);
		return -1;
	}

#if FT6336U_INVERTED_X
	point.x = ts->x_max - point.x;
#endif
#if FT6336U_INVERTED_Y
	point.y = ts->y_max - point.y;
#endif

#ifdef FT6336U_REPEAT_FILTER
	if(point.state == FT6336U_INDEV_STATE_CONTACT) {
		if((ts->last_input_x == point.x) && (ts->last_input_y == point.y)) {
			return 0;
		} else {
			//move event.
		}
	}
#endif

	//send press event
#ifdef AWTK
	XM_TpEventProc (XM_EVENT_TOUCHDOWN, point.x, point.y, 0);
#else
	lv_indev_data_t indata = {0};
	indata.point.x = point.x;
	indata.point.y = point.y;

	if(point.state == FT6336U_INDEV_STATE_CONTACT) {
		indata.state = LV_INDEV_STATE_PR;
	} else if(point.state == FT6336U_INDEV_STATE_DOWN) {
		indata.state = LV_INDEV_STATE_PR;
		//printf("++++++ id[%d]: x:%d, y:%d\n", id, point.x, point.y);
	} else /*if(point.state == FT6336U_INDEV_STATE_UP)*/ {
		indata.state = LV_INDEV_STATE_REL;
		//printf("------ id[%d]: x:%d, y:%d\n", id, point.x, point.y);
	}

	SendTouchInputEvent(&indata);
#endif

	ts->last_input_x = point.x;
	ts->last_input_y = point.y;
	ts->last_input_state = point.state;

//	FT6336U_DBG("id[%d]:x:%d, y:%d, state:0x%x\n", id, point.x, point.y, point.state);

	return 0;
}

static void ft6336u_process_events(struct ft6336u_ts_data *ts)
{
	uint8_t ts_num, i;

	ts_num = ft6336u_ts_read_input_report(ts);

	for(i=0; i<ts_num; i++) {
		ft6336u_ts_report_touch(ts, i);
	}

	if(ts_num == 0) {	/* report the last point release event */
#ifdef AWTK
		XM_TpEventProc (XM_EVENT_TOUCHUP, ts->last_input_x, ts->last_input_y, 0);
#else
		lv_indev_data_t indata = {0};
		indata.point.x = ts->last_input_x;
		indata.point.y = ts->last_input_y;
		indata.state = LV_INDEV_STATE_REL;
		SendTouchInputEvent(&indata);
#endif
	}
}

#ifdef FT6336U_USE_RELEASE_TIMER
static void ft6336_timeout_callback(TimerHandle_t timer)
{
	struct ft6336u_ts_data *ts = (struct ft6336u_ts_data *)pvTimerGetTimerID(timer);
	if(!ts) {
		FT6336U_ERR("Invalid ts.\n");
		return;
	}

	if(ts->last_input_state != FT6336U_INDEV_STATE_UP) {	//drop release event.
#ifdef AWTK
		XM_TpEventProc (XM_EVENT_TOUCHUP, ts->last_input_x, ts->last_input_y, 0);
#else
		lv_indev_data_t indata = {0};
		indata.point.x = ts->last_input_x;
		indata.point.y = ts->last_input_y;
		indata.state = LV_INDEV_STATE_REL;
		SendTouchInputEventFromISR(&indata);
#endif
		FT6336U_ERR("Force report release event\n");
	}
}
#endif

static void ft6336u_ts_irq_task(void *param)
{
	uint32_t ulNotifiedValue;
	struct ft6336u_ts_data *ts;

	for(;;) {
		xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
					0xffffffff, /* Reset the notification value to 0 on exit. */
					&ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
					portMAX_DELAY);

		ts = (struct ft6336u_ts_data *)ulNotifiedValue;
		if(!ts) {
			FT6336U_ERR("Invalid ts.\n");
			continue;
		}

#ifdef FT6336U_USE_RELEASE_TIMER
		if(ts->release_timer)
			xTimerStop(ts->release_timer, 0);
#endif
		ft6336u_process_events(ts);

#ifdef FT6336U_USE_RELEASE_TIMER
		if(ts->release_timer)
		{
			xTimerReset(ts->release_timer, 0);
			xTimerStart(ts->release_timer, 0);
		}
#endif
	}
}

static void ft6336u_ts_irq_handler(void *param)
{
	struct ft6336u_ts_data *ts = (struct ft6336u_ts_data *)param;

	if(ts)
		xTaskNotifyFromISR(ts->irq_task, (uint32_t)ts, eSetValueWithOverwrite, 0);
}

static int ft6336u_request_irq(struct ft6336u_ts_data *ts)
{
	int ret = -1;

	if(GPIO_IS_VALID(ts->gpio_int)) {
		gpio_direction_input(ts->gpio_int);
		if(gpio_irq_request(ts->gpio_int, ts->irq_flags, ft6336u_ts_irq_handler, ts)) {
			FT6336U_ERR("gpio irq request failed\n");
			return -1;
		}
		ret = 0;
	}

	return ret;
}

static int ft6336u_configure_dev(struct ft6336u_ts_data *ts)
{
	int ret;

	ret = xTaskCreate(ft6336u_ts_irq_task, "ft6336u irq task", 1024, ts, 10, &ts->irq_task);
	if(ret != pdPASS) {
		FT6336U_ERR("create ft6336u irq task fail.\n");
		return -1;
	}

	ret = ft6336u_request_irq(ts);
	if(ret) {
		FT6336U_ERR("request IRQ failed\n");
		if(ts->irq_task)
			vTaskDelete(ts->irq_task);
		return ret;
	}

	return 0;
}

static int ft6336u_read_chip_id(struct ft6336u_ts_data *ts)
{
	int retry = 3;
	int ret;

	do {
		ret = ft6336u_i2c_read(ts->adap, FT6336U_CHIP_ID, &ts->id, 1);
		if(ret == 0) {
			FT6336U_DBG("FT6336U ID %d\n", ts->id);
			return 0;
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	} while(retry--);

	return -1;
}

static int ft6336u_get_chip_para(struct ft6336u_ts_data *ts)
{
	int ret;

	ret = ft6336u_read_chip_id(ts);
	if(ret) {
		FT6336U_ERR("read chip id failed\n");
		return ret;
	}

	if(ts->id != 0x64) {
		FT6336U_ERR("Invalid chip ID:0x%2x\n", ts->id);
		return -1;
	}

	ts->irq_flags		= IRQ_TYPE_EDGE_FALLING;
	ts->max_touch_num	= FT6336U_MAX_CONTACTS;
	ts->x_max			= FT6336U_CONFIG_X_MAX;
	ts->y_max			= FT6336U_CONFIG_Y_MAX;

	return 0;
}

static int ft6336u_reset(struct ft6336u_ts_data *ts)
{
	if(GPIO_IS_VALID(ts->gpio_rst)) {
		gpio_direction_output(ts->gpio_rst, 0);
		vTaskDelay(pdMS_TO_TICKS(10));				/* Trst: > 5ms */
		gpio_direction_output(ts->gpio_rst, 1);
		vTaskDelay(pdMS_TO_TICKS(200));				/* tpon: 200ms */
		return 0;
	}

	return -1;
}

static int ft6336u_ts_probe(struct i2c_adapter *adap)
{
	struct ft6336u_ts_data *ts;
	int ret;

	ts = pvPortMalloc(sizeof(*ts));
	if(!ts) {
		FT6336U_ERR("malloc failed.\n");
		return -ENOMEM;
	}
	memset(ts, 0, sizeof(*ts));

	ts->adap 		= adap;
	ts->adap->addr	= FT6336U_SLAVE_ADDR;
	ts->gpio_int	= FT6336U_GPIO_INT;
	ts->gpio_rst	= FT6336U_GPIO_RST;

	ret = ft6336u_reset(ts);
	if(ret) {
		FT6336U_ERR("Controller reset failed.\n");
//		goto exit;
	}

	ret = ft6336u_get_chip_para(ts);
	if(ret) {
		FT6336U_ERR("version id unmatches.\n");
		goto exit;
	}
#ifdef FT6336U_USE_RELEASE_TIMER
	ts->release_timer = xTimerCreate("FT6336 Release Timer",
							pdMS_TO_TICKS(200),
							pdFALSE,
							ts,
							ft6336_timeout_callback
						);
#endif
	ret = ft6336u_configure_dev(ts);
	if(ret) {
		FT6336U_ERR("ft6336u_configure_dev failed.\n");
		goto exit;
	}

	return 0;
exit:
	vPortFree(ts);

	return -1;
}

int ft6336u_init(void)
{
	struct i2c_adapter *adap = NULL;

	if(!(adap = i2c_open("i2c0"))) {
		FT6336U_ERR("open i2c0 fail.\n");
		return -1;
	}

	if(ft6336u_ts_probe(adap)) {
		FT6336U_ERR("ft6336u probe fail\n");
		i2c_close(adap);
		return -1;
	}

	return 0;
}
#endif

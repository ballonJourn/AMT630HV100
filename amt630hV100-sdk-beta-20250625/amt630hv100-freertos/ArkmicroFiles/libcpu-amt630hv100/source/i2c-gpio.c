#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "errno.h"
#include "timer.h"

#ifdef ANALOG_I2C_SUPPORT

struct i2c_gpio_private_data {
	struct i2c_adapter adap;
	struct i2c_algo_bit_data bit_data;
	struct i2c_gpio_platform_data pdata;
};

/* Toggle SDA by changing the direction of the pin */
static void i2c_gpio_setsda_dir(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	if (state)
		gpio_direction_input(pdata->sda_pin);
	else
		gpio_direction_output(pdata->sda_pin, 0);
}

/*
 * Toggle SDA by changing the output value of the pin. This is only
 * valid for pins configured as open drain (i.e. setting the value
 * high effectively turns off the output driver.)
 */
static void i2c_gpio_setsda_val(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_direction_output(pdata->sda_pin, state);

	gpio_set_value(pdata->sda_pin, state);
}

/* Toggle SCL by changing the direction of the pin. */
static void i2c_gpio_setscl_dir(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	if (state)
		gpio_direction_input(pdata->scl_pin);
	else
		gpio_direction_output(pdata->scl_pin, 0);
}

/*
 * Toggle SCL by changing the output value of the pin. This is used
 * for pins that are configured as open drain and for output-only
 * pins. The latter case will break the i2c protocol, but it will
 * often work in practice.
 */
static void i2c_gpio_setscl_val(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_direction_output(pdata->scl_pin, state);

	gpio_set_value(pdata->scl_pin, state);
}

static int i2c_gpio_getsda(void *data)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_direction_input(pdata->sda_pin);

	return gpio_get_value(pdata->sda_pin);
}

static int i2c_gpio_getscl(void *data)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_direction_input(pdata->scl_pin);

	return gpio_get_value(pdata->scl_pin);
}

/* --- setting states on the bus with the right timing: ---------------	*/

#define setsda(adap, val)	adap->setsda(adap->data, val)
#define setscl(adap, val)	adap->setscl(adap->data, val)
#define getsda(adap)		adap->getsda(adap->data)
#define getscl(adap)		adap->getscl(adap->data)

static __INLINE void sdalo(struct i2c_algo_bit_data *adap)
{
	setsda(adap, 0);
	udelay((adap->udelay + 1) / 2);
}

static __INLINE void sdahi(struct i2c_algo_bit_data *adap)
{
	setsda(adap, 1);
	udelay((adap->udelay + 1) / 2);
}

static __INLINE void scllo(struct i2c_algo_bit_data *adap)
{
	setscl(adap, 0);
	udelay(adap->udelay / 2);
}

/*
 * Raise scl line, and do checking for delays. This is necessary for slower
 * devices.
 */
static int sclhi(struct i2c_algo_bit_data *adap)
{
	unsigned long start;

	setscl(adap, 1);

	/* Not all adapters have scl sense line... */
	if (!adap->getscl)
		goto done;

	start = xTaskGetTickCount();
	while (!getscl(adap)) {
		/* This hw knows how to read the clock line, so we wait
		 * until it actually gets high.  This is safer as some
		 * chips may hold it low ("clock stretching") while they
		 * are processing data internally.
		 */
		if (xTaskGetTickCount() > start + adap->timeout) {
			/* Test one last time, as we may have been preempted
			 * between last check and timeout test.
			 */
			if (getscl(adap))
				break;
			return -ETIMEDOUT;
		}
		taskYIELD();
	}

done:
	udelay(adap->udelay);
	return 0;
}


/* --- other auxiliary functions --------------------------------------	*/
static void i2c_start(struct i2c_algo_bit_data *adap)
{
	/* assert: scl, sda are high */
	setsda(adap, 0);
	udelay(adap->udelay);
	scllo(adap);
}

static void i2c_repstart(struct i2c_algo_bit_data *adap)
{
	/* assert: scl is low */
	sdahi(adap);
	sclhi(adap);
	setsda(adap, 0);
	udelay(adap->udelay);
	scllo(adap);
}


static void i2c_stop(struct i2c_algo_bit_data *adap)
{
	/* assert: scl is low */
	sdalo(adap);
	sclhi(adap);
	setsda(adap, 1);
	udelay(adap->udelay);
}



/* send a byte without start cond., look for arbitration,
   check ackn. from slave */
/* returns:
 * 1 if the device acknowledged
 * 0 if the device did not ack
 * -ETIMEDOUT if an error occurred (while raising the scl line)
 */
static int i2c_outb(struct i2c_adapter *i2c_adap, unsigned char c)
{
	int i;
	int sb;
	int ack;
	struct i2c_algo_bit_data *adap = i2c_adap->algo_data;

	/* assert: scl is low */
	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 1;
		setsda(adap, sb);
		udelay((adap->udelay + 1) / 2);
		if (sclhi(adap) < 0) { /* timed out */
			TRACE_DEBUG("i2c_outb: 0x%02x, timeout at bit #%d\n", (int)c, i);
			return -ETIMEDOUT;
		}
		/* FIXME do arbitration here:
		 * if (sb && !getsda(adap)) -> ouch! Get out of here.
		 *
		 * Report a unique code, so higher level code can retry
		 * the whole (combined) message and *NOT* issue STOP.
		 */
		scllo(adap);
	}
	sdahi(adap);	//---
	//sdalo(adap);	//+++
	if (sclhi(adap) < 0) { /* timeout */
		TRACE_DEBUG("i2c_outb: 0x%02x, timeout at ack\n", (int)c);
		return -ETIMEDOUT;
	}

	/* read ack: SDA should be pulled down by slave, or it may
	 * NAK (usually to report problems with the data we wrote).
	 */
	ack = !getsda(adap);    /* ack: sda is pulled low -> success */
	TRACE_DEBUG("i2c_outb: 0x%02x %s\n", (int)c, ack ? "A" : "NA");

	scllo(adap);
	//sdalo(adap);	//+++

	return ack;
	/* assert: scl is low (sda undef) */
}


static int i2c_inb(struct i2c_adapter *i2c_adap)
{
	/* read byte via i2c port, without start/stop sequence	*/
	/* acknowledge is sent in i2c_read.			*/
	int i;
	unsigned char indata = 0;
	struct i2c_algo_bit_data *adap = i2c_adap->algo_data;

	/* assert: scl is low */
	sdahi(adap);	//---
	for (i = 0; i < 8; i++) {
		if (sclhi(adap) < 0) { /* timeout */
			TRACE_DEBUG("i2c_inb: timeout at bit #%d\n", 7 - i);
			return -ETIMEDOUT;
		}
		indata *= 2;
		if (getsda(adap))
			indata |= 0x01;
		setscl(adap, 0);
		udelay(i == 7 ? adap->udelay / 2 : adap->udelay);
	}
	/* assert: scl is low */
	return indata;
}

/* try_address tries to contact a chip for a number of
 * times before it gives up.
 * return values:
 * 1 chip answered
 * 0 chip did not answer
 * -x transmission error
 */
static int try_address(struct i2c_adapter *i2c_adap,
		       unsigned char addr, int retries)
{
	struct i2c_algo_bit_data *adap = i2c_adap->algo_data;
	int i, ret = 0;

	for (i = 0; i <= retries; i++) {
		ret = i2c_outb(i2c_adap, addr);
		if (ret == 1 || i == retries)
			break;
		TRACE_DEBUG("emitting stop condition\n");
		i2c_stop(adap);
		udelay(adap->udelay);
		taskYIELD();
		TRACE_DEBUG("emitting start condition\n");
		i2c_start(adap);
	}
	if (i && ret)
		TRACE_DEBUG("Used %d tries to %s client at "
			"0x%02x: %s\n", i + 1,
			addr & 1 ? "read from" : "write to", addr >> 1,
			ret == 1 ? "success" : "failed, timeout?");
	return ret;
}

static int sendbytes(struct i2c_adapter *i2c_adap, struct i2c_msg *msg)
{
	const unsigned char *temp = msg->buf;
	int count = msg->len;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
	int retval;
	int wrcount = 0;

	while (count > 0) {
		retval = i2c_outb(i2c_adap, *temp);

		/* OK/ACK; or ignored NAK */
		if ((retval > 0) || (nak_ok && (retval == 0))) {
			count--;
			temp++;
			wrcount++;

		/* A slave NAKing the master means the slave didn't like
		 * something about the data it saw.  For example, maybe
		 * the SMBus PEC was wrong.
		 */
		} else if (retval == 0) {
			TRACE_ERROR("sendbytes: NAK bailout.\n");
			return -EIO;

		/* Timeout; or (someday) lost arbitration
		 *
		 * FIXME Lost ARB implies retrying the transaction from
		 * the first message, after the "winning" master issues
		 * its STOP.  As a rule, upper layer code has no reason
		 * to know or care about this ... it is *NOT* an error.
		 */
		} else {
			TRACE_ERROR("sendbytes: error %d\n", retval);
			return retval;
		}
	}
	return wrcount;
}

static int acknak(struct i2c_adapter *i2c_adap, int is_ack)
{
	struct i2c_algo_bit_data *adap = i2c_adap->algo_data;

	/* assert: sda is high */
	if (is_ack)		/* send ack */
		setsda(adap, 0);
	udelay((adap->udelay + 1) / 2);
	if (sclhi(adap) < 0) {	/* timeout */
		TRACE_ERROR("readbytes: ack/nak timeout\n");
		return -ETIMEDOUT;
	}
	scllo(adap);
	return 0;
}

static int readbytes(struct i2c_adapter *i2c_adap, struct i2c_msg *msg)
{
	int inval;
	int rdcount = 0;	/* counts bytes read */
	unsigned char *temp = msg->buf;
	int count = msg->len;
	const unsigned flags = msg->flags;

	while (count > 0) {
		inval = i2c_inb(i2c_adap);
		if (inval >= 0) {
			*temp = inval;
			rdcount++;
		} else {   /* read timed out */
			break;
		}

		temp++;
		count--;

		/* Some SMBus transactions require that we receive the
		   transaction length as the first read byte. */
		if (rdcount == 1 && (flags & I2C_M_RECV_LEN)) {
			if (inval <= 0 || inval > I2C_SMBUS_BLOCK_MAX) {
				if (!(flags & I2C_M_NO_RD_ACK))
					acknak(i2c_adap, 0);
				TRACE_ERROR("readbytes: invalid block length (%d)\n", inval);
				return -EPROTO;
			}
			/* The original count value accounts for the extra
			   bytes, that is, either 1 for a regular transaction,
			   or 2 for a PEC transaction. */
			count += inval;
			msg->len += inval;
		}

		TRACE_DEBUG( "readbytes: 0x%02x %s\n",
			inval,
			(flags & I2C_M_NO_RD_ACK)
				? "(no ack/nak)"
				: (count ? "A" : "NA"));

		if (!(flags & I2C_M_NO_RD_ACK)) {
			inval = acknak(i2c_adap, count);
			if (inval < 0)
				return inval;
		}
	}
	return rdcount;
}

/* doAddress initiates the transfer by generating the start condition (in
 * try_address) and transmits the address in the necessary format to handle
 * reads, writes as well as 10bit-addresses.
 * returns:
 *  0 everything went okay, the chip ack'ed, or IGNORE_NAK flag was set
 * -x an error occurred (like: -ENXIO if the device did not answer, or
 *	-ETIMEDOUT, for example if the lines are stuck...)
 */
static int bit_doAddress(struct i2c_adapter *i2c_adap, struct i2c_msg *msg)
{
	unsigned short flags = msg->flags;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
	struct i2c_algo_bit_data *adap = i2c_adap->algo_data;

	unsigned char addr;
	int ret, retries;

	retries = nak_ok ? 0 : i2c_adap->retries;

	if (flags & I2C_M_TEN) {
		/* a ten bit address */
		addr = 0xf0 | ((msg->addr >> 7) & 0x06);
		TRACE_DEBUG("addr0: %d\n", addr);
		/* try extended address code...*/
		ret = try_address(i2c_adap, addr, retries);
		if ((ret != 1) && !nak_ok)  {
			TRACE_ERROR("died at extended address code\n");
			return -ENXIO;
		}
		/* the remaining 8 bit address */
		ret = i2c_outb(i2c_adap, msg->addr & 0xff);
		if ((ret != 1) && !nak_ok) {
			/* the chip did not ack / xmission error occurred */
			TRACE_ERROR("died at 2nd address code\n");
			return -ENXIO;
		}
		if (flags & I2C_M_RD) {
			TRACE_DEBUG("emitting repeated start condition\n");
			i2c_repstart(adap);
			/* okay, now switch into reading mode */
			addr |= 0x01;
			ret = try_address(i2c_adap, addr, retries);
			if ((ret != 1) && !nak_ok) {
				TRACE_ERROR("died at repeated address code\n");
				return -EIO;
			}
		}
	} else {		/* normal 7bit address	*/
		addr = msg->addr << 1;
		if (flags & I2C_M_RD)
			addr |= 1;
		if (flags & I2C_M_REV_DIR_ADDR)
			addr ^= 1;
		ret = try_address(i2c_adap, addr, retries);
		if ((ret != 1) && !nak_ok)
			return -ENXIO;
	}

	return 0;
}

static int bit_xfer(struct i2c_adapter *i2c_adap,
		    struct i2c_msg msgs[], int num)
{
	struct i2c_msg *pmsg;
	struct i2c_algo_bit_data *adap = i2c_adap->algo_data;
	int i, ret;
	unsigned short nak_ok;

	TRACE_DEBUG("emitting start condition\n");
	i2c_start(adap);
	for (i = 0; i < num; i++) {
		pmsg = &msgs[i];
		nak_ok = pmsg->flags & I2C_M_IGNORE_NAK;
		if (!(pmsg->flags & I2C_M_NOSTART)) {
			if (i) {
				if (msgs[i - 1].flags & I2C_M_STOP) {
					TRACE_DEBUG("emitting enforced stop/start condition\n");
					i2c_stop(adap);
					i2c_start(adap);
				} else {
					TRACE_DEBUG("emitting repeated start condition\n");
					i2c_repstart(adap);
				}
			}
			ret = bit_doAddress(i2c_adap, pmsg);
			if ((ret != 0) && !nak_ok) {
				TRACE_DEBUG("NAK from device addr 0x%02x msg #%d\n", msgs[i].addr, i);
				goto bailout;
			}
		}
		if (pmsg->flags & I2C_M_RD) {
			/* read bytes into buffer*/
			ret = readbytes(i2c_adap, pmsg);
			if (ret >= 1)
				TRACE_DEBUG("read %d byte%s\n", ret, ret == 1 ? "" : "s");
			if (ret < pmsg->len) {
				if (ret >= 0)
					ret = -EIO;
				goto bailout;
			}
		} else {
			/* write bytes from buffer */
			ret = sendbytes(i2c_adap, pmsg);
			if (ret >= 1)
				TRACE_DEBUG("wrote %d byte%s\n", ret, ret == 1 ? "" : "s");
			if (ret < pmsg->len) {
				if (ret >= 0)
					ret = -EIO;
				goto bailout;
			}
		}
	}
	ret = i;

bailout:
	TRACE_DEBUG("emitting stop condition\n");
	i2c_stop(adap);

	return ret;
}

struct i2c_gpio_platform_data i2c_gpio[] = {
	{
		.devid = 0,
		.sda_pin = I2C_GPIO0_SDA_PIN,
		.scl_pin = I2C_GPIO0_SCL_PIN,
		.udelay = 5,	/* clk freq 500/udelay kHz */
		.timeout = configTICK_RATE_HZ / 10,	/* 100ms */
		.sda_is_open_drain = 0,
		.scl_is_open_drain = 0,
		.scl_is_output_only = 1,
	}
};

const struct i2c_algorithm i2c_bit_algo = {
	.master_xfer	= bit_xfer,
};

int i2c_bit_add_bus(struct i2c_adapter *adap)
{
	int ret;

	/* register new adapter to i2c module... */
	adap->algo = &i2c_bit_algo;
	adap->retries = 3;

	ret = i2c_add_adapter(adap);
	if (ret < 0)
		return ret;

	return 0;
}

static int i2c_gpio_add_device(struct i2c_gpio_platform_data *pdevdata)
{
	struct i2c_gpio_private_data *priv;
	struct i2c_gpio_platform_data *pdata;
	struct i2c_algo_bit_data *bit_data;
	struct i2c_adapter *adap;
	int ret;

	priv = pvPortMalloc(sizeof(*priv));
	if (!priv) {
		TRACE_ERROR("[%s] pvPortMalloc failed, devid:%d\n", __func__, pdevdata->devid);
		return -ENOMEM;
	}
	memset(priv, 0, sizeof(*priv));

	adap = &priv->adap;
	bit_data = &priv->bit_data;
	pdata = &priv->pdata;

	memcpy(pdata, pdevdata, sizeof(*pdata));
	gpio_request(pdata->sda_pin);
	gpio_request(pdata->scl_pin);

	if (pdata->sda_is_open_drain) {
		gpio_direction_output(pdata->sda_pin, 1);
		bit_data->setsda = i2c_gpio_setsda_val;
	} else {
		gpio_direction_input(pdata->sda_pin);
		bit_data->setsda = i2c_gpio_setsda_dir;
	}

	if (pdata->scl_is_open_drain || pdata->scl_is_output_only) {
		gpio_direction_output(pdata->scl_pin, 1);
		bit_data->setscl = i2c_gpio_setscl_val;
	} else {
		gpio_direction_input(pdata->scl_pin);
		bit_data->setscl = i2c_gpio_setscl_dir;
	}

	if (!pdata->scl_is_output_only)
		bit_data->getscl = i2c_gpio_getscl;
	bit_data->getsda = i2c_gpio_getsda;

	if (pdata->udelay)
		bit_data->udelay = pdata->udelay;
	else if (pdata->scl_is_output_only)
		bit_data->udelay = 50;			/* 10 kHz */
	else
		bit_data->udelay = 5;			/* 100 kHz */

	if (pdata->timeout)
		bit_data->timeout = pdata->timeout;
	else
		bit_data->timeout = configTICK_RATE_HZ / 10;		/* 100 ms */

	bit_data->data = pdata;

	snprintf(adap->name, sizeof(adap->name), "i2c-gpio%d", pdata->devid);

	adap->algo_data = bit_data;

	ret = i2c_bit_add_bus(adap);
	if (ret) {
		TRACE_ERROR("[%s] i2c_bit_add_bus failed, devid:%d\n", __func__, pdata->devid);
		return ret;
	}

	TRACE_INFO("using pins %u (SDA) and %u (SCL%s)\n",
		 pdata->sda_pin, pdata->scl_pin,
		 pdata->scl_is_output_only
		 ? ", no clock stretching" : "");

	return 0;
}

void i2c_gpio_init(void)
{
	int i;

	for(i=0; i<sizeof(i2c_gpio)/sizeof(i2c_gpio[0]); i++) {
		i2c_gpio_add_device(&i2c_gpio[i]);
	}
}
#endif

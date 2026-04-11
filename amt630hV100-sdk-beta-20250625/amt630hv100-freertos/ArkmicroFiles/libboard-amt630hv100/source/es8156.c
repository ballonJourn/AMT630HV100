#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "soc-dai.h"


#if AUDIO_CODEC_DAC_IC == AUDIO_CODEC_DAC_ES8156
/* Reset Control */
#define ES8156_RESET_REG00				0x00
/* Clock Managerment */
#define ES8156_MAINCLOCK_CTL_REG01		0x01
#define ES8156_SCLK_MODE_REG02			0x02
#define ES8156_LRCLK_DIV_H_REG03		0x03
#define ES8156_LRCLK_DIV_L_REG04		0x04
#define ES8156_SCLK_DIV_REG05			0x05
#define ES8156_NFS_CONFIG_REG06			0x06
#define ES8156_MISC_CONTROL1_REG07		0x07
#define ES8156_CLOCK_ON_OFF_REG08		0x08
#define ES8156_MISC_CONTROL2_REG09		0x09
#define ES8156_TIME_CONTROL1_REG0A		0x0a
#define ES8156_TIME_CONTROL2_REG0B		0x0b
/* System Control */
#define ES8156_CHIP_STATUS_REG0C		0x0c
#define ES8156_P2S_CONTROL_REG0D		0x0d
#define ES8156_DAC_OSR_COUNTER_REG10	0x10
/* SDP Control */
#define ES8156_DAC_SDP_REG11			0x11
#define ES8156_AUTOMUTE_SET_REG12		0x12
#define ES8156_DAC_MUTE_REG13			0x13
#define ES8156_VOLUME_CONTROL_REG14		0x14

/* ALC Control */
#define ES8156_ALC_CONFIG1_REG15		0x15
#define ES8156_ALC_CONFIG2_REG16		0x16
#define ES8156_ALC_CONFIG3_REG17		0x17
#define ES8156_MISC_CONTROL3_REG18		0x18
#define ES8156_EQ_CONTROL1_REG19		0x19
#define ES8156_EQ_CONTROL2_REG1A		0x1a
/* Analog System Control */
#define ES8156_ANALOG_SYS1_REG20		0x20
#define ES8156_ANALOG_SYS2_REG21		0x21
#define ES8156_ANALOG_SYS3_REG22		0x22
#define ES8156_ANALOG_SYS4_REG23		0x23
#define ES8156_ANALOG_LP_REG24			0x24
#define ES8156_ANALOG_SYS5_REG25		0x25
/* Chip Information */
#define ES8156_I2C_PAGESEL_REGFC		0xFC
#define ES8156_CHIPID1_REGFD			0xFD
#define ES8156_CHIPID0_REGFE			0xFE
#define ES8156_CHIP_VERSION_REGFF		0xFF

/* Slave device address */
#define ES8156_SLAVE_ADDR				0x09

#define ES8156_DRIVER_ENABLE

#define ES8156_ERR(fmt, args...)		printf("###ERR:[%s]"fmt, __func__, ##args)
#define ES8156_DBG(fmt, args...)		//printf("DBG:[%s]"fmt, __func__, ##args)

typedef struct es8156_private_data {
	struct i2c_adapter *adap;
	unsigned short slave_addr;
	int rates;
	int channels;
	int bits;
	int master;
	int mclk;
	int fmt;
	int mute;
} es8156_client;

#ifdef ES8156_DRIVER_ENABLE
static es8156_client *g_es8156_client = NULL;

/**
 * es8156_i2c_read - read data from a register of the i2c slave device.
 *
 * @adap: i2c device.
 * @reg: the register to read from.
 * @buf: raw write data buffer.
 * @len: length of the buffer to write
 */
static int es8156_i2c_read(es8156_client *client, uint8_t reg)
{
	struct i2c_msg msgs[2];
	uint8_t wbuf[2];
	uint8_t rbuf = 0;
	int ret;
	int i;

	wbuf[0] = reg;

	msgs[0].flags = 0;
	msgs[0].addr  = client->slave_addr;
	msgs[0].len   = 1;
	msgs[0].buf   = wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->slave_addr;
	msgs[1].len   = 1;
	msgs[1].buf   = &rbuf;

	for(i=0; i<5; i++)
	{
		ret = i2c_transfer(client->adap, msgs, 2);
		if(ret == 2)
			break;
	}

	return (ret==ARRAY_SIZE(msgs)) ? rbuf : -EIO;
}

/**
 * es8156_i2c_write - write data to a register of the i2c slave device.
 *
 * @adap: i2c device.
 * @reg: the register to write to.
 * @buf: raw data buffer to write.
 * @len: length of the buffer to write
 */
static int es8156_i2c_write(es8156_client *client, uint8_t reg, uint8_t val)
{
	struct i2c_msg msg;
	uint8_t addr_buf[2];
	int ret;
	int i;

	addr_buf[0] = reg;
	addr_buf[1] = val;

	msg.flags = 0;
	msg.addr = client->slave_addr;
	msg.buf = addr_buf;
	msg.len = 2;

	for(i=0; i<5; i++)
	{
		ret = i2c_transfer(client->adap, &msg, 1);
		if(ret == 1)
			break;
	}

	ES8156_DBG("reg[0x%x], val:0x%x, read:0x%x\n", reg, val, es8156_i2c_read(client, reg));

	return (ret != 1 ? -EIO : 0);
}

int es8156_i2c_write_bits(es8156_client *client, u8 reg, u8 mask, u8 offset, u8 value)
{
	int val = es8156_i2c_read(client, reg);
	if(val < 0)
	{
		ES8156_ERR("%s read failed, reg:0x%x, val:0x%x\n", __func__, reg, val);
		return -1;
	}

	val &= ~(mask << offset);
	val |= (value << offset);
	if(es8156_i2c_write(client, reg, val))
	{
		ES8156_ERR("%s write failed, reg:0x%x, val:0x%x\n", __func__, reg, val);
		return -1;
	}

	return 0;
}

#if 0
static int es8156_reset(es8156_client *client)
{
	es8156_i2c_write(client, ES8156_RESET_REG00, 0x1c);
	mdelay(5);
	es8156_i2c_write(client, ES8156_RESET_REG00, 0x03);

	return 0;
}
#endif

int es8156_sleep(int state)
{
	es8156_client *client = g_es8156_client;
	int regv;

	if(!client)
	{
		ES8156_ERR("%s(), Invalid client\n.", __func__);
		return -1;
	}

	if(state)
	{
		es8156_i2c_write(client, ES8156_ANALOG_SYS5_REG25, 0x07); //power down analog
		regv = es8156_i2c_read(client, ES8156_CLOCK_ON_OFF_REG08);
		regv &= 0xf0;
		es8156_i2c_write(client, ES8156_CLOCK_ON_OFF_REG08, regv); //disable clock

		ES8156_DBG("%s() ,es8156 off\n.", __func__);
	}
	else
	{
		regv = es8156_i2c_read(client, ES8156_CLOCK_ON_OFF_REG08);
		regv |= 0x0f;
		es8156_i2c_write(client, ES8156_CLOCK_ON_OFF_REG08, regv); //enable clock
		es8156_i2c_write(client, ES8156_ANALOG_SYS5_REG25, 0x20); //power up analog

		ES8156_DBG("%s() ,es8156 startup\n.", __func__);
	}

	return 0;
}

static int es8156_set_volume(int volume)
{
	es8156_client *client = g_es8156_client;

	if(!client)
	{
		ES8156_ERR("%s(), Invalid client.\n", __func__);
		return -1;
	}

	if(volume < 0 && volume > 0xff)
	{
		ES8156_ERR("%s(), Invalid volume.", __func__);
		return -1;
	}

	if(es8156_i2c_write(client, ES8156_VOLUME_CONTROL_REG14, volume))
	{
		ES8156_ERR("%s(), i2c write fail.\n", __func__);
		return -1;
	}

	return 0;
}

static int es8156_probe(struct i2c_adapter *adap, struct snd_soc_dai_ops *ops)
{
	es8156_client *client;

	client = pvPortMalloc(sizeof(es8156_client));
	if(!client)
	{
		ES8156_ERR("%s, pvPortMalloc fail.\n", __func__);
		return -ENOMEM;
	}
	memset(client, 0, sizeof(es8156_client));
	g_es8156_client = client;

	client->adap = adap;
	client->slave_addr = ES8156_SLAVE_ADDR;

#if 0	//set default value;
	client->rates = 44100;
	client->channels = 2;
	client->bits = 16;
	client->mute = 0;
	client->master = 1;
	client->mclk = client->rates * 256;
	client->fmt = SND_SOC_DAIFMT_I2S;			//interface type.
#endif

	if(ops)
	{
		//es8156 Free Driver(免驱).
		ops->init		= NULL;
		ops->hw_params	= NULL;
		ops->startup	= NULL;
		ops->set_fmt	= NULL;
		ops->set_mute	= NULL;
		ops->set_mclk	= NULL;
		ops->set_volume = es8156_set_volume;
	}

	//es8156_reset(client);

	return 0;
}
#endif

int audio_codec_dac_init(struct snd_soc_dai_ops *ops)
{
	(void)ops;
#ifdef ES8156_DRIVER_ENABLE
	struct i2c_adapter *adap = NULL;

	if(!(adap = i2c_open("i2c0")))
	{
		ES8156_ERR("%s, open i2c0 fail.\n", __func__);
		return -1;
	}

	es8156_probe(adap, ops);
#endif
	return 0;
}
#endif

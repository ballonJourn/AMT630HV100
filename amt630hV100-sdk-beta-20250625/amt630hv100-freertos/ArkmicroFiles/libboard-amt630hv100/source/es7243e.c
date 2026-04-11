#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "soc-dai.h"

#if AUDIO_CODEC_ADC_IC == AUDIO_CODEC_ADC_ES7243E

/* Slave device address */
#define ES7243E_SLAVE_ADDR				0x10

typedef enum adc_data_select {
	ADC_OUTPUT_L_R,
	ADC_OUTPUT_L_L,
	ADC_OUTPUT_R_R,
	ADC_OUTPUT_R_L
} adc_data_sel;

typedef enum adc_polarity_invert {
	ADC_INV_NORMAL,
	ADC_R_INV,
	ADC_L_INV,
	ADC_LR_INV
} adc_pol_inv;


struct _coeff_div {
	u32 mclk;			//mclk frequency
	u32 sr_rate;		//sample rate
	u8 osr;				//adc over sample rate
	u8 prediv_premulti;	//adcclk and dacclk divider
	u8 cf_dsp_div;		//adclrck divider and daclrck divider
	u8 scale;
	u8 lrckdiv_h;
	u8 lrckdiv_l;
	u8 bclkdiv;			//sclk divider
};

typedef struct _adc_private {
	adc_data_sel adc_data_sel;
	adc_pol_inv adc_inv;
} adc_private;

typedef struct es7243e_private_data {
	struct i2c_adapter *adap;
	unsigned short slave_addr;
	int rates;
	int channels;
	int bits;
	int master;
	int mclk;
	int fmt;
	int mute;
	adc_private adc_priv;
} es7243e_client;

static int es7243e_mute(int mute);

static es7243e_client *g_es7243e_client = NULL;


/**
 * es7243e_i2c_read - read data from a register of the i2c slave device.
 *
 * @adap: i2c device.
 * @reg: the register to read from.
 * @buf: raw write data buffer.
 * @len: length of the buffer to write
 */
static int es7243e_i2c_read(es7243e_client *client, uint8_t reg)
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
 * es7243e_i2c_write - write data to a register of the i2c slave device.
 *
 * @adap: i2c device.
 * @reg: the register to write to.
 * @buf: raw data buffer to write.
 * @len: length of the buffer to write
 */
static int es7243e_i2c_write(es7243e_client *client, uint8_t reg, uint8_t val)
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

	//printf("reg[0x%x], val:0x%x, read:0x%x\n", reg, val, es7243e_i2c_read(client, reg));

	return (ret != 1 ? -EIO : 0);
}

int es7243e_i2c_write_bits(es7243e_client *client, u8 reg, u8 mask, u8 offset, u8 val)
{
	int value = es7243e_i2c_read(client, reg);
	if(value < 0)
	{
		printf("%s read failed, reg:0x%x, val:0x%x\n", __func__, reg, value);
		return -1;
	}

	value &= ~(mask << offset);
	value |= (val << offset);
	if(es7243e_i2c_write(client, reg, value&0xFF))
	{
		printf("%s write failed, reg:0x%x, val:0x%x\n", __func__, reg, value);
		return -1;
	}

	return 0;
}

/* codec mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
	//mclk    lrck,  osr, pre, div  ,scale, lrckdiv_h, lrckdiv_l, bclkdiv
	/* 24.576MHZ */
	{24576000, 8000 , 0x20, 0x50, 0x00, 0x00, 0x0b, 0xff, 0x2f},
	{24576000, 12000, 0x20, 0x30, 0x00, 0x00, 0x07, 0xff, 0x1f},
	{24576000, 16000, 0x20, 0x20, 0x00, 0x00, 0x05, 0xff, 0x17},
	{24576000, 24000, 0x20, 0x10, 0x00, 0x00, 0x03, 0xff, 0x0f},
	{24576000, 32000, 0x20, 0x21, 0x00, 0x00, 0x02, 0xff, 0x0b},
	{24576000, 48000, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	/* 12.288MHZ */
	{12288000, 8000 , 0x20, 0x20, 0x00, 0x00, 0x05, 0xff, 0x17},
	{12288000, 12000, 0x20, 0x10, 0x00, 0x00, 0x03, 0xff, 0x0f},
	{12288000, 16000, 0x20, 0x21, 0x00, 0x00, 0x02, 0xff, 0x0b},
	{12288000, 24000, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	{12288000, 32000, 0x20, 0x22, 0x00, 0x00, 0x01, 0x7f, 0x05},
	{12288000, 48000, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	/* 6.144MHZ */
	{6144000 , 8000 , 0x20, 0x21, 0x00, 0x00, 0x02, 0xff, 0x0b},
	{6144000 , 12000, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	{6144000 , 16000, 0x20, 0x22, 0x00, 0x00, 0x01, 0x7f, 0x05},
	{6144000 , 24000, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	{6144000 , 32000, 0x20, 0x23, 0x00, 0x00, 0x00, 0xbf, 0x02},
	{6144000 , 48000, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	/* 3.072MHZ */
	{3072000 , 8000 , 0x20, 0x22, 0x00, 0x00, 0x01, 0x7f, 0x05},
	{3072000 , 12000, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	{3072000 , 16000, 0x20, 0x23, 0x00, 0x00, 0x00, 0xbf, 0x02},
	{3072000 , 24000, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	{3072000 , 32000, 0x10, 0x03, 0x20, 0x04, 0x00, 0x5f, 0x02},
	{3072000 , 48000, 0x20, 0x03, 0x00, 0x00, 0x00, 0x3f, 0x00},
	/* 1.536MHZ */
	{1536000 , 8000 , 0x20, 0x23, 0x00, 0x00, 0x00, 0xbf, 0x02},
	{1536000 , 12000, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	{1536000 , 16000, 0x10, 0x03, 0x20, 0x04, 0x00, 0x5f, 0x00},
	{1536000 , 24000, 0x20, 0x03, 0x00, 0x00, 0x00, 0x3f, 0x00},

	/* 32.768MHZ */
	{32768000, 8000 , 0x20, 0x70, 0x00, 0x00, 0x0f, 0xff, 0x3f},
	{32768000, 16000, 0x20, 0x30, 0x00, 0x00, 0x07, 0xff, 0x1f},
	{32768000, 32000, 0x20, 0x10, 0x00, 0x00, 0x03, 0xff, 0x0f},
	/* 16.384MHZ */
	{16384000, 8000 , 0x20, 0x30, 0x00, 0x00, 0x07, 0xff, 0x1f},
	{16384000, 16000, 0x20, 0x10, 0x00, 0x00, 0x03, 0xff, 0x0f},
	{16384000, 32000, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	/* 8.192MHZ */
	{8192000 , 8000 , 0x20, 0x10, 0x00, 0x00, 0x03, 0xff, 0x0f},
	{8192000 , 16000, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	{8192000 , 32000, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	/* 4.096MHZ */
	{4096000 , 8000 , 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	{4096000 , 16000, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	{4096000 , 32000, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	/* 2.048MHZ */
	{2048000 , 8000 , 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	{2048000 , 16000, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	{2048000 , 32000, 0x20, 0x03, 0x00, 0x00, 0x00, 0x3f, 0x00},
	/* 1.024MHZ */
	{1024000 , 8000 , 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	{1024000 , 16000, 0x20, 0x03, 0x00, 0x00, 0x00, 0x3f, 0x00},

	/* 22.5792MHz */
	{22579200, 11025, 0x20, 0x30, 0x00, 0x00, 0x07, 0xff, 0x1f},
	{22579200, 22050, 0x20, 0x10, 0x00, 0x00, 0x03, 0xff, 0x0f},
	{22579200, 44100, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	/* 11.2896MHz */ 
	{11289600, 11025, 0x20, 0x10, 0x00, 0x00, 0x03, 0xff, 0x0f},
	{11289600, 22050, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	{11289600, 44100, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	/* 5.6448MHz */ 
	{56448000, 11025, 0x20, 0x00, 0x00, 0x00, 0x01, 0xff, 0x07},
	{56448000, 22050, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	{56448000, 44100, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	/* 2.8224MHz */ 
	{28224000, 11025, 0x20, 0x01, 0x00, 0x00, 0x00, 0xff, 0x03},
	{28224000, 22050, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	{28224000, 44100, 0x20, 0x03, 0x00, 0x00, 0x00, 0x3f, 0x00},
	/* 1.4112MHz */ 
	{14112000, 11025, 0x20, 0x02, 0x00, 0x00, 0x00, 0x7f, 0x01},
	{14112000, 22050, 0x20, 0x03, 0x00, 0x00, 0x00, 0x3f, 0x00},
};

static inline int get_coeff(int mclk, int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].sr_rate == rate && coeff_div[i].mclk == mclk) {
			printf("%s() get_coeff find index:%d\n", __func__, i);
			return i;
		}
	}
	printf("%s() get_coeff not find\n", __func__);

	return -EINVAL;
}


static int es7243e_pcm_hw_params(es7243e_client *client)
{ 
	u8 regv;
	int coeff;

#if 1
	coeff = get_coeff(client->mclk, client->rates);

	if(coeff < 0)
	{
		printf("Unable to configure sample rate %dHz with %dHz MCLK", client->rates, client->mclk);
		return coeff;
	}

	/*
	* set clock parameters
	*/
	//printf("%s():coeff = %d\n",__func__, coeff);
	if(coeff >= 0)
	{
		es7243e_i2c_write(client, 0x03, coeff_div[coeff].osr);
		es7243e_i2c_write(client, 0x04, coeff_div[coeff].prediv_premulti);
		es7243e_i2c_write(client, 0x05, coeff_div[coeff].cf_dsp_div);
		es7243e_i2c_write_bits(client, 0x0D, 0x07, 0, coeff_div[coeff].scale);
		es7243e_i2c_write(client, 0x03, coeff_div[coeff].osr);
		es7243e_i2c_write_bits(client, 0x07, 0x0F, 0, coeff_div[coeff].lrckdiv_h & 0x0f);
		es7243e_i2c_write(client, 0x06, coeff_div[coeff].bclkdiv);
		//es7243e_i2c_write(client, 0x06, 0x07);//BCLK:1.536
	}
#endif

#if 1//add-
	regv = es7243e_i2c_read(client, 0x0B);
	regv &= 0xe3;

	switch (client->bits) 
	{
		case 16:
			regv |= 0x0c;
			break;
		case 20:
			regv |= 0x04;
			break;
		case 24:
			break;
		case 32:
			regv |= 0x10;
			break;
		default:
			regv |= 0x0c;
			break;
	}
	es7243e_i2c_write(client, 0x0B, regv);

	/*
	*	unmute SDP for MT8516 and MT8167 platform
	*/
	msleep(500);
	es7243e_mute(1);
#endif
	return 0;
}

static int es7243e_set_bias_level(es7243e_client *client, enum snd_soc_bias_level level)
{		 
	u8 regv;

	switch (level) {
		case SND_SOC_BIAS_ON:
			printf("%s on\n", __func__);
			break;
		case SND_SOC_BIAS_PREPARE:
			printf("%s prepare\n", __func__);
			break;
		case SND_SOC_BIAS_STANDBY:
			printf("%s standby\n", __func__);
			/*
			* enable clock 
			*/
			regv = es7243e_i2c_read(client, 0x01); 
			regv |= 0x0A;							  
			es7243e_i2c_write(client, 0x01, regv); 
			es7243e_i2c_write(client, 0x16, 0x00); //power up analog
			/*
			* enable mic input 1
			*/
			regv = es7243e_i2c_read(client, 0x20);
			regv |= 0x10;
			es7243e_i2c_write(client, 0x20, regv);
			/*
			* enable mic input 2
			*/
			regv = es7243e_i2c_read(client, 0x21);
			regv |= 0x10;
			es7243e_i2c_write(client, 0x21, regv);
			msleep(100);
			es7243e_mute(0);
			break;
		case SND_SOC_BIAS_OFF:
			printf("%s off\n", __func__);
			es7243e_mute(1);
			/*
			* disable mic input 1
			*/
			regv = es7243e_i2c_read(client, 0x20);
			regv &= 0xef;
			es7243e_i2c_write(client, 0x20, regv);
			/*
			* disable mic input 2
			*/
			regv = es7243e_i2c_read(client, 0x21);
			regv &= 0xef;
			es7243e_i2c_write(client, 0x21, regv);

			es7243e_i2c_write(client, 0x16, 0xff); //power down analog

			/*
			* disable clock
			*/
			regv = es7243e_i2c_read(client, 0x01);
			regv &= 0xf5;
			es7243e_i2c_write(client, 0x01, regv);
			break;
	}

	return 0;
}


static int es7243e_init_regs(es7243e_client *client)
{
	if(client->master)
	{
		//ES7243e master mode
		es7243e_i2c_write(client, 0x01, 0x3A);
		es7243e_i2c_write(client, 0x00, 0x80);
		es7243e_i2c_write(client, 0x04, 0x02);
		es7243e_i2c_write(client, 0x04, 0x01);
		es7243e_i2c_write(client, 0xF9, 0x01);

		es7243e_i2c_write(client, 0x00, 0x1E);
		es7243e_i2c_write(client, 0x01, 0x00);
		es7243e_i2c_write(client, 0x02, 0x00);	//default:0x00	(0x01 :BCLK invert)
		es7243e_i2c_write(client, 0x03, 0x20);
		//es7243e_i2c_write(client, 0x0D, 0x40);//default:0x00   (settable:0x40	0x80)
		es7243e_i2c_write_bits(client, 0x0D, 0x0f, 4, (client->adc_priv.adc_data_sel<<2) | client->adc_priv.adc_inv);
		es7243e_i2c_write(client, 0xF9, 0x00);
		es7243e_i2c_write(client, 0x04, 0x02);
		es7243e_i2c_write(client, 0x04, 0x01);
		es7243e_i2c_write(client, 0x05, 0x00);
		//es7243e_i2c_write(client, 0x06, 0x07);//test: set BCLK 1.536M
		es7243e_i2c_write(client, 0x07, 0x00);//default:0x00  0x02
		es7243e_i2c_write(client, 0x08, 0xFF);//default:0xFF
		es7243e_i2c_write(client, 0x09, 0xC5);
		es7243e_i2c_write(client, 0x0A, 0x81);
		es7243e_i2c_write(client, 0x0B, 0x0C);//default:0x00(24bit) 0x0c(16 bit)  add:0x0d  20210722
		es7243e_i2c_write(client, 0x0E, 0xBF);
		es7243e_i2c_write(client, 0x0F, 0x80);
		es7243e_i2c_write(client, 0x14, 0x0C);
		es7243e_i2c_write(client, 0x15, 0x0C);
		es7243e_i2c_write(client, 0x17, 0x02);
		es7243e_i2c_write(client, 0x18, 0x26);
		es7243e_i2c_write(client, 0x19, 0x77);
		es7243e_i2c_write(client, 0x1A, 0xF4);
		es7243e_i2c_write(client, 0x1B, 0x66);
		es7243e_i2c_write(client, 0x1C, 0x44);
		es7243e_i2c_write(client, 0x1E, 0x00);
		es7243e_i2c_write(client, 0x1F, 0x0C);
		es7243e_i2c_write(client, 0x20, 0x1c);//default:0x19  Analog gain(min:0x01	 max:0x1e)
		es7243e_i2c_write(client, 0x21, 0x1c);//default:0x19  Analog gain(min:0x01	 max:0x1e)
		es7243e_i2c_write(client, 0x00, 0xC0);
		es7243e_i2c_write(client, 0x01, 0x3A);
		es7243e_i2c_write(client, 0x16, 0x3F);
		es7243e_i2c_write(client, 0x16, 0x00);
		es7243e_i2c_write(client, 0x13, 0x3f);//add
		es7243e_i2c_write(client, 0x0f, 0x40);//add
	}
	else
	{
		//ES7243e slave mode
		es7243e_i2c_write(client, 0x01, 0x3A);
		es7243e_i2c_write(client, 0x00, 0x80);
		es7243e_i2c_write(client, 0x04, 0x02);
		es7243e_i2c_write(client, 0x04, 0x01);
		es7243e_i2c_write(client, 0xF9, 0x01);

		es7243e_i2c_write(client, 0x00, 0x1E);
		es7243e_i2c_write(client, 0x01, 0x00);
		es7243e_i2c_write(client, 0x02, 0x00);
		es7243e_i2c_write(client, 0x03, 0x20);
		//es7243e_i2c_write(client, 0x0D, 0x00);
		es7243e_i2c_write_bits(client, 0x0D, 0x0f, 4, (client->adc_priv.adc_data_sel<<2) | client->adc_priv.adc_inv);
		es7243e_i2c_write(client, 0xF9, 0x00);
		es7243e_i2c_write(client, 0x04, 0x02);
		es7243e_i2c_write(client, 0x04, 0x01);
		es7243e_i2c_write(client, 0x05, 0x00);
		es7243e_i2c_write(client, 0x07, 0x00);//add
		es7243e_i2c_write(client, 0x09, 0xC5);
		es7243e_i2c_write(client, 0x0A, 0x81);
		es7243e_i2c_write(client, 0x0b, 0x00);
		es7243e_i2c_write(client, 0x0E, 0xBF);
		es7243e_i2c_write(client, 0x0F, 0x80);
		es7243e_i2c_write(client, 0x14, 0x0C);
		es7243e_i2c_write(client, 0x15, 0x0C);
		es7243e_i2c_write(client, 0x17, 0x02);//add
		es7243e_i2c_write(client, 0x18, 0x26);
		es7243e_i2c_write(client, 0x19, 0x77);
		es7243e_i2c_write(client, 0x1A, 0xF4);
		es7243e_i2c_write(client, 0x1B, 0x66);
		es7243e_i2c_write(client, 0x1C, 0x44);
		es7243e_i2c_write(client, 0x1E, 0x00);
		es7243e_i2c_write(client, 0x1F, 0x0C);
		es7243e_i2c_write(client, 0x20, 0x19);
		es7243e_i2c_write(client, 0x21, 0x19);
		es7243e_i2c_write(client, 0x00, 0x80);
		es7243e_i2c_write(client, 0x01, 0x3A);
		es7243e_i2c_write(client, 0x16, 0x3F);
		es7243e_i2c_write(client, 0x16, 0x00);
	}

	return 0;
}

static int es7243e_set_fmt(unsigned int fmt)
{
	u8 iface = 0;
	u8 adciface = 0;
	u8 clksel = 0;

	es7243e_client *client = g_es7243e_client;
	if(!client)
	{
		printf("%s(), Invalid client\n", __func__);
		return -1;
	}
	adciface = es7243e_i2c_read(client, 0x0B);	//get interface format
	iface = es7243e_i2c_read(client, 0x00);		//get spd interface
	clksel = es7243e_i2c_read(client, 0x02);	//get spd interface

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK)
	{
		case SND_SOC_DAIFMT_CBM_CFM:	// MASTER MODE
			printf("Enter into %s(), I2S master\n", __func__);
			iface |= 0x40;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:	// SLAVE MODE
			printf("Enter into %s(), I2S slave\n", __func__);
			iface &= 0xbf;
			break;
		default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK)
	{
		case SND_SOC_DAIFMT_I2S:
			printf("Enter into %s(), I2S format \n", __func__);
			adciface &= 0xFC;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			return -EINVAL;
		case SND_SOC_DAIFMT_LEFT_J:
			printf("Enter into %s(), LJ format \n", __func__);
			adciface &= 0xFC;
			adciface |= 0x01;
			break;
		case SND_SOC_DAIFMT_DSP_A:
			printf("Enter into %s(), DSP-A format \n", __func__);
			adciface &= 0xDC;
			adciface |= 0x03;
			break;
		case SND_SOC_DAIFMT_DSP_B:
			printf("Enter into %s(), DSP-B format \n", __func__);
			adciface &= 0xDC;
			adciface |= 0x23;
			break;
		default:
			return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK)
	{
		case SND_SOC_DAIFMT_NB_NF:
			printf("Enter into %s(), BCLK noinvert, MCLK noinvert \n", __func__);
			adciface &= 0xdf;
			clksel &= 0xfe;
			break;
		case SND_SOC_DAIFMT_IB_IF:
			printf("Enter into %s(), BCLK invert, MCLK invert \n", __func__);
			adciface |= 0x20;
			clksel |= 0x01;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			printf("Enter into %s(), BCLK invert, MCLK noinvert \n", __func__);
			adciface &= 0xdf;
			clksel |= 0x01;
			break;
		case SND_SOC_DAIFMT_NB_IF:
			printf("Enter into %s(), BCLK noinvert, MCLK invert \n", __func__);
			adciface |= 0x20;
			clksel &= 0xfe;
			break;
		default:
			return -EINVAL;
	}

	es7243e_i2c_write(client, 0x00, iface);
	es7243e_i2c_write(client, 0x02, clksel);
	es7243e_i2c_write(client, 0x0B, adciface);

	return 0;
}

static int es7243e_mute(int mute)
{
	es7243e_client *client = g_es7243e_client;
	if(!client)
	{
		printf("%s(), Invalid client\n", __func__);
		return -1;
	}

	printf("%s %d\n", __func__, mute);
	if (mute)
	{
		es7243e_i2c_write_bits(client, 0x0B, 0x3, 6, 0x3);
		client->mute = 1;
	}
	else
	{
		es7243e_i2c_write_bits(client, 0x0B, 0x3, 6, 0x0);
		client->mute = 0;
	}
	return 0;
}

static int es7243e_set_mclk(int freq, int dir)
{
    return 0;
}

static int es7243e_hw_params(struct snd_soc_hw_params *params)
{ 
	es7243e_client *client = g_es7243e_client;
	if(!client)
	{
		printf("%s(), Invalid client\n", __func__);
		return -1;
	}

	if(!params)
	{
		printf("%s(), Invalid params\n", __func__);
		return -1;
	}

	client->rates = params->rates;
	client->channels = params->channels;
	client->bits = params->bits;
	client->mclk = client->rates * 256;

	es7243e_set_bias_level(client, SND_SOC_BIAS_OFF);

	es7243e_pcm_hw_params(client);

	return 0;
}

static int es7243e_startup(int start)
{
	es7243e_client *client = g_es7243e_client;
	if(!client)
	{
		printf("%s(), Invalid client\n", __func__);
		return -1;
	}

	if(start)
		es7243e_set_bias_level(client, SND_SOC_BIAS_STANDBY);
	else
		es7243e_set_bias_level(client, SND_SOC_BIAS_OFF);

	return 0;
}

static int es7243e_hw_init(int master)
{
	es7243e_client *client = g_es7243e_client;
	if(!client)
	{
		printf("%s(), Invalid client\n", __func__);
		return -1;
	}

	client->master = !!master;
	client->fmt = SND_SOC_DAIFMT_I2S;			//interface type.
	client->fmt |= SND_SOC_DAIFMT_NB_NF;		//normal bclk + normal frame.
	client->fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
	if(client->master)
		client->fmt |= SND_SOC_DAIFMT_CBM_CFM;	//master
	else
		client->fmt |= SND_SOC_DAIFMT_CBS_CFS;	//slave

	client->adc_priv.adc_data_sel = ADC_OUTPUT_L_R;
	client->adc_priv.adc_inv = ADC_INV_NORMAL;

	es7243e_init_regs(client);

	es7243e_set_fmt(client->fmt);

	return 0;
}

static int es7243e_reset(es7243e_client *client)
{

	return 0;
}


static int es7243e_probe(struct i2c_adapter *adap, struct snd_soc_dai_ops *ops)
{
	es7243e_client *client;

	client = pvPortMalloc(sizeof(es7243e_client));
	if (!client)
	{
		printf("%s, pvPortMalloc fail.\n", __func__);
		return -ENOMEM;
	}
	memset(client, 0, sizeof(es7243e_client));
	g_es7243e_client = client;

	client->adap = adap;
	client->slave_addr = ES7243E_SLAVE_ADDR;

	if(ops)
	{
		ops->init		= es7243e_hw_init;
		ops->hw_params	= es7243e_hw_params;
		ops->startup	= es7243e_startup;
		ops->set_fmt	= es7243e_set_fmt;
		ops->set_mute	= es7243e_mute;
		ops->set_mclk	= es7243e_set_mclk;
	}

	es7243e_reset(client);

	es7243e_mute(1);

	return 0;
}

int audio_codec_adc_init(struct snd_soc_dai_ops *ops)
{
	struct i2c_adapter *adap = NULL;

	if (!(adap = i2c_open("i2c0"))) {
		printf("%s, open i2c0 fail.\n", __func__);
		return -1;
	}

	return es7243e_probe(adap, ops);
}
#endif

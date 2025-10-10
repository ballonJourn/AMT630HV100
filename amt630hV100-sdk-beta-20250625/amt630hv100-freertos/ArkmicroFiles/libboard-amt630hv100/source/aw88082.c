/******************************************************
*
* Please copy the contents of aw88262.c to the main.c *
*
* External API are as follows:
* 1.aw88082_init();
* 2.aw_pa_start();
* 3.aw_pa_stop();
*
******************************************************/

/****************************************************************************
*
* Please modify the following information(#1~#4) according to the platform **
*
* Taking the STM32HAL library as an example *********************************
*
****************************************************************************/
#include "stdio.h"
#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define	AW_FAIL		(-1)
#define	AW_OK		(0)

struct i2c_adapter *adap = NULL;

/* #1.Device I2C address, taking 0x40 as an example*/
#define    I2C_ADDR    (0x34)


/* #2.Print Function*/
#define aw_printf(format, ...) \
	do { \
		printf("[Awinic]%s: " format "\r\n", __func__, ##__VA_ARGS__); \
	} while (0)


/* #3.Delay Function*/
// #define MS_DELAY(time) HAL_Delay(time)
#define MS_DELAY(time) vTaskDelay(time)

/* #4.Encapsulation I2C read/write function*/
int i2c_write_func(uint16_t dev_addr, uint8_t reg_addr,
			uint8_t *pdata, uint16_t len)
{
	// if (HAL_I2C_Mem_Write(&hi2c1, (dev_addr << 1), reg_addr,
	// 			I2C_MEMADD_SIZE_8BIT, pdata, len, 1000) != HAL_OK){
	// 	return -1;
	// } else {
	// 	return 0;
	// }

	return 0;
}

int i2c_read_func(uint16_t dev_addr, uint8_t reg_addr,
				uint8_t *pdata, uint16_t len)
{
	// if (HAL_I2C_Mem_Read(&hi2c1, (dev_addr << 1), reg_addr,
	// 				I2C_MEMADD_SIZE_8BIT, pdata, len, 1000) != HAL_OK) {
	// 	return -1;
	// } else {
	// 	return 0;
	// }
	return 0;
}

/************************************************************
*
****************** Function definition **********************
*
************************************************************/

/******* General I2C Read/Write API *******/

int i2c_read(unsigned char reg_addr, unsigned int *reg_data)
{
	int ret = AW_FAIL;
	// unsigned char cnt = 0;
	// unsigned char buf[2] = { 0 };
	uint16_t data = 0;

	// while (cnt < 5) {
	// 	ret = i2c_read_func(I2C_ADDR, reg_addr, buf, 2);
	// 	if (ret < 0) {
	// 		aw_printf("i2c_read cnt=%d error=%d", cnt, ret);
	// 	} else {
	// 				data = (uint16_t)(buf[0] & 0x00ff);
	// 				data <<= 8;
	// 				data |= (uint16_t)(buf[1] & 0x00ff);
	// 				*reg_data = data;
	// 		break;
	// 	}
	// 	cnt++;
	// }

	struct i2c_msg msgs[2];
	uint8_t wbuf[2];
	uint8_t rbuf[2] = {0};
	// int ret;
	int i;

	wbuf[0] = reg_addr;

	msgs[0].flags = 0;
	msgs[0].addr  = I2C_ADDR;
	msgs[0].len   = 1;
	msgs[0].buf   = wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = I2C_ADDR;
	msgs[1].len   = 2;
	msgs[1].buf   = rbuf;

	for(i=0; i<5; i++)
	{
		ret = i2c_transfer(adap, msgs, 2);
		if(ret == 2)
			break;
	}
	
	aw_printf("aw8082 read :%x  %x\n",rbuf[0],rbuf[1]);

	data = (uint16_t)(rbuf[0] & 0x00ff);
	data <<= 8;
	data |= (uint16_t)(rbuf[1] & 0x00ff);
	*reg_data = data;
	
	return (ret==ARRAY_SIZE(msgs)) ? 0 : -EIO;

}

int i2c_write(unsigned char reg_addr, unsigned int reg_data)
{
	int ret = AW_FAIL;
	// unsigned char cnt = 0;
	unsigned char buf[2];

	buf[0] = (reg_data&0xff00)>>8;
	buf[1] = (reg_data&0x00ff)>>0;

	// while (cnt < 5) {
	// 	ret = i2c_write_func(I2C_ADDR, reg_addr, buf, 2);
	// 	if (ret < 0) {
	// 		aw_printf("i2c_write cnt=%d error=%d", cnt, ret);
	// 	} else {
	// 		break;
	// 	}
	// 	cnt++;
	// }

	struct i2c_msg msg;
	uint8_t addr_buf[3];
	// int ret;
	int i;

	addr_buf[0] = reg_addr;
	addr_buf[1] = buf[0];
	addr_buf[2] = buf[1];

	msg.flags = 0;
	msg.addr = I2C_ADDR;
	msg.buf = addr_buf;
	msg.len = 3;

	for(i=0; i<5; i++)
	{
		ret = i2c_transfer(adap, &msg, 1);
		if(ret == 1)
			break;
	}

	//printf("reg[0x%x], val:0x%x, read:0x%x\n", reg, val, es7243e_i2c_read(client, reg));

	return (ret != 1 ? -EIO : 0);

	// return ret;
}

/*General I2C Write bits API*/
int i2c_write_bits(unsigned char reg_addr, unsigned int mask, unsigned int reg_data)
{
	int ret = -1;
	unsigned int reg_val = 0;
	ret = i2c_read(reg_addr, &reg_val);
	if (ret < 0) {
		return ret;
	}
	reg_val &= mask;
	reg_val |= reg_data;
	ret = i2c_write(reg_addr, reg_val);
	if (ret < 0) {
		return ret;
	}
	return AW_OK;
}
/*NOTE: i2c read/write concurrency is not allowed at this time, please use LOCK protection*/

/******* Relevant internal API of aw88082_init() *******/

/* aw88082_init():register configuration sequence*/
const uint16_t config_register[] = {
0x03,0x61FF,
0x04,0x2246,
0x05,0x8000,
0x06,0x04E8,
0x07,0x0010,
0x08,0x0056,
0x09,0x5F6A,
0x0A,0x000F,
0x0B,0x01E0,
0x0C,0x1C1E,
0x0D,0x00D8,
0x0E,0x94B5,
0x11,0x025F,
0x13,0x1B00,
0x14,0x2180,
0x15,0x8398,
0x16,0x02BE,
0x17,0x441A,
0x18,0x4001,
0x19,0x0000,
0x30,0x460C,
0x31,0xC989,
0x32,0x3541,
0x33,0x4DB8,
0x34,0xD6F9,
0x35,0x7ACE,
0x36,0xEC7C,
0x37,0x000C,
0x38,0x0400,
0x40,0x2250,
0x41,0x9D0B,
0x42,0x1A60,
0x54,0x2768,
0x55,0x0202,
0x56,0x3800,
0x57,0x8C88,
0x58,0x8909,
0x59,0x28A4,
0x5A,0x0000,
0x5B,0x0000,
0x5C,0x0140,
0x6F,0x0002,
0x70,0x0000,
0x71,0x0043,
};

static void aw_reg_update()
{
	int i = 0;
	uint16_t *data = NULL;
	int data_len;
	unsigned int reg_addr = 0;
	unsigned int reg_val = 0;

	data = (uint16_t *)config_register;
	data_len = sizeof(config_register) / sizeof(uint16_t);

	for (i = 0; i < data_len; i += 2) {
		reg_addr = data[i];
		reg_val = data[i + 1];

		/*Operate the specified register according to the timing requirements*/
		if (reg_addr == 0x04) {
			reg_val |= (1<<0);  // power down, set bit0 1
			reg_val &= ~(1<<2); // pll disable, set bit2 0
			reg_val &= ~(1<<1); // class D disable, set bit1 0
			reg_val |= (1<<8); // mute enable, set bit8 1
		}
		i2c_write(reg_addr, reg_val);	/*update register sequence*/
	}
}


/*PA initialization API*/
void aw88082_init(void)
{
	
	if (!(adap = i2c_open("i2c1"))) {
		aw_printf("%s, open i2c1 fail.\n", __func__);
		return ;
	}

	/*step1:configuration register*/
	aw_reg_update();

	/*step2:   read version*/
	unsigned int reg_val = 0;
	i2c_read(0x00,&reg_val);
	aw_printf("=======> version 0x%x \n",reg_val);

	aw_printf("done \n");
}

/******* Relevant internal API of aw_pa_stop() *******/
void aw_pa_stop(void)
{
	unsigned int reg_val = 0;

	/*step1: clear inturrupt*/
	i2c_read(0x02, &reg_val);
	i2c_read(0x02, &reg_val);
	
	i2c_write(0x03, 0xffff);

	/*step2:set mute*/
	i2c_write_bits(0x04, (~(1<<8)), 1<<8);

	MS_DELAY(5);         /*delay 5ms*/

	/*step3:amppd down*/
	i2c_write_bits(0x04, (~(1<<1)), 0<<0);

	/*step4:power down*/
	i2c_write_bits(0x04, (~(1<<2)), 0<<0);
	i2c_write_bits(0x04, (~(1<<0)), 1<<0);

	aw_printf("done");
}

/******* Relevant internal API of aw_pa_start() *******/
static int aw_mode1_pll_check()
{
	int ret = AW_FAIL;
	unsigned char i;
	unsigned int reg_val = 0;

	for (i = 0; i < 10; i++) {		/*check 10 times*/
		i2c_read(0x01, &reg_val);
		if (reg_val & (1<<0)) {
			ret = AW_OK;
			break;
		} else {
			MS_DELAY(2);
		}
	}

	return ret;
}
/* aw_pa_start():mode2 check in syspll check*/
static int aw_mode2_pll_check()
{
	int ret = AW_FAIL;
	unsigned int reg_val = 0;

	i2c_read(0x54, &reg_val);
	reg_val &= (1<<13);
	if (reg_val == (0<<13)) {	/*If DIVIDED,check failed*/
		return ret;
	}

	i2c_write_bits(0x54, (~(1<<13)), 0<<13);		/*Change to DIVIDED*/
	ret = aw_mode1_pll_check();
	i2c_write_bits(0x54, (~(1<<13)), 1<<13);		/*Change to BYPASS*/
	if (ret == 0) {
		MS_DELAY(2);
		ret = aw_mode1_pll_check();
	}
	return ret;
}

/*step2 of aw_pa_start():syspll check*/
static int aw_syspll_check()
{
	int ret = AW_FAIL;

	ret = aw_mode1_pll_check();
	if (ret < 0) {
		aw_printf("pll_check failed");	/*if mode1 check failed,check mode2*/
		aw_printf("mode1_pll_check failed");
		ret= aw_mode2_pll_check();
		if (ret < 0) {					/*if mode2 check failed,syspll check failed*/
			aw_printf("mode2_pll_check failed");
			return ret;
		}
	}
	return ret;
}

/*step4 of aw_pa_start():sys check*/
static int aw_sysst_check()
{
	int ret = AW_FAIL;
	unsigned char i = 0;
	unsigned int reg_val = 0;

	for (i = 0; i < 10; i++) {		/*check 10 times*/
		i2c_read(0x01, &reg_val);
		aw_printf("st = 0x%x", reg_val);
		if (((reg_val & 0x4b1b) & 0x111) == 0x111) {
			ret = AW_OK;		/*check pass*/
			break;
		} else {
			MS_DELAY(2);		/*if check failed,delay 2ms and check again*/
		}
	}
	return ret;
}

/*PA start API*/
int aw_pa_start(void)
{
	int ret = AW_FAIL;
	unsigned int reg_val = 0;

	/*step1:power on*/
	i2c_write_bits(0x04, (~(1<<0)), 0<<0);
    i2c_write_bits(0x04, (~(1<<2)), 1<<2);

	MS_DELAY(2);		/*delay 2ms*/

	/*step2:syspll check*/
	ret = aw_syspll_check();
	if (ret < 0) {
		/*if syspll check failed,PA power down*/
		aw_printf("syspll check failed");
		/*power down*/
		i2c_write_bits(0x04, (~(1<<0)), 1<<0);
		return ret;
	}

	/*step3:amppd on*/
	i2c_write_bits(0x04, (~(1<<1)), 1<<1);

	MS_DELAY(2);		/*delay 1ms*/

	/*step4:sys check*/
	ret = aw_sysst_check();
	if (ret < 0) {
		/*if sys check failed,PA stop*/
		aw_printf("sysst check failed");
		aw_pa_stop();
		return ret;
	}

	/*5tep5:disable mute*/
	i2c_write_bits(0x04, (~(1<<8)), 0<<4);

	/*step6: clear inturrupt*/
	i2c_read(0x02, &reg_val);
	i2c_read(0x02, &reg_val);

	aw_printf("done");
	return AW_OK;
}



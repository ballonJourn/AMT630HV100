#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"
#include "ark7116.h"

#ifdef VIDEO_DECODER_ARK7116

#define ARK7116_RST_GPIO		7

PanlstaticPara  AV1_staticPara[]=
{
//GLOBAL
    {0XFD02,0X00}, 
    {0XFD0A,0X48}, 
    {0XFD0B,0X1D}, 
    {0XFD0C,0X33}, 
    {0XFD0D,0X20}, 
    {0XFD0E,0X2C}, 
    {0XFD0F,0X09}, 
    {0XFD10,0X01}, 
    {0XFD11,0XFF}, 
    {0XFD12,0XFF}, 
    {0XFD13,0XFF}, 
    {0XFD14,0X02}, 
    {0XFD15,0X02}, 
    {0XFD16,0X02}, 
    {0XFD1A,0X45}, 
    {0XFD19,0X0A}, 
//PWM
//DECODER
    {0XFE01,0X06}, 
    {0XFE02,0X00}, 
    {0XFE06,0X07}, 
    {0XFE07,0X80}, 
    {0XFE08,0X0E}, 
    {0XFE0C,0X10}, 
    {0XFE0F,0X07}, 
    {0XFE13,0X16}, 
    {0XFE14,0X22}, 
    {0XFE15,0X05}, 
    {0XFE26,0X0E}, 
    {0XFE27,0X00}, 
    {0XFE48,0X87}, 
    {0XFE54,0X00}, 
    {0XFE55,0X01}, 
    {0XFE63,0XC0}, 
    {0XFE83,0XFF}, 
    {0XFED5,0X81}, 
    {0XFED7,0XF7}, 
    {0XFEDC,0X00}, 
    {0XFE44,0X20}, 
    {0XFE45,0X80}, 
    {0XFE43,0X80}, 
    {0XFECB,0X00}, 
    {0XFE56,0X07}, 
    {0XFEC9,0X01}, 
    {0XFE46,0X00}, 
    {0XFE42,0X00}, 
    {0XFE19,0X82}, 
    {0XFE1A,0X50}, 
    {0XFE1B,0X22}, 
    {0XFE1C,0X61}, 
    {0XFE1D,0X70}, 
//VP
    {0XFFB0,0X67}, 
    {0XFFB1,0X0F}, 
    {0XFFB2,0X10}, 
    {0XFFB3,0X10}, 
    {0XFFB4,0X10}, 
    {0XFFB7,0X10}, 
    {0XFFB8,0X10}, 
    {0XFFB9,0X22}, 
    {0XFFBA,0X20}, 
    {0XFFBB,0XFF}, 
    {0XFFBC,0X10}, 
    {0XFFC7,0X31}, 
    {0XFFC8,0X03}, 
    {0XFFC9,0X10}, 
    {0XFFCB,0X80}, 
    {0XFFCC,0X80}, 
    {0XFFCD,0X00}, 
    {0XFFCE,0X10}, 
    {0XFFCF,0X80}, 
    {0XFFD0,0X80}, 
    {0XFFD2,0X4F}, 
    {0XFFD3,0X80}, 
    {0XFFD4,0X80}, 
    {0XFFD7,0X05}, 
    {0XFFD8,0X80}, 
    {0XFFE7,0X50}, 
    {0XFFE8,0X10}, 
    {0XFFE9,0X22}, 
    {0XFFEA,0X20}, 
    {0XFFF0,0X2F}, 
    {0XFFF1,0XE1}, 
    {0XFFF2,0XEE}, 
    {0XFFF3,0XDC}, 
    {0XFFF4,0XFD}, 
    {0XFFF5,0X29}, 
    {0XFFF6,0XF7}, 
    {0XFFF7,0X9E}, 
    {0XFFF8,0X10}, 
    {0XFFF9,0X3D}, 
    {0XFFFA,0X4E}, 
    {0XFFFB,0X81}, 
    {0XFFD5,0X00}, 
    {0XFFD6,0X35}, 
//TCON
    {0XFC00,0X48}, 
    {0XFC01,0X00}, 
    {0XFC02,0X00}, 
    {0XFC09,0X06}, 
    {0XFC0A,0X33}, 
//SCALE
    {0XFC90,0X42}, 
    {0XFC91,0X00}, 
    {0XFC92,0X00}, 
    {0XFC93,0X0C}, 
    {0XFC94,0X00}, 
    {0XFC95,0X00}, 
    {0XFC96,0XE0}, 
    {0XFC97,0X03}, 
    {0XFC98,0X00}, 
    {0XFC99,0X04}, 
    {0XFC9A,0X57}, 
    {0XFC9B,0X03}, 
    {0XFC9C,0X02}, 
    {0XFC9D,0X00}, 
    {0XFC9E,0X06}, 
    {0XFC9F,0X00}, 
    {0XFCA0,0X23}, 
    {0XFCA1,0X00}, 
    {0XFCA2,0X1D}, 
    {0XFCA3,0X04}, 
    {0XFCA4,0X06}, 
    {0XFCA5,0X00}, 
    {0XFCA6,0X08}, 
    {0XFCA7,0X00}, 
    {0XFCA8,0X0A}, 
    {0XFCA9,0X00}, 
    {0XFCAA,0X09}, 
    {0XFCAB,0X01}, 
    {0XFCAC,0X12}, 
    {0XFCAD,0X00}, 
    {0XFCAE,0X00}, 
    {0XFCAF,0X00}, 
    {0XFCB0,0X00}, 
    {0XFCB1,0X00}, 
    {0XFCB2,0X00}, 
    {0XFCB3,0X00}, 
    {0XFCB4,0X00}, 
    {0XFCB5,0X00}, 
    {0XFCB7,0X0E}, 
    {0XFCB8,0X02}, 
    {0XFCBB,0X3B}, 
    {0XFCBC,0X01}, 
    {0XFCBD,0X00}, 
    {0XFCBE,0X00}, 
    {0XFCBF,0X0C}, 
    {0XFCC0,0X00}, 
    {0XFCC1,0X00}, 
    {0XFCC2,0X00}, 
    {0XFCC3,0X04}, 
    {0XFCC4,0X00}, 
    {0XFCC5,0X04}, 
    {0XFCC6,0X60}, 
    {0XFCC7,0X03}, 
    {0XFCC8,0X02}, 
    {0XFCC9,0X00}, 
    {0XFCCA,0X06}, 
    {0XFCCB,0X00}, 
    {0XFCCC,0X28}, 
    {0XFCCD,0X00}, 
    {0XFCCE,0XF8}, 
    {0XFCCF,0X02}, 
    {0XFCD1,0X00}, 
    {0XFCD2,0X15}, 
    {0XFCD3,0X00}, 
    {0XFCD4,0X0A}, 
    {0XFCD5,0X00}, 
    {0XFCD6,0X2F}, 
    {0XFCD7,0X01}, 
    {0XFCD8,0X00}, 
    {0XFCD9,0X07}, 
    {0XFCDA,0X00}, 
    {0XFCDB,0X00}, 
    {0XFCDC,0X00}, 
    {0XFCDD,0X14}, 
    {0XFCDE,0X00}, 
    {0XFCDF,0X00}, 
    {0XFCE0,0X00}, 
    {0XFCE1,0X02}, 
    {0XFCE3,0X01}, 
    {0XFCE4,0X02}, 
    {0XFCE7,0X03}, 
    {0XFCD0,0X0A}, 
    {0XFCE2,0X00}, 
    {0XFCB6,0X00}, 
    {0XFB35,0X00}, 
    {0XFB89,0X00}, 
//GAMMA
    {0XFF00,0X03}, 
    {0XFF01,0X0F}, 
    {0XFF02,0X1E}, 
    {0XFF03,0X2B}, 
    {0XFF04,0X36}, 
    {0XFF05,0X40}, 
    {0XFF06,0X4B}, 
    {0XFF07,0X55}, 
    {0XFF08,0X5F}, 
    {0XFF09,0X69}, 
    {0XFF0A,0X73}, 
    {0XFF0B,0X7D}, 
    {0XFF0C,0X85}, 
    {0XFF0D,0X8E}, 
    {0XFF0E,0X96}, 
    {0XFF0F,0X9F}, 
    {0XFF10,0XA7}, 
    {0XFF11,0XAE}, 
    {0XFF12,0XB6}, 
    {0XFF13,0XBD}, 
    {0XFF14,0XC4}, 
    {0XFF15,0XCA}, 
    {0XFF16,0XD1}, 
    {0XFF17,0XD7}, 
    {0XFF18,0XDD}, 
    {0XFF19,0XE3}, 
    {0XFF1A,0XE8}, 
    {0XFF1B,0XED}, 
    {0XFF1C,0XF1}, 
    {0XFF1D,0XF5}, 
    {0XFF1E,0XF8}, 
    {0XFF1F,0XFC}, 
    {0XFF20,0X0F}, 
    {0XFF21,0X1E}, 
    {0XFF22,0X2B}, 
    {0XFF23,0X36}, 
    {0XFF24,0X40}, 
    {0XFF25,0X4B}, 
    {0XFF26,0X55}, 
    {0XFF27,0X5F}, 
    {0XFF28,0X69}, 
    {0XFF29,0X73}, 
    {0XFF2A,0X7D}, 
    {0XFF2B,0X85}, 
    {0XFF2C,0X8E}, 
    {0XFF2D,0X96}, 
    {0XFF2E,0X9F}, 
    {0XFF2F,0XA7}, 
    {0XFF30,0XAE}, 
    {0XFF31,0XB6}, 
    {0XFF32,0XBD}, 
    {0XFF33,0XC4}, 
    {0XFF34,0XCA}, 
    {0XFF35,0XD1}, 
    {0XFF36,0XD7}, 
    {0XFF37,0XDD}, 
    {0XFF38,0XE3}, 
    {0XFF39,0XE8}, 
    {0XFF3A,0XED}, 
    {0XFF3B,0XF1}, 
    {0XFF3C,0XF5}, 
    {0XFF3D,0XF8}, 
    {0XFF3E,0XFC}, 
    {0XFF3F,0X0F}, 
    {0XFF40,0X1E}, 
    {0XFF41,0X2B}, 
    {0XFF42,0X36}, 
    {0XFF43,0X40}, 
    {0XFF44,0X4B}, 
    {0XFF45,0X55}, 
    {0XFF46,0X5F}, 
    {0XFF47,0X69}, 
    {0XFF48,0X73}, 
    {0XFF49,0X7D}, 
    {0XFF4A,0X85}, 
    {0XFF4B,0X8E}, 
    {0XFF4C,0X96}, 
    {0XFF4D,0X9F}, 
    {0XFF4E,0XA7}, 
    {0XFF4F,0XAE}, 
    {0XFF50,0XB6}, 
    {0XFF51,0XBD}, 
    {0XFF52,0XC4}, 
    {0XFF53,0XCA}, 
    {0XFF54,0XD1}, 
    {0XFF55,0XD7}, 
    {0XFF56,0XDD}, 
    {0XFF57,0XE3}, 
    {0XFF58,0XE8}, 
    {0XFF59,0XED}, 
    {0XFF5A,0XF1}, 
    {0XFF5B,0XF5}, 
    {0XFF5C,0XF8}, 
    {0XFF5D,0XFC}, 
    {0XFF5E,0XFF}, 
    {0XFF5F,0XFF}, 
    {0XFF60,0XFF}, 
};

/*点屏 PAD MUX 参数*/
PanlstaticPara  AMT_PadMuxStaticPara[]=
{
//PAD MUX
    {0XFD30,0X22}, 
    {0XFD32,0X11}, 
    {0XFD33,0X11}, 
    {0XFD34,0X55}, 
    {0XFD35,0X55}, 
    {0XFD36,0X55}, 
    {0XFD37,0X55}, 
    {0XFD38,0X55}, 
    {0XFD39,0X55}, 
    {0XFD3A,0X11}, 
    {0XFD3B,0X11}, 
    {0XFD3C,0X44}, 
    {0XFD3D,0X44}, 
    {0XFD3E,0X44}, 
    {0XFD3F,0X44}, 
    {0XFD40,0X44}, 
    {0XFD41,0X44}, 
    {0XFD44,0X01}, 
    {0XFD45,0X00}, 
    {0XFD46,0X00}, 
    {0XFD47,0X00}, 
    {0XFD48,0X00}, 
    {0XFD49,0X00}, 
    {0XFD4A,0X00}, 
    {0XFD4B,0X00}, 
    {0XFD50,0X0B}, 
    {0XFD51,0X55}, 
    {0XFD52,0X55}, 
    {0XFD53,0X57}, 
    {0XFD54,0X57}, 
    {0XFD55,0X55}, 
    {0XFD56,0X55}, 
    {0XFD57,0X55}, 
    {0XFD58,0X55}, 
    {0XFD59,0X5D}, 
    {0XFD5A,0X55}, 
};

/*-----------------------------------------------------------*/

static int ark7116_i2c_write (struct i2c_adapter *adap, UINT16 RegAddr, unsigned int data)
{
	struct i2c_msg msg;
	int ret = -1;
	u8 retries = 0;
	u8 buf[2];
    	UINT8 ucDeviceAddr;
	UINT8 uctmpDeviceAddr;
	UINT8 ucSubAddr;

	uctmpDeviceAddr = (unsigned char)((RegAddr>>8)&0XFF);
	ucSubAddr = (UINT8)(RegAddr&0XFF);

	switch(uctmpDeviceAddr)
	{
	    case 0XF9:
	    case 0XFD:
			 ucDeviceAddr= 0XB0;
			 break;

		case 0XFA:
			 ucDeviceAddr= 0XBE;
			 break;
			 	
		case 0XFB:
			 ucDeviceAddr= 0XB6;
			 break;

	    case 0XFC:
			 ucDeviceAddr= 0XB8;
			 break;

		case 0XFE:
			 ucDeviceAddr= 0XB2;
			 break;

		case 0XFF:
			 ucDeviceAddr= 0XB4;
			 break;

		case 0X00:
			ucDeviceAddr = 0XBE;
			break;
			
		default:
			 ucDeviceAddr= 0XB0;
			 break;			
	}

   
	buf[0] = (ucSubAddr);
	buf[1] = (data & 0xFF);

	msg.flags = !I2C_M_RD;
	msg.addr  = (ucDeviceAddr>>1);
	msg.len   = sizeof(buf);
	msg.buf   = buf;
	//printf("msg.addr is %x\n", msg.addr);
	//printf("msg.ucSubAddr is %x\n",buf[0]);
	//printf("msg.data is %x\n", buf[1]);
	while(retries < 10)
	{
	    //printf("retries is %x\n", retries);
		ret = i2c_transfer(adap, &msg, 1);
		//printf("ret is %x\n", ret);
		if (ret == 1)
			break;
		retries++;
		vTaskDelay (20);
	}
		
	if (retries >= 10)
	{
		printf("%s timeout11\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

static void ARK7116_reset(void)
{
	gpio_direction_output(ARK7116_RST_GPIO, 0);
	vTaskDelay(10);
	gpio_direction_output(ARK7116_RST_GPIO, 1);
	vTaskDelay(10);
	gpio_direction_output(ARK7116_RST_GPIO, 0);
    vTaskDelay(10);
}

int ConfigSlaveMode(struct i2c_adapter *adap)
{   
    unsigned char AddrBuff[6] = {0xa1,0xa2,0xa3,0xa4,0xa5,0xa6};
    unsigned char DataBuff[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char i;
	
    DataBuff[0] = 0X55;
    DataBuff[1] = 0xAA;
    DataBuff[2] = 0X03;
    DataBuff[3] = 0X50;  //slave mode
    DataBuff[4] = 0;     // crc val
    DataBuff[5] = DataBuff[2]^DataBuff[3]^DataBuff[4];

	printf("ConfigSlaveMode\n");
    ark7116_i2c_write(adap,MCU_CFG_ADDR,0X40);
	ark7116_i2c_write(adap,RSTN,0X00);
	ark7116_i2c_write(adap,RSTN,0X5A);	 
	
	if(ark7116_i2c_write(adap,BUS_STATUS_ADDR, 0x00) < 0)  //I2c Write Start
		return -1;
		
	for(i =0;i < 6;i++)
	{
	   if(ark7116_i2c_write(adap,AddrBuff[i], DataBuff[i]) < 0)
			return -1;
	}

	if(ark7116_i2c_write(adap,BUS_STATUS_ADDR, 0x11) < 0)  //I2c Write End
		return -1;

	vTaskDelay (200);
        return 0;
}

int AMT_WriteStaticPara(struct i2c_adapter *adap,PanlstaticPara * dataPt,UINT16 num)
{
	struct i2c_msg msg;
	int ret = -1;
	u8 retries = 0;
	u8 buf[2];
    	u8 ucDeviceAddr;
	u8 uctmpDeviceAddr;
	u8 ucSubAddr;
	u8 ucRegVal;

	while(num--)
	{
		uctmpDeviceAddr = (unsigned char)(((*dataPt).addr>>8)&0XFF);
		ucSubAddr = (unsigned char)(((*dataPt).addr)&0XFF);
		ucRegVal = (*dataPt).dat;

		switch(uctmpDeviceAddr)
		{
		case 0XF9:
		case 0XFD:
			ucDeviceAddr= 0XB0;
			break;

		case 0XFA:
			ucDeviceAddr= 0XBE;
			break;
		 	
		case 0XFB:
			ucDeviceAddr= 0XB6;
			break;

		case 0XFC:
			ucDeviceAddr= 0XB8;
			break;

		case 0XFE:
			ucDeviceAddr= 0XB2;
			break;

		case 0XFF:
			ucDeviceAddr= 0XB4;
			break;

		case 0X00:
			ucDeviceAddr = 0XBE;
			break;

		default:
			ucDeviceAddr= 0XB0;
			break;			
		}


		buf[0] = (ucSubAddr);
		buf[1] = (ucRegVal & 0xFF);

		msg.flags = !I2C_M_RD;
		msg.addr  = (ucDeviceAddr>>1);
		msg.len   = sizeof(buf);
		msg.buf   = buf;

		while(retries < 10)
		{
			ret = i2c_transfer(adap, &msg, 1);
			if (ret == 1)
			break;
			retries++;
			vTaskDelay (20);
		}

		if (retries >= 10)
		{
			return -1;
		}
		dataPt++;
	}	

	return 0;
}

void ConfigStaticPara(struct i2c_adapter *adap)
{    
 	 //CurretSource = CurretSource;	
	 AMT_WriteStaticPara(adap,AV1_staticPara,sizeof(AV1_staticPara)/sizeof(AV1_staticPara[0]));
}
void ConfigPadMuxPara(struct i2c_adapter *adap)
{   
	AMT_WriteStaticPara(adap,AMT_PadMuxStaticPara,
		sizeof(AMT_PadMuxStaticPara)/sizeof(AMT_PadMuxStaticPara[0]));
}

void InitGlobalPara(struct i2c_adapter *adap) //165
{	
	printf("InitGlobalPara\n");
	ark7116_i2c_write(adap,ENH_PLL,0X20);
	ConfigStaticPara(adap); 
	ConfigPadMuxPara(adap); 
	
	ark7116_i2c_write(adap,0xFD0C,0X33);
	ark7116_i2c_write(adap,0xFD14,0X02);
	//黑屏
	ark7116_i2c_write(adap,0xFFCE,0X00);
	ark7116_i2c_write(adap,0xFFCF,0X80);
	ark7116_i2c_write(adap,0xFFD0,0X80);
	ark7116_i2c_write(adap,ENH_PLL,0X2C);
	#if 1 //20190117 增加每次上电初始化完之后复位decoder
	//复位Decoder
	vTaskDelay (200);
	ark7116_i2c_write(adap,DECODER_RST,0X03);
	vTaskDelay (10);
	ark7116_i2c_write(adap,DECODER_RST,0X02);
	#endif
}

int ark7116_init(void)
{
	struct i2c_adapter *adap = NULL;

	if (!(adap = i2c_open("i2c1"))) {
		printf("open i2c1 fail.\n");
		return -1;
	}
	
	ARK7116_reset();
	
	if(ConfigSlaveMode(adap) < 0)
		return -1;

	InitGlobalPara(adap);

	return 0;
}

#endif

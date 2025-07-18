#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"

#ifdef VIDEO_DECODER_RN6752

#define RN6752_RST_GPIO		7
#define RN6752_SLAVE_ADDR	(0x58 >> 1)


/*-----------------------------------------------------------*/

static int rn6752_i2c_write (struct i2c_adapter *adap, unsigned int addr, unsigned int data)
{
	struct i2c_msg msg;
	int ret = -1;
	u8 retries = 0;
	u8 buf[2];

	buf[0] = (addr & 0xFF);
	buf[1] = (data & 0xFF);

	msg.flags = !I2C_M_RD;
	msg.addr  = RN6752_SLAVE_ADDR;
	msg.len   = sizeof(buf);
	msg.buf   = buf;
	
	while(retries < 5)
	{
		ret = i2c_transfer(adap, &msg, 1);
		if (ret == 1)
			break;
		retries++;
	}
		
	if (retries >= 5)
	{
		printf("%s timeout\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

/* static unsigned int rn6752_i2c_read(struct i2c_adapter *adap, unsigned int addr)
{
	struct i2c_msg msgs[2];
	int retries = 0;
	int ret = -1;
	u8 buf;

	buf = addr & 0xFF;
	
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = RN6752_SLAVE_ADDR;
	msgs[0].len   = 1;
	msgs[0].buf   = &buf;
	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = RN6752_SLAVE_ADDR;
	msgs[1].len   = 1;
	msgs[1].buf   = &buf;
	
	while(retries < 5)
	{
		ret = i2c_transfer(adap, msgs, 2);
		if(ret == 2)
			break;
		retries++;
	}
	
	if (retries >= 5)
	{
		printf( "%s timeout\n", __FUNCTION__);
		return 0;
	}

	return buf;
} */

static void rn6752_reset(void)
{
	gpio_direction_output(RN6752_RST_GPIO, 1);
	vTaskDelay(10);
	gpio_direction_output(RN6752_RST_GPIO, 0);
	vTaskDelay(10);
	gpio_direction_output(RN6752_RST_GPIO, 1);
	vTaskDelay(100);

}

typedef struct _RXCCHIPstaticPara
{
    unsigned int addr;
    unsigned int dat;
} RXCHIPstaticPara;

#if VIDEO_IN_FORMAT == VIN_AHD_720P_25
const RXCHIPstaticPara rn6752m_720p_25_staticPara[]=
{
	//RN6752M-601-720P(°üÀ¨Í¬Öá¿ØÖÆ)
	// 720P@25 BT601
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	{0x81, 0x01}, // turn on video decoder
	{0xA3, 0x04},
	{0xDF, 0xFE}, // enable HD format
	{0x88, 0x40}, // disable SCLK0B out
	{0xF6, 0x40}, // disable SCLK3A out

	// ch0
	{0xFF, 0x00}, // switch to ch0 (default; optional)
	{0x2C, 0x30},
	{0x2D, 0xF0},
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x00}, // No Insert Channel ID in SAV/EAV code
	{0x3F, 0x10}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x02}, // 720p resolution
	{0x56, 0x05}, // BT 72M mode and BT601 mode
	{0x5F, 0x40}, // blank level
	{0x63, 0xF5}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x42}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xE1}, // scale factor1
	{0x52, 0x88}, // scale factor2
	{0x53, 0x12}, // scale factor3
	{0x5B, 0x07}, // H-scaling control
	{0x5E, 0x08}, // enable H-scaling control
	{0x6A, 0x82}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x01, 0x08}, // brightness
	{0x02, 0x80}, // contrast
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x03}, // sharpness
	{0x09, 0xC8}, // EQ
	{0x34, 0x02}, // OB
	{0x57, 0x15}, // black/white stretch
	{0x68, 0x32}, // coring

	{0x00, 0x20}, // internal use*
	{0x0D, 0x20}, // cagc initial value
	//{0x2D, 0xF2}, // cagc adjust
	{0x37, 0X33},
	{0x61, 0X6C},

	//{0x30, 0X30},//	V30H_0836_NEW
	//{0x0d, 0X50},
	
	//{0x3A, 0x04},
	//{0x3E, 0x32},
	{0x3A, 0x02},//P-MOS
	{0x3E, 0xf6},//Í¬ÖáÓ³Éäµ½AVID½Å
	    
	{0x40, 0x04},
	{0x46, 0x23},

	{0x47, 0x30},
	
	//{0x49, 0x84},//85
	{0x6d, 0x00},
	
	{0x8E, 0x00}, // single channel output for VP
	{0x8F, 0x80}, // 720p mode for VP
	{0x8D, 0x31}, // enable VP out
	{0x89, 0x09}, // select 72MHz for SCLK
	{0x88, 0x41}, // enable SCLK out

	{0x96, 0x00}, // select AVID & VBLK as status indicator
	{0x97, 0x0B}, // enable status indicator out on AVID,VBLK & VSYNC 
	{0x98, 0x00}, // video timing pin status
	{0x9A, 0x40}, // select AVID & VBLK as status indicator 
	{0x9B, 0xE1}, // enable status indicator out on HSYNC
	{0x9C, 0x00}, // video timing pin status

	//{0x00, 0xC0},//test bar color
};
#endif

#if VIDEO_IN_FORMAT == VIN_AHD_720P_30
const RXCHIPstaticPara rn6752m_720p_30_staticPara[]=
{
	// 720P@30 BT601
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms
	0x49,0x01,
	0x19,0x07,
	0x81, 0x01, // turn on video decoder
	0xA3, 0x04,
	0xDF, 0xFE, // enable HD format
	0x88, 0x40, // disable SCLK0B out
	0xF6, 0x40, // disable SCLK3A out

	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x20, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x01, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x03, // sync control
	0x50, 0x02, // 720p resolution
	0x56, 0x05, // BT 72M mode and BT601 mode
	0x5F, 0x40, // blank level
	0x63, 0xF5, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x44, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x23, // data for extended register
	0x58, 0x01, // enable extended register write
	0x51, 0xE1, // scale factor1
	0x52, 0x88, // scale factor2
	0x53, 0x12, // scale factor3
	0x5B, 0x07, // H-scaling control
	0x5E, 0x0B, // enable H-scaling control
	0x6A, 0x82, // H-scaling control
	0x28, 0x92, // cropping
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x00, // sharpness
	0x57, 0x23, // black/white stretch
	0x68, 0x32, // coring
	0x3A, 0x04,
	0x3E, 0x32,
	0x40, 0x04,
	0x46, 0x23,

	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out

	0XD3,0X00,//channel 0  0x00   channel 1 0X01

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
};
#endif

#if VIDEO_IN_FORMAT == VIN_CVBS_NTSC
const RXCHIPstaticPara rn6752m_cvbs_ntsc_staticPara[]=
{
	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x00,
	0xDF, 0xFF, // enable CVBS format

	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x00, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x81, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x00, // sync control
	0x50, 0x00, // D1 resolution
	0x56, 0x04, // 27M mode and BT601 mode
	0x5F, 0x00, // blank level
	0x63, 0x75, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x5B, 0x00, // H-scaling control
	0x5E, 0x01, // enable H-scaling control
	0x6A, 0x00, // H-scaling control
	0x28, 0xB2, // cropping
	0x20, 0x24,
	0x23, 0x11,
	0x24, 0x05,
	0x25, 0x11,
	0x26, 0x00,
	0x42, 0x00,
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x03, // sharpness
	0x57, 0x20, // black/white stretch
	0x68, 0x32, // coring
	0x3A, 0x04,
	0x3E, 0x32,
	0x40, 0x04,
	0x46, 0x23,


	0x8E, 0x00, // single channel output for VP
	0x8F, 0x00, // D1 mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x00, // select 27MHz for SCLK
	0x88, 0x41, // enable SCLK out

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
};
#endif

#if VIDEO_IN_FORMAT == VIN_CVBS_PAL
const RXCHIPstaticPara rn6752m_cvbs_pal_staticPara[]=
{    
	// cvbs@25 BT601
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms
	0x49,0x01,
	0x19,0x07,
	0x81, 0x01, // turn on video decoder
	0xA3, 0x04,
	0xDF, 0x0F, // enable CVBS format

	0x88, 0x40, // disable SCLK0B out
	0xF6, 0x40, // disable SCLK3A out

	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x00, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x62, // HD format
	0x2A, 0x81, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x00, // sync control
	0x50, 0x00, // 720p resolution
	0x56, 0x05, // 72M mode and BT601 mode
	0x5F, 0x00, // blank level
	0x63, 0x75, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x5B, 0x00, // H-scaling control
	0x5E, 0x01, // enable H-scaling control
	0x6A, 0x00, // H-scaling control
	0x28, 0xB2, // cropping
	0x20, 0x24,
	0x23, 0x17,
	0x24, 0x37,
	0x25, 0x17,
	0x26, 0x00,
	0x42, 0x00,
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x03, // sharpness
	0x57, 0x20, // black/white stretch
	0x68, 0x32, // coring
	0x3A, 0x04,
	0x3E, 0x32,
	0x40, 0x04,
	0x46, 0x23,

	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
};
#endif

static void rn6752_config(struct i2c_adapter *adap)
{
	int i;
	//int val;
#if VIDEO_IN_FORMAT == VIN_CVBS_PAL
	for (i = 0; i < sizeof(rn6752m_cvbs_pal_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		rn6752_i2c_write(adap, rn6752m_cvbs_pal_staticPara[i].addr, 
			rn6752m_cvbs_pal_staticPara[i].dat);
		
	}
#elif VIDEO_IN_FORMAT == VIN_CVBS_NTSC
		for (i = 0; i < sizeof(rn6752m_cvbs_ntsc_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		rn6752_i2c_write(adap, rn6752m_cvbs_ntsc_staticPara[i].addr, 
			rn6752m_cvbs_ntsc_staticPara[i].dat);
		
	}
#elif  VIDEO_IN_FORMAT == VIN_AHD_720P_25
	for (i = 0; i < sizeof(rn6752m_720p_25_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		rn6752_i2c_write(adap, rn6752m_720p_25_staticPara[i].addr, 
			rn6752m_720p_25_staticPara[i].dat);
		
	}
#elif VIDEO_IN_FORMAT == VIN_AHD_720P_30
	for (i = 0; i < sizeof(rn6752m_720p_30_staticPara) / sizeof(RXCHIPstaticPara); i++) 
	{
		rn6752_i2c_write(adap, rn6752m_720p_30_staticPara[i].addr, 
			rn6752m_720p_30_staticPara[i].dat);
		
	}
#endif
}

int rn6752_init(void)
{
	struct i2c_adapter *adap = NULL;
	
	rn6752_reset();

	if (!(adap = i2c_open("i2c1"))) {
		printf("open i2c1 fail.\n");
		return -1;
	}
	
	rn6752_config(adap);

	return 0;
}

#endif

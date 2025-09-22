#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "amt630h.h"
#include "UartPrint.h"
#include "timer.h"
#include "spi.h"
#include "cp15.h"
#include "sysinfo.h"
#include "crc32.h"
#include "gpio.h"

#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
/* SPI commands */
#define SPI_RXFIFO_ENABLE				(1<<8)
#define SPI_TRANS_COMPLETE				(1<<7)
#define SPI_RXFIFO_OVER					(1<<6)

#define SPI_TXFIFO_FULL					(1<<2)
#define SPI_TXFIFO_REQ					(1<<1)

#define SPI_RXFIFO_FULL					(1<<4)
#define SPI_RXFIFO_NOTEMPTY		    	(1<<3)
#define SPI_TXFIFO_EMPTY				(1<<2)
#define SPI_TXFIFO_NOTFULL				(1<<1)
#define SPI_BUSY						(1<<0)

#define SPI0_CS0_GPIO					32
#define SPI0_IO0_GPIO					34
#define SPI_CS_GPIO						SPI0_CS0_GPIO


/* SPI NAND commands */
#define	SPI_NAND_WRITE_ENABLE			0x06
#define	SPI_NAND_WRITE_DISABLE			0x04
#define	SPI_NAND_GET_FEATURE			0x0f
#define	SPI_NAND_SET_FEATURE			0x1f
#define	SPI_NAND_PAGE_READ				0x13
#define	SPI_NAND_READ_CACHE				0x03
#define	SPI_NAND_FAST_READ_CACHE		0x0b
#define	SPI_NAND_READ_CACHE_X2			0x3b
#define	SPI_NAND_READ_CACHE_X4			0x6b
#define	SPI_NAND_READ_CACHE_DUAL_IO		0xbb
#define	SPI_NAND_READ_CACHE_QUAD_IO		0xeb
#define	SPI_NAND_READ_ID				0x9f
#define	SPI_NAND_PROGRAM_LOAD			0x02
#define	SPI_NAND_PROGRAM_LOAD4			0x32
#define	SPI_NAND_PROGRAM_EXEC			0x10
#define	SPI_NAND_PROGRAM_LOAD_RANDOM	0x84
#define	SPI_NAND_PROGRAM_LOAD_RANDOM4	0xc4
#define	SPI_NAND_BLOCK_ERASE			0xd8
#define	SPI_NAND_RESET					0xff

/* Registers common to all devices */
#define SPI_NAND_LOCK_REG				0xa0
#define SPI_NAND_PROT_UNLOCK_ALL		0x0

#define SPI_NAND_FEATURE_REG			0xb0
#define SPI_NAND_ECC_EN					(1 << 4)
#define SPI_NAND_QUAD_EN				(1 << 0)

#define SPI_NAND_STATUS_REG				0xc0
#define SPI_NAND_STATUS_REG_ECC_S1		(1 << 5)
#define SPI_NAND_STATUS_REG_ECC_S0		(1 << 4)
#define SPI_NAND_STATUS_REG_PROG_FAIL	(1 << 3)
#define SPI_NAND_STATUS_REG_ERASE_FAIL	(1 << 2)
#define SPI_NAND_STATUS_REG_WREN		(1 << 1)
#define SPI_NAND_STATUS_REG_BUSY		(1 << 0)

#define SNF_ECC_ERR_MASK				SPI_NAND_STATUS_REG_ECC_S1
#define SNF_MAX_NAME_SIZE				32
#define SNF_MAX_SPARE_SIZE				128		//64
#define SNF_MAX_PAGE_SIZE				4096	//2048
#define SNF_MAX_BLOCK_COUNT				2048

#define SNF_REPLACE_BLK_PERCENT			2	//default: 2%
#define SNF_BBT_BLK_COUNT				4
#define SNF_BBT_HEADER_SIZE				32
#define SNF_BBT_HEADER_MASK				"SNF BAD BLOCK TABLE"	//<32 bytes.

#define PLANE_SEL(x)					x
#define PLANE_NULL						0
#define PINFO_SEL(x)					x
#define PINFO_NULL						0


enum {
	SNF_OK,
	SNF_ERR,
	SNF_TIMEOUT,
	SNF_BAD_BLK,
};

enum {
	SNF_BLK_BAD = 0,
	SNF_BLK_USED,
	SNF_BLK_GOOD = 0xFF
};

typedef struct _snf_blk_node {
	u8 status;
	u16 replace;	//replace bad block.
} snf_blk_node;

typedef struct _snf_bbt {
	u8 header_mask[SNF_BBT_HEADER_SIZE];
	snf_blk_node node[SNF_MAX_BLOCK_COUNT];
	u32 bad_blocks;
	u32 reserved[8];
	u32 bbt_crc;
} snf_bbt;

typedef struct spi_nand_plane_select
{
	/* plane select is used for block select */
	u32 rpb;	//read page(from nand array to cache) bit.
	u32 rcb;	//read cache(from page cache to app buffer) bit.
	u32 plb;	//program load(write data to cache) bit.
	u32 peb;	//program excute(flush cache to nand array) bit.
} snf_ps;

typedef struct spi_nand_private_info {
	u32		PlaneSelIndex;
	u32		CrbsyBit;		//cache read busy status bit.
	u32		EccErrMask;		//ECC error mask.
} snf_priv_info;

typedef struct spi_nand_cfg_tag
{
	char	Name[SNF_MAX_NAME_SIZE];
	u8		MID;
	u8		DID0;
	u8		DID1;
	u8		BBMType;
	u32		BytePerPage;
	u32		PagePerBlk;
	u32		TotalBlk;
	u32		SprSize;
	u32		PrivInfoIndex;
} spi_nand_cfg;

typedef struct _snf_chip_info
{
	u8		Name[SNF_MAX_NAME_SIZE];
	u8		MID;
	u8		DID0;
	u8		DID1;
	u8		BBMType;
	u32		BytePerPage;
	u32		BytePerBlk;
	u32		TotalBlk;
	u32		SprSize;
	u32		FtlSprSize;
	//
	u32		AvailableBlks;
	u32		Capacity;
	u32		AvailableCapacity;
	u32		PagePerBlk;
	u32		BbtBlkOffset;
	u32		ReplaceBlks;
	snf_bbt	bbt;
	snf_priv_info *PrivInfo;
} snf_chip_info;

static int SpiNandFindAReplaceBlock(snf_chip_info *chip, u32 *block);
static int SpiNandUpdateBbt(void);
static int SpiNandCheckReplaceBlockValid(u32 block);


/* PlaneSelect bit array */
static const snf_ps SNF_PsArray[] = {
	[0] = {0, 0, 0, 0},		//Reserved.
	[1] = {6, 12, 12, 6},	//only for DS35Q2GA.
};

static const snf_priv_info SNF_PrivInfo[] = {
	/* Reserved */
	[0] = {
		PLANE_NULL,	// No plane select.
		0,			// No crbsyBit(use default bit0 for read/write/erase/reset busy status).
		0			/* No eccErrMask (ECC error occur and cannot be corrected, confirm it in the datasheet).
						0		: Not enable EccErrMask in private info, then will use the default SNF_ECC_ERR_MASK.
						others	: SPI_NAND_STATUS_REG_ECC_S0
								  SPI_NAND_STATUS_REG_ECC_S0 | SPI_NAND_STATUS_REG_ECC_S1
					*/
	},

	/* Active below */
	[1] = {PLANE_SEL(1), 0, 0},		/* DS35Q2GA: [plane select(6,12,12,6)][No crbsyBit][No eccErrMask] */
	[2] = {PLANE_NULL, 6, 0},		/* MX35LF1G: [No plane select][crbsyBit in bit6][No eccErrMask] */
	[3] = {PLANE_NULL, 7, 0}, 		/* MX35LF2G: [No plane select][crbsyBit in bit7][No eccErrMask] */
	[4] = {PLANE_NULL, 7, 0}, 		/* MX35LF4G: [No plane select][crbsyBit in bit7][No eccErrMask] */
};

static const spi_nand_cfg SNF_ChipCfgArray[] =
{
	// name,			MID, DID0, DID1, BBM,	BytePerPage, 	PagePerBlk,	TotalBlkCnt,	SpareSize	PrivateInfo
	{ "DS35Q1GA",		0xE5, 0x71, 0x00, 2, 	2048, 			64,			1024,			64,			PINFO_NULL},
	{ "DS35Q2GA",		0xE5, 0x72, 0x00, 2, 	2048, 			64,			2048,			64,			PINFO_SEL(1)},
	{ "GD5F2GM7",		0xc8, 0x92, 0x00, 1, 	2048, 			64,			2048,			64,			PINFO_NULL},
	{ "W25N01GV",		0xef, 0xaa, 0x21, 1, 	2048, 			64,			1024,			64,			PINFO_NULL},
	{ "W25N02KV",		0xef, 0xaa, 0x22, 1, 	2048, 			64,			2048,			64,			PINFO_NULL},
	{ "MX35LF1G",		0xc2, 0x12, 0x00, 1, 	2048,			64,			1024,			64,			PINFO_SEL(2)},
	{ "MX35LF2G",		0xc2, 0x26, 0x03, 1, 	2048,			64,			2048,			64,			PINFO_SEL(3)},
	{ "MX35LF4G",		0xc2, 0x37, 0x03, 1, 	4096,			64,			2048,			128,		PINFO_SEL(4)},
};

static u8 pageReadBuf[SNF_MAX_PAGE_SIZE];
static u8 pageWriteBuf[SNF_MAX_PAGE_SIZE];
static snf_chip_info g_snf = {0};

static void UInt2str(u32 number, char *str)
{
	int i=0, j;
	int temp;
	char buf[20];

	do {
		temp = number % 10;
		buf[i] = temp + 48;
		i++;
		number = number / 10;
	} while (number != 0);

	buf[i]='\0';

	for (j=0; j<i; j++) {
		str[j] = buf[i-j-1];
	}
	str[i]='\0';
}

static void SNF_PrintString(char *variable, char const *func)
{
	SendUartString((char *)func);
	SendUartString(": ");
	SendUartString(variable);
}

static void SNF_Print(char *variable, char const *func, int value)
{
	char buf[20];

	SendUartString((char *)func);
	SendUartString(": ");
	SendUartString(variable);
	if (value < 0) {
		SendUartString("-");
		value = -value;
	}
	UInt2str(value, buf);
	SendUartString(buf);
	SendUartString("\n");
}

static void SNF_Print2(char *variable1, char *variable2, char const *func, int val1, int val2)
{
	char buf[20];

	SendUartString((char *)func);
	SendUartString(": ");
	//var1
	SendUartString(variable1);
	if (val1 < 0) {
		SendUartString("-");
		val1 = -val1;
	}
	UInt2str(val1, buf);
	SendUartString(buf);
	SendUartString(", ");
	//var2
	SendUartString(variable2);
	if (val2 < 0) {
		SendUartString("-");
		val2 = -val2;
	}
	UInt2str(val2, buf);
	SendUartString(buf);
	SendUartString("\n");
}


static void delay(u32 time)
{
	volatile u32 count = time;
	while (count--);
}

static void SetCSGpioEnable(int enable)
{
	gpio_direction_output(SPI_CS_GPIO, !enable);
}

static void SpiWaitIdle(void)
{
	while (rSPI_SR & SPI_BUSY);
	udelay(1);
}

#if 0
static void SpiEmptyRxFIFO(void)
{
	int data = 0;
	(void)data;

	while(rSPI_SR & SPI_RXFIFO_NOTEMPTY)
		data = rSPI_DR;
}
#endif

static void SetSpiDataMode(u32 bitMode)
{
	u32 val = 0;
	SpiWaitIdle();;
	rSPI_SSIENR = 0;
	val = rSPI_CTLR0;
	val &=~(0x1f<<16);
	val |=((bitMode-1)<<16);
	rSPI_CTLR0 = val;
	rSPI_SSIENR = 1;
}

static void SpiSelectPad(void)
{
	u32 val;
	/* pad config */
	val = rSYS_PAD_CTRL02;
	val &= ~0xfff;
	val |= (0x1 << 10)|(0x1 << 8)|(0x1 << 6)|(0x1 << 4)| (0x1 << 2)/* | (1<<0)*/;
	rSYS_PAD_CTRL02 = val;

	/* clock config */
	val = rSYS_SSP_CLK_CFG;
	val |= (0x1<<31) | (0x1<<30);
	rSYS_SSP_CLK_CFG = val;

	//cs inactive first
	SetCSGpioEnable(0);
}

static void SpiSoftReset(void)
{
	u32 val;

	val = rSYS_SOFT_RST;
	val &= ~(0x1<<8);
	rSYS_SOFT_RST = val;
	udelay(10);
	val |= (0x1<<8);
	rSYS_SOFT_RST = val;
}

static void SpiInitialize()
{
	SpiSelectPad();
	SpiSoftReset();

	rSPI_SSIENR = 0;
	rSPI_CTLR0 = 0;
	rSPI_CTLR0 |= (0<<21)|(0x1f<<16)|(0x0<<12)|(0x0<<8)|(0x0<<4);
//	rSPI_CTLR1 = 63;
	rSPI_BAUDR = 4;//2;//16;//2;
	rSPI_SER = 1;
	rSPI_IMR = 0;
	rSPI_SSIENR = 1;
	SetCSGpioEnable(0);
}


static void SpinandSoftreset(void)
{
	SetSpiDataMode(8);
	SetCSGpioEnable(1);

	rSPI_DR = SPI_NAND_RESET;
	SpiWaitIdle();

	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static void SpiNandReadId(void)
{
	u8 val[5];
	int i;

	SetSpiDataMode(8);
	SetCSGpioEnable(1);

	rSPI_DR = SPI_NAND_READ_ID;
	rSPI_DR = 0;
	rSPI_DR = 0;
	rSPI_DR = 0;
	rSPI_DR = 0;

	for (i=0; i<5; i++)
	{
		while (!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		val[i] = rSPI_DR;
	}

	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);

	/* 3.3V:0xE571; 1.1V:0xE521 */
	//PrintVariableValueHex("ManufacturerID", val[2]);
	//PrintVariableValueHex("DeviceID", val[3]);

	g_snf.MID = val[2];
	g_snf.DID0 = val[3];
	g_snf.DID1 = val[4];
}

static void SpiNandWriteReg(u8 opcode, u8 val)
{
	SetSpiDataMode(8);
	SetCSGpioEnable(1);

	rSPI_DR = SPI_NAND_SET_FEATURE;
	rSPI_DR = opcode;
	rSPI_DR = val;

	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static u8 SpiNandReadReg(u8 opcode)
{
	u8 val;
	int i;

	SetSpiDataMode(8);
	SetCSGpioEnable(1);

	rSPI_DR = SPI_NAND_GET_FEATURE;
	rSPI_DR = opcode;
	rSPI_DR = 0;

	for (i = 0; i < 3; i++)
	{
		while (!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		val = rSPI_DR;
	}

	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);

	return val;
}

static void SpiNandWriteEnable(void)
{
	SetSpiDataMode(8);
	SetCSGpioEnable(1);

	rSPI_DR = SPI_NAND_WRITE_ENABLE;
	SpiWaitIdle();

	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static void SpiNandWriteDisable(void)
{
	SetSpiDataMode(8);
	SetCSGpioEnable(1);

	rSPI_DR = SPI_NAND_WRITE_DISABLE;
	SpiWaitIdle();

	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static int SpiNandWaitTillReady(void)
{
	int timeout = 100;
	u8 status;

	while (timeout--)
	{
		status = SpiNandReadReg(SPI_NAND_STATUS_REG);
		//printk("status=0x%x.\n", status);
		if (!(status & SPI_NAND_STATUS_REG_BUSY))
			return status;
		delay(1000);
	}

	return -SNF_TIMEOUT;
}

static int SpiNandWaitReadTillReady(void)
{
	int timeout = 100;
	int crbsyBit = g_snf.PrivInfo->CrbsyBit;
	int status;

	while (timeout--)
	{
		status = SpiNandReadReg(SPI_NAND_STATUS_REG);
		if (!(status & (1 << crbsyBit)))
			return status;
		delay(1000);
	}

	return -SNF_TIMEOUT;
}

static int SpiNandEnableECC(void)
{
	u8 status;

	status = SpiNandReadReg(SPI_NAND_FEATURE_REG);
	status |= SPI_NAND_ECC_EN;
	SpiNandWriteReg(SPI_NAND_FEATURE_REG, status);

	return SNF_OK;
}

static int SpiNandDisableQuad(void)
{
	u8 status;

	status = SpiNandReadReg(SPI_NAND_FEATURE_REG);
	status &= ~SPI_NAND_QUAD_EN;
	SpiNandWriteReg(SPI_NAND_FEATURE_REG, status);

	return SNF_OK;
}

void SpiNandLoadPage(u32 page)
{
	if (g_snf.PrivInfo && g_snf.PrivInfo->PlaneSelIndex) {
		int block = page / g_snf.PagePerBlk;
		if (block % 2)
			page |=  (1 << SNF_PsArray[g_snf.PrivInfo->PlaneSelIndex].rpb);//set plane select bit.
	}

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_NAND_PAGE_READ;
	rSPI_DR = (page >> 16) & 0xFF;
	rSPI_DR = (page >> 8) & 0xFF;
	rSPI_DR = page & 0xFF;
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

void SpiNandStorePage(u32 page)
{
	if (g_snf.PrivInfo && g_snf.PrivInfo->PlaneSelIndex) {
		int block = page / g_snf.PagePerBlk;
		if (block % 2)
			page |= (1 << SNF_PsArray[g_snf.PrivInfo->PlaneSelIndex].peb);
	}

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_NAND_PROGRAM_EXEC;
	rSPI_DR = (page >> 16) & 0xFF;
	rSPI_DR = (page >> 8) & 0xFF;
	rSPI_DR = page & 0xFF;
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

int SpiNandReadCache(u32 page, u32 offset, int length, u8 *read_buf)
{
	int i;
	u8 val = 0;
	u32 addr = (offset & 0xFFF);

    (void)val;

	if (g_snf.PrivInfo && g_snf.PrivInfo->PlaneSelIndex) {
		int block = page / g_snf.PagePerBlk;
		if (block % 2)
			addr |= (1 << SNF_PsArray[g_snf.PrivInfo->PlaneSelIndex].rcb);
	}

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_NAND_READ_CACHE;
	rSPI_DR = (addr >> 8) & 0xFF;
	rSPI_DR = addr & 0xFF;
	rSPI_DR = 0x0;	//dummy byte.

	for (i = 0; i < 4; i++)
	{
		while (!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		val = rSPI_DR;
	}

	for (i = 0; i < length; i++)
	{
		rSPI_DR = 0;
		while (!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		read_buf[i] = rSPI_DR;
	}

	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
	return SNF_OK;
}

static int SpiNandReadPageSpare(u32 page, char *buf)
{
	int status;
	int page_size = g_snf.BytePerPage;
	int spare_size = g_snf.SprSize;
	int eccErrMask = SNF_ECC_ERR_MASK;

	SpiNandLoadPage(page);
	
	if (g_snf.PrivInfo && g_snf.PrivInfo->CrbsyBit) {
		status = SpiNandWaitReadTillReady();
	} else {
		status = SpiNandWaitTillReady();
	}
	if (status < 0)
		return status;

	if (g_snf.PrivInfo && g_snf.PrivInfo->EccErrMask)
		eccErrMask = (1 << g_snf.PrivInfo->EccErrMask);

	if (status & eccErrMask)
	{
		PrintVariableValueHex("SpiNandReadPageSpare ECC error, status:", status);
		SNF_Print("pagenum:", __func__, page);
		return -SNF_BAD_BLK;
	}
	return SpiNandReadCache(page, page_size, spare_size, (u8 *)buf);
}

static int SpiNandStoreCache(u32 page, u32 offset, int length, u8 *write_buf)
{
	int i;
	u32 addr = (offset & 0xFFF);

	if (g_snf.PrivInfo && g_snf.PrivInfo->PlaneSelIndex) {
		int block = page / g_snf.PagePerBlk;
		if (block % 2)
			addr |= (1 << SNF_PsArray[g_snf.PrivInfo->PlaneSelIndex].plb);	//set read cache plane select bit.
	}

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_NAND_PROGRAM_LOAD;
	rSPI_DR = (addr >> 8) & 0xFF;
	rSPI_DR = addr & 0xFF;

	for (i = 0; i < length; i++)
	{
		while (!(rSPI_SR & SPI_TXFIFO_NOTFULL));
		rSPI_DR = write_buf[i];
	}

	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);

	return SNF_OK;
}

static int SpiNandEraseBlock(int block)
{
	u32 page_offset;
	int status;
	int timeout = 0;

retry:
	SpiNandWriteEnable();
	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	page_offset = block * g_snf.PagePerBlk;
	rSPI_DR = SPI_NAND_BLOCK_ERASE;
	rSPI_DR = (page_offset >> 16) & 0xff;
	rSPI_DR = (page_offset >> 8) & 0xff;
	rSPI_DR = page_offset & 0xff;
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);

	status = SpiNandWaitTillReady();
	if (status < 0) {
		if (timeout++ < 3)
			goto retry;
		SNF_Print("block erase timeout, block:", __func__, block);
		return status;
	}

	if (status & SPI_NAND_STATUS_REG_ERASE_FAIL) {
		/*find a new bad block. */
		SNF_Print("block erase failed, block:", __func__, block);
		return -SNF_BAD_BLK;
	}

	return SNF_OK;
}

/* read a few bytes in one page */
static int SpiNandReadBytesInPage(u32 page, u32 offset, u8 *data, u32 size)
{
	snf_chip_info *chip = &g_snf;
	int eccErrMask = SNF_ECC_ERR_MASK;
	int status;

	SpiNandLoadPage(page);
	if (g_snf.PrivInfo && g_snf.PrivInfo->CrbsyBit) {
		status = SpiNandWaitReadTillReady();
	} else {
		status = SpiNandWaitTillReady();
	}
	if (status < 0) {
		SNF_Print2("timeout, block:", "page", __func__, page / chip->PagePerBlk, page % chip->PagePerBlk);
		return status;
	}

	if (g_snf.PrivInfo && g_snf.PrivInfo->EccErrMask)
		eccErrMask = (1 << g_snf.PrivInfo->EccErrMask);

	if (status & eccErrMask)	//ECC error meains bad block.
	{
		SNF_Print2("ECC error blk:", "page:", __func__, page / chip->PagePerBlk, page % chip->PagePerBlk);
		return -SNF_ERR;//-SNF_BAD_BLK;
	}
	return SpiNandReadCache(page, offset, size, data);
}

static int SpiNandReadOnePage(u32 page, u8 *data)
{
	return SpiNandReadBytesInPage(page, 0, data, g_snf.BytePerPage);
}

/* write a few bytes in one page */
static int SpiNandWriteBytesInPage(u32 page, u32 offset, u8 *data, u32 size)
{
	int timeout = 0;
	int ret;

retry:
	SpiNandWriteEnable();
	SpiNandStoreCache(page, offset, size, data);
	SpiNandStorePage(page);
	ret = SpiNandWaitTillReady();
	if (ret < 0) {
		if (timeout++ < 3)
			goto retry;
		ret = -SNF_TIMEOUT;
	} else if (ret & SPI_NAND_STATUS_REG_PROG_FAIL) {
		ret = -SNF_BAD_BLK;
	} else {
		ret = SNF_OK;
	}

	return ret;
}

static int SpiNandWriteOnePage(u32 page, u8 *data)
{
	return SpiNandWriteBytesInPage(page, 0, data, g_snf.BytePerPage);
}

static int SpiNandReadPhysicalBlock(u32 block , u32 offset_in_block, u8 *data, u32 size)
{
	snf_chip_info *chip = &g_snf;
	u8 *pdata = data;
	u32 currpage, BytePerPage;
	u32 offset_in_page;
	u32 read_size;
	int remain = size;
	int ret;

	BytePerPage = chip->BytePerPage;
	currpage = block * chip->PagePerBlk + offset_in_block / BytePerPage;
	offset_in_page = offset_in_block % BytePerPage;

	while (remain > 0) {
		read_size = ((BytePerPage - offset_in_page) >= remain) ? remain : (BytePerPage - offset_in_page);
		ret = SpiNandReadBytesInPage(currpage, offset_in_page, pdata, read_size);
		if (ret != SNF_OK) {
			SNF_Print2("read page falied, page:", "block:", __func__, currpage, block);
			break;
		}

		if (offset_in_page)
			offset_in_page = 0;

		currpage++;
		remain -= read_size;
		pdata += read_size;
	}

	return ret;
}

static int SpiNandReadInBlock(u32 block , u32 offset_in_block, u8 *data, u32 size)
{
	snf_chip_info *chip = &g_snf;
	u32 blk = block;
	int ret;

	//bad block check and replace.
	if (chip->bbt.node[block].status == SNF_BLK_BAD) {
		blk = chip->bbt.node[block].replace;
		ret = SpiNandCheckReplaceBlockValid(blk);
		if ((ret == SNF_OK) && (chip->bbt.node[blk].status != SNF_BLK_BAD)) {
			//find replace block.
			if (chip->bbt.node[blk].status != SNF_BLK_USED) {
				SNF_Print2("block:", "replace blk:", __func__, block, blk);
				chip->bbt.node[blk].status = SNF_BLK_USED;
			}
		} else {
			SNF_Print2("bad block:", "invalid replace blk:", __func__, block, blk);
			return -SNF_BAD_BLK;
		}
	}

	return SpiNandReadPhysicalBlock(blk, offset_in_block, data, size);
}

static int SpiNandReadData(u32 addr, void *data, int size)
{
	snf_chip_info *chip = &g_snf;
	u8 *pdata;
	u32 read_size, bytePerBlk;
	u32 currblk,offset_in_block;
	int remain;
	int ret = SNF_OK;

	pdata = (u8 *)data;
	remain = size;
	bytePerBlk = chip->BytePerBlk;
	currblk = addr / bytePerBlk;
	offset_in_block = addr % bytePerBlk;

	//area check valid.
	if ((addr + size) > chip->AvailableCapacity) {
		SNF_PrintString("burn out of boundry.\n", __func__);
		return -SNF_ERR;
	}

	while (remain > 0) {
		//read block.
		read_size = ((bytePerBlk - offset_in_block) >= remain) ? remain : (bytePerBlk - offset_in_block);
		ret = SpiNandReadInBlock(currblk, offset_in_block, pdata, read_size);
		if (ret != SNF_OK) {
			SNF_Print("SpiNandReadInBlock failed, block:", __func__, currblk);
			break;
		}

		if (offset_in_block)
			offset_in_block = 0;

		currblk++;
		pdata += read_size;
		remain -= read_size;
		if (remain < 0) {
			ret = -SNF_ERR;
			SNF_PrintString("Invalid remain.\n", __func__);
			break;
		}
	}

	return ret;
}

/* offset is at least align to SECOTR_SIZE */
static int SpiNandBurnPage(int page, u8 *buf)
{
	int ret;

	ret = SpiNandWriteOnePage(page, buf);
	if (ret == SNF_OK) {
		u32 *tmp = (u32 *)buf;
		u32 *readBuf = (u32 *)pageReadBuf;
		int wordPerPage = g_snf.BytePerPage / 4;
		int i;

		ret = SpiNandReadOnePage(page, pageReadBuf);
		if (ret == SNF_OK) {
			for (i = 0; i < wordPerPage; i++) {
				if (tmp[i] != readBuf[i]) {
					SNF_Print("burn error, pos:", __func__, i);
					PrintVariableValueHex("write:", tmp[i]);
					PrintVariableValueHex("read:", readBuf[i]);
					return -SNF_ERR;
				}
			}
		} else {
			SNF_Print("read page failed, page:", __func__, page);
			return -SNF_ERR;	//read page should not create a bad block.
		}
	} else {
		SNF_Print("SpiNandWriteOnePage failed, page:", __func__, page);
		return ret;
	}

	return SNF_OK;
}

static int SpiNandBurnBlock(u32 block, u32 offset_in_block, const u8 *data, u32 size)
{
	snf_chip_info *chip = &g_snf;
	u8 *buf = (u8 *)data;
	u32 left_size = size;
	u32 byte_per_page;
	u32 start_page;
	u32 whole_page_num;
	u32 result;
	u32 i;

	byte_per_page = chip->BytePerPage;
	start_page = block * chip->PagePerBlk + offset_in_block / byte_per_page;

	if (offset_in_block % byte_per_page) {
		SNF_PrintString("offset is not align with page.\n", __func__);
		return -SNF_ERR;
	}

	if (left_size > 0) {
		whole_page_num = left_size / byte_per_page;
		for (i=0; i<whole_page_num; i++) {
			result = SpiNandBurnPage(start_page, buf);
			if (result != SNF_OK) {
				SNF_Print2("Burn page failed, page:", "result:", __func__, start_page, result);
				return result;
			}
			buf += byte_per_page;
			left_size -= byte_per_page;
			start_page++;
		}
	}

	if (left_size > 0) {
		memset(pageWriteBuf + left_size, 0xff, byte_per_page - left_size);
		memcpy(pageWriteBuf, buf, left_size);
		result = SpiNandBurnPage(start_page, pageWriteBuf);
		if (result != SNF_OK) {
			SNF_Print2("Burn page failed, page:", "result:", __func__, start_page, result);
			return result;
		}
	}

	return result;
}

static int SpiNandEraseWriteInBlock(u32 block, u32 offset_in_block, void *data, u32 size)
{
	snf_chip_info *chip = &g_snf;
	int ret;
	u32 blk = block;
	u32 bbtUpdate = 0;

	//bad block check and replace.
	if (chip->bbt.node[block].status == SNF_BLK_BAD) {
		blk = chip->bbt.node[block].replace;
		ret = SpiNandCheckReplaceBlockValid(blk);
		if ((ret == SNF_OK) && (chip->bbt.node[blk].status != SNF_BLK_BAD)) {
			//find replace block.
			if (chip->bbt.node[blk].status != SNF_BLK_USED) {
				SNF_Print2("block:", "replace blk:", __func__, block, blk);
				chip->bbt.node[blk].status = SNF_BLK_USED;
			}
		} else {
retry:
			//find a new replace block.
			ret = SpiNandFindAReplaceBlock(chip, &blk);
			if (ret != SNF_OK) {
				SNF_PrintString("SpiNandFindAReplaceBlock failed, no replace block.\n", __func__);
				goto exit;
			}

			SNF_Print2("block:","find a new replace blk:", __func__, block, blk);
			chip->bbt.node[block].replace = blk;
			bbtUpdate = 1;
		}
	}

	if (offset_in_block == 0) {
		ret = SpiNandEraseBlock(blk);//phy block
		if (ret != SNF_OK) {
			SNF_Print("SpiNandEraseBlock failed, mask bad block and try to find a replace block, blk:", __func__, blk);
			chip->bbt.node[blk].status = SNF_BLK_BAD;
			chip->bbt.bad_blocks++;
			bbtUpdate = 1;
			goto retry;
		}
	}

	ret = SpiNandBurnBlock(blk, offset_in_block, data, size);//phy block
	if (ret != SNF_OK) {
		if ((ret == -SNF_BAD_BLK) && (offset_in_block == 0)) {
			SNF_Print("SpiNandWriteData failed, try to find replace block, blk:", __func__, blk);
			chip->bbt.node[blk].status = SNF_BLK_BAD;
			chip->bbt.bad_blocks++;
			bbtUpdate = 1;
			goto retry;
		} else {
			SNF_Print("SpiNandWriteData failed, blk:", __func__, blk);
		}
	}

exit:
	if (bbtUpdate)
		SpiNandUpdateBbt();

	return ret;
}

static int SpiNandCheckReplaceBlockValid(u32 block)
{
	snf_chip_info *chip = &g_snf;

	if ((block >= chip->AvailableBlks) && (block < chip->BbtBlkOffset)) {
		return SNF_OK;
	}

	return -SNF_ERR;
}

static int SpiNandCheckPhysicalBadBlock(u32 blknum)
{
	int page = blknum * g_snf.PagePerBlk;
	int bbm = g_snf.BBMType;
	char spareBuf[SNF_MAX_SPARE_SIZE] = {0};
	int ret;

	ret = SpiNandReadPageSpare(page, spareBuf);
	if (ret != SNF_OK) {
		SNF_Print("SpiNandReadPageSpare page0 failed, ret:", __func__, ret);
		goto exit;
	}

	if (spareBuf[0] == 0xFF) {
		if(bbm == 2) {
			ret = SpiNandReadPageSpare(page+1, spareBuf);
			if (ret != SNF_OK) {
				SNF_Print("SpiNandReadPageSpare page1 failed, ret:", __func__, ret);
				goto exit;
			}
			if (spareBuf[0] == 0xFF) {
				//SNF_Print("good block:", __func__, blknum);
				return SNF_OK;
			} else {
				//SNF_Print("bad block:", __func__, blknum);
			}
		} else {
			return SNF_OK;
		}
	}

exit:
	SNF_Print("bad block:", __func__, blknum);

	return -SNF_BAD_BLK;
}

static int SpiNandFindAReplaceBlock(snf_chip_info *chip, u32 *block)
{
	int i;

	u32 replace_blk_start = chip->AvailableBlks;
	u32 replace_blk_stop = chip->AvailableBlks + chip->ReplaceBlks;

	for (i=replace_blk_stop-1; i>=replace_blk_start; i--) {
		if (chip->bbt.node[i].status == SNF_BLK_GOOD) {
			*block = i;
			chip->bbt.node[i].status = SNF_BLK_USED;
			return SNF_OK;
		}
	}

	SNF_PrintString("there are no valid replace blocks.\n", __func__);

	return -SNF_ERR;
}

static int SpiNandWriteBbt(u8 *buf , u32 size)
{
	snf_chip_info *chip = &g_snf;
	int curr_block;
	int i, ret;
	int bbt_bad_blocks = 0;
	int write_block_num = 0;

	for(i=0; i<SNF_BBT_BLK_COUNT; i++) {
		curr_block = chip->BbtBlkOffset + i;

		if(write_block_num >= 2) {
			break;
		}

		if(chip->bbt.node[curr_block].status == SNF_BLK_BAD) {
			bbt_bad_blocks++;
			continue;
		}

		ret = SpiNandEraseBlock(curr_block);
		if (ret != SNF_OK) {
			SNF_Print2("erase block failed, block: ", "ret:", __func__, curr_block, ret);
			if (ret == -SNF_BAD_BLK) {
				chip->bbt.node[curr_block].status = SNF_BLK_BAD;
				chip->bbt.bad_blocks++;
				bbt_bad_blocks++;
				continue;
			}
			break;
		}

		ret = SpiNandBurnBlock(curr_block, 0, buf, size);
		if (ret != SNF_OK) {
			SNF_Print2("burn block failed, block: ", "ret:", __func__, curr_block, ret);
			if (ret == -SNF_BAD_BLK) {
				chip->bbt.node[curr_block].status = SNF_BLK_BAD;
				chip->bbt.bad_blocks++;
				bbt_bad_blocks++;
				continue;
			}
			break;
		}

		write_block_num++;
	}

	if (bbt_bad_blocks) {
		SNF_Print("%s, bbt has bad blcok: ", __func__, bbt_bad_blocks);
	}

	return ret;
}

static int SpiNandReadBbt(u8* buf, u32 size)
{
	snf_chip_info *chip = &g_snf;
	u32 crc_size;
	u32 checksum;
	int curr_block;
	int ret, i;

	crc_size = size - SNF_BBT_HEADER_SIZE - sizeof(chip->bbt.bbt_crc);

	for(i=0; i<SNF_BBT_BLK_COUNT; i++) {
		curr_block = chip->BbtBlkOffset + i;
		ret = SpiNandReadPhysicalBlock(curr_block, 0, buf, size);
		if (ret != SNF_OK) {
			SNF_Print("SpiNandReadPhysicalBlock failed, block:", __func__, curr_block);
			continue;
		}

		if (memcmp(chip->bbt.header_mask, SNF_BBT_HEADER_MASK, sizeof(SNF_BBT_HEADER_MASK))) {
			SNF_Print("Not fond BBT header, block:", __func__, curr_block);
			ret = -SNF_ERR;
			continue;
		}

		checksum = xcrc32(buf + SNF_BBT_HEADER_SIZE, crc_size, 0xFFFFFFFF);

		if (chip->bbt.bbt_crc != checksum) {
			SNF_Print("BBT information is abnormal, block:", __func__, curr_block);
			ret = -SNF_ERR;
			continue;
		}

		break;
	}

	return ret;
}

static int SpiNandUpdateBbt(void)
{
	snf_chip_info *chip = &g_snf;
	int ret = SNF_OK;
	u32 bbt_size, crc_size;
	u8 *pbuf;

	pbuf = (u8 *)&chip->bbt;
	bbt_size = sizeof(snf_bbt);
	crc_size = bbt_size - SNF_BBT_HEADER_SIZE - sizeof(chip->bbt.bbt_crc);

	chip->bbt.bbt_crc = xcrc32(pbuf + SNF_BBT_HEADER_SIZE, crc_size, 0xFFFFFFFF);

	ret = SpiNandWriteBbt(pbuf, bbt_size);
	if (ret != SNF_OK) {
		SNF_Print("write bbt failed, ret:", __func__, ret);
	}

	return ret;
}

static int SpiNandBbtInit(void)
{
	snf_chip_info *chip = &g_snf;
	int ret = 0;
	u32 bbt_size;
	u8 *pbuf;
	int i;

	pbuf = (u8 *)&chip->bbt;
	bbt_size = sizeof(snf_bbt);

	ret = SpiNandReadBbt(pbuf, bbt_size);
	if (ret != SNF_OK) {
		memset(&chip->bbt, 0, bbt_size);
		SNF_PrintString("Create a new BBT.\n", __func__);
		for (i=0; i<chip->TotalBlk; i++) {
			ret = SpiNandCheckPhysicalBadBlock(i);
			if (ret == SNF_OK) {
				chip->bbt.node[i].status = SNF_BLK_GOOD;
			} else {
				chip->bbt.node[i].status = SNF_BLK_BAD;
				chip->bbt.bad_blocks++;
			}
			chip->bbt.node[i].replace = 0;
		}

		memcpy(chip->bbt.header_mask, SNF_BBT_HEADER_MASK, sizeof(SNF_BBT_HEADER_MASK));
		SpiNandUpdateBbt();
		ret = SNF_OK;
	}

	return ret;
}

static int SpiNandSetAttribute(snf_chip_info *chip)
{
	spi_nand_cfg *cfg = (spi_nand_cfg *)SNF_ChipCfgArray;
	int fond = 0;
	int i;

	if (!chip) {
		SNF_PrintString("SpiNandSetAttribute: Invalid chip.\n", __func__);
		return -SNF_ERR;
	}

	for (i=0; i<sizeof(SNF_ChipCfgArray)/sizeof(spi_nand_cfg); i++) {
		if ((chip->MID == cfg[i].MID) && (chip->DID0 == cfg[i].DID0)) {
			if ((cfg[i].DID1 != 0) && (chip->DID1 != cfg[i].DID1)) {
				continue;
			}
			fond = 1;
			break;
		}
	}

	if (fond) {
		memcpy(chip->Name, cfg[i].Name, SNF_MAX_NAME_SIZE);
		chip->BBMType				= cfg[i].BBMType;
		chip->BytePerPage			= cfg[i].BytePerPage;
		chip->PagePerBlk			= cfg[i].PagePerBlk;
		chip->BytePerBlk			= chip->BytePerPage * chip->PagePerBlk;
		chip->TotalBlk				= cfg[i].TotalBlk;
		chip->SprSize				= cfg[i].SprSize;
		chip->FtlSprSize			= 24;	//reserved.
		chip->Capacity				= chip->BytePerBlk * chip->TotalBlk;
		chip->ReplaceBlks			= chip->TotalBlk * SNF_REPLACE_BLK_PERCENT / 100;
		chip->AvailableBlks			= chip->TotalBlk - chip->ReplaceBlks - SNF_BBT_BLK_COUNT;
		chip->AvailableCapacity		= chip->AvailableBlks * chip->BytePerBlk;
		chip->BbtBlkOffset			= chip->TotalBlk - SNF_BBT_BLK_COUNT;

		if (cfg[i].PrivInfoIndex) {
			u32 index = cfg[i].PrivInfoIndex;

			if (index >= sizeof(SNF_PrivInfo) / sizeof(SNF_PrivInfo[0])) {
				SNF_Print("###ERR: %s: Invalid privInfo index:", __func__, index);
				chip->PrivInfo = NULL;
				return -SNF_ERR;
			}
			chip->PrivInfo = (snf_priv_info *)&SNF_PrivInfo[index];

			//plane select check.
			if (chip->PrivInfo->PlaneSelIndex) {
				if (chip->PrivInfo->PlaneSelIndex >= sizeof(SNF_PsArray)/sizeof(SNF_PsArray[0])) {
					SNF_Print("###ERR: %s: Invalid PlaneSel index:n", __func__, chip->PrivInfo->PlaneSelIndex);
					//chip->PrivInfo->PlaneSelIndex = 0;
					return -SNF_ERR;
				}
			}
		} else {
			chip->PrivInfo = NULL;
		}
	} else {
		SNF_PrintString("Not find spi nand flash.\n", __func__);
		return -SNF_ERR;
	}

	if (chip->BytePerPage > SNF_MAX_PAGE_SIZE) {
		SNF_Print("Invalid SNF_MAX_PAGE_SIZE:", __func__, SNF_MAX_PAGE_SIZE);
		return -SNF_ERR;
	}
	if (chip->SprSize > SNF_MAX_SPARE_SIZE) {
		SNF_Print("Invalid SNF_MAX_SPARE_SIZE:", __func__, SNF_MAX_SPARE_SIZE);
		return -SNF_ERR;
	}
	if (chip->TotalBlk > SNF_MAX_BLOCK_COUNT) {
		SNF_Print("Invalid SNF_MAX_BLOCK_COUNT:", __func__, SNF_MAX_BLOCK_COUNT);
		return -SNF_ERR;
	}

	/* Partition check */
	if (STEPLDRA_OFFSET % chip->BytePerBlk || STEPLDRB_OFFSET % chip->BytePerBlk) {
		SNF_Print("###ERR: %s: STEPLDR OFFSET not align with block size:", __func__, chip->BytePerBlk);
		return -SNF_ERR;
	}

	if (SYSINFOA_OFFSET % chip->BytePerBlk || SYSINFOB_OFFSET % chip->BytePerBlk) {
		SNF_Print("###ERR: %s: SYSINFO OFFSET not align with block size:", __func__, chip->BytePerBlk);
		return -SNF_ERR;
	}

	if (IMAGE_OFFSET % chip->BytePerBlk) {
		SNF_Print("###ERR: %s: IMAGE OFFSET not align with block size:", __func__, chip->BytePerBlk);
		return -SNF_ERR;
	}

	return SNF_OK;
}

int SpiInit(void)
{
	snf_chip_info *snf = &g_snf;
	int ret;

	memset(snf, 0, sizeof(snf_chip_info));

	/* spi controler init */
	SpiInitialize();

	/* spi nand init */
	SpinandSoftreset();
	SpiNandWaitTillReady();
	SpiNandReadId();
	ret = SpiNandSetAttribute(snf);
	if (ret != SNF_OK) {
		SNF_PrintString("Not find spi nand flash.\n", __func__);
		return ret;
	}

	SpiNandWriteReg(SPI_NAND_LOCK_REG, SPI_NAND_PROT_UNLOCK_ALL);
	SpiNandEnableECC();
	SpiNandDisableQuad();
	//SpiNandWriteEnable();
	ret = SpiNandBbtInit();
	if (ret != SNF_OK)
		SNF_PrintString("SpiNandLoderBbt failed.\n", __func__);

	return ret;
}

static void SpiLoadStepldr(void)
{
	UINT32 *buf = (UINT32*)STEPLDR_ENTRY;
	UINT32 offset;
	UINT32 size;
	int ret;
	int count = 3;

retry:
	if (ReadSysInfo()) {
		SNF_PrintString("read sysinfo fail, try to load stepldr part A.\n", __func__);
		offset = STEPLDRA_OFFSET;
		size = STEPLDR_MAX_SIZE;
	} else {
		SysInfo *sysinfo = GetSysInfo();
		offset = sysinfo->stepldr_offset;
		size = sysinfo->stepldr_size;
	}

	PrintVariableValueHex("stepldr offset", offset);

	ret = SpiNandReadData(offset, buf, size);
	if (ret != SNF_OK) {
		if (--count) {
			goto retry;
		} else {
			SNF_PrintString("SpiNandReadData failed.\n", __func__);
		}
	}

#ifdef MMU_ENABLE
	CP15_clean_dcache_for_dma(STEPLDR_ENTRY, STEPLDR_ENTRY + size);
#endif
}

void bootFromSPI(void)
{
	void (*funPtr)(void);

	SpiLoadStepldr();

	funPtr = (void (*)(void))STEPLDR_ENTRY;
	funPtr();
}

/* offset is at least align to PAGE_SIZE */
static int SpiNandBurn(void *buf, u32 offset, u32 size)
{
	snf_chip_info *chip = &g_snf;
	u32 currblk, burn_size;
	u32 bytePerBlk;
	u32 offset_in_block;
	int remain;
	int ret = SNF_OK;
	u8 *pbuf;

	pbuf = (u8 *)buf;
	remain = size;
	bytePerBlk = chip->BytePerBlk;
	currblk = offset / bytePerBlk;
	offset_in_block = offset % bytePerBlk;

	//area check valid.
	if ((offset + size) > chip->AvailableCapacity) {
		SNF_PrintString("burn out of boundry.\n", __func__);
		return -SNF_ERR;
	}

	//offset check align
	if (offset % chip->BytePerPage) {
		SNF_PrintString("offset is not align with page.\n", __func__);
		return -SNF_ERR;
	}

	while (remain > 0) {
		burn_size = ((bytePerBlk - offset_in_block) >= remain) ? remain : (bytePerBlk - offset_in_block);
		ret = SpiNandEraseWriteInBlock(currblk, offset_in_block, pbuf, burn_size);//logic block
		if (ret != SNF_OK) {
			SNF_Print("erase write block failed, block: ", __func__, currblk);
			break;
		}

		if (offset_in_block)
			offset_in_block = 0;

		currblk++;
		pbuf += burn_size;
		remain -= burn_size;
		if (remain < 0) {
			ret = -SNF_ERR;
			SNF_PrintString("Invalid remain.\n", __func__);
			break;
		}
	}

	SpiNandWriteDisable();

	return ret;
}

int FlashBurn(void *buf, u32 offset, u32 size)
{
	return SpiNandBurn(buf, offset, size);
}

int SpiReadSysInfo(SysInfo *info)
{
	u32 checksum;
	u32 calc_checksum;
	u8 data[512];
	int ret;

	ret = SpiNandReadData(SYSINFOA_OFFSET, data, sizeof(SysInfo));
	if (ret != SNF_OK) {
		SNF_Print("SYSINFOA_OFFSET read failed, ret:", __func__, ret);
	}

	checksum = *(u32*)((u32)data + sizeof(SysInfo) - 4);
	calc_checksum = xcrc32((unsigned char*)data, sizeof(SysInfo) - 4, 0xffffffff);
	if (calc_checksum == checksum) {
		memcpy(info, data, sizeof(SysInfo));
		return 0;
	}

	ret = SpiNandReadData(SYSINFOB_OFFSET, data, sizeof(SysInfo));
	if (ret != SNF_OK) {
		SNF_Print("SYSINFOB_OFFSET read failed, ret:", __func__, ret);
	}

	checksum = *(u32*)((u32)data + sizeof(SysInfo) - 4);
	calc_checksum = xcrc32((unsigned char*)data, sizeof(SysInfo) - 4, 0xffffffff);
	if (calc_checksum == checksum) {
		memcpy(info, data, sizeof(SysInfo));
		return 0;
	}

	return -1;
}

void SpiWriteSysInfo(SysInfo *info)
{
	SpiNandBurn(info, SYSINFOB_OFFSET, sizeof(SysInfo));
	SpiNandBurn(info, SYSINFOA_OFFSET, sizeof(SysInfo));
}
#endif

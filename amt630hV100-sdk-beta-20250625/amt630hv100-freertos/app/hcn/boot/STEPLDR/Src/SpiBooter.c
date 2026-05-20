#include <string.h>
#include "typedef.h"
#include "amt630h.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include "spi.h"
#include "cp15.h"
#include "sysinfo.h"
#include "crc32.h"
#include "update.h"
#include "board.h"
#include "os_adapt.h"

#if DEVICE_TYPE_SELECT == SPI_NOR_FLASH || DEVICE_TYPE_SELECT == EMMC_FLASH
#define SPI_CS_GPIO					32

#define SPI_RXFIFO_FULL				(1<<4)
#define SPI_RXFIFO_NOTEMPTY			(1<<3)
#define SPI_TXFIFO_EMPTY			(1<<2)
#define SPI_TXFIFO_NOTFULL			(1<<1)
#define SPIFLASH_BUSY				(1<<0)

/* SFUD support manufacturer JEDEC ID */
#define SFUD_MF_ID_CYPRESS			0x01
#define SFUD_MF_ID_FUJITSU			0x04
#define SFUD_MF_ID_EON				0x1C
#define SFUD_MF_ID_ATMEL			0x1F
#define SFUD_MF_ID_MICRON			0x20
#define SFUD_MF_ID_AMIC				0x37
#define SFUD_MF_ID_SANYO			0x62
#define SFUD_MF_ID_INTEL			0x89
#define SFUD_MF_ID_ESMT				0x8C
#define SFUD_MF_ID_FUDAN			0xA1
#define SFUD_MF_ID_HYUNDAI			0xAD
#define SFUD_MF_ID_SST				0xBF
#define SFUD_MF_ID_MICRONIX			0xC2
#define SFUD_MF_ID_GIGADEVICE		0xC8
#define SFUD_MF_ID_ISSI				0xD5
#define SFUD_MF_ID_WINBOND			0xEF


static void SpiWriteEnable(void);

/* flash chip information */
typedef struct {
	char *name;						/**< flash chip name */
	uint8_t mf_id;					/**< manufacturer ID */
	uint8_t type_id;				/**< memory type ID */
	uint8_t capacity_id;			/**< capacity ID */
	uint32_t capacity;				/**< flash capacity (bytes) */
} flash_chip;

static const flash_chip flash_chip_table[] =
{
	{"W25Q40BV", SFUD_MF_ID_WINBOND, 0x40, 0x13, 512L*1024L},
	{"W25Q16BV", SFUD_MF_ID_WINBOND, 0x40, 0x15, 2L*1024L*1024L},
	{"W25Q32BV", SFUD_MF_ID_WINBOND, 0x40, 0x16, 4L*1024L*1024L},
	{"W25Q64CV", SFUD_MF_ID_WINBOND, 0x40, 0x17, 8L*1024L*1024L},
	{"W25Q64DW", SFUD_MF_ID_WINBOND, 0x60, 0x17, 8L*1024L*1024L},
	{"W25Q128BV", SFUD_MF_ID_WINBOND, 0x40, 0x18, 16L*1024L*1024L},
	{"W25Q256FV", SFUD_MF_ID_WINBOND, 0x40, 0x19, 32L*1024L*1024L},
	{"W25H256JV", SFUD_MF_ID_WINBOND, 0x90, 0x19, 32L*1024L*1024L},
	{"W25Q512JVFM", SFUD_MF_ID_WINBOND, 0x70, 0x20, 64L*1024L*1024L},
	{"SST25VF080B", SFUD_MF_ID_SST, 0x25, 0x8E, 1L*1024L*1024L},
	{"SST25VF016B", SFUD_MF_ID_SST, 0x25, 0x41, 2L*1024L*1024L},
	{"EN25Q32B", SFUD_MF_ID_EON, 0x30, 0x16, 4L*1024L*1024L},
	{"GD25Q64B", SFUD_MF_ID_GIGADEVICE, 0x40, 0x17, 8L*1024L*1024L},
	{"GD25Q16B", SFUD_MF_ID_GIGADEVICE, 0x40, 0x15, 2L*1024L*1024L},
	{"GD25Q32C", SFUD_MF_ID_GIGADEVICE, 0x40, 0x16, 4L*1024L*1024L},
	{"GD25Q256EY1G", SFUD_MF_ID_GIGADEVICE, 0x40, 0x19, 32L*1024L*1024L},
	{"S25FL216K", SFUD_MF_ID_CYPRESS, 0x40, 0x15, 2L*1024L*1024L},
	{"S25FL032P", SFUD_MF_ID_CYPRESS, 0x02, 0x15, 4L*1024L*1024L},
	{"A25L080", SFUD_MF_ID_AMIC, 0x30, 0x14, 1L*1024L*1024L},
	{"F25L004", SFUD_MF_ID_ESMT, 0x20, 0x13, 512L*1024L},
	{"PCT25VF016B", SFUD_MF_ID_SST, 0x25, 0x41, 2L*1024L*1024L},
	{"IS25LP128F", SFUD_MF_ID_ISSI, 0x60, 0x18, 16L*1024L*1024L},
	{"MX25L6433F", SFUD_MF_ID_MICRONIX, 0x20, 0x17, 8L*1024L*1024L},
	{"MX25L12845G", SFUD_MF_ID_MICRONIX, 0x20, 0x18, 16L*1024L*1024L},
	{"MX25L25645G", SFUD_MF_ID_MICRONIX, 0x20, 0x19, 32L*1024L*1024L},
	{"W25Q512JVFM", SFUD_MF_ID_WINBOND, 0x70, 0x20, 64L*1024L*1024L},
	{"GD25B512ME", SFUD_MF_ID_GIGADEVICE, 0x47, 0x1A, 64L*1024L*1024L},
};

static int addr_in_4_byte = 0;

static void SetCSGpioEnable(int enable)
{
	gpio_direction_output(SPI_CS_GPIO, !enable);
}

static void SetSpiDataMode(unsigned int bitMode)
{
	unsigned int val = 0;

	(void)val;
	while((rSPI_SR & SPI_BUSY));
	rSPI_SSIENR = 0;
	val = rSPI_CTLR0;
	val &=~(0x1f<<16);
	val |=((bitMode-1)<<16);
	rSPI_CTLR0 = val;
	rSPI_SSIENR = 1;
}

static void SpiWaitIdle(void)
{
	while(rSPI_SR & SPIFLASH_BUSY);
	udelay(2);
}

static void SpiEmptyRxFIFO(void)
{
	INT32 data = 0;

	(void)data;

	while(rSPI_SR & SPI_RXFIFO_NOTEMPTY)
		data = rSPI_DR;
}

#ifdef SPI0_QSPI_MODE
static void SpiWriteSta2(uint8_t status)
{
	SpiWriteEnable();
	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_WRITE_STATUS2;
	rSPI_DR = status;
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static UINT8 SpiReadSta2(void)
{
	UINT8 status;

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_READ_STATUS2;
	rSPI_DR = 0;
	while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
	status = rSPI_DR;
	while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
	status = rSPI_DR;
	PrintVariableValueHex("status s2 :", status);
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
	return status;
}
#endif

static UINT8 SpiReadSta3(void)
{
	UINT8 status;

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_READ_STATUS3;
	rSPI_DR = 0;
	while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
	status = rSPI_DR;
	while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
	status = rSPI_DR;
	PrintVariableValueHex("status s3 :", status);
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
	return status;
}

static UINT8 SpiReadSta(void)
{
	UINT8 status;

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_READ_STATUS;
	rSPI_DR = 0;
	while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
	status = rSPI_DR;
	while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
	status = rSPI_DR;
	//PrintVariableValueHex("status:", status);
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
	return status;
}
static void SpiWriteSta(uint8_t status)
{
	SpiWriteEnable();
	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_WRITE_STATUS;
	rSPI_DR = status;
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}
static void SpiDisable4ByteMode(void)
{
	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_DISABLE_4BYTE_MODE;
	while(!(rSPI_SR & SPI_TXFIFO_EMPTY));
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static void SpiEnable4ByteMode(void)
{
	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_ENABLE_4BYTE_MODE;
	while(!(rSPI_SR & SPI_TXFIFO_EMPTY));
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static void SpiReadPage(UINT32 pagenum, UINT32 *buf)
{
	UINT32 addr;
	UINT32 val = 0;
	INT32 i, j;
	UINT8 tmpaddr[4];
	UINT8 *data = (UINT8*)buf;

	(void)val;

	addr = pagenum*BYTESPERPAGE;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	tmpaddr[3] = addr>>24;

	SpiEmptyRxFIFO();
	SetCSGpioEnable(1);

	if (addr_in_4_byte) {
		SetSpiDataMode(8);
		rSPI_DR = SPI_4BYTEADDR_READ_DATA;
		rSPI_DR = tmpaddr[3];
		rSPI_DR = tmpaddr[2];
		rSPI_DR = tmpaddr[1];
		rSPI_DR = tmpaddr[0];
		for (i = 0; i < 5; i++) {
			while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
			val = rSPI_DR;
		}
	} else {
		rSPI_DR = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_READ_DATA;
		while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		val = rSPI_DR;
	}

	if (addr_in_4_byte) {
		for (i = 0; i < 4; i++) {
			for (j = 0; j < WORDSPERPAGE; j++) {
				while(!(rSPI_SR & SPI_TXFIFO_NOTFULL));
				rSPI_DR = 0;
			}
			for (j = 0; j < WORDSPERPAGE; j++) {
				while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
				*data++ = rSPI_DR;
			}
		}
	} else {
		for (i = 0; i < WORDSPERPAGE; i++) {
			while(!(rSPI_SR & SPI_TXFIFO_NOTFULL));
			rSPI_DR = 0;
		}
		for(i = 0; i < WORDSPERPAGE; i++) {
			while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
			*buf++ = rSPI_DR;
		}
	}

	SpiWaitIdle();
	SetCSGpioEnable(0);
	if (addr_in_4_byte)
		SetSpiDataMode(32);
}

#ifdef SPI0_QSPI_MODE
//********format*********
//00 – Standard SPI Format
// 01 – Dual SPI Format
// 10 – Quad SPI Format
// 11 – Reserved
//********tmod**********
//00 –- Transmit & Receive
//01 –- Transmit Only
//10 –- Receive Only
//11 –- EEPROM Read
static void SpiSetFrameFormatMode(unsigned char format, unsigned char tmod)
{
	unsigned int val = 0;

	while((rSPI_SR & SPI_BUSY));
	rSPI_SSIENR = 0;
	val = rSPI_CTLR0;
	val &= ~((0x3 << 21) | (0x3 << 8));
	val |= ((format << 21) | (tmod << 8));
	rSPI_CTLR0 = val;
	rSPI_SSIENR = 1;
}

static void SpiReadPageQuadMode(UINT32 pagenum, UINT32 *buf)
{
	UINT32 addr;
	UINT32 val = 0;
	INT32 i;
	UINT8 tmpaddr[4];

	(void)val;

	addr = pagenum*BYTESPERPAGE;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	tmpaddr[3] = addr>>24;

	SpiEmptyRxFIFO();
	SetCSGpioEnable(1);

	SpiSetFrameFormatMode(2, 2);
	rSPI_SSIENR = 0;
	if (addr_in_4_byte) {
		//set instruction 8bits,1byte mode; addr 32bit,4bytes mode; 6 dummy clocks
		rSPI_SPI_CTRLR0 = (6 << 11) | (2 << 8) | (8 << 2) | 1;
		rSPI_CTLR1 = WORDSPERPAGE - 1;
		rSPI_SSIENR = 1;

		rSPI_DR = SPI_QUAD_IO_READ_DATA + 1;
		rSPI_DR = (tmpaddr[0] << 24) | (tmpaddr[1] << 16) | (tmpaddr[2] << 8) | tmpaddr[3];
	} else {
		//set instruction 8bits,1byte mode; addr 24bit,4bytes mode; 6 dummy clocks
		rSPI_SPI_CTRLR0 = (6 << 11) | (2 << 8) | (6 << 2) | 1;
		rSPI_CTLR1 = WORDSPERPAGE - 1;
		rSPI_SSIENR = 1;

		rSPI_DR = SPI_QUAD_IO_READ_DATA;
		rSPI_DR = (tmpaddr[0] << 16) | (tmpaddr[1] << 8) | (tmpaddr[2] << 0);
	}

	for(i=0; i<WORDSPERPAGE; i++)
	{
		while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		*buf++ = rSPI_DR;
	}

	SpiWaitIdle();
	rSPI_SSIENR = 0;
	rSPI_SPI_CTRLR0 = 0;
	rSPI_CTLR1 = 0;
	rSPI_SSIENR = 1;
	SpiSetFrameFormatMode(0, 0);

	SetCSGpioEnable(0);
}
#endif

void SpiSelectPad()
{
	UINT32 val;
	val = rSYS_PAD_CTRL02;
	val &= ~0xfff;
	val |= (0x1<<10)|(0x1 << 8)|(0x1 << 6)|(0x1 << 4)| (0x1 << 2);
	rSYS_PAD_CTRL02 = val;

	val = rSYS_SSP_CLK_CFG;
	val &= ~((0x1<<31)|(0x1<<30));
	val |= (0x1<<31)|(0x1<<30);
	rSYS_SSP_CLK_CFG = val;

	//cs inactive first
	SetCSGpioEnable(0);
}

void Reset(void)
{
	SetSpiDataMode(8);
	rSPI_DR = 0x66;
	while((SpiReadSta() & SPI_BUSY));
//	while(!(SpiReadSta() & SPIFLASH_WRITEENABLE));
	rSPI_DR = 0x99;
	while((SpiReadSta() & SPI_BUSY));
//	while(!(SpiReadSta() & SPIFLASH_WRITEENABLE));

	SetSpiDataMode(32);
}

#define SPI0_CS0_GPIO	32
#define SPI0_IO0_GPIO	34
static void dwspi_jedec252_reset(void)
{
	int i;
	int si = 0;
	UINT32 val;

	val = rSYS_PAD_CTRL02;
 	val &= ~((3 << 4) | 3);
	rSYS_PAD_CTRL02 = val;

	gpio_direction_output(SPI0_CS0_GPIO, 1);
	gpio_direction_output(SPI0_IO0_GPIO, 1);
	udelay(300);

	for (i = 0; i < 4; i++) {
		gpio_direction_output(SPI0_CS0_GPIO, 0);
		gpio_direction_output(SPI0_IO0_GPIO, si);
		si = !si;
		udelay(300);
		gpio_direction_output(SPI0_CS0_GPIO, 1);
		udelay(300);
	}
}

static void SpiReadDeviceId(UINT8 *mfid, UINT8 *devid)
{
	UINT8 val[6];
	int i;
	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_MF_DEVICE_ID;
	rSPI_DR = 0;
	rSPI_DR = 0;
	rSPI_DR = 0;
	rSPI_DR = 0;
	rSPI_DR = 0;
	for (i = 0; i < 6; i++)
	{
		while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		val[i] = rSPI_DR;
	}
	*mfid = val[4];
	*devid = val[5];
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static void SpiReadJedecId(UINT8 *mfid, UINT8 *memid, UINT8 *capid)
{
	UINT8 val[4];
	int i;

	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_READ_JEDEC_ID;
	rSPI_DR = 0;
	rSPI_DR = 0;
	rSPI_DR = 0;

	for (i = 0; i < 4; i++)
	{
		while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		val[i] = rSPI_DR;
	}
	*mfid = val[1];
	*memid = val[2];
	*capid = val[3];
	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

static void SpiuId(void)
{
	UINT8 val[13];
	int i;

	(void)val;

	SetSpiDataMode(8);
	SetCSGpioEnable(1);

	rSPI_DR = 0x4b;
	for (i = 0; i < 12; i++)
		rSPI_DR = 0;

	for (i = 0; i < 13; i++) {
		while(!(rSPI_SR & SPI_RXFIFO_NOTEMPTY));
		val[i] = rSPI_DR;
	}

	SpiWaitIdle();
	SetCSGpioEnable(0);
	SetSpiDataMode(32);
}

void SpiReadId(void)
{
	UINT8 mfid,devid,memid,capid;

	SpiReadDeviceId(&mfid,&devid);
	SpiReadJedecId(&mfid,&memid,&capid);
	PrintVariableValueHex("ManufacturerID: ", mfid);
	PrintVariableValueHex("DeviceID: ", devid);
	PrintVariableValueHex("Memory Type ID: ", memid);
	PrintVariableValueHex("Capacity ID: ", capid);
}

int SpiInit(void)
{
	uint8_t mfid, typeid, capid;
	unsigned int val;
	int i;

	dwspi_jedec252_reset();
	SpiSelectPad();

	val = rSYS_SOFT_RST;
	val &= ~(0x1<<8);
	rSYS_SOFT_RST = val;
	udelay(10);
	val |= (0x1<<8);
	rSYS_SOFT_RST = val;

	rSPI_SSIENR = 0;
	rSPI_CTLR0 = 0;
	rSPI_CTLR0 |=(0<<21)|(0x1f<<16)|(0x0<<12)|(0x0<<8)|(0x0<<4);
	//rSPI_CTLR1 = 63;
	rSPI_BAUDR = 4;//42;//16;//2;
	rSPI_SER = 1;
	rSPI_IMR = 0;
	rSPI_SSIENR = 1;
	SpiuId();
//	Reset();
	SpiReadJedecId(&mfid, &typeid, &capid);
	for (i = 0; i < sizeof(flash_chip_table) / sizeof(flash_chip); i++) {
		if ((flash_chip_table[i].mf_id == mfid)
				&& (flash_chip_table[i].type_id == typeid)
				&& (flash_chip_table[i].capacity_id == capid)) {
			if (flash_chip_table[i].capacity > 0x1000000) {
				addr_in_4_byte = 1;
				SendUartString("flash addr 4bytes mode\n");
				SpiEnable4ByteMode();
			}
			break;
		}
	}

	if (!addr_in_4_byte) {
		SendUartString("flash addr 1bytes mode\n");
		SpiDisable4ByteMode();
	}

#ifdef SPI0_QSPI_MODE
	if((i<sizeof(flash_chip_table)/sizeof(flash_chip))&&(SFUD_MF_ID_ISSI==flash_chip_table[i].mf_id))
	{
		uint8_t status =SpiReadSta();
		status |= SPI_SSI_QE;
		SpiWriteSta(status);
	}
	else if((i<sizeof(flash_chip_table)/sizeof(flash_chip))&&(SFUD_MF_ID_MICRONIX==flash_chip_table[i].mf_id))
	{
		uint8_t status =SpiReadSta();
		status |= SPI_MIC_QE;
		SpiWriteSta(status);
	}
	else
	{
		uint8_t status =SpiReadSta2();
		status |= SPI_QE;
		SpiWriteSta2(status);
	}
#endif
	SpiReadSta3();

	udelay(10000);
	return 0;
}

#define SPI_READ_MAXLEN		BYTESPERPAGE

static void SpiLoadImage(void (*readfunc)(UINT32, UINT32 *))
{
	unsigned int i = 0;
	UINT32 *buf = (UINT32*)IMAGE_ENTRY;
	UpFileHeader *header;
	UpFileInfo *appfile;
	UINT32 nPageCount;
	UINT32 nPageStart;
	UINT32 appsize;
	SysInfo *sysinfo = GetSysInfo();

	PrintVariableValueHex("image_offset", sysinfo->image_offset);
	readfunc(sysinfo->image_offset / BYTESPERPAGE, buf);
	header = (UpFileHeader*)buf;
	appfile = &header->files[0];
    PrintVariableValueHex("header_size", header->size);
    PrintVariableValueHex("header_reserve1", header->reserved1);
    PrintVariableValueHex("header_reserve2", header->reserved2);
    PrintVariableValueHex("appfile_offset", appfile->offset);
    PrintVariableValueHex("appfile_size", appfile->size);
	if (appfile->offset & (BYTESPERPAGE - 1)) {
		SendUartString("\nImage positon is not align to flash pagesize, can't load.\r\n");
		while(1);
	}

	if (sysinfo->app_size)
		appsize = sysinfo->app_size;
	else
		appsize = appfile->size;
        PrintVariableValueHex("appsize", appsize);
	nPageCount = (appsize + BYTESPERPAGE - 1) / BYTESPERPAGE;
	nPageStart = (appfile->offset + sysinfo->image_offset) / BYTESPERPAGE;
	for(i = nPageStart; i < nPageStart + nPageCount; i++)
	{
		readfunc(i, buf);
		buf += BYTESPERPAGE/4;
	}

#ifdef MMU_ENABLE
	CP15_clean_dcache_for_dma(IMAGE_ENTRY, IMAGE_ENTRY + appsize);
#endif
}

static void SpiWriteEnable(void)
{
	SetSpiDataMode(8);
	SetCSGpioEnable(1);
	rSPI_DR = SPI_WRITE_ENABLE;
	SpiWaitIdle();
	SetCSGpioEnable(0);
	while((SpiReadSta() & SPI_BUSY));
	while(!(SpiReadSta() & SPIFLASH_WRITEENABLE));
	SetSpiDataMode(32);
}

static void SpiEraseSector(UINT32 sectorNum)
{
	UINT32 addr;
	UINT8 tmpaddr[4];

	addr = BYTESPERSECTOR*sectorNum;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	tmpaddr[3] = addr>>24;

	SpiWriteEnable();
	SetCSGpioEnable(1);
	if (addr_in_4_byte) {
		SetSpiDataMode(8);
		rSPI_DR = SPI_4BYTEADD_SECTOR_ERASE;
		rSPI_DR = tmpaddr[3];
		rSPI_DR = tmpaddr[2];
		rSPI_DR = tmpaddr[1];
		rSPI_DR = tmpaddr[0];
	} else {
		rSPI_DR = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_SECTOR_ERASE;
	}
	SpiWaitIdle();
	SetCSGpioEnable(0);
	while((SpiReadSta() & SPIFLASH_WRITEENABLE));
}

static void SpiEraseBlock(UINT32 blockNum)
{
	UINT32 addr;
	UINT8 tmpaddr[4];

	addr = BYTESPERBLOCK*blockNum;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	tmpaddr[3] = addr>>24;

	SpiWriteEnable();
	SetCSGpioEnable(1);
	if (addr_in_4_byte) {
		SetSpiDataMode(8);
		rSPI_DR = SPI_4BYTEADD_BLOCK_ERASE;
		rSPI_DR = tmpaddr[3];
		rSPI_DR = tmpaddr[2];
		rSPI_DR = tmpaddr[1];
		rSPI_DR = tmpaddr[0];
	} else {
		rSPI_DR = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_BLOCK_ERASE;
	}
	SpiWaitIdle();
	SetCSGpioEnable(0);
	while((SpiReadSta() & SPIFLASH_WRITEENABLE));
}

static void SpiWritePage(UINT32 pagenum, UINT32 *buf)
{
	UINT32 addr;
	UINT32 val = 0;;
	INT32 i;
	UINT8 tmpaddr[4];
	UINT8 *data = (UINT8*)buf;

	(void)val;

	addr = pagenum*BYTESPERPAGE;
	tmpaddr[0] = addr;
	tmpaddr[1] = addr>>8;
	tmpaddr[2] = addr>>16;
	tmpaddr[3] = addr>>24;

	SpiWriteEnable();
	SetCSGpioEnable(1);
	if (addr_in_4_byte) {
		SetSpiDataMode(8);
		rSPI_DR = SPI_4BYTEADD_PAGE_PROGRAM;
		rSPI_DR = tmpaddr[3];
		rSPI_DR = tmpaddr[2];
		rSPI_DR = tmpaddr[1];
		rSPI_DR = tmpaddr[0];
	} else {
		rSPI_DR = (tmpaddr[0]<<24) | (tmpaddr[1]<<16) | (tmpaddr[2]<<8) | SPI_PAGE_PROGRAM;
	}

	if (addr_in_4_byte) {
		for (i = 0; i < BYTESPERPAGE; i++) {
			while(!(rSPI_SR & SPI_TXFIFO_NOTFULL));
			rSPI_DR = *data++;
		}
	} else {
		for (i = 0; i < WORDSPERPAGE; i++) {
			while(!(rSPI_SR & SPI_TXFIFO_NOTFULL));
			rSPI_DR = *buf++;
		}
	}
	SpiWaitIdle();
	SetCSGpioEnable(0);
	while(SpiReadSta() & SPI_BUSY);
}

void bootFromSPI(void)
{
	void (*funPtr)(void);

#ifdef SPI0_QSPI_MODE
	SpiLoadImage(SpiReadPageQuadMode);
#else
	SpiLoadImage(SpiReadPage);
#endif

	funPtr = (void (*)(void))IMAGE_ENTRY;
	funPtr();
}

static unsigned int pagecheck[WORDSPERPAGE];
/* offset is at least align to SECOTR_SIZE */
static int SpiNorBurnPage(int pagenum, unsigned int *buf)
{
	int timeout = 3;
	unsigned int *tmp = (unsigned int *)buf;
	int i;

retry:
	SpiWritePage(pagenum, buf);
	SpiReadPage(pagenum, pagecheck);
	for (i = 0; i < WORDSPERPAGE; i++) {
		if (tmp[i] != pagecheck[i]) {
			if (timeout-- > 0) {
				PrintVariableValueHex("write: ", tmp[i]);
				PrintVariableValueHex("read: ", pagecheck[i]);
				SendUartString("burn check data fail, retry...\r\n");
				goto retry;
			} else {
				SendUartString("burn error!\r\n");
				return -1;
			}
		}
	}

	return 0;
}

/* offset is at least align to SECOTR_SIZE */
static int SpiNorBurn(void *buf, unsigned int offset, unsigned int size, int show_progress)
{
	int i, j;
	int secstart, secnum;
	int blkstart, blknum;
	int pagestart, pagenum, pageburned;
	int remain = size;
	unsigned int *pbuf = (unsigned int *)buf;

	pagestart = offset / BYTESPERPAGE;
	pagenum = (size + BYTESPERPAGE - 1) / BYTESPERPAGE;
	pageburned = 0;

	while (remain > 0) {
		unsigned int tmp = offset & (BYTESPERBLOCK - 1);
		if (tmp) {
			tmp = (BYTESPERBLOCK - tmp) / BYTESPERSECTOR;
			secnum = (remain + BYTESPERSECTOR - 1) / BYTESPERSECTOR;
			secnum = min(tmp, secnum);
			secstart = offset / BYTESPERSECTOR;
			for (i = 0; i < secnum; i++) {
				SpiEraseSector(secstart + i);
				for (j = 0; j < PAGESPERSECTORS; j++) {
					if (SpiNorBurnPage(pagestart + j, pbuf))
						return -1;
					pbuf += WORDSPERPAGE;
					pageburned++;
					remain -= BYTESPERPAGE;
					if (remain <= 0) goto finish;
					if (show_progress)
						update_progress_set(pageburned * 100 / pagenum);
				}
				pagestart += PAGESPERSECTORS;
			}
			offset += secnum * BYTESPERSECTOR;
		} else {
			blkstart = offset / BYTESPERBLOCK;
			blknum = remain / BYTESPERBLOCK;
			for (i = 0; i < blknum; i++) {
				SpiEraseBlock(blkstart + i);
				for (j = 0; j < PAGESPERSECTORS * SECTORSPERBLOCK; j++) {
					if (SpiNorBurnPage(pagestart + j, pbuf))
						return -1;
					pbuf += WORDSPERPAGE;
					pageburned++;
					remain -= BYTESPERPAGE;
					if (remain <= 0) goto finish;
					if (show_progress)
						update_progress_set(pageburned * 100 / pagenum);
				}
				pagestart += PAGESPERSECTORS * SECTORSPERBLOCK;
			}
			offset += blknum * BYTESPERBLOCK;
			if (remain > 0) {
				secstart = offset / BYTESPERSECTOR;
				secnum = (remain + BYTESPERSECTOR - 1) / BYTESPERSECTOR;
				for (i = 0; i < secnum; i++) {
					SpiEraseSector(secstart + i);
					for (j = 0; j < PAGESPERSECTORS; j++) {
						if (SpiNorBurnPage(pagestart + j, pbuf))
							return -1;
						pbuf += WORDSPERPAGE;
						pageburned++;
						remain -= BYTESPERPAGE;
						if (remain <= 0) goto finish;
						if (show_progress)
							update_progress_set(pageburned * 100 / pagenum);
					}
					pagestart += PAGESPERSECTORS;
				}
				offset += secnum * BYTESPERSECTOR;
			}
		}
	}

finish:
	if (show_progress)
		update_progress_set(100);

	return 0;
}

int FlashBurn(void *buf, unsigned int offset, unsigned int size, int show_progress)
{
	return SpiNorBurn(buf, offset, size, show_progress);
}

int SpiReadSysInfo(SysInfo *info)
{
	UINT32 *buf = (UINT32*)IMAGE_ENTRY;
	int pagestart;
	int pagenum;
	UINT32 checksum;
	UINT32 calc_checksum;
	int i;

	pagestart = SYSINFOA_OFFSET / BYTESPERPAGE;
	pagenum = (sizeof(SysInfo) + BYTESPERPAGE - 1) / BYTESPERPAGE;
	for(i = pagestart; i < pagestart + pagenum; i++)
	{
		SpiReadPage(i, buf);
		buf += BYTESPERPAGE/4;
	}

	checksum = *(UINT32*)(IMAGE_ENTRY + sizeof(SysInfo) - 4);
	calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, sizeof(SysInfo) - 4, 0xffffffff);
	if (calc_checksum == checksum) {
		memcpy(info, (void*)IMAGE_ENTRY, sizeof(SysInfo));
		return 0;
	}

	buf = (UINT32*)IMAGE_ENTRY;
	pagestart = SYSINFOB_OFFSET / BYTESPERPAGE;
	for(i = pagestart; i < pagestart + pagenum; i++)
	{
		SpiReadPage(i, buf);
		buf += BYTESPERPAGE/4;
	}

	checksum = *(UINT32*)(IMAGE_ENTRY + sizeof(SysInfo) - 4);
	calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, sizeof(SysInfo) - 4, 0xffffffff);
	if (calc_checksum == checksum) {
		memcpy(info, (void*)IMAGE_ENTRY, sizeof(SysInfo));
		return 0;
	}

	return -1;
}

void SpiWriteSysInfo(SysInfo *info)
{
	SpiNorBurn(info, SYSINFOB_OFFSET, sizeof(SysInfo), 0);
	SpiNorBurn(info, SYSINFOA_OFFSET, sizeof(SysInfo), 0);
}

unsigned int SpiGetUpfileOffset(int type)
{
	SysInfo *sysinfo = GetSysInfo();

	if (type == UPFILE_TYPE_WHOLE) {
		if (sysinfo->image_offset == IMAGE_OFFSET)
			sysinfo->image_offset = IMAGEB_OFFSET;
		else
			sysinfo->image_offset = IMAGE_OFFSET;
		return sysinfo->image_offset;
	} else if (type == UPFILE_TYPE_FIRSTLDR) {
		return LOADER_OFFSET;
	} else if (type == UPFILE_TYPE_STEPLDR) {
		if (sysinfo->stepldr_offset == STEPLDRA_OFFSET)
			sysinfo->stepldr_offset = STEPLDRB_OFFSET;
		else
			sysinfo->stepldr_offset = STEPLDRA_OFFSET;
		return sysinfo->stepldr_offset;
	} else {
		unsigned int buf[WORDSPERPAGE];
		UpFileHeader *header;
		UpFileInfo *appfile;
		unsigned int offset;
		int i;

		SpiReadPage(sysinfo->image_offset / BYTESPERPAGE, buf);
		header = (UpFileHeader*)buf;
		if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
			SendUartString("old update file isn't found in flash, can't support module update.\n");
			return 0xffffffff;
		}
		for (i = 0; i < header->filenum; i++) {
			appfile = &header->files[i];
			if ((appfile->magic == UPFILE_APP_MAGIC && type == UPFILE_TYPE_APP) ||
				(appfile->magic == MKTAG('R', 'O', 'M', 'A') && type == UPFILE_TYPE_RESOURCE) ||
				(appfile->magic == MKTAG('B', 'A', 'N', 'I') && type == UPFILE_TYPE_ANIMATION)) {
				offset = appfile->offset;
				if (offset & (BYTESPERSECTOR - 1)) {
					SendUartString("offset isn't align to sector erase size, can't support module update.\n");
					return 0xffffffff;
				}
				return offset + sysinfo->image_offset;
			}
		}
	}

	return 0xffffffff;
}
#endif

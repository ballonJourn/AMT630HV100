#ifndef __SPI_H__
#define __SPI_H__

#include "update.h"

#define MAX_WAIT_LOOP_COUNT			100000

#define SPI_WRITE_ENABLE			0x06
#define SPI_WRITE_DISABLE			0x04
#define SPI_READ_STATUS				0x05
#define SPI_READ_STATUS2            0x35
#define SPI_READ_STATUS3            0x15
#define SPI_WRITE_STATUS			0x01
#define SPI_WRITE_STATUS2			0x31
#define SPI_READ_DATA				0x03

#define SPI_4BYTEADDR_READ_DATA		0x13

#define SPI_FAST_READ				0x0B
#define SPI_PAGE_PROGRAM			0x02
#define SPI_4BYTEADD_PAGE_PROGRAM	0x12
#define SPI_SECTOR_ERASE			0x20
#define SPI_4BYTEADD_SECTOR_ERASE	0x21
#define SPI_SECTOR_ERASE_1			0xD7
#define SPI_BLOCK_ERASE				0xD8
#define SPI_4BYTEADD_BLOCK_ERASE	0xDC
#define SPI_BLOCK_ERASE_1			0x52
#define SPI_CHIP_ERASE				0xC7
#define SPI_CHIP_ERASE_1			0x60
#define SPI_POWER_DOWN				0xB9
#define SPI_READ_JEDEC_ID			0x9F
#define SPI_READ_ID_1				0xAB
#define SPI_MF_DEVICE_ID			0x90
#define SPI_MF_DEVICE_ID_1			0x15
#define SPI_READ_ELECTRON_SIGN		0xAB

#define SPI_MF_WINBOND				0xEF
#define SPI_MF_EON					0x1C
#define SPI_MF_AMIC					0x37
#define SPI_MF_ATMEL				0x1F
#define SPI_MF_SST					0xBF
#define SPI_MF_MXIC					0xC2

#define SPI_ENABLE_4BYTE_MODE		0xB7
#define SPI_DISABLE_4BYTE_MODE		0xE9
#define SPI_QUAD_IO_READ_DATA		0xEB



#define SPI_BUSY				(1<<0)
#define SPIFLASH_WRITEENABLE		(1<<1)

#define SPI_QE					(1 << 1)
#define SPI_SSI_QE					(1 << 6)
#define SPI_MIC_QE					(1 << 6)

#define SPI_WRITE_DMA  
#define SPI_READ_DMA    

#define WORDSPERPAGE			64
#define BYTESPERPAGE			256
#define PAGESPERSECTORS			16
#define SECTORSPERBLOCK			16
#define BLOCKSPERFLASH			64


#define BYTESPERBLOCK			(BYTESPERPAGE*PAGESPERSECTORS*SECTORSPERBLOCK)
#define BYTESPERSECTOR			(BYTESPERPAGE*PAGESPERSECTORS)


#define PAGES_PER_BLOCK		64
#define BYTES_PER_PAGE		2048

void SpiSelectPad(void);
int SpiInit(void);
void SpiBurnLoad(void);
void SpiReadId(void);
int FlashBurn(void *buf, unsigned int offset, unsigned int size, int show_progress);
unsigned int SpiGetUpfileOffset(int type);

#endif


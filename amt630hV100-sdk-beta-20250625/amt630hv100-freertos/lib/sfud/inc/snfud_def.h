/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the configure head file for this library.
 * Created on: 2016-04-23
 */

#ifndef _SNFUD_DEF_H_
#define _SNFUD_DEF_H_

#include "board.h"


#ifdef SPI0_QSPI_MODE
#define SNFUD_USING_QSPI
#endif

#define SNFUD_CMD_WRITE_DISABLE							0x04
#define SNFUD_CMD_WRITE_ENABLE							0x06
#define SNFUD_CMD_GET_FEATURE							0x0F
#define SNFUD_CMD_SET_FEATURE							0x1F
#define SNFUD_CMD_LOCK_REG								0xa0
#define SNFUD_CMD_FEATURE_REG							0xb0
#define SNFUD_CMD_STATUS_REG							0xc0
#define	SNFUD_CMD_READ_ID								0x9F
#define	SNFUD_CMD_PAGE_READ								0x13
#define	SNFUD_CMD_READ_CACHE							0x03
#define	SNFUD_CMD_FAST_READ_CACHE						0x0b
#define	SNFUD_CMD_READ_CACHE_X2							0x3b
#define	SNFUD_CMD_READ_CACHE_X4							0x6b
#define	SNFUD_CMD_READ_CACHE_DUAL_IO					0xbb
#define	SNFUD_CMD_READ_CACHE_QUAD_IO					0xeb
#define	SNFUD_CMD_PROGRAM_EXEC							0x10
#define	SNFUD_CMD_PROGRAM_LOAD							0x02
#define	SNFUD_CMD_PROGRAM_LOAD4							0x32
#define	SNFUD_CMD_PROGRAM_LOAD_RANDOM					0x84
#define	SNFUD_CMD_PROGRAM_LOAD_RANDOM4					0xc4
#define	SNFUD_CMD_BLOCK_ERASE							0xd8
#define SNFUD_CMD_RESET									0xFF

#define SNFUD_ECC_EN									(1 << 4)
#define SNFUD_QUAD_EN									(1 << 0)

#define SNFUD_PROT_UNLOCK_ALL							0x0
#define SPI_NAND_STATUS_REG_ECC_S1						(1 << 5)
#define SPI_NAND_STATUS_REG_ECC_S0						(1 << 4)
#define SNFUD_STATUS_REG_PROG_FAIL						(1 << 3)
#define SNFUD_STATUS_REG_ERASE_FAIL						(1 << 2)
#define SNFUD_STATUS_REG_WREN							(1 << 1)
#define SNFUD_STATUS_REG_BUSY							(1 << 0)

#define SNFUD_ECC_ERR_MASK								SPI_NAND_STATUS_REG_ECC_S1
#define SNFUD_REPLACE_BLK_PERCENT						2	//default: 2%
#define SNFUD_BBT_BLK_COUNT								4
#define SNFUD_BBT_HEADER_SIZE							32
#define SNFUD_BBT_HEADER_MASK							"SNF BAD BLOCK TABLE"	//< 20 bytes.

/*
 *	To compatible with the spi nor flash interface calls(they have different erase size), we add this macro.
 *		The driver will manage the erase and write operations automatically.
 *
 *		When write a few data within a block, the driver will first backup the block data with a buffer,
 *		then update the new data to the buffer, and then erase the block, finally write back the buffer data.
 */
#define SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
#define SNFUD_CACHE_BLOCK_COUNT				4
#define SNFUD_CACHE_FLUSH_TIMEOUT			30000
#define SNFUD_MAX_BLOCK_COUNT				2048
#else
#error "ERROR! Must define SNFUD_ERASE_WRITE_DYNAMIC_MANAGE."
#endif

#define PLANE_SEL(x)					x
#define PLANE_NULL						0
#define PINFO_SEL(x)					x
#define PINFO_NULL						0


enum {
	SNF_BLK_BAD = 0,		//block is bad.
	SNF_BLK_USED,			//block have been used.
	SNF_BLK_GOOD = 0xFF,	//block is good.
};

typedef struct spi_nand_plane_select
{
	/* plane select is used for block select */
	uint32_t rpb;	//read page(from nand array to cache) bit.
	uint32_t rcb;	//read cache(from page cache to app buffer) bit.
	uint32_t plb;	//program load(write data to cache) bit.
	uint32_t peb;	//program excute(flush cache to nand array) bit.
} sfud_ps;

typedef struct spi_nand_private_info {
	uint32_t	planeSelIndex;
	uint32_t	crbsyBit;		//cache read busy status bit.
	uint32_t	eccErrMask;		//ECC error mask.
} snf_priv_info;

typedef struct _snf_blk_node {
	uint8_t status;
	uint16_t replace;	//replace bad block.
} snf_blk_node;

typedef struct _snf_bbt {
	uint8_t header_mask[SNFUD_BBT_HEADER_SIZE];
	snf_blk_node node[SNFUD_MAX_BLOCK_COUNT];
	uint32_t bad_blocks;
	uint32_t reserved[8];
	uint32_t bbt_crc;
} snf_bbt;

#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
typedef struct _snf_cache_block {
	int32_t blk_id;
	uint32_t timestamp;
	uint8_t need_flush;
	uint8_t *buf;
} snf_cache_blk_t;

typedef struct _snf_cache_info {
	uint8_t *blk_backup_buf;	/* block backup buffer */
	snf_cache_blk_t cache_blks[SNFUD_CACHE_BLOCK_COUNT];
} snf_cache_info_t;
#endif

/* flash chip information */
typedef struct {
	char *name; 								 	/**< flash chip name */
	uint8_t mf_id;								 	/**< manufacturer ID */
	uint8_t dev_id0;							 	 /**< device ID0 */
	uint8_t dev_id1;							 	 /**< device ID1 */
	uint8_t bbm_type;
	uint32_t byte_per_page;
	uint32_t page_per_blk;
	uint32_t total_blk;
	uint32_t spare_size;
	uint32_t privInfoIndex;

	//
	uint32_t available_blks;
	uint32_t capacity;
	uint32_t available_capacity;
	uint32_t byte_per_blk;
	uint32_t bbt_blk_offset;
	uint32_t replace_blks;
	snf_bbt *bbt;
	sfud_flash *flash;
	snf_priv_info *privInfo;

#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
	snf_cache_info_t *cache_info;
#endif
} snfud_flash_chip;
#endif /* _SFUD_CFG_H_ */

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
 * Function: serial flash operate functions by SFUD lib.
 * Created on: 2016-04-23
 */

#include "sfud.h"
#include "snfud_def.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "source/crc32.h"


#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
#define SNF_GET_CHIP(flash)			((flash)->user_data)
#define SNF_SET_CHIP(chip, flash)	(flash)->user_data = chip
#define SNFUD_ASSERT(EXPR) if (!(EXPR)) {SFUD_INFO("(%s, %d) has assert failed at %s.", #EXPR, __func__, __LINE__); while (1);}

/* user configured flash device information table */
static sfud_flash flash_table[] = {
	[0] = {.spi.name = "spi0"},
};

/* Some spi nand flash need to select the plane.
	it is defined in the user manual, so you shoud read the manual to confirm that whether
	the spi nand flash need to select the plane.
	for example DS35Q2GA:
		odd blocks for one 1G plane, and even blocks for another 1G plane,
*/
static const sfud_ps SNF_PsArray[] = {
	[0] = {0, 0, 0, 0},		//Reserved (NULL).
	[1] = {6, 12, 12, 6},	//only for DS35Q2GA.
};

/* spi nand flash private infomation.
	such as: plane select, cache read busy status bit, ecc error mask bit and so on...
	check the user manual to conform the private infomation.
*/
static const snf_priv_info SNF_PrivInfo[] = {
	/* Reserved */
	[0] = {
		PLANE_NULL,	// No plane select.
		0,			// No crbsyBit(use default bit0 for read/write/erase/reset busy status).
		0			/* No eccErrMask (ECC error occur and cannot be corrected, confirm it in the user manual).
						0		: Not enable EccErrMask in private info, then will use the default SNF_ECC_ERR_MASK.
						others	: SPI_NAND_STATUS_REG_ECC_S0
								  SPI_NAND_STATUS_REG_ECC_S0 | SPI_NAND_STATUS_REG_ECC_S1
					*/
	},

	/* Active below */
	[1] = {PLANE_SEL(1), 0, 0},		/* DS35Q2GA: [plane select(6,12,12,6)][No crbsyBit][No eccErrMask] */
	[2] = {PLANE_NULL, 6, 0},		/* MX35LF1G: [No plane select][crbsyBit in bit6][No eccErrMask] */
	[3] = {PLANE_NULL, 7, 0}, 		/* MX35LF2G: [No plane select][crbsyBit in bit7][No eccErrMask] */
};

/* parameter description:
	name:	spi nand flash name.
	MID:	manufacturer identification
	DID0:	device identification (1 byte mode: device id;	2 byte mode: device id high byte)
	DID1:	device identification1(1 byte mode: must set 0;	2 byte mode: device id low byte)
	BBM:	bad block judgement management flag (set 1 by default)
			BBM = 1: the 1st Byte in the spare area of the 1st page does not contain FFh is a Bad Block
			BBM = 2: the 1st Byte in the spare area of the 1st or 2nd page(if the 1st page is Bad) does not contain FFh is a Bad Block
			read the datasheet if you not sure.
	PrivateInfoIndex:
			spi nand flash private infomation index.
			such as: plane select, cache read busy status bit, ecc error mask bit and so on...
			check the user manual to conform the private infomation.
			set PINFO_NULL if no private information, otherwise add your private information to the array SNF_PrivInfo[]
			and select the private information index with PINFO_SEL() in the array snf_chip_table[].
*/
static const snfud_flash_chip snf_chip_table[] = {
	// name,			MID, DID0, DID1,	BBM,	bytePerPage,	pagePerBlk, blkCnt, spare	PrivateInfoIndex
	{ "DS35Q1GA",		0xE5, 0x71, 0x00,	2,		2048,			64,			1024,	64,		PINFO_NULL},
	{ "DS35Q2GA",		0xE5, 0x72, 0x00,	2,		2048,			64,			2048,	64,		PINFO_SEL(1)},
	{ "GD5F1GM7UE",		0xc8, 0x91, 0x00, 	1, 		2048,			64,			1024,	64,		PINFO_NULL},
	{ "GD5F1GM7RE",		0xc8, 0x81, 0x00, 	1, 		2048,			64,			1024,	64,		PINFO_NULL},
	{ "GD5F2GM7UE",		0xc8, 0x92, 0x00, 	1, 		2048,			64,			2048,	64,		PINFO_NULL},
	{ "GD5F2GM7RE",		0xc8, 0x82, 0x00, 	1, 		2048,			64,			2048,	64,		PINFO_NULL},
	{ "W25N01GV",		0xef, 0xaa, 0x21, 	1, 		2048,			64,			1024,	64,		PINFO_NULL},
	{ "W25N02KV",		0xef, 0xaa, 0x22, 	1, 		2048,			64,			2048,	64,		PINFO_NULL},
	{ "MX35LF1G",		0xc2, 0x12, 0x00,	1,		2048,			64,			1024,	64,		PINFO_SEL(2)},
	{ "MX35LF2G",		0xc2, 0x26, 0x03,	1,		2048,			64,			2048,	64,		PINFO_SEL(3)},
	{ "MX35LF4G",		0xc2, 0x37, 0x03,	1,		4096,			64,			2048,	128,	PINFO_SEL(3)},
	{ "IS37SML01G",    	0xc8, 0x21, 0x00, 	1, 		2048,			64,			1024,	64,		PINFO_NULL},
};

/* ../port/sfup_port.c */
extern void sfud_log_debug(const char *file, const long line, const char *format, ...);
extern void sfud_log_info(const char *format, ...);
static sfud_err snf_update_bbt(snfud_flash_chip *chip);
static sfud_err snf_find_a_replace_block(snfud_flash_chip *chip, uint32_t *block);
static int snf_check_replace_block_valid(snfud_flash_chip *chip, uint32_t block);
static sfud_err snf_read_within_block(snfud_flash_chip *chip, uint32_t blk, uint32_t offset, uint8_t *data, uint32_t size);
static sfud_err snf_erase_write_one_block(snfud_flash_chip *chip, uint32_t blk, uint8_t *data);
static int snf_erase_block(snfud_flash_chip *chip, uint32_t blk);

#ifdef SNFUD_USING_QSPI
static void qspi_set_read_cmd_format(sfud_flash *flash, uint8_t ins, uint8_t ins_lines, uint8_t addr_lines,
		uint8_t dummy_cycles, uint8_t data_lines)
{
	flash->read_cmd_format.instruction = ins;
	flash->read_cmd_format.address_size = 24;
	flash->read_cmd_format.instruction_lines = ins_lines;
	flash->read_cmd_format.address_lines = addr_lines;
	flash->read_cmd_format.alternate_bytes_lines = 0;
	flash->read_cmd_format.dummy_cycles = dummy_cycles;
	flash->read_cmd_format.data_lines = data_lines;
}

/**
 * Enbale the fast read mode in QSPI flash mode. Default read mode is normal SPI mode.
 *
 * it will find the appropriate fast-read instruction to replace the read instruction(0x03)
 * fast-read instruction @see SFUD_FLASH_EXT_INFO_TABLE
 *
 * @note When Flash is in QSPI mode, the method must be called after sfud_device_init().
 *
 * @param flash flash device
 * @param data_line_width the data lines max width which QSPI bus supported, such as 1, 2, 4
 *
 * @return result
 */
static sfud_err snf_qspi_fast_read_enable(sfud_flash *flash, uint8_t data_line_width)
{
	sfud_err result = SFUD_SUCCESS;

	SNFUD_ASSERT(flash);

	if (!(data_line_width == 1 || data_line_width == 2 || data_line_width == 4)) {
		SFUD_INFO("%s, Error: data line width not support.", __func__);
		return -1;
	}

	/* determine qspi supports which read mode and set read_cmd_format struct */
	switch (data_line_width) {
		case 1:
			qspi_set_read_cmd_format(flash, SNFUD_CMD_FAST_READ_CACHE, 1, 1, 0, 1);
			break;
		case 2:
			qspi_set_read_cmd_format(flash, SNFUD_CMD_READ_CACHE_X2, 1, 1, 0, 2);
			break;
		case 4:
			qspi_set_read_cmd_format(flash, SNFUD_CMD_READ_CACHE_X4, 1, 1, 0, 4);
			break;
	}

	return result;
}

sfud_err sfud_qspi_fast_read_enable(sfud_flash *flash, uint8_t data_line_width)
{
	//return snf_qspi_fast_read_enable(flash, data_line_width);
	return SFUD_SUCCESS;
}

sfud_err enter_qspi_mode(const sfud_spi *spi)
{
	sfud_err result = SFUD_SUCCESS;
	uint8_t status;

	result = sfud_read_status(spi->flash, &status, SNFUD_CMD_FEATURE_REG);
	if (result != SFUD_SUCCESS) {
		printf("Error: Read_status register2 failed.");
		return result;
	}

	status |= (1 << 0);
	result = sfud_write_status(spi->flash, false, status, SNFUD_CMD_FEATURE_REG);
	if (result != SFUD_SUCCESS) {
		printf("Error: Write_status register2 failed.");
		return result;
	}

	return result;
}

sfud_err exit_qspi_mode(const sfud_spi *spi)
{
	sfud_err result = SFUD_SUCCESS;
	uint8_t status;

	result = sfud_read_status(spi->flash, &status, SNFUD_CMD_FEATURE_REG);
	if (result != SFUD_SUCCESS) {
		printf("Error: Read_status register2 failed.");
		return result;
	}

	status &= ~(1 << 0);
	result = sfud_write_status(spi->flash, false, status, SNFUD_CMD_FEATURE_REG);
	if (result != SFUD_SUCCESS) {
		printf("Error: Write_status register2 failed.");
		return result;
	}

	return result;
}
#endif /* SNFUD_USING_QSPI */

static int snf_reg_read(const sfud_flash *flash, uint8_t opcode, uint8_t *value)
{
	int result = SFUD_SUCCESS;
	uint8_t cmd[3];
	uint8_t recv_data[3] = {0};

	SNFUD_ASSERT(flash);

	cmd[0] = SNFUD_CMD_GET_FEATURE;
	cmd[1] = opcode;
	cmd[2] = 0;

	result = flash->spi.wr(&flash->spi, cmd, sizeof(cmd), recv_data, sizeof(recv_data));
	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("snf_reg_read success.");
	} else {
		SFUD_INFO("Error: snf_reg_read failed.");
		return result;
	}
	*value = recv_data[2];

	return result;
}

static int snf_reg_write(const sfud_flash *flash, uint8_t opcode, uint8_t value)
{
	int result;
	uint8_t cmd[3];

	SNFUD_ASSERT(flash);

	cmd[0] = SNFUD_CMD_SET_FEATURE;
	cmd[1] = opcode;
	cmd[2] = value;
	result = flash->spi.wr(&flash->spi, cmd, sizeof(cmd), NULL, 0);
	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("snf_reg_write success.");
	} else {
		SFUD_INFO("Error: snf_reg_write failed.");
	}
	return result;
}

sfud_err sfud_read_status(const sfud_flash *flash, uint8_t *value, uint8_t opcode)
{
	return snf_reg_read(flash, opcode, value);
}

sfud_err sfud_write_status(const sfud_flash *flash, bool is_volatile, uint8_t value, uint8_t opcode)
{
	return snf_reg_write(flash, opcode, value);
}

static sfud_err snf_wait_busy(const sfud_flash *flash, uint8_t *status)
{
	size_t retry_times = flash->retry.times;
	sfud_err result = SFUD_SUCCESS;

	SNFUD_ASSERT(flash);

	while (1) {
		result = snf_reg_read(flash, SNFUD_CMD_STATUS_REG, status);
		if (result == SFUD_SUCCESS) {
			if (!(*status & SNFUD_STATUS_REG_BUSY)) {
				return SFUD_SUCCESS;
			}
		}
		/* retry counts */
		SFUD_RETRY_PROCESS(flash->retry.delay, retry_times, result);
	}

	return SFUD_ERR_TIMEOUT;
}

static sfud_err snf_wait_read_busy(const sfud_flash *flash, uint8_t *status)
{
	snfud_flash_chip *chip;
	size_t retry_times = flash->retry.times;
	sfud_err result = SFUD_SUCCESS;
	int crbsyBit;

	SNFUD_ASSERT(flash);
	chip = SNF_GET_CHIP(flash);
	crbsyBit = chip->privInfo->crbsyBit;

	while (1) {
		result = snf_reg_read(flash, SNFUD_CMD_STATUS_REG, status);
		if (result == SFUD_SUCCESS) {
			if (!(*status & (1 << crbsyBit))) {
				return SFUD_SUCCESS;
			}
		}
		/* retry counts */
		SFUD_RETRY_PROCESS(flash->retry.delay, retry_times, result);
	}

	return SFUD_ERR_TIMEOUT;
}

static sfud_err snf_all_block_unlock(const sfud_flash *flash)
{
	int result = SFUD_SUCCESS;

	SNFUD_ASSERT(flash);

	result = snf_reg_write(flash, SNFUD_CMD_LOCK_REG, SNFUD_PROT_UNLOCK_ALL);
	if (result != SFUD_SUCCESS)
		return SFUD_ERR_WRITE;

	return SFUD_SUCCESS;
}

static sfud_err snf_internal_4bit_ecc_enable(const sfud_flash *flash)
{
	int result = SFUD_SUCCESS;
	uint8_t value = 0;

	SNFUD_ASSERT(flash);

	result = snf_reg_read(flash, SNFUD_CMD_FEATURE_REG, &value);
	if (result != SFUD_SUCCESS) {
		return SFUD_ERR_READ;
	}

	value |= SNFUD_ECC_EN;
	result = snf_reg_write(flash, SNFUD_CMD_FEATURE_REG, value);
	if (result != SFUD_SUCCESS) {
		return SFUD_ERR_WRITE;
	}

	return SFUD_SUCCESS;
}

static sfud_err snf_read_id(sfud_flash *flash)
{
	sfud_err result = SFUD_SUCCESS;
	sfud_spi *spi;
	uint8_t cmd_data[1];
	uint8_t recv_data[4] = {0};

	SNFUD_ASSERT(flash);
	spi = (sfud_spi *)&flash->spi;

	cmd_data[0] = SNFUD_CMD_READ_ID;
	result = spi->wr(spi, cmd_data, sizeof(cmd_data), recv_data, sizeof(recv_data));
	if (result == SFUD_SUCCESS) {
		flash->chip.mf_id = recv_data[1];
		flash->chip.type_id = recv_data[2];
		flash->chip.capacity_id = recv_data[3];
		SFUD_INFO("mid:0x%x, devid0:0x%x, devid1:0x%x", recv_data[1], recv_data[2], recv_data[3]);
	} else {
		SFUD_INFO("Error: Read flash device ID error.");
	}

	return result;
}

/**
 * set the flash write enable or write disable
 *
 * @param flash flash device
 * @param enabled true: enable  false: disable
 *
 * @return result
 */
static sfud_err snf_write_enabled(const sfud_flash *flash, bool enabled)
{
	sfud_err result = SFUD_SUCCESS;
	uint8_t cmd;

	SNFUD_ASSERT(flash);

	if (enabled) {
		cmd = SNFUD_CMD_WRITE_ENABLE;
	} else {
		cmd = SNFUD_CMD_WRITE_DISABLE;
	}

	result = flash->spi.wr(&flash->spi, &cmd, 1, NULL, 0);

	return result;
}

static int snf_send_read_page_cmd(snfud_flash_chip *chip, uint32_t page)
{
	int result;
	uint8_t cmd[4];

	SNFUD_ASSERT(chip);
	SNFUD_ASSERT(chip->flash);

	if (chip->privInfo && chip->privInfo->planeSelIndex) {
		int block = page / chip->page_per_blk;
		if (block % 2)
			page |= (1 << SNF_PsArray[chip->privInfo->planeSelIndex].rpb);
	}

	cmd[0] = SNFUD_CMD_PAGE_READ;
	cmd[1] = (page >> 16) & 0xFF;
	cmd[2] = (page >> 8) & 0xFF;
	cmd[3] = (page >> 0) & 0xFF;
	result = chip->flash->spi.wr(&chip->flash->spi, cmd, sizeof(cmd), NULL, 0);
	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("%s success.", __func__);
	} else {
		SFUD_INFO("%s failed\n", __func__);
		return result;
	}

	return result;
}

static int snf_read_page_cache(snfud_flash_chip *chip, uint32_t page, uint32_t offset, uint8_t *data, uint32_t size)
{
	int result;

	SNFUD_ASSERT(chip);
	SNFUD_ASSERT(chip->flash);

	uint32_t addr =(offset & 0xFFFF);
	if (chip->privInfo && chip->privInfo->planeSelIndex) {
		int block = page / chip->page_per_blk;
		if (block % 2)
			addr |= (1 << SNF_PsArray[chip->privInfo->planeSelIndex].rcb);
	}
#ifdef SNFUD_USING_QSPI
	offset = ((addr & 0xFFFF) << 8) | 0x00;
	result = chip->flash->spi.qspi_read(&chip->flash->spi, offset, &chip->flash->read_cmd_format, data, size);
#else
	uint8_t cmd[4];
	cmd[0] = SNFUD_CMD_READ_CACHE;
	cmd[1] = (addr >> 8) & 0xFF;
	cmd[2] = (addr >> 0) & 0xFF;
	cmd[3] = 0x0;
	result = chip->flash->spi.wr(&chip->flash->spi, cmd, sizeof(cmd), data, size);
#endif

	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("write success.");
	} else {
		SFUD_INFO("%s write cmd failed\n", __func__);
		return result;
	}

	return result;
}

static int snf_send_write_page_cache(snfud_flash_chip *chip,uint32_t page, uint32_t offset, uint8_t *data, int size)
{
	static uint8_t *buf = NULL;
	uint32_t len;
	int result;
	uint32_t addr =(offset & 0xFFF);
	uint32_t malloc_len = chip->byte_per_page + chip->spare_size + 4;

	SNFUD_ASSERT(chip);
	SNFUD_ASSERT(chip->flash);

	if (!buf) {
		buf = pvPortMalloc(malloc_len);
		if (!buf) {
			SFUD_INFO("%s pvPortMalloc failed.", __func__);
			return SFUD_ERR_NOT_FOUND;
		}
	}

	len = size + 3;
	if (len > malloc_len) {
		printf("%s Invalid size:%d.\n", __func__, size);
		return SFUD_ERR_WRITE;
	}

	if (chip->privInfo && chip->privInfo->planeSelIndex) {
		int block = page / chip->page_per_blk;
		if (block % 2)
			addr |= (1 << SNF_PsArray[chip->privInfo->planeSelIndex].plb);
	}

	buf[0] = SNFUD_CMD_PROGRAM_LOAD;
	buf[1] = (addr >> 8) & 0xFF;
	buf[2] = (addr >> 0) & 0xFF;
	memcpy(&buf[3], data, size);
	result = chip->flash->spi.wr(&chip->flash->spi, buf, len, NULL, 0);
	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("%s success.", __func__);
	} else {
		SFUD_INFO("%s failed\n", __func__);
	}

	return result;
}

static int snf_send_write_page_cmd(snfud_flash_chip *chip, uint32_t page)
{
	int result;
	uint8_t cmd[4];

	SNFUD_ASSERT(chip);
	SNFUD_ASSERT(chip->flash);

	if (chip->privInfo && chip->privInfo->planeSelIndex) {
		int block = page / chip->page_per_blk;
		if (block % 2)
			page |= (1 << SNF_PsArray[chip->privInfo->planeSelIndex].peb);//set plane select bit.
	}

	cmd[0] = SNFUD_CMD_PROGRAM_EXEC;
	cmd[1] = (page >> 16) & 0xFF;
	cmd[2] = (page >> 8) & 0xFF;
	cmd[3] = (page >> 0) & 0xFF;
	result = chip->flash->spi.wr(&chip->flash->spi, cmd, sizeof(cmd), NULL, 0);
	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("%s success.", __func__);
	} else {
		SFUD_INFO("%s failed\n", __func__);
		return result;
	}

	return result;
}

/* read a few bytes in a page with bad block check */
static int snf_read_bytes_in_page(snfud_flash_chip *chip, uint32_t page, uint32_t offset, uint8_t *data, uint32_t size)
{
	int eccErrMask = SNFUD_ECC_ERR_MASK;
	uint8_t status;
	int result;

	SNFUD_ASSERT(chip);

	result = snf_send_read_page_cmd(chip, page);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_send_read_page_cmd failed\n", __func__);
		return result;
	}
	if (chip->privInfo && chip->privInfo->crbsyBit) {
		result = snf_wait_read_busy(chip->flash, &status);
	} else {
		result = snf_wait_busy(chip->flash, &status);
	}
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_wait_busy failed\n", __func__);
		return result;
	}

	if (chip->privInfo && chip->privInfo->eccErrMask) {
		eccErrMask = (1 << chip->privInfo->eccErrMask);
	}

	if (status & eccErrMask) {	//ECC error meains bad block.
		SFUD_INFO("%s (blk:%d, page:%d) ECC error, status:0x%x..\n", __func__, page /chip->page_per_blk , page, status);
#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
		memset((void *)data, 0, size);
#endif
		return SFUD_ERR_READ_ECC;
	}

	result = snf_read_page_cache(chip, page, offset, data, size);

	return result;
}

static int snf_read_page_spare(snfud_flash_chip *chip, uint32_t page, uint8_t *data, uint32_t size)
{
	SNFUD_ASSERT(chip);

	return snf_read_bytes_in_page(chip, page, chip->byte_per_page, data, size);
}

/* write a few bytes in a page without bad block check */
static int snf_write_bytes_in_page(snfud_flash_chip *chip, uint32_t page, uint32_t offset, uint8_t *data, uint32_t size)
{
	uint8_t status;
	int result;

	//SNFUD_ASSERT(chip);
	//SNFUD_ASSERT(chip->flash);

	snf_write_enabled(chip->flash, true);
	result = snf_send_write_page_cache(chip, page, offset, data, size);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_send_read_page_cmd failed\n", __func__);
		return result;
	}

	result = snf_send_write_page_cmd(chip, page);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_send_write_page_cmd failed\n", __func__);
		return result;
	}

	result = snf_wait_busy(chip->flash, &status);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_wait_busy failed\n", __func__);
		return result;
	}

	if (status & SNFUD_STATUS_REG_PROG_FAIL) {
		SFUD_INFO("%s program failed\n", __func__);
		return SFUD_ERR_BAD_BLK;
	}

	return SFUD_SUCCESS;
}

static int snf_erase_block(snfud_flash_chip *chip, uint32_t blk)
{
	sfud_err result = SFUD_SUCCESS;
	uint32_t page_offset;
	uint8_t status;
	uint8_t cmd[4];

	SNFUD_ASSERT(chip);

	snf_write_enabled(chip->flash, true);

	page_offset = blk * chip->page_per_blk;
	cmd[0] = SNFUD_CMD_BLOCK_ERASE;
	cmd[1] = (page_offset >> 16) & 0xFF;
	cmd[2] = (page_offset >> 8) & 0xFF;
	cmd[3] = (page_offset >> 0) & 0xFF;
	result = chip->flash->spi.wr(&chip->flash->spi, cmd, sizeof(cmd), NULL, 0);
	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("%s success.", __func__);
	} else {
		SFUD_INFO("%s failed\n", __func__);
		return result;
	}

	result = snf_wait_busy(chip->flash, &status);
	if (result == SFUD_SUCCESS) {
		if (status & SNFUD_STATUS_REG_ERASE_FAIL) {
			SFUD_INFO("%s erase failed\n", __func__);
			return SFUD_ERR_ERASE;
		}
	}

	return SFUD_SUCCESS;
}

static int snf_check_physical_bad_block(snfud_flash_chip *chip, uint32_t block)
{
	uint8_t spare_buf[4];
	uint32_t start_page;
	int result;

	SNFUD_ASSERT(chip);

	start_page = block * chip->page_per_blk;

	result = snf_read_page_spare(chip, start_page, spare_buf, sizeof(spare_buf));
	if (result != SFUD_SUCCESS) {
		printf("%s, snf_read_page_spare page0 failed, result:%d.\n", __func__, result);
		goto exit;
	}
	if (spare_buf[0] == 0xFF) {
		if (chip->bbm_type == 2) {
			result = snf_read_page_spare(chip, start_page + 1, spare_buf, sizeof(spare_buf));
			if (result != SFUD_SUCCESS) {
				printf("%s, snf_read_page_spare page1 failed, result:%d.\n", __func__, result);
				goto exit;
			}
			if (spare_buf[0] == 0xFF) {
				return SFUD_SUCCESS;
			} else {
				result = SFUD_ERR_BAD_BLK;
			}
		} else {
			return SFUD_SUCCESS;
		}
	}

exit:
	SFUD_INFO("%s bad block:%d.", __func__, block);

	return SFUD_ERR_BAD_BLK;
}

static int snf_write_physical_blcok(snfud_flash_chip *chip, uint32_t blk, uint32_t offset, uint8_t *data, uint32_t size)
{
	uint8_t *buf = (uint8_t *)data;
	uint32_t byte_per_page;
	uint32_t curr_page;
	uint32_t offset_in_page;
	uint32_t write_size;
	uint32_t result = SFUD_ERR_WRITE;
	int remain;

	SNFUD_ASSERT(chip);

	remain = size;
	byte_per_page = chip->byte_per_page;
	curr_page = blk * chip->page_per_blk + offset / byte_per_page;
	offset_in_page = offset % byte_per_page;

	while (remain > 0) {
		write_size = ((byte_per_page - offset_in_page) >= remain) ? remain : (byte_per_page - offset_in_page);
		result = snf_write_bytes_in_page(chip, curr_page, offset_in_page, buf, write_size);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s write page failed, result:%d\n", __func__, result);
			return result;
		}

		if (offset_in_page)
			offset_in_page = 0;

		curr_page++;
		remain -= write_size;
		buf += write_size;
	}

	return result;
}

static int snf_read_physical_blcok(snfud_flash_chip *chip, uint32_t blk, uint32_t offset, uint8_t *data, uint32_t size)
{
	uint8_t *buf = (uint8_t *)data;
	uint32_t byte_per_page;
	uint32_t curr_page;
	uint32_t offset_in_page;
	uint32_t read_size;
	uint32_t result = SFUD_ERR_READ;
	int remain;

	SNFUD_ASSERT(chip);

	remain = size;
	byte_per_page = chip->byte_per_page;
	curr_page = blk * chip->page_per_blk + offset / byte_per_page;
	offset_in_page = offset % byte_per_page;

	while (remain > 0) {
		read_size = ((byte_per_page - offset_in_page) >= remain) ? remain : (byte_per_page - offset_in_page);
		result = snf_read_bytes_in_page(chip, curr_page, offset_in_page, buf, read_size);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s read page failed, result:%d\n", __func__, result);
			return result;
		}

		if (offset_in_page)
			offset_in_page = 0;

		curr_page++;
		remain -= read_size;
		buf += read_size;
	}

	return result;
}

static int snf_read_data(snfud_flash_chip *chip, uint32_t addr, uint8_t *data, uint32_t size)
{
	uint32_t result = SFUD_ERR_READ;
	uint32_t curr_blk, offset_in_blk;
	uint32_t read_size, byte_per_blk;
	uint8_t *buf;
	int remain;

	SNFUD_ASSERT(chip);

	buf = data;
	byte_per_blk = chip->byte_per_blk;
	curr_blk = addr / chip->byte_per_blk;
	offset_in_blk = addr % chip->byte_per_blk;
	remain = size;

	while (remain > 0) {
		read_size = ((byte_per_blk - offset_in_blk) >= remain) ? remain : (byte_per_blk - offset_in_blk);
		result = snf_read_within_block(chip, curr_blk, offset_in_blk, buf, read_size);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s read page failed, result:%d.\n", __func__, result);
			return result;
		}

		if (offset_in_blk)
			offset_in_blk = 0;

		curr_blk++;
		remain -= read_size;
		buf += read_size;
	}

	return result;
}

#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
static snf_cache_blk_t *snf_seek_cache_block(snfud_flash_chip *chip, uint32_t blk)
{
	SNFUD_ASSERT(chip);

	for (int i = 0; i < SNFUD_CACHE_BLOCK_COUNT; i++) {
		if (chip->cache_info->cache_blks[i].blk_id == blk) {
			return &chip->cache_info->cache_blks[i];
		}
	}

	return NULL;
}

static sfud_err snf_skip_read_within_block(snfud_flash_chip *chip, uint32_t blk, uint32_t skip_offset, uint32_t skip_size, uint8_t *data)
{
	sfud_err result = SFUD_SUCCESS;
	uint8_t *pdata = data;
	uint32_t read_offset, read_size;

	if (skip_offset) {
		result = snf_read_within_block(chip, blk, 0, pdata, skip_offset);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s read failed, result:%d.", __func__, result);
			return result;
		}
	}

	read_offset = skip_offset + skip_size;
	read_size = chip->byte_per_blk - read_offset;
	pdata += read_offset;

	if (read_offset < chip->byte_per_blk) {
		result = snf_read_within_block(chip, blk, read_offset, pdata, read_size);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s read failed, result:%d.", __func__, result);
			return result;
		}
	}

	return result;
}

static void snf_repair_read_ecc_error_block(snfud_flash_chip *chip, uint32_t blk, uint32_t replace_blk)
{
	int i;
	int start_page;
	uint8_t *blk_cache, *pdata;

	start_page = replace_blk * chip->page_per_blk;	//phy blk
	blk_cache = (uint8_t *)pvPortMalloc(chip->byte_per_blk);
	if (!blk_cache) {
		SFUD_INFO("%s malloc blk_cache failed.", __func__);
		return;
	}

	memset(blk_cache, 0, chip->byte_per_blk);
	pdata = blk_cache;

	for (i = 0;i < chip->page_per_blk; i++) {
		if (snf_read_bytes_in_page(chip, start_page + i, 0, pdata, chip->byte_per_page) != SFUD_SUCCESS) {
			SFUD_INFO("%s read one page failed.\n", __func__); //shield ECC exception
		}
		pdata += chip->byte_per_page;
	}

	snf_erase_write_one_block(chip, blk, blk_cache);	//logic blk
	vPortFree(blk_cache);

	return;
}

static sfud_err snf_read_within_block(snfud_flash_chip *chip, uint32_t blk, uint32_t offset, uint8_t *data, uint32_t size)
{
	sfud_err result = SFUD_SUCCESS;
	snf_cache_blk_t *cache_blk;
	uint32_t block;

	SNFUD_ASSERT(chip);

	cache_blk = snf_seek_cache_block(chip, blk);
	if (cache_blk) {
		memcpy((void *)data, (void *)(cache_blk->buf + offset), size);
		return SFUD_SUCCESS;
	}

	block = blk;
	if (chip->bbt->node[blk].status == SNF_BLK_BAD) {
		block = chip->bbt->node[blk].replace;
		if (snf_check_replace_block_valid(chip, block) != SFUD_SUCCESS) {
			SFUD_INFO("###ERR: %s,blk:%d is bad block, it's replace block:%d is Invalid.\n", __func__, blk, block);
			/*
				1.The bad block replace value is 0 when the bad block table is initialized.
				2.The bad block table did not have a valid replacement block 
					when the bad block was looking for a replacement block.
			*/
			snf_repair_read_ecc_error_block(chip, blk, blk);
			return SFUD_ERR_BAD_BLK;
		}
	}

	result = snf_read_physical_blcok(chip, block, offset, data, size);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s read physical block failed, result:%d\n", __func__, result);
		if (result == SFUD_ERR_READ_ECC)
			snf_repair_read_ecc_error_block(chip, blk, block);
	}

	return result;
}

static sfud_err snf_erase_write_within_block(snfud_flash_chip *chip, uint32_t blk, uint32_t offset, uint8_t *data, uint32_t size)
{
	sfud_err result = SFUD_SUCCESS;
	uint32_t byte_per_blk, block;
	uint8_t *buf = data;
	uint8_t bbtUpdate = 0;

	SNFUD_ASSERT(chip);
	byte_per_blk = chip->byte_per_blk;
	block = blk;

	if (chip->bbt->node[block].status != SNF_BLK_GOOD) {
		block = chip->bbt->node[block].replace;
		if (snf_check_replace_block_valid(chip, block) != SFUD_SUCCESS) {
			SFUD_INFO("%s, blk:%d is bad block, it's replace block:%d is Invalid.\n", __func__, blk, block);
retry:
			bbtUpdate = 1;
			result = snf_find_a_replace_block(chip, &block);
			if (result != SFUD_SUCCESS) {
				SFUD_INFO("%s, blk:%d is bad block, not available replace blk.\n", __func__, blk);
				goto exit;
			}

			chip->bbt->node[blk].replace = block;
		}
	}

	if (size != byte_per_blk) {
		if (!chip->cache_info->blk_backup_buf) {
			chip->cache_info->blk_backup_buf = (uint8_t *)pvPortMalloc(byte_per_blk);
			if (!chip->cache_info->blk_backup_buf) {
				SFUD_INFO("%s malloc blk_backup_buf failed.", __func__);
				result = SFUD_ERR_NOT_FOUND;
				goto exit;
			}
		}
		buf = chip->cache_info->blk_backup_buf;

		result = snf_skip_read_within_block(chip, blk, offset, size, buf);//logic blk
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s read the page failed, result:%d.\n", __func__, result);
			result = SFUD_ERR_READ;
			goto exit;
		}

		memcpy(buf + offset, data, size);
		size = byte_per_blk;
	} else {
		if (offset) {
			printf("%s Invalid ofsset:%d.\n", __func__, offset);
			offset = 0;
		}
	}

	result = snf_erase_block(chip, block);//phy blk
	if (result != SFUD_SUCCESS) {
		printf("%s erase block failed, result:%d.\n", __func__, result);
		chip->bbt->node[block].status = SNF_BLK_BAD;
		chip->bbt->bad_blocks++;
		bbtUpdate = 1;
		goto retry;
	}

	result = snf_write_physical_blcok(chip, block, offset, buf, size);//phy blk
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s data write failed, result:%d.\n", __func__, result);
		chip->bbt->node[block].status = SNF_BLK_BAD;
		chip->bbt->bad_blocks++;
		bbtUpdate = 1;
		goto retry;
	}

exit:
	if (bbtUpdate)
		snf_update_bbt(chip);

	return result;
}

static sfud_err snf_erase_write_one_block(snfud_flash_chip *chip, uint32_t blk, uint8_t *data)
{
	return snf_erase_write_within_block(chip, blk, 0, data, chip->byte_per_blk);
}

static snf_cache_blk_t *snf_get_free_cache_block(snfud_flash_chip *chip)
{
	sfud_err result = SFUD_SUCCESS;
	uint32_t index;
	uint32_t min_time;

	SNFUD_ASSERT(chip);

	index = 0;
	min_time = chip->cache_info->cache_blks[0].timestamp;

	for (int i = 0; i < SNFUD_CACHE_BLOCK_COUNT; i++) {
		if (!chip->cache_info->cache_blks[i].need_flush) {
			return &chip->cache_info->cache_blks[i];
		}

		if (chip->cache_info->cache_blks[i].timestamp < min_time) {
			index = i;
			min_time = chip->cache_info->cache_blks[i].timestamp;
		}
	}

	result = snf_erase_write_one_block(chip, chip->cache_info->cache_blks[index].blk_id, (uint8_t *)chip->cache_info->cache_blks[index].buf);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s erase write blk failed, result:%d.", __func__, result);
		return NULL;
	}

	chip->cache_info->cache_blks[index].need_flush = 0;

	return &chip->cache_info->cache_blks[index];
}

static sfud_err snf_write_within_cache_block(snfud_flash_chip *chip, uint32_t blk, uint32_t offset, uint8_t *data, uint32_t size)
{
	sfud_err result = SFUD_SUCCESS;
	snf_cache_blk_t *cache_blk = NULL;

	SNFUD_ASSERT(chip);
	SNFUD_ASSERT(chip->flash);

	cache_blk = snf_seek_cache_block(chip, blk);
	if (cache_blk == NULL) {
		cache_blk = snf_get_free_cache_block(chip);
		if (cache_blk == NULL ) {
			SFUD_INFO("%s get cache block failed.", __func__);
			return SFUD_ERR_NOT_FOUND;
		}

		result = snf_skip_read_within_block(chip, blk, offset, size, (uint8_t *)cache_blk->buf);//logic blk
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s read failed, result:%d.", __func__, result);
			return result;
		}

		cache_blk->blk_id = blk;
		cache_blk->timestamp = xTaskGetTickCount();
	}

	memcpy((void *)(cache_blk->buf + offset), data, size);
	if (!cache_blk->need_flush)
		cache_blk->need_flush = 1;

	return result;
}

static void snf_cache_flush_thread(void *param)
{
	sfud_err result = SFUD_SUCCESS;
	sfud_flash *flash = (sfud_flash *)param;
	snf_cache_blk_t *cache_blk = NULL;
	snfud_flash_chip *chip;
	sfud_spi *spi;
	uint32_t tick;

	spi = (sfud_spi *)&flash->spi;
	chip = (snfud_flash_chip *)SNF_GET_CHIP(flash);
	SNFUD_ASSERT(chip);

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000));

		/* lock SPI */
		if (spi->lock) {
			spi->lock(spi);
		}
		tick = xTaskGetTickCount();

		for (int i = 0; i < SNFUD_CACHE_BLOCK_COUNT; i++) {
			cache_blk = &chip->cache_info->cache_blks[i];
			if (cache_blk->need_flush) {
				if ((tick - chip->cache_info->cache_blks[i].timestamp) >= SNFUD_CACHE_FLUSH_TIMEOUT) {
					result = snf_erase_write_one_block(chip, cache_blk->blk_id, (uint8_t *)cache_blk->buf);
					if (result != SFUD_SUCCESS) {
						SFUD_INFO("%s flush cache block:%d failed, result:%d.", __func__, cache_blk->blk_id, result);
					}
					cache_blk->need_flush = 0;
				}
			}
		}

		/* set the flash write disable */
		snf_write_enabled(flash, false);

		/* unlock SPI */
		if (spi->unlock) {
			spi->unlock(spi);
		}
	}
}

static sfud_err snf_erase_write(snfud_flash_chip *chip, uint32_t addr, size_t size, const uint8_t *data)
{
	sfud_err result = SFUD_ERR_WRITE;
	uint32_t start_blk, start_offset_in_blk;
	uint32_t byte_per_blk, left_size;
	uint8_t * pdata;

	SNFUD_ASSERT(chip);

	pdata = (uint8_t *)data;
	byte_per_blk = chip->byte_per_blk;
	start_blk = addr / chip->byte_per_blk;
	start_offset_in_blk = addr % chip->byte_per_blk;
	left_size = size;

	/* erase write the start block */
	if (start_offset_in_blk) {
		uint32_t erase_write_size = ((byte_per_blk - start_offset_in_blk) >= size) ? size : (byte_per_blk - start_offset_in_blk);
		result = snf_write_within_cache_block(chip, start_blk, start_offset_in_blk, pdata, erase_write_size);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s erase write the first blk failed, result:%d.", __func__, result);
			return result;
		}
		start_blk++;
		left_size -= erase_write_size;
		pdata += erase_write_size;
	}

	/* erase write the middle blocks which are complete blocks */
	if (left_size > 0) {
		uint32_t whole_blk_num = left_size / byte_per_blk;
		for (uint32_t i = 0; i < whole_blk_num; i++) {
			snf_cache_blk_t *cache_blk = snf_seek_cache_block(chip, start_blk);
			result = snf_erase_write_one_block(chip, start_blk, pdata);
			if (result != SFUD_SUCCESS) {
				SFUD_INFO("%s erase write the blk failed,result:%d.", __func__, result);
//				if (cache_blk) {
//					memcpy((void *)cache_blk->buf, data, chip->byte_per_blk);
//					cache_blk->need_flush = 1;
//					result = SFUD_SUCCESS;
//				}
				return result;
			}
			if (cache_blk) {
				cache_blk->blk_id = -1;
				cache_blk->need_flush = 0;
			}
			start_blk++;
			left_size -= byte_per_blk;
			pdata += byte_per_blk;
		}
	}

	/* erase write the last block */
	if (left_size > 0) {
		uint32_t stop_offset_in_blk = left_size % byte_per_blk;
		if (stop_offset_in_blk) {
			result = snf_write_within_cache_block(chip, start_blk, 0, pdata, stop_offset_in_blk);
			if (result != SFUD_SUCCESS) {
				SFUD_INFO("%s erase write the last blk failed,result:%d.", __func__, result);
				return result;
			}
		}
	}

	return result;
}
#endif

/**
 * flush cache data to flash
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_flush(const sfud_flash *flash)
{
#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
	sfud_err result = SFUD_SUCCESS;
	snf_cache_blk_t *cache_blk;

	SNFUD_ASSERT(flash);
	/* must be call this function after initialize OK */
	if (!flash->init_ok) {
		SFUD_INFO("%s, Error: flash initialization is not complete.", __func__);
		return -1;
	}
	sfud_spi *spi = (sfud_spi *)&flash->spi;
	snfud_flash_chip *chip = (snfud_flash_chip *)SNF_GET_CHIP(flash);

	SNFUD_ASSERT(chip);

	/* lock SPI */
	if (spi->lock) {
		spi->lock(spi);
	}

	for (int i = 0; i < SNFUD_CACHE_BLOCK_COUNT; i++)
	{
		cache_blk = &chip->cache_info->cache_blks[i];
		if (cache_blk->need_flush) {
			result = snf_erase_write_one_block(chip, cache_blk->blk_id, (uint8_t *)cache_blk->buf);
			if (result != SFUD_SUCCESS) {
				SFUD_INFO("%s erase write blk failed, result:%d.", __func__, result);
			}
			cache_blk->need_flush = 0;
		}
	}

	/* set the flash write disable */
	snf_write_enabled(flash, false);

	/* unlock SPI */
	if (spi->unlock) {
		spi->unlock(spi);
	}
#endif
	return SFUD_SUCCESS;
}

/**
 * read flash data
 *
 * @param flash flash device
 * @param addr start address
 * @param size read size
 * @param data read data pointer
 *
 * @return result
 */
sfud_err sfud_read(const sfud_flash *flash, uint32_t addr, size_t size, uint8_t *data)
{
	sfud_err result = SFUD_SUCCESS;
	sfud_spi *spi;
	snfud_flash_chip *chip;
	uint8_t status;

	SNFUD_ASSERT(flash);
	/* must be call this function after initialize OK */
	if (!flash->init_ok) {
		printf("%s flash initialization is not complete.\n", __func__);
		return -1;
	}

	spi = (sfud_spi *)&flash->spi;
	chip = (snfud_flash_chip *)SNF_GET_CHIP(flash);

	/* check the flash address bound */
	if (addr + size > chip->available_capacity) {
		SFUD_INFO("Error: Flash address is out of bound.");
		return SFUD_ERR_ADDR_OUT_OF_BOUND;
	}

	/* lock SPI */
	if (spi->lock) {
		spi->lock(spi);
	}

	if (chip->privInfo && chip->privInfo->crbsyBit) {
		result = snf_wait_read_busy(flash, &status);
	} else {
		result = snf_wait_busy(flash, &status);
	}
	if (result == SFUD_SUCCESS)
	{
		result = snf_read_data(chip, addr, data, size);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("%s failed, result:%d.", __func__, result);
		}
	} else {
		SFUD_INFO("%s snf_wait_busy timeout.", __func__);
	}

	/* unlock SPI */
	if (spi->lock && spi->unlock) {
		spi->unlock(spi);
	}

	return result;
}

/**
 * erase all flash data
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_chip_erase(const sfud_flash *flash)
{
	snfud_flash_chip *chip;
	sfud_spi *spi;
	sfud_err result = SFUD_SUCCESS;
	int i;

	SNFUD_ASSERT(flash);

	spi = (sfud_spi *)&flash->spi;
	chip = (snfud_flash_chip *)SNF_GET_CHIP(flash);

	if (spi->lock) {
		spi->lock(spi);
	}

	for (i=0; i<chip->total_blk; i++) {
		result = snf_erase_block(chip, i);
	}

	if (spi->unlock) {
		spi->unlock(spi);
	}
	return result;
}


/**
 * erase flash data
 *
 * @note It will erase align by erase granularity.
 *
 * @param flash flash device
 * @param addr start address
 * @param size erase size
 *
 * @return result
 */
sfud_err sfud_erase(const sfud_flash *flash, uint32_t addr, size_t size)
{
#ifndef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
	snfud_flash_chip *chip;
	sfud_err result = SFUD_SUCCESS;
	sfud_spi *spi;
	int start;
	int num;
	int i;

	SNFUD_ASSERT(flash);
	/* must be call this function after initialize OK */
	if (!flash->init_ok) {
		SFUD_INFO("%s, Error: flash initialization is not complete.", __func__);
		return -1;
	}

	spi = (sfud_spi *)&flash->spi;
	chip = (snfud_flash_chip *)SNF_GET_CHIP(flash);

	if ((addr % chip->byte_per_blk) ) {
		SFUD_INFO("%s, Error: invalid start address, not align with block size.", __func__, addr);
		return SFUD_ERR_ADDR_OUT_OF_BOUND;
	}
	if (size % chip->byte_per_blk) {
		SFUD_INFO("%s, Error: invalid size, not align with block size.", __func__, size);
		return SFUD_ERR_ADDR_OUT_OF_BOUND;
	}

	/* check the flash address bound */
	if (addr + size > chip->available_capacity) {
		SFUD_INFO("%s, Error: Flash address or size is out of bound.", __func__);
		return SFUD_ERR_ADDR_OUT_OF_BOUND;
	}

	/* lock SPI */
	if (spi->lock) {
		spi->lock(spi);
	}

	start = addr / chip->byte_per_blk;
	num = size / chip->byte_per_blk;

	for (i = 0; i < num; i++) {
		snf_erase_block(SNF_GET_CHIP(flash), start + i);
	}

	/* set the flash write disable */
	snf_write_enabled(flash, false);

	/* unlock SPI */
	if (spi->unlock) {
		spi->unlock(spi);
	}

	return result;
#else
	return SFUD_SUCCESS;
#endif
}

/**
 * write flash data (no erase operate)
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
sfud_err sfud_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data)
{
	sfud_err result = SFUD_SUCCESS;

	SNFUD_ASSERT(flash);
	/* must be call this function after initialize OK */
	if (!flash->init_ok) {
		SFUD_INFO("%s, Error: flash initialization is not complete.", __func__);
		return -1;
	}

	sfud_spi *spi = (sfud_spi *)&flash->spi;
	snfud_flash_chip *chip = (snfud_flash_chip *)SNF_GET_CHIP(flash);

	/* check the flash address bound */
	if (addr + size > chip->available_capacity) {
		SFUD_INFO("%s, Error: Flash address or size is out of bound.", __func__);
		return SFUD_ERR_ADDR_OUT_OF_BOUND;
	}

	/* lock SPI */
	if (spi->lock) {
		spi->lock(spi);
	}
	result = snf_erase_write(chip, addr, size, data);

	/* set the flash write disable */
	snf_write_enabled(flash, false);

	/* unlock SPI */
	if (spi->unlock) {
		spi->unlock(spi);
	}

	return result;
}

/**
 * erase and write flash data
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
sfud_err sfud_erase_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data)
{
	sfud_err result = SFUD_SUCCESS;

	if (!data || !size) {
		printf("%s, Invalid data:%p or size:%d\n", __func__, data, size);
		return SFUD_ERR_WRITE;
	}

#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
	result = sfud_write(flash, addr, size, data);
#else
	result = sfud_erase(flash, addr, size);
	if (result == SFUD_SUCCESS) {
		result = sfud_write(flash, addr, size, data);
	}
#endif
	return result;
}

static sfud_err snf_reset(const sfud_flash *flash)
{
	sfud_err result = SFUD_SUCCESS;
	sfud_spi *spi;
	uint8_t cmd_data;
	uint8_t status;

	SNFUD_ASSERT(flash);
	spi = (sfud_spi *)&flash->spi;
	cmd_data = SNFUD_CMD_RESET;
	result = spi->wr(spi, &cmd_data, 1, NULL, 0);

	if (result == SFUD_SUCCESS) {
		result = snf_wait_busy(flash, &status);
		if (result != SFUD_SUCCESS) {
			SFUD_INFO("snf_wait_busy failed.");
		}
	}

	if (result == SFUD_SUCCESS) {
		SFUD_DEBUG("Flash device reset success.");
	} else {
		SFUD_INFO("Error: Flash device reset failed.");
	}

	return result;
}

static int snf_check_replace_block_valid(snfud_flash_chip *chip, uint32_t block)
{
	if ((block >= chip->available_blks) && (block < chip->bbt_blk_offset)) {
		return SFUD_SUCCESS;
	}

	return SFUD_ERR_NOT_FOUND;
}

static sfud_err snf_find_a_replace_block(snfud_flash_chip *chip, uint32_t *block)
{
	uint32_t i;
	uint32_t replace_blk_start = chip->available_blks;
	uint32_t replace_blk_stop = chip->available_blks + chip->replace_blks;

	/* Scan the free replace blocks form endding to startting.
	 *	When you decrease the number of replacement blocks, some blocks in the starting position
	 *	which will be reseased may have already been used, and the used blocks will be saved in bbt.
	 *	These blocks cannot be used as logic blocks again, so we scan then free replace blocks form
	 *	endding to startting, avoid these released blocks being used in bbt.
	 */
	i = replace_blk_stop - 1;

retry:
	for (; i>=replace_blk_start; i--) {
		if (chip->bbt->node[i].status == SNF_BLK_GOOD) {
			break;
		}
	}

	if (i >= replace_blk_start) {
		if (snf_erase_block(chip, i) == SFUD_SUCCESS) {
			*block = i;
			chip->bbt->node[i].status = SNF_BLK_USED;
			return SFUD_SUCCESS;
		} else {
			chip->bbt->node[i].status = SNF_BLK_BAD;
			goto retry;
		}
	}

	printf("%s failed, there are no valid replace blocks.\n", __func__);

	return SFUD_ERR_NOT_FOUND;
}

static sfud_err snf_write_bbt(snfud_flash_chip *chip, uint8_t *buf, uint32_t size)
{
	sfud_err result = SFUD_SUCCESS;
	int i, curr_block;
	int bbt_bad_blocks = 0;
	int write_block_num = 0;

	for (i=0; i<SNFUD_BBT_BLK_COUNT; i++) {
		curr_block = chip->bbt_blk_offset + i;

		if (write_block_num >= 2) {
			break;
		}

		if (chip->bbt->node[curr_block].status == SNF_BLK_BAD) {
			bbt_bad_blocks++;
			continue;
		}

		result = snf_erase_block(chip, curr_block);
		if (result != SFUD_SUCCESS) {
			printf("%s erase block(%d) failed, result=%d.\n", __func__, curr_block, result);
			if (result == SFUD_ERR_ERASE) {
				chip->bbt->node[curr_block].status = SNF_BLK_BAD;
				chip->bbt->bad_blocks++;
				bbt_bad_blocks++;
				continue;
			}
			break;
		}

		result = snf_write_physical_blcok(chip, curr_block, 0, buf, size);
		if (result != SFUD_SUCCESS) {
			printf("%s write block(%d) failed, result=%d\n", __func__, curr_block, result);
			if (result == SFUD_ERR_BAD_BLK) {
				chip->bbt->node[curr_block].status = SNF_BLK_BAD;
				chip->bbt->bad_blocks++;
				bbt_bad_blocks++;
				continue;
			}
			break;
		}

		write_block_num++;
	}

	if (bbt_bad_blocks) {
		printf("%s, bbt has %d bad blcoks.\n", __func__, bbt_bad_blocks);
	}

	return result;
}

static sfud_err snf_read_bbt(snfud_flash_chip *chip, uint8_t *buf, int size)
{
	sfud_err result = SFUD_SUCCESS;
	uint32_t crc_size;
	uint32_t checksum;
	int curr_block;
	int i;

	SNFUD_ASSERT(chip);

	crc_size = size - SNFUD_BBT_HEADER_SIZE - sizeof(chip->bbt->bbt_crc);

	for (i=0; i<SNFUD_BBT_BLK_COUNT; i++) {
		curr_block = chip->bbt_blk_offset + i;
		result = snf_read_physical_blcok(chip, curr_block, 0, buf, size);
		if (result != SFUD_SUCCESS) {
			printf("%s snf_read_physical_blcok failed(block: %d).\n", __func__, curr_block);
			continue;
		}

		if (memcmp(chip->bbt->header_mask, SNFUD_BBT_HEADER_MASK, sizeof(SNFUD_BBT_HEADER_MASK))) {
			printf("%s Not fond BBT header(block: %d).\n", __func__, curr_block);
			result = SFUD_ERR_NOT_FOUND;
			continue;
		}

		checksum = xcrc32(buf + SNFUD_BBT_HEADER_SIZE, crc_size, 0xffffffff);

		if (chip->bbt->bbt_crc != checksum) {
			printf("%s BBT information is abnormal(block: %d).\n", __func__, curr_block);
			result = SFUD_ERR_NOT_FOUND;
			continue;
		}

		break;
	}

	return result;
}

static sfud_err snf_update_bbt(snfud_flash_chip *chip)
{
	sfud_err result = SFUD_SUCCESS;
	uint8_t *pbuf;
	uint32_t crc_size, bbt_size;

	SNFUD_ASSERT(chip);

	pbuf = (uint8_t *)chip->bbt;
	bbt_size = sizeof(snf_bbt);
	crc_size = bbt_size - SNFUD_BBT_HEADER_SIZE - sizeof(chip->bbt->bbt_crc);
	chip->bbt->bbt_crc = xcrc32(pbuf + SNFUD_BBT_HEADER_SIZE, crc_size, 0xffffffff);

	result = snf_write_bbt(chip, pbuf, bbt_size);
	if (result != SFUD_SUCCESS) {
		printf("%s write bbt failed, result:%d.\n", __func__, result);
	}

	return result;
}

static sfud_err snf_bbt_init(snfud_flash_chip *chip)
{
	sfud_err result = SFUD_SUCCESS;
	uint8_t *pbuf;
	int bbt_size;
	int i;

	SNFUD_ASSERT(chip);

	bbt_size = sizeof(snf_bbt);
	chip->bbt = (snf_bbt *)pvPortMalloc(bbt_size);
	if (!chip->bbt) {
		SFUD_INFO("%s pvPortMalloc failed.", __func__);
		return SFUD_ERR_NOT_FOUND;
	}

	memset(chip->bbt, 0, bbt_size);
	pbuf = (uint8_t *)chip->bbt;

	//check and create a BBT.
	result = snf_read_bbt(chip, pbuf, bbt_size);
	if (result != SFUD_SUCCESS) {
		memset(chip->bbt, 0, bbt_size);
		printf("%s Create a new BBT.\n", __func__);
		for (i=0; i<chip->total_blk; i++) {
			result = snf_check_physical_bad_block(chip, i);
			if (result == SFUD_SUCCESS) {
				chip->bbt->node[i].status = SNF_BLK_GOOD;
			} else {
				chip->bbt->node[i].status = SNF_BLK_BAD;
				chip->bbt->bad_blocks++;
			}
			chip->bbt->node[i].replace = 0;
		}
		memcpy(chip->bbt->header_mask, SNFUD_BBT_HEADER_MASK, sizeof(SNFUD_BBT_HEADER_MASK));
		snf_update_bbt(chip);
		result = SFUD_SUCCESS;
	}
#ifdef SNFUD_BBT_DEBUG	//Only for test (print bbt info).
	else {
		int count = 0;
		sfud_err ret;

		printf("\n###### BBT scan start...\n");
		for (i=0; i<chip->total_blk; i++) {
			if (chip->bbt->node[i].status == SNF_BLK_BAD) {
				if (i < chip->available_blks) {
					printf("	available area: bad block:%d, replace block:%d.\n", i, chip->bbt->node[i].replace);
				} else if (i < chip->bbt_blk_offset) {
					printf("	replace area: bad block:%d.\n", i);
				} else {
					printf("	bbt area: bad block:%d.\n", i);
				}
				ret = snf_check_physical_bad_block(chip, i);
				if (ret == SFUD_SUCCESS) {
					printf("****** pseudo bad block(%d).\n", i);
				}
				count++;
			} else {
				//ret = snf_check_physical_bad_block(chip, i);
				//if (ret != SFUD_SUCCESS) {
				//	printf("physical bad block(%d).\n", i);
				//}
			}
		}
		printf("###### %d bad blocks fond.\n", count);
		printf("###### BBT scan stop.\n\n");
	}
#endif

	return result;
}

/**
 * hardware initialize
 */
static sfud_err snf_hardware_init(sfud_flash *flash)
{
	extern sfud_err sfud_spi_port_init(sfud_flash * flash);
	snfud_flash_chip *chip = NULL;
	sfud_err result = SFUD_SUCCESS;
	int i;

	SNFUD_ASSERT(flash);

	result = sfud_spi_port_init(flash);
	if (result != SFUD_SUCCESS) {
		return result;
	}

	/* SPI write read function must be initialize */
	if (!flash->spi.wr) {
		SFUD_INFO("%s SPI write read function is not initialized.");
		return -1;
	}

	/* reset flash device */
	result = snf_reset(flash);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_reset failed.", __func__);
		return result;
	}

	/* read spi nand flash ID include manufacturer ID, device ID */
	result = snf_read_id(flash);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_read_id failed.", __func__);
		return result;
	}

	for (i=0; i<sizeof(snf_chip_table)/sizeof(snf_chip_table[0]); i++) {
		if ((flash->chip.mf_id == snf_chip_table[i].mf_id) && (flash->chip.type_id == snf_chip_table[i].dev_id0)) {
			/* some spi nand flash have no dev_id1, so do not judge when snf_chip_table[i].dev_id1=0 */
			if ((snf_chip_table[i].dev_id1 != 0) && (flash->chip.capacity_id != snf_chip_table[i].dev_id1)) {
				continue;
			}
			SNF_SET_CHIP((void *)&snf_chip_table[i], flash);
			chip = SNF_GET_CHIP(flash);
			break;
		}
	}

	if (!chip) {
		SFUD_INFO("%s Not fond spi nand flash in %s.", __func__, flash->spi.name);
		SNF_SET_CHIP(NULL, flash);
		return SFUD_ERR_NOT_FOUND;
	}

	chip->byte_per_blk = chip->byte_per_page * chip->page_per_blk;
	chip->replace_blks = chip->total_blk / 100 * SNFUD_REPLACE_BLK_PERCENT;
	chip->available_blks = chip->total_blk - chip->replace_blks - SNFUD_BBT_BLK_COUNT;
	chip->capacity = chip->total_blk * chip->byte_per_blk;
	chip->available_capacity = chip->available_blks * chip->byte_per_blk;
	chip->bbt_blk_offset = chip->total_blk - SNFUD_BBT_BLK_COUNT;

	SNFUD_ASSERT(chip->byte_per_blk > chip->byte_per_page);
	SNFUD_ASSERT(chip->replace_blks < chip->total_blk);
	SNFUD_ASSERT(chip->available_blks < chip->total_blk);
	SNFUD_ASSERT(chip->available_capacity < chip->capacity);

	//printf("[SNF]:%s, Capacity:%dMB , AvailableCapacity:%dMB.\n",
	//		chip->name, chip->capacity/1024/1024, chip->available_capacity/1024/1024);

	flash->chip.capacity = chip->capacity;
	flash->chip.erase_gran = chip->byte_per_blk;
	//flash->chip.name = chip->name;
	flash->name = chip->name;
	flash->addr_in_4_byte = false;
	chip->flash = flash;

	if (chip->privInfoIndex) {
		uint32_t index = chip->privInfoIndex;
		if (index >= sizeof(SNF_PrivInfo) / sizeof(SNF_PrivInfo[0])) {
			printf("###ERR: %s: Invalid privInfo index = %d.\n", __func__, index);
			chip->privInfo = NULL;
			return -SFUD_ERR_NOT_FOUND;
		}
		chip->privInfo = (snf_priv_info *)&SNF_PrivInfo[index];

		//plane select check.
		if (chip->privInfo->planeSelIndex) {
			if (chip->privInfo->planeSelIndex >= sizeof(SNF_PsArray)/sizeof(SNF_PsArray[0])) {
				printf("###ERR: %s: Invalid PlaneSel index = %d\n", __func__, chip->privInfo->planeSelIndex);
				//chip->privInfo->planeSelIndex = 0;
				return -SFUD_ERR_NOT_FOUND;
			}
		}
	} else {
		chip->privInfo = NULL;
	}

	if (chip->total_blk > SNFUD_MAX_BLOCK_COUNT) {
		printf("###ERR: %s: SNFUD_MAX_BLOCK_COUNT(%d) is small than flash total block count(%d).\n",
			__func__, SNFUD_MAX_BLOCK_COUNT, chip->total_blk);
		return -SFUD_ERR_NOT_FOUND;
	}

	/* Partition check */
	SNFUD_ASSERT(chip->byte_per_blk);
	if (STEPLDRA_OFFSET % chip->byte_per_blk || STEPLDRB_OFFSET % chip->byte_per_blk) {
		printf("###ERR: %s: STEPLDR OFFSET not align with block size(0x%x).\n", __func__, chip->byte_per_blk);
		return -SFUD_ERR_NOT_FOUND;
	}

	if (SYSINFOA_MEDIA_OFFSET % chip->byte_per_blk || SYSINFOB_MEDIA_OFFSET % chip->byte_per_blk) {
		printf("###ERR: %s: SYSINFOA MEDIA OFFSET not align with block size(0x%x).\n", __func__, chip->byte_per_blk);
		return -SFUD_ERR_NOT_FOUND;
	}

	if (UPDATEFILE_MEDIA_OFFSET % chip->byte_per_blk || UPDATEFILE_MEDIA_B_OFFSET % chip->byte_per_blk) {
		printf("###ERR: %s: UPDATEFILE MEDIA OFFSET not align with block size(0x%x).\n", __func__, chip->byte_per_blk);
		return -SFUD_ERR_NOT_FOUND;
	}

	if (OTA_MEDIA_OFFSET % chip->byte_per_blk) {
		printf("###ERR: %s: OTA_MEDIA_OFFSET not align with block size(0x%x).\n", __func__, chip->byte_per_blk);
		return -SFUD_ERR_NOT_FOUND;
	}

	if (!flash->chip.capacity) {
		SFUD_INFO("Warning: This flash device is not found or not support.");
		return SFUD_ERR_NOT_FOUND;
	}

	SFUD_INFO("[SNF]:%s fond, Total capacity:%dMB (%d Bytes), available:%dMB.", chip->name, flash->chip.capacity/1024/1024, flash->chip.capacity, chip->available_capacity/1024/1024);

	result = snf_all_block_unlock(flash);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_all_block_unlock failed.", __func__);
	}

	result = snf_internal_4bit_ecc_enable(flash);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_internal_4bit_ecc_enable failed.", __func__);
	}

	result = snf_write_enabled(flash, true);
	if (result != SFUD_SUCCESS) {
		SFUD_INFO("%s snf_write_enabled failed.", __func__);
	}

#ifdef SNFUD_USING_QSPI
	snf_qspi_fast_read_enable(flash, 4);
#endif /* SNFUD_USING_QSPI */

	return SFUD_SUCCESS;
}

/**
 * software initialize
 *
 * @param flash flash device
 *
 * @return result
 */
static sfud_err snf_software_init(const sfud_flash *flash)
{
	sfud_err result = SFUD_SUCCESS;
	snfud_flash_chip *chip;

	SNFUD_ASSERT(flash);
	chip = (snfud_flash_chip *)SNF_GET_CHIP(flash);
	SNFUD_ASSERT(chip);

#ifdef SNFUD_ERASE_WRITE_DYNAMIC_MANAGE
	chip->cache_info = (snf_cache_info_t *)pvPortMalloc(sizeof(snf_cache_info_t));
	if (!chip->cache_info) {
		SFUD_INFO("%s pvPortMalloc cache_info failed.", __func__);
		return SFUD_ERR_NOT_FOUND;
	}
	memset(chip->cache_info, 0, sizeof(snf_cache_info_t));

	uint8_t *cache_buf  = (uint8_t *)pvPortMalloc(chip->byte_per_blk * SNFUD_CACHE_BLOCK_COUNT);
	if (!cache_buf) {
		SFUD_INFO("%s pvPortMalloc cache_buf failed.", __func__);
		return SFUD_ERR_NOT_FOUND;
	}
	memset(cache_buf, 0xff, chip->byte_per_blk * SNFUD_CACHE_BLOCK_COUNT);

	for (int i = 0; i < SNFUD_CACHE_BLOCK_COUNT; i++) {
		chip->cache_info->cache_blks[i].buf = cache_buf + i * chip->byte_per_blk;
		chip->cache_info->cache_blks[i].need_flush = 0;
		chip->cache_info->cache_blks[i].blk_id = -1;
	}

	if (xTaskCreate(snf_cache_flush_thread, "snf_cache_flush_thread", configMINIMAL_STACK_SIZE*4, (void *)flash,
		configMAX_PRIORITIES / 2, NULL) != pdPASS) {
		printf("create snf flush task fail.\n");
		return SFUD_ERR_NOT_FOUND;
	}
#endif

	snf_bbt_init(chip);

	return result;
}

/**
 * get flash device by its index which in the flash information table
 *
 * @param index the index which in the flash information table  @see flash_table
 *
 * @return flash device
 */
sfud_flash *sfud_get_device(size_t index) {
	if (index < sfud_get_device_num()) {
		return &flash_table[index];
	} else {
		return NULL;
	}
}

/**
 * get flash device total number on flash device information table  @see flash_table
 *
 * @return flash device total number
 */
size_t sfud_get_device_num(void) {
	return sizeof(flash_table) / sizeof(sfud_flash);
}

/**
 * get flash device information table  @see flash_table
 *
 * @return flash device table pointer
 */
const sfud_flash *sfud_get_device_table(void) {
	return flash_table;
}

/**
 * SFUD initialize by flash device
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_device_init(sfud_flash *flash) {
	sfud_err result = SFUD_SUCCESS;

	/* hardware initialize */
	result = snf_hardware_init(flash);
	if (result == SFUD_SUCCESS) {
		result = snf_software_init(flash);
	}
	if (result == SFUD_SUCCESS) {
		flash->init_ok = true;
		SFUD_INFO("%s flash device is initialize success.", flash->spi.name);
	} else {
		flash->init_ok = false;
		SFUD_INFO("Error: %s flash device is initialize fail.", flash->spi.name);
	}

	return result;
}

/**
 * SFUD library initialize.
 *
 * @return result
 */
sfud_err sfud_init(void) {
	sfud_err cur_flash_result = SFUD_SUCCESS, all_flash_result = SFUD_SUCCESS;
	size_t i;

	SFUD_DEBUG("Start initialize Serial Flash Universal Driver(SFUD) V%s.", SFUD_SW_VERSION);
	SFUD_DEBUG("You can get the latest version on https://github.com/armink/SFUD .");
	/* initialize all flash device in flash device table */
	for (i = 0; i < sizeof(flash_table) / sizeof(sfud_flash); i++) {
		/* initialize flash device index of flash device information table */
		flash_table[i].index = i;
		cur_flash_result = sfud_device_init(&flash_table[i]);

		if (cur_flash_result != SFUD_SUCCESS) {
			all_flash_result = cur_flash_result;
		}
	}

	return all_flash_result;
}
#endif

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
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

#include <sfud.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "spi.h"
#include "errno.h"
#include "os_adapt.h"
#include "timer.h"


#define SFUD_SPI_MAX_HZ 35000000
#define SFUD_QSPI_MAX_HZ SFUD_SPI_MAX_HZ

/* read the JEDEC SFDP command must run at 50 MHz or less */
#define SFUD_DEFAULT_SPI_CFG					\
{												\
	.mode = SPI_MODE_0,							\
	.data_width = 8,							\
	.max_hz = SFUD_SPI_MAX_HZ,					\
	.qspi_max_hz = SFUD_QSPI_MAX_HZ,			\
}

static char log_buf[256];

void sfud_log_debug(const char *file, const long line, const char *format, ...);
#ifdef SFUD_USING_QSPI
sfud_err enter_qspi_mode(const sfud_spi *spi);
sfud_err exit_qspi_mode(const sfud_spi *spi);
#endif


/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
		size_t read_size) {
	sfud_err result = SFUD_SUCCESS;
	struct spi_slave *slave;

	configASSERT(spi);

	if (write_size) {
		configASSERT(write_buf);
	}
	if (read_size) {
		configASSERT(read_buf);
	}

	slave = spi->user_data;
	configASSERT(slave);

	if (write_size && read_size) {
		if (spi_send_then_recv(slave, write_buf, write_size, read_buf, read_size) != ENOERR) {
			result = SFUD_ERR_TIMEOUT;
		}
	} else if (write_size) {
		if (spi_send(slave, write_buf, write_size) < 0) {
			result = SFUD_ERR_TIMEOUT;
		}
	} else {
		if (spi_recv(slave, read_buf, read_size) < 0) {
			result = SFUD_ERR_TIMEOUT;
		}
	}

	return result;
}

#ifdef SFUD_USING_QSPI
/**
 * read flash data by QSPI
 */
static sfud_err qspi_read(const sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format,
		uint8_t *read_buf, size_t read_size) {
	sfud_err result = SFUD_SUCCESS;
	struct spi_slave *slave;
	struct qspi_message qspi_message = {0};
	uint32_t align_size, left_size;
	uint32_t retries = 0;

	configASSERT(spi);
	configASSERT(qspi_read_cmd_format);
	configASSERT(read_buf);

	slave = spi->user_data;
	configASSERT(slave);

	configASSERT(slave != NULL);

    xSemaphoreTake(slave->xMutex, portMAX_DELAY);

retry:
	align_size = read_size & ~(ARCH_DMA_MINALIGN - 1);
	left_size = read_size & (ARCH_DMA_MINALIGN - 1);
	if (align_size) {
		enter_qspi_mode(spi);

		/* initial message */
		qspi_message.message.recv_buf = read_buf;
		qspi_message.message.length = align_size;
		qspi_message.message.cs_take	= 1;
		qspi_message.message.cs_release = 1;
		qspi_message.instruction.content = qspi_read_cmd_format->instruction;
		qspi_message.instruction.qspi_lines = qspi_read_cmd_format->instruction_lines;
		qspi_message.address.content = addr;
		qspi_message.address.size = qspi_read_cmd_format->address_size;
		qspi_message.address.qspi_lines = qspi_read_cmd_format->address_lines;
		qspi_message.dummy_cycles = qspi_read_cmd_format->dummy_cycles;
		qspi_message.qspi_data_lines = qspi_read_cmd_format->data_lines;

		/* transfer message */
		result = slave->qspi_read(slave, &qspi_message);

		exit_qspi_mode(spi);
		if (result) {
			if (retries++ < 3) {
				goto retry;
			}
			result = SFUD_ERR_READ;
			printf("%s(), read retry failed.\n", __func__);
			goto __exit;
		}
	}

	if (left_size) {
		uint8_t cmd_data[5], cmd_size;
		int i;

		cmd_data[0] = SFUD_CMD_READ_DATA;
		cmd_size = spi->flash->addr_in_4_byte ? 5 : 4;
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
		addr += (align_size << 8);	//2byte addr + 1dummy byte.
#else //#elif DEVICE_TYPE_SELECT == SPI_NOR_FLASH
		addr += align_size;
#endif
		for (i = 1; i < cmd_size; i++)
			cmd_data[i] = (addr >> ((cmd_size - (i + 1)) * 8)) & 0xFF;

		result = spi->wr(spi, cmd_data, cmd_size, read_buf + align_size, left_size);
	}

__exit:
    xSemaphoreGive(slave->xMutex);

	return result;
}
#endif /* SFUD_USING_QSPI */

static void spi_lock(const sfud_spi *spi)
{
	struct spi_slave *slave;

	configASSERT(spi);

	slave = spi->user_data;
	configASSERT(slave);

	xSemaphoreTake(slave->xSfudMutex, portMAX_DELAY);
}

static void spi_unlock(const sfud_spi *spi)
{
	struct spi_slave *slave;

	configASSERT(spi);

	slave = spi->user_data;
	configASSERT(slave);

	xSemaphoreGive(slave->xSfudMutex);
}

static void retry_delay_100us(void) {
	/* 100 microsecond delay */
	//vTaskDelay((configTICK_RATE_HZ * 1 + 9999) / 10000);
	int time = get_timer(0);
	while (get_timer(0) - time < 100) {
         taskYIELD();
	}
}

sfud_err sfud_spi_port_init(sfud_flash *flash) {
	sfud_err result = SFUD_SUCCESS;
	struct spi_slave *slave;
	struct spi_configuration config = SFUD_DEFAULT_SPI_CFG;

	/**
	 * add your port spi bus and device object initialize code like this:
	 * 1. rcc initialize
	 * 2. gpio initialize
	 * 3. spi device initialize
	 * 4. flash->spi and flash->retry item initialize
	 *	flash->spi.wr = spi_write_read; //Required
	 *	flash->spi.qspi_read = qspi_read; //Required when QSPI mode enable
	 *	flash->spi.lock = spi_lock;
	 *	flash->spi.unlock = spi_unlock;
	 *	flash->spi.user_data = &spix;
	 *	flash->retry.delay = null;
	 *	flash->retry.times = 10000; //Required
	 */

	slave = spi_open(flash->spi.name);
	if (!slave) {
		printf("%s open fail.\n", flash->spi.name);
		return SFUD_ERR_NOT_FOUND;
	}
	slave->xSfudMutex = xSemaphoreCreateMutex();
	spi_configure(slave, &config);

	flash->spi.wr = spi_write_read;
#ifdef 	SFUD_USING_QSPI
	flash->spi.qspi_read = qspi_read;
#endif
	flash->spi.lock = spi_lock;
	flash->spi.unlock = spi_unlock;
	flash->spi.user_data = slave;
	/* 100 microsecond delay */
	flash->retry.delay = retry_delay_100us;
	/* 60 seconds timeout */
	flash->retry.times = 60 * 10000;
	flash->spi.flash = flash;

	return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...) {
	va_list args;

	/* args point to the first variable parameter */
	va_start(args, format);
	printf("[SFUD](%s:%ld) ", file, line);
	/* must use vprintf to print */
	vsnprintf(log_buf, sizeof(log_buf), format, args);
	printf("%s\n", log_buf);
	va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char *format, ...) {
	va_list args;

	/* args point to the first variable parameter */
	va_start(args, format);
	printf("[SFUD]");
	/* must use vprintf to print */
	vsnprintf(log_buf, sizeof(log_buf), format, args);
	printf("%s\n", log_buf);
	va_end(args);
}

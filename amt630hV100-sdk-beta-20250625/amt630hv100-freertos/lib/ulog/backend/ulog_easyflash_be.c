/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2014-2018, Armink, <armink.ztl@gmail.com>
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
 * The ulog backend implementation for EasyFlash.
 * Created on: 2018-10-22
 */

#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <easyflash.h>
#include "board.h"

#define LOG_TAG              "easyflash"
#include <ulog.h>

#define ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

#ifdef ULOG_EASYFLASH_BACKEND_ENABLE

#if defined(ULOG_ASYNC_OUTPUT_BY_THREAD) && ULOG_ASYNC_OUTPUT_THREAD_STACK < 1024
#error "The thread stack size must more than 1024 when using async output by thread (ULOG_ASYNC_OUTPUT_BY_THREAD)"
#endif

static struct ulog_backend flash_backend;
static uint32_t log_saving_lvl = LOG_FILTER_LVL_ALL;

/**
 * Read and output log to console.
 *
 * @param index index for saved log.
 *        Minimum index is 0.
 *        Maximum index is log used flash total size - 1.
 * @param size
 */
void read_flash_log(uint8_t *logbuf, size_t index, size_t size)
{
    /* 64 bytes buffer */
    uint32_t buf[512] = { 0 };
    size_t log_total_size = ef_log_get_used_size();
    size_t buf_size = sizeof(buf);
    size_t read_size = 0;

    /* word alignment for index and size */
    index = ALIGN_DOWN(index, 4);
    size = ALIGN_DOWN(size, 4);
    if (index + size > log_total_size)
    {
        printf("The output position and size is out of bound. The max size is %d.\n", log_total_size);
        return;
    }

    while (1)
    {
        if (read_size + buf_size < size)
        {
            ef_log_read(index + read_size, buf, buf_size);
			memcpy(logbuf + read_size, buf, buf_size);
            read_size += buf_size;
        }
        else
        {
            ef_log_read(index + read_size, buf, size - read_size);
			memcpy(logbuf + read_size, buf, size - read_size);
            break;
        }
    }
}

/**
 * Read and output recent log which saved in flash.
 *
 * @param size recent log size
 */
size_t read_recent_flash_log(void *buf, size_t size)
{
    size_t max_size = ef_log_get_used_size();

    if (size == 0)
    {
        return 0;
    }
    else if (size > max_size)
    {
		size = max_size;
    }

	read_flash_log(buf, max_size - size, size);
	return size;
}


/**
 * clean all log which in flash
 */
void ulog_ef_log_clean(void)
{
    EfErrCode clean_result = EF_NO_ERR;

    /* clean all log which in flash */
    clean_result = ef_log_clean();

    if (clean_result == EF_NO_ERR)
    {
        TRACE_INFO("All logs which in flash is clean OK.");
    }
    else
    {
        TRACE_ERROR("Clean logs which in flash has an error!");
    }
}

static void ulog_easyflash_backend_output(struct ulog_backend *backend, uint32_t level, const char *tag, int is_raw,
        const char *log, size_t len)
{
    /* write some '\r' for word alignment */
    char write_overage_c[4] = { '\r', '\r', '\r', '\r' };
    size_t write_size_temp = 0;
    EfErrCode result = EF_NO_ERR;

    /* saving level filter for flash log */
    if (level <= log_saving_lvl)
    {
        /* calculate the word alignment write size */
        write_size_temp = ALIGN_DOWN(len, 4);

        result = ef_log_write((uint32_t *) log, write_size_temp);
        /* write last word alignment data */
        if ((result == EF_NO_ERR) && (write_size_temp != len))
        {
            memcpy(write_overage_c, log + write_size_temp, len - write_size_temp);
            ef_log_write((uint32_t *) write_overage_c, sizeof(write_overage_c));
        }
    }
}

/**
 * Set flash log saving level. The log which level less than setting will stop saving to flash.
 *
 * @param level setting level
 */
void ulog_ef_log_lvl_set(uint32_t level)
{
    log_saving_lvl = level;
}

int ulog_ef_backend_init(void)
{
    flash_backend.output = ulog_easyflash_backend_output;

    ulog_backend_register(&flash_backend, "easyflash", pdTRUE);

    return 0;
}

#endif /* ULOG_EASYFLASH_BACKEND_ENABLE */

/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
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
 * Function: Portable interface for SFUD flash driver.
 * Created on: 2015-01-16
 */

#include <easyflash.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sfud.h>
#include <FreeRTOS.h>
#include "semphr.h"
#include "board.h"
#include "mmcsd_core.h"

#ifdef ULOG_EASYFLASH_BACKEND_ENABLE

/* default ENV set for user */
static const ef_env default_env_set[] = {
        {"iap_need_copy_app", "0"},
        {"iap_need_crc32_check", "0"},
        {"iap_copy_app_size", "0"},
        {"stop_in_bootloader", "0"},
};

#define CONSOLEBUF_SIZE	256
static char log_buf[CONSOLEBUF_SIZE];
static SemaphoreHandle_t env_cache_lock;

#if DEVICE_TYPE_SELECT	!= EMMC_FLASH
static const sfud_flash *flash;
#endif

/**
 * Flash port for hardware initialize.
 *
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size) {
    EfErrCode result = EF_NO_ERR;

    *default_env = default_env_set;
    *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

	env_cache_lock = xSemaphoreCreateMutex();
#if DEVICE_TYPE_SELECT	!= EMMC_FLASH	
    flash = sfud_get_device(0);
#endif

    return result;
}

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size) {
    EfErrCode result = EF_NO_ERR;

#if DEVICE_TYPE_SELECT	!= EMMC_FLASH
    sfud_read(flash, addr, size, (uint8_t *)buf);
#else
	if (emmc_read(addr, size, (uint8_t*)buf))
		result = EF_READ_ERR;
#endif

    return result;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, size_t size) {
    EfErrCode result = EF_NO_ERR;
#if DEVICE_TYPE_SELECT	!= EMMC_FLASH	
    sfud_err sfud_result = SFUD_SUCCESS;
#endif

    /* make sure the start address is a multiple of FLASH_ERASE_MIN_SIZE */
    EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

#if DEVICE_TYPE_SELECT	!= EMMC_FLASH
    sfud_result = sfud_erase(flash, addr, size);

    if(sfud_result != SFUD_SUCCESS) {
        result = EF_ERASE_ERR;
    }
#else
	if (emmc_write(addr, size, NULL))
		result = EF_ERASE_ERR;
#endif

    return result;
}
/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size) {
    EfErrCode result = EF_NO_ERR;

#if DEVICE_TYPE_SELECT	!= EMMC_FLASH	
    sfud_err sfud_result = SFUD_SUCCESS;

    sfud_result = sfud_write(flash, addr, size, (const uint8_t *)buf);

    if(sfud_result != SFUD_SUCCESS) {
        result = EF_WRITE_ERR;
    }
#else
	if (emmc_write(addr, size, (uint8_t*)buf))
		result = EF_WRITE_ERR;
#endif

    return result;
}

/**
 * lock the ENV ram cache
 */
void ef_port_env_lock(void) {
    xSemaphoreTake(env_cache_lock, portMAX_DELAY);
}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void) {
    xSemaphoreGive(env_cache_lock);
}

/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void ef_log_debug(const char *file, const long line, const char *format, ...) {

#ifdef PRINT_DEBUG

    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    ef_print("[Flash] (%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    ef_print("%s", log_buf);
    va_end(args);

#endif

}

/**
 * This function is print flash routine info.
 *
 * @param format output format
 * @param ... args
 */
void ef_log_info(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    ef_print("[Flash] ");
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    ef_print("%s", log_buf);
    va_end(args);
}
/**
 * This function is print flash non-package info.
 *
 * @param format output format
 * @param ... args
 */
void ef_print(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    printf("%s", log_buf);
    va_end(args);
}

#endif /* ULOG_EASYFLASH_BACKEND_ENABLE */
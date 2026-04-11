/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-07     ChenYong     first version
 */

#include <FreeRTOS.h>
#include <ff_stdio.h>
#include "board.h"
#include "ulog.h"
#include "ulog_file.h"

#define ULOG_FILE_BE_NAME    "file"

#ifndef ULOG_FILE_ROOT_PATH
#define ULOG_FILE_ROOT_PATH  "/logs"
#endif
#ifndef ULOG_FILE_NAME_BASE
#define ULOG_FILE_NAME_BASE  "ulog.log"
#endif

#ifndef ULOG_FILE_MAX_NUM
#define ULOG_FILE_MAX_NUM    5
#endif
#ifndef ULOG_FILE_MAX_SIZE
#define ULOG_FILE_MAX_SIZE   (1024 * 512)
#endif

#define ULOG_FILE_PATH_LEN   128

#ifdef ULOG_FILE_BACKEND_ENABLE

#if defined(ULOG_ASYNC_OUTPUT_THREAD_STACK) && (ULOG_ASYNC_OUTPUT_THREAD_STACK < 2048)
#error "The value of ULOG_ASYNC_OUTPUT_THREAD_STACK must be greater than 2048."
#endif

static struct ulog_backend ulog_file;
static char g_file_path[ULOG_FILE_PATH_LEN] = {0};
static FF_FILE *g_file_fd = NULL;

/* rotate the log file xxx.log.n-1 => xxx.log.n, and xxx.log => xxx.log.0 */
static int ulog_file_rotate(void)
{
#define SUFFIX_LEN          10
    /* mv xxx.log.n-1 => xxx.log.n, and xxx.log => xxx.log.0 */
    static char old_path[ULOG_FILE_PATH_LEN], new_path[ULOG_FILE_PATH_LEN];
    int index = 0, err = 0;
	FF_FILE *file_fd = NULL;
    size_t base_len = 0;
    int result = pdTRUE;

    memcpy(old_path, g_file_path, ULOG_FILE_PATH_LEN);
    memcpy(new_path, g_file_path, ULOG_FILE_PATH_LEN);
    base_len = strlen(ULOG_FILE_ROOT_PATH) + strlen(ULOG_FILE_NAME_BASE) + 1;

    if (g_file_fd)
    {
        ff_fclose(g_file_fd);
    }

    for (index = ULOG_FILE_MAX_NUM - 2; index >= 0; --index)
    {
        snprintf(old_path + base_len, SUFFIX_LEN, index ? ".%d" : "", index - 1);
        snprintf(new_path + base_len, SUFFIX_LEN, ".%d", index);
        /* remove the old file */
        if ((file_fd = ff_fopen(new_path, "rb")) != NULL)
        {
            ff_fclose(file_fd);
            ff_remove(new_path);
        }
        /* change the new log file to old file name */
        if ((file_fd = ff_fopen(old_path , "rb")) != NULL)
        {
            ff_fclose(file_fd);
            err = ff_rename(old_path, new_path, 1);
        }

        if (err < 0)
        {
            result = pdFALSE;
            goto __exit;
        }
    }

__exit:
    /* reopen the file */
    g_file_fd = ff_fopen(g_file_path, "a+");

    return result;
}

static void ulog_file_backend_output(struct ulog_backend *backend, uint32_t level,
            const char *tag, int is_raw, const char *log, size_t len)
{
    size_t file_size = 0;

    /* check log file directory  */
    if (ff_finddir(ULOG_FILE_ROOT_PATH) == pdFALSE)
    {
        ff_mkdir(ULOG_FILE_ROOT_PATH);
    }

    if (g_file_fd == NULL)
    {
        snprintf(g_file_path, ULOG_FILE_PATH_LEN, "%s/%s", ULOG_FILE_ROOT_PATH, ULOG_FILE_NAME_BASE);
        g_file_fd = ff_fopen(g_file_path, "a+");
        if (g_file_fd == NULL)
        {
            printf("ulog file(%s) open failed.", g_file_path);
            return;
        }
    }

    ff_fseek(g_file_fd, 0, FF_SEEK_END);
	file_size = ff_filelength(g_file_fd);
    if (file_size > ULOG_FILE_MAX_SIZE)
    {
        if (!ulog_file_rotate())
        {
            return;
        }
    }

    ff_fwrite(log, 1, len, g_file_fd);
    /* flush file cache */
    ff_fclose(g_file_fd);
	g_file_fd = ff_fopen(g_file_path, "a+");
}

/* initialize the ulog file backend */
int ulog_file_backend_init(void)
{
    ulog_file.output = ulog_file_backend_output;
    ulog_backend_register(&ulog_file, ULOG_FILE_BE_NAME, pdFALSE);
    return 0;
}

/* uninitialize the ulog file backend */
int ulog_file_backend_deinit(void)
{
    if (g_file_fd)
    {
        ff_fclose(g_file_fd);
        g_file_fd = NULL;
    }
    ulog_backend_unregister(&ulog_file);
    return 0;
}

#endif /* ULOG_FILE_BACKEND_ENABLE */

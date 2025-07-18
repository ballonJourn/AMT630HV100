#include <stdio.h>
#include <FreeRTOS.h>
#include "board.h"
#include "ulog.h"

#ifdef ULOG_BACKEND_USING_CONSOLE

#if defined(ULOG_ASYNC_OUTPUT_BY_THREAD) && ULOG_ASYNC_OUTPUT_THREAD_STACK < 384
#error "The thread stack size must more than 384 when using async output by thread (ULOG_ASYNC_OUTPUT_BY_THREAD)"
#endif

static struct ulog_backend console;

void ulog_console_backend_output(struct ulog_backend *backend, uint32_t level, const char *tag, int is_raw,
        const char *log, size_t len)
{
	int i;

	for (i = 0; i < len; i++)
    	putchar(log[i]);
}

int ulog_console_backend_init(void)
{
    console.output = ulog_console_backend_output;

    ulog_backend_register(&console, "console", pdFALSE);

    return 0;
}

#endif /* ULOG_BACKEND_USING_CONSOLE */

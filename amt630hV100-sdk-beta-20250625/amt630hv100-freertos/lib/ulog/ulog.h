#ifndef _ULOG_H_
#define _ULOG_H_

#include <stdarg.h>
#include <FreeRTOS.h>
#include "list.h"
#include "board.h"
#include "ulog_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ULOG_VERSION_STR               "0.1.1"

/*
 * ulog init and deint
 */
int ulog_init(void);
void ulog_deinit(void);

/*
 * output different level log by LOG_X API
 *
 * NOTE: The `LOG_TAG` and `LOG_LVL` must be defined before including the <ulog.h> when you want to use LOG_X API.
 *
 * #define LOG_TAG              "example"
 * #define LOG_LVL              LOG_LVL_DBG
 * #include <ulog.h>
 *
 * Then you can using LOG_X API to output log
 *
 * TRACE_DEBUG("this is a debug log!");
 * TRACE_ERROR("this is a error log!");
 */
#define TRACE_FATAL(...)               ulog_f(LOG_TAG, __VA_ARGS__)
#define TRACE_ERROR(...)               ulog_e(LOG_TAG, __VA_ARGS__)
#define TRACE_WARNING(...)             ulog_w(LOG_TAG, __VA_ARGS__)
#define TRACE_INFO(...)                ulog_i(LOG_TAG, __VA_ARGS__)
#define TRACE_DEBUG(...)               ulog_d(LOG_TAG, __VA_ARGS__)
#define LOG_RAW(...)                   ulog_raw(__VA_ARGS__)
#define LOG_HEX(name, width, buf, size)      ulog_hex(name, width, buf, size)

/*
 * backend register and unregister
 */
int ulog_backend_register(ulog_backend_t backend, const char *name, int support_color);
int ulog_backend_unregister(ulog_backend_t backend);

#ifdef ULOG_USING_FILTER
/*
 * log filter setting
 */
int ulog_tag_lvl_filter_set(const char *tag, uint32_t level);
uint32_t ulog_tag_lvl_filter_get(const char *tag);
List_t *ulog_tag_lvl_list_get(void);
void ulog_global_filter_lvl_set(uint32_t level);
uint32_t ulog_global_filter_lvl_get(void);
void ulog_global_filter_tag_set(const char *tag);
const char *ulog_global_filter_tag_get(void);
void ulog_global_filter_kw_set(const char *keyword);
const char *ulog_global_filter_kw_get(void);
#endif /* ULOG_USING_FILTER */

/*
 * flush all backends's log
 */
void ulog_flush(void);

#ifdef ULOG_USING_ASYNC_OUTPUT
/*
 * asynchronous output API
 */
void ulog_async_output(void);
void ulog_async_waiting_log(uint32_t time);
#endif

/*
 * dump the hex format data to log
 */
void ulog_hexdump(const char *tag, size_t width, uint8_t *buf, size_t size);

/*
 * Another log output API. This API is more difficult to use than LOG_X API.
 */
void ulog_voutput(uint32_t level, const char *tag, int newline, const char *format, va_list args);
void ulog_output(uint32_t level, const char *tag, int newline, const char *format, ...);
void ulog_raw(const char *format, ...);

void read_flash_log(uint8_t *logbuf, size_t index, size_t size);
size_t read_recent_flash_log(void *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _ULOG_H_ */

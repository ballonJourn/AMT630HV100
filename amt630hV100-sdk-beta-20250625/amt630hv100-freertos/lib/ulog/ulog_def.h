#ifndef _ULOG_DEF_H_
#define _ULOG_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

/* logger level, the number is compatible for syslog */
#define LOG_LVL_ASSERT                 0
#define LOG_LVL_FATAL                  1
#define LOG_LVL_ERROR                  3
#define LOG_LVL_WARNING                4
#define LOG_LVL_INFO                   6
#define LOG_LVL_DBG                    7

/* the output silent level and all level for filter setting */
#ifndef ULOG_USING_SYSLOG
#define LOG_FILTER_LVL_SILENT         0
#define LOG_FILTER_LVL_ALL            7
#else
#define LOG_FILTER_LVL_SILENT         1
#define LOG_FILTER_LVL_ALL            255
#endif /* ULOG_USING_SYSLOG */

/* compatible for trace */
#undef TRACE_DEBUG
#undef TRACE_INFO
#undef TRACE_WARNING
#undef TRACE_ERROR
#undef TRACE_FATAL
#undef TRACE_LEVEL_DEBUG
#undef TRACE_LEVEL_INFO
#undef TRACE_LEVEL_WARNING
#undef TRACE_LEVEL_ERROR
#undef TRACE_LEVEL_FATAL
#define TRACE_LEVEL_FATAL	           LOG_LVL_FATAL			
#define TRACE_LEVEL_ERROR              LOG_LVL_ERROR
#define TRACE_LEVEL_WARNING            LOG_LVL_WARNING
#define TRACE_LEVEL_INFO               LOG_LVL_INFO
#define TRACE_LEVEL_DEBUG              LOG_LVL_DBG
#define dbg_log(level, ...)                                \
    if ((level) <= LOG_LVL)                                \
    {                                                      \
        ulog_output(level, LOG_TAG, pdFALSE, __VA_ARGS__);\
    }

#if !defined(LOG_TAG)
    /* compatible for trace */
    #if defined(TRACE_TAG)
        #define LOG_TAG                TRACE_TAG
    #else
        #define LOG_TAG                "NO_TAG"
    #endif
#endif /* !defined(LOG_TAG) */

#if !defined(TRACE_LEVEL)
#define TRACE_LEVEL TRACE_LEVEL_INFO
#endif

/* setting static output log level */
#ifndef ULOG_OUTPUT_LVL
#define ULOG_OUTPUT_LVL                LOG_LVL_INFO
#endif

#if !defined(LOG_LVL)
    /* compatible for trace */
    #if defined(TRACE_LEVEL)
        #define LOG_LVL                TRACE_LEVEL
    #else
        #define LOG_LVL                LOG_LVL_DBG
    #endif
#endif /* !defined(LOG_LVL) */

#if (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG)
    #define ulog_d(TAG, ...)           ulog_output(LOG_LVL_DBG, TAG, pdTRUE, __VA_ARGS__)
#else
    #define ulog_d(TAG, ...)
#endif /* (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG) */

#if (LOG_LVL >= LOG_LVL_INFO) && (ULOG_OUTPUT_LVL >= LOG_LVL_INFO)
    #define ulog_i(TAG, ...)           ulog_output(LOG_LVL_INFO, TAG, pdTRUE, __VA_ARGS__)
#else
    #define ulog_i(TAG, ...)
#endif /* (LOG_LVL >= LOG_LVL_INFO) && (ULOG_OUTPUT_LVL >= LOG_LVL_INFO) */

#if (LOG_LVL >= LOG_LVL_WARNING) && (ULOG_OUTPUT_LVL >= LOG_LVL_WARNING)
    #define ulog_w(TAG, ...)           ulog_output(LOG_LVL_WARNING, TAG, pdTRUE, __VA_ARGS__)
#else
    #define ulog_w(TAG, ...)
#endif /* (LOG_LVL >= LOG_LVL_WARNING) && (ULOG_OUTPUT_LVL >= LOG_LVL_WARNING) */

#if (LOG_LVL >= LOG_LVL_ERROR) && (ULOG_OUTPUT_LVL >= LOG_LVL_ERROR)
    #define ulog_e(TAG, ...)           ulog_output(LOG_LVL_ERROR, TAG, pdTRUE, __VA_ARGS__)
#else
    #define ulog_e(TAG, ...)
#endif /* (LOG_LVL >= LOG_LVL_ERROR) && (ULOG_OUTPUT_LVL >= LOG_LVL_ERROR) */

#if (LOG_LVL >= LOG_LVL_FATAL) && (ULOG_OUTPUT_LVL >= LOG_LVL_FATAL)
    #define ulog_f(TAG, ...)           {ulog_output(LOG_LVL_FATAL, TAG, pdTRUE, __VA_ARGS__); while(1);}
#else
    #define ulog_f(TAG, ...)
#endif /* (LOG_LVL >= LOG_LVL_FATAL) && (ULOG_OUTPUT_LVL >= LOG_LVL_FATAL) */


#if (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG)
    #define ulog_hex(TAG, width, buf, size)     ulog_hexdump(TAG, width, buf, size)
#else
    #define ulog_hex(TAG, width, buf, size)
#endif /* (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG) */    
    
/* assert for developer. */
#ifdef ULOG_ASSERT_ENABLE
    #define ULOG_ASSERT(EXPR)                                                 \
    if (!(EXPR))                                                              \
    {                                                                         \
        ulog_output(LOG_LVL_ASSERT, LOG_TAG, pdTRUE, "(%s) has assert failed at %s:%ld.", #EXPR, __FUNCTION__, __LINE__); \
        ulog_flush();                                                         \
        while (1);                                                            \
    }
#else
    #define ULOG_ASSERT(EXPR)
#endif

/* ASSERT API definition */
#if !defined(ASSERT)
    #define ASSERT           ULOG_ASSERT
#endif

/* buffer size for every line's log */
#ifndef ULOG_LINE_BUF_SIZE
#define ULOG_LINE_BUF_SIZE             128
#endif

/* output filter's tag max length */
#ifndef ULOG_FILTER_TAG_MAX_LEN
#define ULOG_FILTER_TAG_MAX_LEN        23
#endif

/* output filter's keyword max length */
#ifndef ULOG_FILTER_KW_MAX_LEN
#define ULOG_FILTER_KW_MAX_LEN         15
#endif

#ifndef ULOG_NEWLINE_SIGN
#define ULOG_NEWLINE_SIGN              "\r\n"
#endif

#define ULOG_FRAME_MAGIC               0x10

/* tag's level filter */
struct ulog_tag_lvl_filter
{
    char tag[ULOG_FILTER_TAG_MAX_LEN + 1];
    uint32_t level;
    ListItem_t list;
};
typedef struct ulog_tag_lvl_filter *ulog_tag_lvl_filter_t;

struct ulog_frame
{
    /* magic word is 0x10 ('lo') */
    uint32_t magic:8;
    uint32_t is_raw:1;
    uint32_t log_len:23;
    uint32_t level;
    const char *log;
    const char *tag;
};
typedef struct ulog_frame *ulog_frame_t;

struct ulog_backend
{
    char name[ULOG_NAME_MAX];
    int support_color;
    void (*init)  (struct ulog_backend *backend);
    void (*output)(struct ulog_backend *backend, uint32_t level, const char *tag, int is_raw, const char *log, size_t len);
    void (*flush) (struct ulog_backend *backend);
    void (*deinit)(struct ulog_backend *backend);
    ListItem_t list;
};
typedef struct ulog_backend *ulog_backend_t;

#ifdef __cplusplus
}
#endif

#endif /* _ULOG_DEF_H_ */

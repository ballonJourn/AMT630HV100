/**
 * @file timer_compat.h
 * @brief AWTK timer_add / idle_queue → LVGL lv_task 兼容层
 *
 * 关键线程安全设计:
 *   - AWTK idle_queue() 是线程安全的 (从ISR/任意线程投递到GUI线程)
 *   - LVGL 7.x lv_async_call() 不保证线程安全
 *   - 本模块使用 FreeRTOS Queue 作为中间层:
 *     ISR/其他线程 → xQueueSendFromISR() → Queue → lv_task轮询 → GUI线程执行
 *
 * @date  2026-05-20
 * @note  M004 里程碑
 */

#ifndef __TIMER_COMPAT_H__
#define __TIMER_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/* 引入类型定义 (避免循环依赖, 重复定义) */
#ifndef __AWTK_TO_LVGL_H__
typedef int ret_t;
#define RET_OK      (0)
#define RET_FAIL    (-1)
#define RET_REPEAT  (1)
#define RET_REMOVE  (2)

typedef struct _timer_info_compat_t {
    void     *ctx;
    uint32_t  id;
} timer_info_t;

typedef ret_t (*timer_func_t)(const timer_info_t *info);

typedef struct _idle_info_compat_t {
    void *ctx;
} idle_info_t;

typedef ret_t (*idle_func_t)(const idle_info_t *idle);
#endif

/** 最大定时器数 (HCN 项目用3个: 50ms, 100ms, 500ms) */
#define TIMER_COMPAT_MAX  (16)

/** idle_queue 的 FreeRTOS Queue 深度 */
#define IDLE_QUEUE_DEPTH  (32)

/**
 * @brief 初始化定时器兼容层
 * @note  必须在 lv_init() 之后、GUI 线程启动前调用
 */
void timer_compat_init(void);

/**
 * @brief 添加定时器 (AWTK 兼容)
 * @param cb          回调函数
 * @param ctx         用户上下文
 * @param interval_ms 间隔 (毫秒)
 * @return 定时器 ID (>0), 0 表示失败
 */
uint32_t timer_add(timer_func_t cb, void *ctx, uint32_t interval_ms);

/**
 * @brief 移除定时器
 * @param id  timer_add 返回的 ID
 */
void timer_remove(uint32_t id);

/**
 * @brief 查找定时器是否存在
 * @param id  timer_add 返回的 ID
 * @return true=存在
 */
bool timer_find(uint32_t id);

/**
 * @brief 线程安全的异步投递 (AWTK idle_queue 兼容)
 * @param cb   回调函数 (将在 GUI 线程中执行)
 * @param ctx  用户上下文
 * @return RET_OK=成功投递
 *
 * @note 可从 ISR 或任意 FreeRTOS 线程调用
 */
ret_t idle_queue(idle_func_t cb, void *ctx);

/**
 * @brief GUI 线程中轮询 idle_queue
 * @note  此函数在 lv_task 中自动调用, 无需手动调用
 */
void timer_compat_poll_idle_queue(void);

#ifdef __cplusplus
}
#endif

#endif /* __TIMER_COMPAT_H__ */

/**
 * @file timer_compat.c
 * @brief AWTK timer/idle → LVGL lv_task 兼容层实现
 *
 * 架构:
 *
 *   [ISR / 其他线程]                 [GUI 线程]
 *        |                               |
 *   idle_queue(cb, ctx)                  |
 *        |                               |
 *   xQueueSendFromISR()                  |
 *        |                               |
 *        +------→ FreeRTOS Queue ------→ poll_idle_queue()
 *                                        |
 *                                    cb(idle_info)
 *                                        |
 *                                   (LVGL API 安全)
 *
 * 定时器:
 *   timer_add(cb, ctx, ms) → lv_task_create(wrapper, ms, prio, slot)
 *   wrapper 内构造 timer_info_t 并调用原始 cb
 *   若 cb 返回 RET_REMOVE → lv_task_del()
 *   若 cb 返回 RET_REPEAT → 继续
 *
 * @date  2026-05-20
 * @note  M004 里程碑
 */

#include "timer_compat.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <string.h>
#include <stdio.h>

/* ======================================================================
 * 定时器管理
 * ====================================================================== */

typedef struct {
    uint32_t       id;          /* 唯一 ID (从1开始递增) */
    timer_func_t   cb;          /* 用户回调 */
    void          *ctx;         /* 用户上下文 */
    lv_task_t     *lv_task;     /* LVGL task 指针 */
    bool           active;      /* 是否有效 */
} timer_slot_t;

static timer_slot_t s_timers[TIMER_COMPAT_MAX];
static uint32_t s_next_timer_id = 1;

/**
 * lv_task 回调 wrapper
 * LVGL 7.x lv_task 回调签名: void (*lv_task_cb_t)(lv_task_t *task)
 * user_data 指向 timer_slot_t 在数组中的索引
 */
static void timer_lv_task_cb(lv_task_t *task)
{
    int slot_idx = (int)(uintptr_t)task->user_data;
    if (slot_idx < 0 || slot_idx >= TIMER_COMPAT_MAX) return;

    timer_slot_t *slot = &s_timers[slot_idx];
    if (!slot->active || slot->cb == NULL) return;

    /* 构造 AWTK 兼容的 timer_info_t */
    timer_info_t info;
    info.ctx = slot->ctx;
    info.id  = slot->id;

    ret_t result = slot->cb(&info);

    if (result == RET_REMOVE) {
        /* 回调请求移除: 销毁 lv_task 并释放 slot */
        lv_task_del(slot->lv_task);
        slot->active  = false;
        slot->lv_task = NULL;
        slot->cb      = NULL;
        slot->ctx     = NULL;
    }
    /* RET_REPEAT 或 RET_OK: lv_task 自动重复, 无需处理 */
}

/* ======================================================================
 * idle_queue 异步投递
 * ====================================================================== */

typedef struct {
    idle_func_t  cb;
    void        *ctx;
} idle_queue_item_t;

static QueueHandle_t s_idle_queue = NULL;
static lv_task_t    *s_idle_poll_task = NULL;

/**
 * GUI 线程中的轮询 task
 * 每 5ms 检查一次 Queue, 处理所有待执行的 idle 回调
 */
static void idle_poll_lv_task_cb(lv_task_t *task)
{
    (void)task;
    timer_compat_poll_idle_queue();
}

/* ======================================================================
 * 公开 API
 * ====================================================================== */

void timer_compat_init(void)
{
    /* 清空定时器槽 */
    memset(s_timers, 0, sizeof(s_timers));
    s_next_timer_id = 1;

    /* 创建 idle_queue FreeRTOS Queue */
    s_idle_queue = xQueueCreate(IDLE_QUEUE_DEPTH, sizeof(idle_queue_item_t));
    if (s_idle_queue == NULL) {
        printf("[timer_compat] FATAL: cannot create idle queue\n");
        return;
    }

    /* 创建 GUI 线程中的轮询 lv_task (5ms 间隔, 高优先级) */
    s_idle_poll_task = lv_task_create(idle_poll_lv_task_cb, 5,
                                      LV_TASK_PRIO_HIGH, NULL);
    if (s_idle_poll_task == NULL) {
        printf("[timer_compat] FATAL: cannot create idle poll lv_task\n");
    }

    printf("[timer_compat] initialized (queue depth=%d)\n", IDLE_QUEUE_DEPTH);
}

uint32_t timer_add(timer_func_t cb, void *ctx, uint32_t interval_ms)
{
    if (cb == NULL) return 0;

    /* 找空闲 slot */
    int slot_idx = -1;
    for (int i = 0; i < TIMER_COMPAT_MAX; i++) {
        if (!s_timers[i].active) {
            slot_idx = i;
            break;
        }
    }

    if (slot_idx < 0) {
        printf("[timer_compat] ERROR: no free slot (max=%d)\n", TIMER_COMPAT_MAX);
        return 0;
    }

    /* 创建 lv_task */
    lv_task_t *lt = lv_task_create(timer_lv_task_cb, interval_ms,
                                    LV_TASK_PRIO_MID,
                                    (void *)(uintptr_t)slot_idx);
    if (lt == NULL) {
        printf("[timer_compat] ERROR: lv_task_create failed\n");
        return 0;
    }

    /* 填充 slot */
    uint32_t id = s_next_timer_id++;
    if (s_next_timer_id == 0) s_next_timer_id = 1; /* 防止溢出到0 */

    s_timers[slot_idx].id      = id;
    s_timers[slot_idx].cb      = cb;
    s_timers[slot_idx].ctx     = ctx;
    s_timers[slot_idx].lv_task = lt;
    s_timers[slot_idx].active  = true;

    return id;
}

void timer_remove(uint32_t id)
{
    if (id == 0) return;

    for (int i = 0; i < TIMER_COMPAT_MAX; i++) {
        if (s_timers[i].active && s_timers[i].id == id) {
            if (s_timers[i].lv_task != NULL) {
                lv_task_del(s_timers[i].lv_task);
            }
            s_timers[i].active  = false;
            s_timers[i].lv_task = NULL;
            s_timers[i].cb      = NULL;
            s_timers[i].ctx     = NULL;
            return;
        }
    }
}

bool timer_find(uint32_t id)
{
    if (id == 0) return false;

    for (int i = 0; i < TIMER_COMPAT_MAX; i++) {
        if (s_timers[i].active && s_timers[i].id == id) {
            return true;
        }
    }
    return false;
}

ret_t idle_queue(idle_func_t cb, void *ctx)
{
    if (cb == NULL || s_idle_queue == NULL) return RET_FAIL;

    idle_queue_item_t item;
    item.cb  = cb;
    item.ctx = ctx;

    /**
     * 检测是否在 ISR 上下文中
     * FreeRTOS: xPortIsInsideInterrupt() (ARM Cortex-M)
     * 或者用 __get_IPSR() > 0 判断
     */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* 尝试用 ISR 安全版本, 如果不在 ISR 中也能工作 */
    BaseType_t result = xQueueSendFromISR(s_idle_queue, &item,
                                           &xHigherPriorityTaskWoken);

    if (result != pdPASS) {
        /**
         * Queue 满: 这意味着 GUI 线程被阻塞太久无法轮询。
         * 在生产中这是严重问题, 但不能在 ISR 中 printf。
         * 先尝试非 ISR 版本。
         */
        result = xQueueSend(s_idle_queue, &item, 0);
        if (result != pdPASS) {
            return RET_FAIL;
        }
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return RET_OK;
}

void timer_compat_poll_idle_queue(void)
{
    if (s_idle_queue == NULL) return;

    idle_queue_item_t item;

    /* 一次轮询处理所有待执行项 (最多处理 IDLE_QUEUE_DEPTH 次防止死循环) */
    int count = 0;
    while (count < IDLE_QUEUE_DEPTH &&
           xQueueReceive(s_idle_queue, &item, 0) == pdPASS) {
        if (item.cb != NULL) {
            idle_info_t info;
            info.ctx = item.ctx;
            item.cb(&info);
        }
        count++;
    }
}

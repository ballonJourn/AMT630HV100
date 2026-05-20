/**
 * @file navigator.h
 * @brief 导航器接口 — LVGL 版本
 *
 * M023: 保持与 AWTK 版完全相同的函数签名,
 * 使下游 108 个文件无需改 include。
 *
 * 类型依赖: ret_t, bool_t, event_func_t, widget_t
 * 均通过 awtk_to_lvgl.h 定义。
 */

#ifndef APP_NAVIGATOR_H
#define APP_NAVIGATOR_H

#include "view/home_view/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 请求打开指定窗口 */
ret_t navigator_to(const char *target);

/** 打开窗口并传递上下文 */
ret_t navigator_to_with_context(const char *target, void *ctx);

/** 打开窗口并关闭当前窗口 */
ret_t navigator_replace(const char *target);

/** 切换到目标窗口 (已存在则直切, 不存在则创建) */
ret_t navigator_switch_to(const char *target, bool_t close_current);

/** 回到主屏 */
ret_t navigator_back_to_home(void);

/** 关闭当前窗口, 回到前一窗口 */
ret_t navigator_back(void);

/** 关闭指定窗口 */
ret_t navigator_close(const char *target);

/** 请求关闭指定窗口 */
ret_t navigator_request_close(const char *target);

/** 注册全局事件 */
ret_t navigator_global_widget_on(uint32_t type, event_func_t on_event, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* APP_NAVIGATOR_H */

/**
 * @file screen_manager.h
 * @brief LVGL 屏幕/页面管理器
 *
 * 替代 AWTK 的 window_manager + navigator 体系。
 * 提供屏幕栈、页面切换、按键分发等功能。
 *
 * HCN DC002 页面结构:
 *   home_page (主页, 默认)
 *     └─ link_page   (互联页, 可从主页进入)
 *     └─ device_page (设备页, 长按进入)
 *     └─ update_page (升级页, OTA触发)
 *
 * @date  2026-05-20
 * @note  M003 里程碑
 */

#ifndef __SCREEN_MANAGER_H__
#define __SCREEN_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include <stdbool.h>

/** 屏幕栈最大深度 */
#define SCREEN_STACK_MAX  (8)

/** 屏幕名最大长度 */
#define SCREEN_NAME_MAX   (32)

/**
 * 屏幕初始化回调类型
 * @param screen  新创建的 lv_obj_t* screen
 * @param ctx     用户上下文
 * @return 0=成功
 */
typedef int (*screen_init_func_t)(lv_obj_t *screen, void *ctx);

/**
 * @brief 注册一个屏幕的初始化函数
 * @param name    屏幕名 (如 "home_page")
 * @param init_fn 初始化回调
 */
void screen_mgr_register(const char *name, screen_init_func_t init_fn);

/**
 * @brief 初始化屏幕管理器
 */
void screen_mgr_init(void);

/**
 * @brief 导航到指定屏幕 (入栈)
 * @param name  屏幕名
 * @return 0=成功
 */
int screen_mgr_to(const char *name);

/**
 * @brief 导航到指定屏幕, 附带上下文
 */
int screen_mgr_to_with_ctx(const char *name, void *ctx);

/**
 * @brief 替换当前屏幕 (不入栈)
 * @param name  屏幕名
 * @return 0=成功
 */
int screen_mgr_replace(const char *name);

/**
 * @brief 切换到指定屏幕
 * @param name         屏幕名
 * @param close_current 是否关闭当前屏幕
 * @return 0=成功
 */
int screen_mgr_switch(const char *name, bool close_current);

/**
 * @brief 回退到上一个屏幕
 * @return 0=成功, -1=已在栈底
 */
int screen_mgr_back(void);

/**
 * @brief 回退到主页
 * @return 0=成功
 */
int screen_mgr_back_to_home(void);

/**
 * @brief 获取当前屏幕名
 * @return 屏幕名字符串, 或 NULL
 */
const char *screen_mgr_get_top_name(void);

/**
 * @brief 获取当前屏幕 lv_obj_t*
 * @return lv_obj_t* 或 NULL
 */
lv_obj_t *screen_mgr_get_top_screen(void);

/**
 * @brief 强制关闭指定屏幕
 * @param name  屏幕名
 */
void screen_mgr_close(const char *name);

/* ======================================================================
 * AWTK 兼容 wrapper (实现在 screen_manager.c)
 *
 * 这些函数保持与 navigator.c / view_manager.c 中原有调用签名一致
 * ====================================================================== */

/**
 * window_manager() → 返回一个占位对象
 * 在 AWTK 中, window_manager() 是全局单例。
 * 在 LVGL 中, 用 screen_mgr 内部管理, 此处返回 NULL 或 dummy。
 */
lv_obj_t *window_manager(void);

lv_obj_t *window_manager_get_top_window(lv_obj_t *wm);
lv_obj_t *window_manager_get_top_main_window(lv_obj_t *wm);
lv_obj_t *widget_child(lv_obj_t *wm, const char *name);
lv_obj_t *window_open_and_close(const char *name, lv_obj_t *to_close);
int       window_manager_close_window_force(lv_obj_t *wm, lv_obj_t *win);
int       window_manager_switch_to(lv_obj_t *wm, lv_obj_t *curr, lv_obj_t *target, bool close);
int       window_manager_back_to_home(lv_obj_t *wm);
int       window_manager_back(lv_obj_t *wm);
int       dialog_modal(lv_obj_t *win);

#ifdef __cplusplus
}
#endif

#endif /* __SCREEN_MANAGER_H__ */

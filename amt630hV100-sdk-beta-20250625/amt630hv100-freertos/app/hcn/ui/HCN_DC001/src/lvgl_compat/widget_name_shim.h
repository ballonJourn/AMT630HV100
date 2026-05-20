/**
 * @file widget_name_shim.h
 * @brief 解决 AWTK widget_t->name 字段访问问题
 *
 * 问题:
 *   AWTK widget_t 有 char* name 字段, 业务代码直接访问:
 *     widget_t* top_win_ = window_manager_get_top_window(wm);
 *     const char* top_win_name = top_win_->name;
 *
 *   LVGL lv_obj_t 没有 name 字段。
 *
 * 解决方案:
 *   在 view_manager.c 改写 HCN_KEY_DISPATCH 宏, 使用
 *   screen_mgr_get_top_name() 代替 top_win_->name。
 *
 *   这个头文件提供辅助宏, 使代码改动最小化:
 *   - HCN_GET_TOP_WIN_NAME() → screen_mgr_get_top_name()
 *   - HCN_KEY_DISPATCH 宏的 LVGL 版本
 *
 * @date  2026-05-20
 * @note  M003 补充
 */

#ifndef __WIDGET_NAME_SHIM_H__
#define __WIDGET_NAME_SHIM_H__

#include "screen_manager.h"
#include <string.h>

/**
 * 获取当前顶层窗口名称 (替代 top_win_->name)
 */
#define HCN_GET_TOP_WIN_NAME()  screen_mgr_get_top_name()

/**
 * LVGL 版 HCN_KEY_DISPATCH 宏
 *
 * 原始 AWTK 版本:
 *   widget_t* top_win_ = window_manager_get_top_window(window_manager());
 *   if (top_win_ == NULL) return;
 *   const char* top_win_name = top_win_->name;
 *   if (tk_str_eq(top_win_name, HOME_PAGE)) { ... }
 *
 * LVGL 版本使用 screen_mgr_get_top_name():
 */
#define HCN_KEY_DISPATCH_LVGL(keyType)  do { \
    const char* _top_name = screen_mgr_get_top_name(); \
    if (_top_name == NULL) return; \
    if (strcmp(_top_name, HOME_PAGE) == 0) { \
        if ((current_level) == (MENU_LEVEL_0)) { \
            home_page_deal_key_##keyType(); \
        } else if (((current_level) == (MENU_LEVEL_1)) && ((current_dock) == (ICON_MUSIC))) { \
            music_page_deal_key_##keyType(); \
        } else { \
            set_page_deal_key_##keyType(); \
        } \
    } else if (strcmp(_top_name, LINK_PAGE) == 0) { \
        link_page_deal_key_##keyType(); \
    } else if (strcmp(_top_name, DEVICE_PAGE) == 0) { \
        device_page_deal_key_##keyType(); \
    } \
} while(0)

#endif /* __WIDGET_NAME_SHIM_H__ */

/**
 * @file navigator.c
 * @brief 导航器实现 — LVGL 版本 (screen_manager 的 thin wrapper)
 *
 * M023: 所有函数委托给 screen_manager API
 * 保持接口签名与 AWTK 版完全一致
 */

#include "navigator.h"
#include "lvgl_compat/screen_manager.h"
#include <string.h>
#include <stdio.h>

ret_t navigator_to(const char *target)
{
    return navigator_to_with_context(target, NULL);
}

ret_t navigator_to_with_context(const char *target, void *ctx)
{
    if (target == NULL || *target == '\0') return RET_BAD_PARAMS;

    int ret = screen_mgr_to_with_ctx(target, ctx);
    return (ret == 0) ? RET_OK : RET_FAIL;
}

ret_t navigator_replace(const char *target)
{
    if (target == NULL || *target == '\0') return RET_BAD_PARAMS;

    int ret = screen_mgr_replace(target);
    return (ret == 0) ? RET_OK : RET_FAIL;
}

ret_t navigator_switch_to(const char *target, bool_t close_current)
{
    if (target == NULL || *target == '\0') return RET_BAD_PARAMS;

    int ret = screen_mgr_switch(target, close_current);
    return (ret == 0) ? RET_OK : RET_FAIL;
}

ret_t navigator_back_to_home(void)
{
    int ret = screen_mgr_back_to_home();
    return (ret == 0) ? RET_OK : RET_FAIL;
}

ret_t navigator_back(void)
{
    int ret = screen_mgr_back();
    return (ret == 0) ? RET_OK : RET_FAIL;
}

ret_t navigator_close(const char *target)
{
    if (target == NULL || *target == '\0') return RET_BAD_PARAMS;

    screen_mgr_close(target);
    return RET_OK;
}

ret_t navigator_request_close(const char *target)
{
    /* HCN 中无模态对话框, 直接关闭 */
    return navigator_close(target);
}

ret_t navigator_global_widget_on(uint32_t type, event_func_t on_event, void *ctx)
{
    /**
     * AWTK: widget_on(window_manager(), type, on_event, ctx)
     * LVGL: 使用 lv_group 的事件回调
     *
     * view_manager.c 中调用此函数注册 EVT_KEY_DOWN / EVT_KEY_LONG_PRESS
     * 在 M029 重写 view_manager 时, 这些会改为 lv_group event callback
     * 此处暂存回调引用, 供 view_manager LVGL 版使用
     */
    (void)type;
    (void)on_event;
    (void)ctx;
    printf("[navigator] global_widget_on type=0x%x (deferred to M029)\n", type);
    return RET_OK;
}

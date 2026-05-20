/**
 * @file application.c
 * @brief HCN 应用初始化入口
 *
 * M021: 从 AWTK 迁移为 LVGL
 * - 移除 awtk.h, qr_register, chart_view_register
 * - 移除 custom_widgets_register() (AWTK 控件注册)
 * - navigator_to → screen_mgr_to (通过 navigator.c wrapper)
 */

#include "view/home_view/common.h"
#include "common/navigator.h"
#include "lvgl_compat/screen_manager.h"

#ifndef APP_START_PAGE
#define APP_START_PAGE "home_page"
#endif

/**
 * Forward declarations of screen init functions
 * Each page registers via screen_mgr_register() then navigator_to() loads it.
 * Implementations will be in ui_build/ui_*.c (M024-M027)
 */
extern int ui_home_page_init(lv_obj_t *screen, void *ctx);
extern int ui_link_page_init(lv_obj_t *screen, void *ctx);
extern int ui_device_page_init(lv_obj_t *screen, void *ctx);
extern int ui_update_page_init(lv_obj_t *screen, void *ctx);

/**
 * 注册所有页面的 init 回调
 */
static void register_all_screens(void)
{
    screen_mgr_register("home_page",   ui_home_page_init);
    screen_mgr_register("link_page",   ui_link_page_init);
    screen_mgr_register("device_page", ui_device_page_init);
    screen_mgr_register("update_page", ui_update_page_init);
}

/**
 * 初始化程序 — 由 main_hcn_lvgl.c 调用
 */
int application_init(void)
{
    register_all_screens();

    /* 导航到起始页 */
    navigator_to(APP_START_PAGE);

    return 0;
}

/**
 * 退出程序
 */
int application_exit(void)
{
    printf("application_exit\n");
    return 0;
}

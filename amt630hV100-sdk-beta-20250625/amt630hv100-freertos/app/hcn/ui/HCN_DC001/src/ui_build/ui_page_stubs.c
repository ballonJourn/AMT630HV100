/**
 * @file ui_page_stubs.c
 * @brief 页面 screen init 桩实现
 *
 * M024-M027: 每个页面的 init 回调。
 * 当前为桩代码 — 仅创建基本容器和占位 label。
 * 后续里程碑将 XML 布局逐页转写为完整 LVGL 控件树。
 *
 * 各页面的完整 UI 构建将在这些文件中展开:
 *   ui_home_page.c   (M024)  — 仪表盘主页
 *   ui_link_page.c   (M025)  — 手机互联页
 *   ui_device_page.c (M026)  — 设备信息页
 *   ui_update_page.c (M027)  — OTA升级页
 *
 * @date  2026-05-20
 */

#include "view/home_view/common.h"
#include "lvgl_compat/widget_registry.h"
#include "lvgl_compat/screen_manager.h"

/* Forward declarations of AWTK-era page init functions (in pages/*.c) */
/* These will be called from the screen init callbacks below */
extern ret_t home_page_init(widget_t *win, void *ctx);
extern ret_t link_page_init(widget_t *win, void *ctx);
extern ret_t device_page_init(widget_t *win, void *ctx);
extern ret_t update_page_init(widget_t *win, void *ctx);

/**
 * Home page — implemented in ui_home_page.c (M024)
 * extern int ui_home_page_init(lv_obj_t *screen, void *ctx);
 */

/**
 * Link (phone mirror) page screen init
 */
int ui_link_page_init(lv_obj_t *screen, void *ctx)
{
    lv_obj_t *label = lv_label_create(screen, NULL);
    lv_label_set_text(label, "Phone Link Page (M025 stub)");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

    widget_reg_add("link_stub_label", label);

    /* TODO M025: Build widget tree from link_page.xml, then call:
     * link_page_init(screen, ctx);
     */
    (void)ctx;
    printf("[ui_build] link_page stub initialized\n");
    return 0;
}

/**
 * Device info page screen init
 */
int ui_device_page_init(lv_obj_t *screen, void *ctx)
{
    lv_obj_t *label = lv_label_create(screen, NULL);
    lv_label_set_text(label, "Device Info Page (M026 stub)");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

    widget_reg_add("device_stub_label", label);

    (void)ctx;
    printf("[ui_build] device_page stub initialized\n");
    return 0;
}

/**
 * OTA Update page screen init
 */
int ui_update_page_init(lv_obj_t *screen, void *ctx)
{
    lv_obj_t *label = lv_label_create(screen, NULL);
    lv_label_set_text(label, "OTA Update Page (M027 stub)");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

    widget_reg_add("update_stub_label", label);

    (void)ctx;
    printf("[ui_build] update_page stub initialized\n");
    return 0;
}

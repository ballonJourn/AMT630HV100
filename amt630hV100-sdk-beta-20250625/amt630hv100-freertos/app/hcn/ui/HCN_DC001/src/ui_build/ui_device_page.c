/**
 * @file ui_device_page.c
 * @brief device_page.xml → LVGL C 代码转写
 *
 * M026: 将 device_page.xml 的控件树用 LVGL API 构建
 * 所有命名控件注册到 widget_registry, 供 device_view.c / device_logic.c 使用
 *
 * 控件名来源 (device_view.c::device_widget_name[]):
 *   "uuid_status", "uuid", "bluetooth", "bluetooth_ver",
 *   "ota", "sn", "version", "carBit",
 *   "ota_state", "progress_circle", "lab_state", "lab_error"
 *
 * XML 结构:
 *   <window name="device_page" bg_color=#000000>
 *     <view name="view" x=13 y=12 w=998 h=575>
 *       标题 "OTA" + 8行标签+值对 + ota_state
 *     </view>
 *     <view name="view1" x=351 y=165 w=324 h=260>
 *       进度环 + 状态/错误标签 (OTA升级时覆盖显示)
 *     </view>
 *   </window>
 *
 * 屏幕布局: 1024x600, 32bpp
 *
 * @date  2026-05-20
 */

#include "view/home_view/common.h"
#include "lvgl_compat/widget_registry.h"
#include "lvgl_compat/screen_manager.h"
#include <stdio.h>

/* ======================================================================
 * 辅助函数 (与 ui_home_page.c / ui_link_page.c 保持一致)
 * ====================================================================== */

static lv_obj_t *mk_label(lv_obj_t *parent, const char *name,
                           lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                           const char *text)
{
    lv_obj_t *lb = lv_label_create(parent, NULL);
    lv_obj_set_pos(lb, x, y);
    lv_obj_set_size(lb, w, h);
    if (text) lv_label_set_text(lb, text);
    lv_label_set_long_mode(lb, LV_LABEL_LONG_CROP);
    if (name) widget_reg_add(name, lb);
    return lb;
}

static lv_obj_t *mk_cont(lv_obj_t *parent, const char *name,
                           lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *c = lv_cont_create(parent, NULL);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_cont_set_layout(c, LV_LAYOUT_OFF);
    lv_obj_set_style_local_bg_opa(c, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(c, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_pad_all(c, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    if (name) widget_reg_add(name, c);
    return c;
}

/* ======================================================================
 * 构建设备信息区域 (view)
 *
 * 左列: 固定标签 (UUID: / Bluetooth Name: / ...)
 * 右列: 动态值 (uuid / bluetooth / ... )
 * ====================================================================== */

static void build_info_view(lv_obj_t *scr)
{
    lv_obj_t *view = mk_cont(scr, "view", 13, 12, 998, 575);

    /* 标题: "OTA" */
    lv_obj_t *title = mk_label(view, "title_lbl", 393, 12, 205, 44, "OTA");
    lv_obj_set_style_local_text_color(title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* ---- 左列: 固定标签 ---- */
    /* 这些标签在XML中 name="label" (无唯一名), 不注册到registry */
    lv_obj_t *lbl;

    lbl = mk_label(view, NULL, 129, 68,  218, 39, "UUID:");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    lbl = mk_label(view, NULL, 129, 136, 218, 39, "Bluetooth Name:");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    lbl = mk_label(view, NULL, 129, 206, 218, 39, "Bluetooth Ver:");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    lbl = mk_label(view, NULL, 129, 275, 218, 39, "OTA:");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    lbl = mk_label(view, NULL, 129, 346, 218, 39, "SN:");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    lbl = mk_label(view, NULL, 129, 415, 218, 39, "Version:");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    lbl = mk_label(view, NULL, 129, 485, 218, 39, "CarBit:");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    lbl = mk_label(view, NULL, 129, 549, 218, 39, "OTA_state");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* ---- 右列: 动态值 (注册到 registry) ---- */

    /* uuid_status: x=800 y=64 w=78 h=45 font_size=25 align_h=right */
    lbl = mk_label(view, "uuid_status", 800, 64, 78, 45, "--");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* uuid: x=562 y=64 w=216 h=45 font_size=32 align_h=right */
    lbl = mk_label(view, "uuid", 562, 64, 216, 45, "");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* bluetooth: x=660 y=132 w=218 h=45 */
    lbl = mk_label(view, "bluetooth", 660, 132, 218, 45, "--");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* bluetooth_ver: x=660 y=202 w=218 h=45 */
    lbl = mk_label(view, "bluetooth_ver", 660, 202, 218, 45, "--");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* ota: x=660 y=271 w=218 h=45 */
    lbl = mk_label(view, "ota", 660, 271, 218, 45, "--");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* sn: x=660 y=342 w=218 h=45 */
    lbl = mk_label(view, "sn", 660, 342, 218, 45, "--");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* version: x=660 y=411 w=218 h=45 */
    lbl = mk_label(view, "version", 660, 411, 218, 45, "--");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* carBit: x=660 y=481 w=218 h=45 */
    lbl = mk_label(view, "carBit", 660, 481, 218, 45, "--");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* ota_state: x=660 y=544 w=218 h=45 color=#14FF3C */
    lbl = mk_label(view, "ota_state", 660, 544, 218, 45, "OTA_no_start");
    lv_obj_set_style_local_text_color(lbl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0x14FF3C));
}

/* ======================================================================
 * 构建 OTA 进度区域 (view1)
 *
 * <view name="view1" x=351 y=165 w=324 h=260>
 *   <progress_circle name="progress_circle" x=59 y=49 w=202 h=160
 *     start_angle=-90 line_width=13 format="%d%%" value=0>
 *   <label name="lab_state" x=56 y=1 w=205 h=34 font_size=28>
 *   <label name="lab_error" x=59 y=211 w=205 h=35 font_size=28>
 * </view>
 *
 * progress_circle → lv_arc (与 home_page 中的 speed arc 类似)
 * start_angle=-90 表示从正上方(12点钟)开始
 * device_view.c::device_refresh_bar() 调用 progress_circle_set_value()
 * 兼容层已将 progress_circle_set_value 映射为 lv_arc_set_value
 * ====================================================================== */

static void build_ota_view(lv_obj_t *scr)
{
    lv_obj_t *view1 = mk_cont(scr, "view1", 351, 165, 324, 260);

    /*
     * progress_circle: start_angle=-90° (即270°), 满圈360°
     * LVGL lv_arc: bg_angles 用 0-360 (0=3点钟方向)
     * AWTK start_angle=-90° 即从12点钟方向(270° in LVGL坐标系)
     * 范围: 完整一圈 270→270+360 (即 270→630, 但LVGL自动取模)
     * 简化: start=270, end=270+360=630 → LVGL 实际 270→269(全圈)
     */
    lv_obj_t *arc = lv_arc_create(view1, NULL);
    lv_obj_set_pos(arc, 59, 49);
    lv_obj_set_size(arc, 202, 160);
    lv_arc_set_bg_angles(arc, 270, 630);   /* 从12点钟方向顺时针一整圈 */
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_local_line_width(arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, 13);
    lv_obj_set_style_local_line_width(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, 13);
    lv_obj_set_style_local_line_color(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT,
                                       lv_color_hex(0xDBDBDB));
    lv_obj_set_style_local_text_color(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));
    widget_reg_add("progress_circle", arc);

    /* lab_state: x=56 y=1 w=205 h=34 font_size=28 — OTA 状态文本 */
    lv_obj_t *state_lb = mk_label(view1, "lab_state", 56, 1, 205, 34, "");
    lv_obj_set_style_local_text_color(state_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* lab_error: x=59 y=211 w=205 h=35 font_size=28 — OTA 错误文本 */
    lv_obj_t *error_lb = mk_label(view1, "lab_error", 59, 211, 205, 35, "");
    lv_obj_set_style_local_text_color(error_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));
}

/* ======================================================================
 * 页面入口 — 由 screen_manager 调用
 * ====================================================================== */

extern ret_t device_page_init(widget_t *win, void *ctx);

int ui_device_page_init(lv_obj_t *screen, void *ctx)
{
    printf("[ui_build] Building device_page widget tree...\n");

    /* 屏幕背景色: #000000 */
    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,
                                     lv_color_hex(0x000000));

    build_info_view(screen);
    build_ota_view(screen);

    printf("[ui_build] device_page widget tree built\n");

    /* 调用原 AWTK-era 的 device_page_init → device_init → device_view_init + device_timer_init */
    device_page_init(screen, ctx);

    printf("[ui_build] device_page_init completed\n");
    return 0;
}

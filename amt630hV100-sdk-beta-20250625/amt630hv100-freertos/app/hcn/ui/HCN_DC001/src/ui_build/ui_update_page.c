/**
 * @file ui_update_page.c
 * @brief update_page.xml → LVGL C 代码转写
 *
 * M027: 将 update_page.xml 的控件树用 LVGL API 构建
 * 所有命名控件注册到 widget_registry, 供 update_view.c / update_logic.c 使用
 *
 * 控件名来源 (update_view.c::update_widget_name[]):
 *   "update_bar", "lab_state", "lab_type", "lab_error"
 *
 * XML 结构:
 *   <window name="update_page" bg_color=#000000>
 *     <view name="view" x=146 y=82 w=733 h=426
 *       border_color=#FFFFFF border_width=1 round_radius=13>
 *       <progress_bar name="update_bar" x=110 y=206 w=513 h=28
 *         fg_color=#0070C0 bg_color=#FFFFFF19 show_text=true value=1>
 *       <label name="lab_state" x=197 y=88 w=338 h=62 font_size=38>
 *       <label name="lab_type" x=264 y=152 w=205 h=35 font_size=25>
 *       <label name="lab_error" x=241 y=264 w=251 h=42
 *         text_color=#FF0000 font_size=32>
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
 * 辅助函数
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

static lv_obj_t *mk_bar(lv_obj_t *parent, const char *name,
                          lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                          int min_val, int max_val, int init_val)
{
    lv_obj_t *bar = lv_bar_create(parent, NULL);
    lv_obj_set_pos(bar, x, y);
    lv_obj_set_size(bar, w, h);
    lv_bar_set_range(bar, min_val, max_val);
    lv_bar_set_value(bar, init_val, LV_ANIM_OFF);
    if (name) widget_reg_add(name, bar);
    return bar;
}

/* ======================================================================
 * 构建 update_page 控件树
 * ====================================================================== */

static void build_update_view(lv_obj_t *scr)
{
    /*
     * <view name="view" x=146 y=82 w=733 h=426
     *   border_color=#FFFFFF border_width=1 round_radius=13>
     * 这是一个有圆角白边框的容器
     */
    lv_obj_t *view = mk_cont(scr, "view", 146, 82, 733, 426);
    /* 应用边框样式 */
    lv_obj_set_style_local_border_opa(view, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_border_color(view, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,
                                         lv_color_hex(0xFFFFFF));
    lv_obj_set_style_local_border_width(view, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);
    lv_obj_set_style_local_radius(view, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 13);

    /*
     * <progress_bar name="update_bar" x=110 y=206 w=513 h=28
     *   fg_color=#0070C0 bg_color=#FFFFFF19 show_text=true value=1>
     * bg_color #FFFFFF19 = 白色 ~10% 不透明
     */
    lv_obj_t *bar = mk_bar(view, "update_bar", 110, 206, 513, 28, 0, 100, 1);
    lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_BG, LV_STATE_DEFAULT,
                                     lv_color_hex(0xFFFFFF));
    lv_obj_set_style_local_bg_opa(bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, LV_OPA_10);
    lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT,
                                     lv_color_hex(0x0070C0));

    /*
     * <label name="lab_state" x=197 y=88 w=338 h=62
     *   font_size=38 text_color=#FFFFFF tr_text="null">
     * 状态文本 (updating / update_success / ...)
     */
    lv_obj_t *state_lb = mk_label(view, "lab_state", 197, 88, 338, 62, "");
    lv_obj_set_style_local_text_color(state_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /*
     * <label name="lab_type" x=264 y=152 w=205 h=35
     *   font_size=25 text_color=#FFFFFF>
     * 升级类型文本 (USB-SOC / USB-MCU / SD-CARD / OTA)
     */
    lv_obj_t *type_lb = mk_label(view, "lab_type", 264, 152, 205, 35, "");
    lv_obj_set_style_local_text_color(type_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /*
     * <label name="lab_error" x=241 y=264 w=251 h=42
     *   text_color=#FF0000 font_size=32 tr_text="null">
     * 错误文本 (error_crc / error_flash / error_file_type)
     */
    lv_obj_t *err_lb = mk_label(view, "lab_error", 241, 264, 251, 42, "");
    lv_obj_set_style_local_text_color(err_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFF0000));
}

/* ======================================================================
 * 页面入口 — 由 screen_manager 调用
 * ====================================================================== */

extern ret_t update_page_init(widget_t *win, void *ctx);

int ui_update_page_init(lv_obj_t *screen, void *ctx)
{
    printf("[ui_build] Building update_page widget tree...\n");

    /* 屏幕背景色: #000000 */
    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,
                                     lv_color_hex(0x000000));

    build_update_view(screen);

    printf("[ui_build] update_page widget tree built\n");

    /* 调用原 AWTK-era 的 update_page_init → update_init → update_view_init + update_timer_init */
    update_page_init(screen, ctx);

    printf("[ui_build] update_page_init completed\n");
    return 0;
}

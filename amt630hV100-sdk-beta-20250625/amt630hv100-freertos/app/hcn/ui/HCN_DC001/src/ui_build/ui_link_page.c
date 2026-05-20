/**
 * @file ui_link_page.c
 * @brief link_page.xml → LVGL C 代码转写
 *
 * M025: 将 link_page.xml 的控件树用 LVGL API 构建
 * 所有命名控件注册到 widget_registry, 供 link_view.c / link_view_logic.c 使用
 *
 * 控件名来源 (link_view.c::link_view_widget_name[]):
 *   "speed_lab", "unit_lab", "ride_mode_img", "gear_view",
 *   "power_lab", "power_bar", "elec_lab", "elec_bar", "elec_unit"
 *   "link_qr"  (widget_qr)
 *
 * 注意: 图片资源尚未转换 (M036), 图片控件暂用占位
 *       样式/字体尚未转换 (M028/M037), 暂用 LVGL 默认样式
 *       QR 控件暂用占位 (M034 实现 qrencode + lv_canvas)
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
 * 辅助宏/函数 (与 ui_home_page.c 保持一致的创建模式)
 * ====================================================================== */

/* 创建 label 并注册 */
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

/* 创建 image 占位 (M036 后替换为真实图片) */
static lv_obj_t *mk_img(lv_obj_t *parent, const char *name,
                          lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *img = lv_obj_create(parent, NULL);
    lv_obj_set_pos(img, x, y);
    lv_obj_set_size(img, w, h);
    lv_obj_set_style_local_bg_opa(img, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(img, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    if (name) widget_reg_add(name, img);
    return img;
}

/* 创建 container (view) */
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

/* 创建 progress_bar (→ lv_bar) */
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

/* 创建 slider */
static lv_obj_t *mk_slider(lv_obj_t *parent, const char *name,
                             lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                             int min_val, int max_val)
{
    lv_obj_t *sl = lv_slider_create(parent, NULL);
    lv_obj_set_pos(sl, x, y);
    lv_obj_set_size(sl, w, h);
    lv_slider_set_range(sl, min_val, max_val);
    lv_slider_set_value(sl, 0, LV_ANIM_OFF);
    if (name) widget_reg_add(name, sl);
    return sl;
}

/* ======================================================================
 * 构建 link_page 控件树
 *
 * XML 结构:
 *   <window name="link_page">
 *     <view name="view" x=754 y=20 w=250 h=250>     — 速度/单位 (含背景)
 *     <image name="ride_mode_img" ...>                — 驾驶模式图标
 *     <slide_menu name="gear_view" ...>               — 档位 N/D/R
 *     <label name="power_lab" ...>                    — 功率数字
 *     <slider name="power_bar" ...>                   — 功率条
 *     <qr name="link_qr" ...>                         — QR 码
 *     <label name="label1" ...>                       — "w" 功率单位
 *     <label name="elec_lab" ...>                     — 续航里程数字
 *     <progress_bar name="elec_bar" ...>              — 续航电量条
 *     <label name="elec_unit" ...>                    — 续航单位 "km"
 *     <image name="image3" ...>                       — 电池头部图标
 *     <?include component.xml?>                       — 来电弹窗
 *   </window>
 * ====================================================================== */

static void build_speed_area(lv_obj_t *scr)
{
    /*
     * <view name="view" x="754" y="20" w="250" h="250"
     *   style:normal:bg_image="interconnection_bg">
     * 背景图片 interconnection_bg → M036 处理, 先透明占位
     */
    lv_obj_t *view = mk_cont(scr, "view", 754, 20, 250, 250);

    /* <label name="speed_lab" x=0 y=-14 w=151 h=124 font_size=124 text="0"> */
    lv_obj_t *speed_lb = mk_label(view, "speed_lab", 0, -14, 151, 124, "0");
    lv_obj_set_style_local_text_color(speed_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* <label name="unit_lab" x=152 y=45 w=83 h=41 font_size=35 text="km/h" color=#606060> */
    lv_obj_t *unit_lb = mk_label(view, "unit_lab", 152, 45, 83, 41, "km/h");
    lv_obj_set_style_local_text_color(unit_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0x606060));
}

static void build_gear_view(lv_obj_t *scr)
{
    /*
     * <slide_menu name="gear_view" x=878 y=110 w=116 h=52 value=1>
     *   <image name="gear_N" ...>
     *   <image name="gear_D" ...>
     *   <image name="gear_R" ...>
     * LVGL: container + 3 image children
     * link_view.c::link_refresh_gear() 遍历子控件设 state
     */
    lv_obj_t *gear = mk_cont(scr, "gear_view", 878, 110, 116, 52);
    mk_img(gear, "gear_N", 0,  0, 38, 52);
    mk_img(gear, "gear_D", 38, 0, 38, 52);
    mk_img(gear, "gear_R", 76, 0, 38, 52);
}

static void build_power_area(lv_obj_t *scr)
{
    /* <label name="power_lab" x=769 y=174 w=61 h=35 text="0" color=#FFFFFF align_h=right> */
    lv_obj_t *pwr_lb = mk_label(scr, "power_lab", 769, 174, 61, 35, "0");
    lv_obj_set_style_local_text_color(pwr_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /* <slider name="power_bar" x=769 y=228 w=100 h=22 bar_size=5 value=0
     *   bg_color=#848484 fg_color=#FFFFFF> */
    lv_obj_t *pwr_sl = mk_slider(scr, "power_bar", 769, 228, 100, 22, 0, 100);
    lv_obj_set_style_local_bg_color(pwr_sl, LV_SLIDER_PART_BG, LV_STATE_DEFAULT,
                                     lv_color_hex(0x848484));
    lv_obj_set_style_local_bg_color(pwr_sl, LV_SLIDER_PART_INDIC, LV_STATE_DEFAULT,
                                     lv_color_hex(0xFFFFFF));

    /* <label name="label1" x=838 y=172 w=33 h=35 text="w" color=#327889> — 功率单位 */
    lv_obj_t *pwr_unit = mk_label(scr, "label1", 838, 172, 33, 35, "w");
    lv_obj_set_style_local_text_color(pwr_unit, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0x327889));
}

static void build_electrical_area(lv_obj_t *scr)
{
    /* <label name="elec_lab" x=893 y=174 w=61 h=35 text="0" color=#FFFFFF align_h=right> */
    lv_obj_t *elec_lb = mk_label(scr, "elec_lab", 893, 174, 61, 35, "0");
    lv_obj_set_style_local_text_color(elec_lb, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0xFFFFFF));

    /*
     * <progress_bar name="elec_bar" x=902 y=220 w=91 h=30
     *   fg_color=#FF0000 bg_color=#FFFFFF border_width=4 border_color=#FFFFFF
     *   round_radius=6 format="%d%%" value=0>
     */
    lv_obj_t *elec_bar = mk_bar(scr, "elec_bar", 902, 220, 91, 30, 0, 100, 0);
    lv_obj_set_style_local_bg_color(elec_bar, LV_BAR_PART_BG, LV_STATE_DEFAULT,
                                     lv_color_hex(0xFFFFFF));
    lv_obj_set_style_local_bg_color(elec_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT,
                                     lv_color_hex(0xFF0000));
    lv_obj_set_style_local_border_width(elec_bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, 4);
    lv_obj_set_style_local_border_color(elec_bar, LV_BAR_PART_BG, LV_STATE_DEFAULT,
                                         lv_color_hex(0xFFFFFF));
    lv_obj_set_style_local_radius(elec_bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_radius(elec_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, 6);

    /* <label name="elec_unit" x=965 y=172 w=33 h=35 text="km" color=#327889> */
    lv_obj_t *elec_unit = mk_label(scr, "elec_unit", 965, 172, 33, 35, "km");
    lv_obj_set_style_local_text_color(elec_unit, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT,
                                       lv_color_hex(0x327889));

    /* <image name="image3" x=992 y=228 w=9 h=15 image="link_elecRect_head"> */
    mk_img(scr, "image3", 992, 228, 9, 15);
}

static void build_qr_area(lv_obj_t *scr)
{
    /*
     * <qr name="link_qr" x=12 y=19 w=239 h=235 value="http://...">
     *
     * QR 控件在 M034 中实现 (qrencode + lv_canvas)
     * 暂用 container 占位, link_view.c 中的 link_refresh_qr()
     * 仅调用 widget_set_visible, 占位即可工作
     */
    lv_obj_t *qr = mk_cont(scr, "link_qr", 12, 19, 239, 235);
    /* 给占位一个可见的边框, 便于调试 (M034 替换) */
    lv_obj_set_style_local_border_opa(qr, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_50);
    lv_obj_set_style_local_border_color(qr, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,
                                         lv_color_hex(0x808080));
    lv_obj_set_style_local_border_width(qr, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);

    /* QR 区域内的占位 label */
    lv_obj_t *qlb = lv_label_create(qr, NULL);
    lv_label_set_text(qlb, "QR\n(M034)");
    lv_obj_align(qlb, NULL, LV_ALIGN_CENTER, 0, 0);
}

static void build_component(lv_obj_t *scr)
{
    /* component.xml: 来电动画条 (与 home_page 相同) */
    lv_obj_t *call = mk_cont(scr, "call_aniamtor", 223, -64, 580, 64);
    lv_obj_set_hidden(call, true); /* 默认隐藏, 来电时显示 */
    mk_img(call,   "image2",        49,  9, 51, 50);
    mk_img(call,   "image7",        470, 10, 51, 50);
    mk_label(call, "call_pop_tips", 127, 0, 321, 35, "");
}

/* ======================================================================
 * 页面入口 — 由 screen_manager 调用
 * ====================================================================== */

extern ret_t link_page_init(widget_t *win, void *ctx);

int ui_link_page_init(lv_obj_t *screen, void *ctx)
{
    printf("[ui_build] Building link_page widget tree...\n");

    /* 屏幕背景色: #00000000 (黑色透明 → 用纯黑) */
    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,
                                     lv_color_hex(0x000000));

    /* 驾驶模式图标 */
    /* <image name="ride_mode_img" x=783 y=118 w=27 h=21 image="drv_mode_0"> */
    mk_img(screen, "ride_mode_img", 783, 118, 27, 21);

    /* 构建各区域 */
    build_speed_area(screen);
    build_gear_view(screen);
    build_power_area(screen);
    build_electrical_area(screen);
    build_qr_area(screen);
    build_component(screen);

    printf("[ui_build] link_page widget tree built\n");

    /* 调用原 AWTK-era 的 link_page_init → link_init → link_view_init + link_timer_init */
    link_page_init(screen, ctx);

    printf("[ui_build] link_page_init completed\n");
    return 0;
}

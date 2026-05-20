/**
 * @file ui_home_page.c
 * @brief home_page.xml → LVGL C 代码转写
 *
 * M024: 将 home_page.xml 的控件树用 LVGL API 构建
 * 所有命名控件注册到 widget_registry, 供 speed_view.c 等现有视图代码使用
 *
 * 注意: 图片资源尚未转换 (M036), 图片控件暂用占位
 *       样式/字体尚未转换 (M028/M037), 暂用 LVGL 默认样式
 *
 * 屏幕布局: 1024x600, 32bpp
 */

#include "view/home_view/common.h"
#include "lvgl_compat/widget_registry.h"
#include "lvgl_compat/screen_manager.h"
#include <stdio.h>

/* ======================================================================
 * 辅助宏: 创建控件并注册到 widget_registry
 * ====================================================================== */

#define REG_OBJ(parent, name_str, obj_ptr) \
    do { widget_reg_add(name_str, obj_ptr); } while(0)

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
    /* 暂用空 obj 占位, M036 资源转换后替换为 lv_img_create + lv_img_set_src */
    lv_obj_t *img = lv_obj_create(parent, NULL);
    lv_obj_set_pos(img, x, y);
    lv_obj_set_size(img, w, h);
    /* 透明背景 */
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
 * 构建各区域
 * ====================================================================== */

static void build_time_view(lv_obj_t *scr)
{
    lv_obj_t *tv = mk_cont(scr, "time_view", 1, 6, 99, 50);
    mk_label(tv, "min",   0,  0, 51, 50, "00");
    mk_label(tv, "colon", 32, -8, 41, 52, ":");
    mk_label(tv, "sec",   49, 0,  60, 50, "00");
}

static void build_dock_view(lv_obj_t *scr)
{
    lv_obj_t *dv = mk_cont(scr, "dock_view", 13, 154, 49, 294);
    /* 5 个 dock 图标, 垂直排列, 间距约 53px */
    mk_img(dv, "icon_info",    0,   0, 49, 50);
    mk_img(dv, "icon_navi",    0,  63, 49, 50);
    mk_img(dv, "icon_music",   0, 126, 49, 50);
    mk_img(dv, "icon_phone",   0, 189, 49, 50);
    mk_img(dv, "icon_setting", 0, 252, 49, 50);
}

static void build_signal_view(lv_obj_t *scr)
{
    lv_obj_t *sv = mk_cont(scr, "signal_view", 100, 0, 924, 62);
    mk_img(sv, "icon_GMS",       0,   0, 52, 52);
    mk_img(sv, "icon_gps",       47,  0, 52, 52);
    mk_img(sv, "icon_bt",        99,  0, 52, 52);
    mk_img(sv, "icon_high_beam", 163, 0, 52, 52);
    mk_img(sv, "icon_left_bg",   284, 0, 52, 52);
    mk_img(sv, "icon_left",      284, 0, 52, 52);
    mk_img(sv, "icon_ready_bg",  348, 0, 130, 52);
    mk_img(sv, "icon_ready",     348, 0, 130, 52);
    mk_img(sv, "icon_right_bg",  489, 0, 52, 52);
    mk_img(sv, "icon_right",     489, 0, 52, 52);
    mk_img(sv, "icon_near_beam", 559, 0, 52, 52);
    mk_img(sv, "icon_abs",       668, 0, 52, 52);
    mk_img(sv, "icon_ecu",       727, 0, 52, 52);
    mk_img(sv, "icon_tcs",       790, 0, 52, 52);
    mk_img(sv, "icon_engine",    860, 0, 52, 52);
}

static void build_mileage_view(lv_obj_t *scr)
{
    lv_obj_t *mv = mk_cont(scr, "mileage_view", 1, 548, 1023, 52);
    mk_label(mv, NULL,         0,   5, 55, 35, "TRIP");
    mk_label(mv, "trip_label", 55,  9, 70, 35, "0.0");
    mk_label(mv, "trip_unit",  131, 9, 49, 35, "km");
    mk_label(mv, NULL,         964, 6, 55, 35, "ODO");
    mk_label(mv, "odo_label",  809, 9, 102, 35, "0");
    mk_label(mv, "odo_unit",   916, 9, 44, 35, "km");
}

static void build_electrical_view(lv_obj_t *scr)
{
    lv_obj_t *ev = mk_cont(scr, "electrical_view", 228, 548, 568, 52);
    mk_bar(ev,   "electrical_bar",   192, 20, 186, 12, 0, 100, 100);
    mk_label(ev, "electrical_percentage", 95,  10, 38, 35, "100");
    mk_label(ev, NULL,                    132, 10, 31, 35, "%");
    mk_label(ev, "electrical_value",      379, 10, 61, 35, "100");
    mk_label(ev, "electrical_unit",       445, 10, 52, 35, "km");
}

static void build_speed_view(lv_obj_t *parent)
{
    /* speed_view 是核心仪表盘: 圆环 + 指针 + 数字速度 */
    lv_obj_t *spv = mk_cont(parent, "speed_view", 303, 0, 418, 418);

    /* 背景层 */
    mk_img(spv, "bg_cricle", 0,  0,  418, 418);
    mk_img(spv, "bg_halo",   49, 49, 320, 320);
    mk_img(spv, "bg_center",  73, 73, 272, 272);

    /* 速度数字 (image_value → 用 label 暂替) */
    mk_label(spv, "speed_value", 73, 73, 272, 272, "0");

    /**
     * progress_circle → lv_arc
     * start_angle=135°, 范围=270° (135→405=135+270)
     */
    lv_obj_t *arc = lv_arc_create(spv, NULL);
    lv_obj_set_pos(arc, 0, 0);
    lv_obj_set_size(arc, 418, 418);
    lv_arc_set_bg_angles(arc, 135, 405);    /* 270° 范围 */
    lv_arc_set_range(arc, 0, 270);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_local_line_width(arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, 15);
    lv_obj_set_style_local_line_width(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, 15);
    widget_reg_add("progress_circle", arc);

    /* gauge_pointer → img 占位 (将通过 lv_img_set_angle 旋转) */
    mk_img(spv, "dashboard_pointer", 209, 0, 10, 209);

    /* 速度单位 / 驾驶模式 */
    mk_img(spv, "speed_unit",    170, 290, 79, 29);
    mk_img(spv, "driving_mode",  196, 94,  26, 21);

    /* gear_view (slide_menu → 用 container + 3个 img 子项) */
    lv_obj_t *gear = mk_cont(spv, "gear_view", 90, 354, 238, 55);
    mk_img(gear, "gear_N", 0,   0, 79, 55);
    mk_img(gear, "gear_D", 79,  0, 79, 55);
    mk_img(gear, "gear_R", 158, 0, 79, 55);
}

static void build_power_view(lv_obj_t *parent)
{
    lv_obj_t *pv = mk_cont(parent, "power_view", 688, 63, 336, 300);
    mk_img(pv,    "power_bg",       50,  243, 256, 32);
    mk_label(pv,  "power_value",    0,   83,  336, 98, "0");
    mk_slider(pv, "power_progress", 64,  243, 232, 32, 0, 100);
}

static void build_dock_select_view(lv_obj_t *parent)
{
    /**
     * dock_selec_view 包含 slide_view (6个子页):
     * info_page, navi_page, music_page, phone_page, setting_page, music_expand_page
     *
     * AWTK slide_view → LVGL 中用 lv_tabview 或手动切换 container
     * 此处用 container 堆叠, 通过 visible/hidden 切换
     */
    lv_obj_t *dsv = mk_cont(parent, "dock_selec_view", 0, 23, 648, 372);
    lv_obj_t *sv  = mk_cont(dsv, "dock_slider_view", 0, 0, 648, 372);

    /* info_page */
    lv_obj_t *info = mk_cont(sv, "info_page", 0, 0, 336, 372);
    mk_img(info,   "image",         133, 80, 52, 52);
    mk_img(info,   "image1",        133, 201, 52, 52);
    mk_label(info, "ride_time",     84,  142, 157, 35, "0h 0min");
    mk_label(info, "ride_distance", 84,  263, 157, 35, "0 km");

    /* navi_page */
    lv_obj_t *navi = mk_cont(sv, "navi_page", 0, 0, 336, 372);
    lv_obj_set_hidden(navi, true);
    lv_obj_t *nav_slide = mk_cont(navi, "nav_slide_view", 65, 86, 242, 258);

    lv_obj_t *qr_view = mk_cont(nav_slide, "qr_view", 0, 0, 242, 258);
    mk_img(qr_view,   "navi_qr", 19, 12, 166, 162); /* QR码占位 */
    mk_label(qr_view, NULL,       12, 196, 183, 35, "Scan QR");

    lv_obj_t *nav_view = mk_cont(nav_slide, "navigate_view", 0, 0, 242, 258);
    lv_obj_set_hidden(nav_view, true);
    mk_img(nav_view,   "navi_image",      58,  4,  115, 115);
    mk_label(nav_view, "label_distance",  28,  130, 175, 59, "");
    mk_label(nav_view, "label_road",      18,  189, 205, 52, "");

    /* music_page */
    lv_obj_t *music = mk_cont(sv, "music_page", 0, 0, 336, 372);
    lv_obj_set_hidden(music, true);
    mk_img(music,   "music_image",  128, 118, 77, 75);
    mk_label(music, "music_title",  64,  232, 205, 35, "No Music");
    mk_label(music, "music_clyric", 64,  282, 214, 50, "");

    /* phone_page */
    lv_obj_t *phone = mk_cont(sv, "phone_page", 0, 0, 336, 372);
    lv_obj_set_hidden(phone, true);
    lv_obj_t *bt_conn = mk_cont(phone, "bt_connected", 68, 84, 257, 268);
    lv_obj_set_hidden(bt_conn, true);
    mk_img(bt_conn,   "phone_state_img", 46, 71, 128, 125);
    mk_label(bt_conn, "phone_tips",      0,  0,  229, 71, "");
    mk_label(bt_conn, "phone_num",       0,  219, 229, 49, "");
    lv_obj_t *bt_no = mk_cont(phone, "bt_no_connected", 68, 84, 257, 268);
    mk_img(bt_no, "image3", 40, 71, 128, 125);

    /* setting_page (empty in home_pg context) */
    lv_obj_t *set_page = mk_cont(sv, "setting_page", 0, 0, 336, 372);
    lv_obj_set_hidden(set_page, true);

    /* music_expand_page */
    lv_obj_t *music_ex = mk_cont(sv, "music_expand_page", 0, 0, 648, 372);
    lv_obj_set_hidden(music_ex, true);
    mk_img(music_ex,    "music_image_ex", 274, 27, 77, 75);
    mk_label(music_ex,  "music_title_ex", 120, 128, 384, 35, "No Music");
    mk_label(music_ex,  "music_lyric_ex", 102, 176, 421, 35, "");
    mk_bar(music_ex,    "music_bar",      120, 226, 384, 15, 0, 100, 0);
    mk_img(music_ex,    "music_prev",     133, 263, 64, 62);
    mk_img(music_ex,    "music_state",    280, 263, 64, 62);
    mk_img(music_ex,    "music_next",     408, 263, 64, 62);
    mk_label(music_ex,  "total_time",     447, 241, 58, 29, "00:00");
    mk_label(music_ex,  "curr_time",      120, 241, 58, 29, "00:00");
}

static void build_setting_pg(lv_obj_t *pages_cont)
{
    lv_obj_t *sp = mk_cont(pages_cont, "setting_pg", 0, 0, 1024, 418);
    lv_obj_set_hidden(sp, true); /* 默认隐藏, pages切换时显示 */

    /* scroll_menu (左侧设置菜单) */
    lv_obj_t *smenu = mk_cont(sp, "scroll_menu", 73, 55, 180, 312);
    mk_label(smenu, "tmps",       0, 0,   180, 62, "TPMS");
    mk_label(smenu, "ride_ele",   0, 62,  180, 62, "Energy");
    mk_label(smenu, "connect",    0, 125, 180, 62, "Connect");
    mk_label(smenu, "language",   0, 188, 180, 62, "Language");
    mk_label(smenu, "brightness", 0, 250, 180, 62, "Brightness");
    mk_label(smenu, "unit",       0, 312, 180, 62, "Unit");
    mk_label(smenu, "clock",      0, 375, 180, 62, "Clock");
    mk_label(smenu, "display",    0, 438, 180, 62, "Display");
    mk_label(smenu, "device",     0, 500, 180, 62, "Device");

    /* setting_menu (右侧内容, slide_view → container 堆叠) */
    lv_obj_t *sm = mk_cont(sp, "setting_menu", 324, 9, 700, 400);

    /* 各设置子页 (简化版, 完整在 M033) */
    /* TPMS */
    lv_obj_t *tmps_v = mk_cont(sm, "tmps_view", 0, 0, 700, 400);
    mk_label(tmps_v, "tmps_value_front", 64,  139, 42, 45, "0.0");
    mk_label(tmps_v, "tmps_value_rear",  572, 146, 42, 45, "0.0");
    mk_label(tmps_v, "tmps_temp_front",  64,  281, 46, 45, "--");

    /* Cycling energy */
    lv_obj_t *ride_v = mk_cont(sm, "ride_ele_view", 0, 0, 700, 400);
    lv_obj_set_hidden(ride_v, true);
    mk_label(ride_v, "ride_5km",  306, 36, 92, 35, "5km");
    mk_label(ride_v, "ride_20km", 411, 36, 92, 35, "20km");
    mk_label(ride_v, "last_elec", 28,  329, 123, 35, "0wh");
    mk_label(ride_v, "curr_elec", 218, 329, 123, 35, "0wh");
    mk_label(ride_v, "avg_ele",   396, 329, 123, 35, "0wh");
    /* chart_view 占位 */
    mk_cont(ride_v, "chartview", 13, 96, 543, 185);
    mk_cont(ride_v, "line_series", 0, 0, 1, 1); /* series 占位 */

    /* BT connect */
    lv_obj_t *conn_v = mk_cont(sm, "connect_view", 0, 0, 700, 400);
    lv_obj_set_hidden(conn_v, true);
    mk_label(conn_v, "bt_on_option",  63, 122, 576, 75, "BT ON");
    mk_label(conn_v, "bt_off_option", 63, 219, 576, 75, "BT OFF");
    mk_img(conn_v,   "bt_state",      504, 38, 102, 50);
    mk_label(conn_v, "bt_name",       169, 45, 280, 35, "");
    mk_label(conn_v, "bt_phone_info", 120, 314, 461, 49, "");

    /* Language */
    lv_obj_t *lang_v = mk_cont(sm, "language_view", 0, 0, 700, 400);
    lv_obj_set_hidden(lang_v, true);
    mk_label(lang_v, "chinese_option", 63, 122, 576, 75, "中文");
    mk_label(lang_v, "english_option", 63, 219, 576, 75, "English");

    /* Brightness */
    lv_obj_t *br_v = mk_cont(sm, "brightness_view", 0, 0, 700, 400);
    lv_obj_set_hidden(br_v, true);
    mk_label(br_v, "brightness_auto_option", 0, 0, 576, 75, "Auto");
    mk_label(br_v, "brightness_1_option",    0, 75, 576, 75, "1");
    mk_label(br_v, "brightness_2_option",    0, 150, 576, 75, "2");
    mk_label(br_v, "brightness_3_option",    0, 225, 576, 75, "3");
    mk_label(br_v, "brightness_4_option",    0, 300, 576, 75, "4");

    /* Unit */
    lv_obj_t *unit_v = mk_cont(sm, "unit_view", 0, 0, 700, 400);
    lv_obj_set_hidden(unit_v, true);
    mk_label(unit_v, "unit_km_option",   63, 122, 576, 75, "km/h");
    mk_label(unit_v, "unit_mile_option", 63, 219, 576, 75, "mph");

    /* Clock */
    lv_obj_t *clk_v = mk_cont(sm, "clock_view", 0, 0, 700, 400);
    lv_obj_set_hidden(clk_v, true);
    mk_label(clk_v, "clock_h_1", 205, 170, 47, 106, "0");
    mk_label(clk_v, "clock_h_2", 252, 170, 47, 106, "0");
    mk_label(clk_v, "clock_m_1", 374, 170, 47, 106, "0");
    mk_label(clk_v, "clock_m_2", 421, 170, 47, 106, "0");

    /* Display */
    lv_obj_t *disp_v = mk_cont(sm, "display_view", 0, 0, 700, 400);
    lv_obj_set_hidden(disp_v, true);
    mk_label(disp_v, "display_auto_option",  63, 122, 576, 75, "Auto");
    mk_label(disp_v, "display_day_option",   63, 209, 576, 75, "Day");
    mk_label(disp_v, "display_night_option", 63, 298, 576, 75, "Night");

    /* Device info */
    lv_obj_t *dev_v = mk_cont(sm, "device_view", 0, 0, 700, 400);
    lv_obj_set_hidden(dev_v, true);
    mk_label(dev_v, "device_sn",  24, 142, 538, 41, "SN");
    mk_label(dev_v, "device_ver", 24, 234, 538, 41, "v1.0.0");
    mk_label(dev_v, "device_mcu", 26, 329, 538, 41, "1.0");
}

static void build_component(lv_obj_t *scr)
{
    /* component.xml: 来电动画条 */
    lv_obj_t *call = mk_cont(scr, "call_aniamtor", 223, -64, 580, 64);
    lv_obj_set_hidden(call, true); /* 默认隐藏, 来电时显示 */
    mk_img(call,   "image2",       49,  9, 51, 50);
    mk_img(call,   "image7",       470, 10, 51, 50);
    mk_label(call, "call_pop_tips", 127, 0, 321, 35, "");
}

/* ======================================================================
 * 页面入口 — 由 screen_manager 调用
 * ====================================================================== */

/**
 * 引用原 AWTK-era 的初始化函数
 * home_page_init 内部调用 home_view_init + set_view_init 等
 * 这些函数使用 widget_lookup 获取控件, 现在走注册表
 */
extern ret_t home_page_init(widget_t *win, void *ctx);

int ui_home_page_init(lv_obj_t *screen, void *ctx)
{
    printf("[ui_build] Building home_page widget tree...\n");

    /* 设置屏幕背景色 (暂用深色, M036 后替换为壁纸图片) */
    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,
                                     lv_color_hex(0x1a1a2e));

    /* 构建各区域控件树 */
    build_time_view(screen);
    build_dock_view(screen);
    build_signal_view(screen);
    build_mileage_view(screen);
    build_electrical_view(screen);

    /* pages 容器 (主内容区, 两页切换) */
    lv_obj_t *pages = mk_cont(screen, "pages", 0, 91, 1024, 418);

    /* home_pg (仪表盘页, 默认显示) */
    lv_obj_t *home_pg = mk_cont(pages, "home_pg", 0, 0, 1024, 418);
    build_speed_view(home_pg);
    build_power_view(home_pg);
    build_dock_select_view(home_pg);

    /* setting_pg (设置页, 默认隐藏) */
    build_setting_pg(pages);

    /* component (来电弹窗) */
    build_component(screen);

    printf("[ui_build] Widget tree built, %d widgets registered\n", 0 /* TODO: count */);

    /* 调用原 AWTK-era 的 home_page_init → view_manager_init 等 */
    home_page_init(screen, ctx);

    printf("[ui_build] home_page_init completed\n");
    return 0;
}

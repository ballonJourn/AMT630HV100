/**
 * @file ui_styles.h
 * @brief HCN 日间/夜间主题 + i18n 字符串表 — 公共接口
 *
 * M028: 替代 AWTK 的 assets_set_global_theme() + locale_info_tr()
 *
 * 使用方法:
 *   #include "ui_build/ui_styles.h"
 *
 *   // 主题切换 (由 hcn_global.c::global_refresh_display 调用)
 *   ui_theme_set_night();
 *   ui_theme_set_day();
 *   if (ui_theme_is_night()) { ... }
 *   const hcn_theme_colors_t *c = ui_theme_get_colors();
 *
 *   // i18n (替代 AWTK tr_text / locale_info_tr)
 *   hcn_i18n_set_lang(HCN_LANG_ZH_CN);
 *   const char *text = hcn_tr("updating");  // → "升级中..."
 *
 * @date  2026-05-20
 */

#ifndef UI_STYLES_H__
#define UI_STYLES_H__

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================
 * 主题颜色结构
 * ====================================================================== */

typedef struct {
    lv_color_t label_text;              /* label 默认文本色 */
    lv_color_t label_text_half;         /* label 半透明文本 (setting_menu normal) */
    lv_color_t setting_menu_selected;   /* setting_menu 选中文本色 */
    lv_color_t setting_option_selected; /* setting_option 选中文本色 */
    lv_color_t mileage_unit_text;       /* 里程单位文本色 */
    lv_color_t hscroll_text;            /* 滚动标签/歌词文本色 */
    lv_color_t window_bg;               /* 窗口默认背景色 */
    lv_color_t progress_bar_bg;         /* 进度条背景色 */
    lv_color_t progress_bar_fg;         /* 进度条前景色 */
    lv_color_t qr_bg;                   /* QR 码背景色 */
    uint8_t    progress_bar_bg_opa;     /* 进度条背景不透明度 (0-100) */
} hcn_theme_colors_t;

/* 获取当前主题颜色 */
const hcn_theme_colors_t *ui_theme_get_colors(void);

/* 切换主题 */
void ui_theme_set_day(void);
void ui_theme_set_night(void);

/* 查询当前主题 */
int ui_theme_is_night(void);

/* ======================================================================
 * i18n 多语言
 * ====================================================================== */

#define HCN_LANG_ZH_CN  0
#define HCN_LANG_EN_US  1

/* 设置/获取当前语言 */
void    hcn_i18n_set_lang(uint8_t lang);
uint8_t hcn_i18n_get_lang(void);

/**
 * 翻译函数 — 替代 AWTK 的 tr_text() / locale_info_tr()
 * @param key  字符串键名 (对应 strings.xml 中的 name)
 * @return 当前语言的翻译文本; 找不到则返回 key 本身
 */
const char *hcn_tr(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* UI_STYLES_H__ */

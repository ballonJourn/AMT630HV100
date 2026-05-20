/**
 * @file ui_styles.c
 * @brief AWTK styles/default.xml (day+night) + strings.xml → LVGL lv_style_t + i18n 表
 *
 * M028: 主题/样式/国际化的 C 代码实现
 *
 * === 主题系统 ===
 * AWTK: 两套 XML styles (default/ + night/) → assets_set_global_theme() 热切换
 * LVGL: 两套 lv_style_t 数组 + ui_theme_apply() 遍历控件切换
 *
 * 日间 vs 夜间 核心差异 (从 XML diff 提取):
 *   - label default text_color:    day=#083557  night=#FFFFFF
 *   - label setting_menu:          day=#08355780 / #083557  night=#FFFFFF / #79D9F9
 *   - label setting_option:        day=#08355780 / #083557  night=#FFFFFF / #2BF8FF
 *   - label mileage_unit:          day=#B06412   night=#4A93BB
 *   - label call_tips bg:          both=#000000B4 (same)
 *   - hscroll_label text:          day=#083557   night=#FFFFFF
 *   - window bg:                   day=#083557   night=#FFFFFF
 *   - progress_bar bg/fg:          day=bg#08355719 fg#083557  night=bg#FFFFFF19 fg#FFFFFF
 *   - qr bg:                       day=#083557   night=#FFFFFF
 *
 * === i18n 系统 ===
 * AWTK: strings.xml → locale_info_change() + tr_text()
 * LVGL: 静态 C 表 + hcn_tr() 查表
 *
 * @date  2026-05-20
 */

#include "view/home_view/common.h"
#include "lvgl_compat/widget_registry.h"
#include <string.h>
#include <stdio.h>

/* ======================================================================
 * Part 1: 主题颜色定义
 * ====================================================================== */

typedef struct {
    lv_color_t label_text;              /* label default text */
    lv_color_t label_text_half;         /* label 半透明 (setting_menu normal) */
    lv_color_t setting_menu_selected;   /* setting_menu selected text */
    lv_color_t setting_option_selected; /* setting_option selected text */
    lv_color_t mileage_unit_text;       /* mileage_unit label */
    lv_color_t hscroll_text;            /* hscroll_label / music lyrics */
    lv_color_t window_bg;               /* window default bg */
    lv_color_t progress_bar_bg;         /* progress_bar bg */
    lv_color_t progress_bar_fg;         /* progress_bar fg */
    lv_color_t qr_bg;                   /* QR code bg */
    uint8_t    progress_bar_bg_opa;     /* progress_bar bg opacity */
} hcn_theme_colors_t;

static const hcn_theme_colors_t theme_day = {
    .label_text              = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .label_text_half         = LV_COLOR_MAKE(0x08, 0x35, 0x57),  /* 80% opa separate */
    .setting_menu_selected   = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .setting_option_selected = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .mileage_unit_text       = LV_COLOR_MAKE(0xB0, 0x64, 0x12),
    .hscroll_text            = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .window_bg               = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .progress_bar_bg         = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .progress_bar_fg         = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .qr_bg                   = LV_COLOR_MAKE(0x08, 0x35, 0x57),
    .progress_bar_bg_opa     = (uint8_t)(0x19 * 100 / 255), /* ~10% */
};

static const hcn_theme_colors_t theme_night = {
    .label_text              = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .label_text_half         = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .setting_menu_selected   = LV_COLOR_MAKE(0x79, 0xD9, 0xF9),
    .setting_option_selected = LV_COLOR_MAKE(0x2B, 0xF8, 0xFF),
    .mileage_unit_text       = LV_COLOR_MAKE(0x4A, 0x93, 0xBB),
    .hscroll_text            = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .window_bg               = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .progress_bar_bg         = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .progress_bar_fg         = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .qr_bg                   = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
    .progress_bar_bg_opa     = (uint8_t)(0x19 * 100 / 255),
};

static const hcn_theme_colors_t *s_current_theme = &theme_night; /* 默认夜间 */

/* ======================================================================
 * Part 2: 主题 API
 * ====================================================================== */

const hcn_theme_colors_t *ui_theme_get_colors(void)
{
    return s_current_theme;
}

void ui_theme_set_day(void)
{
    s_current_theme = &theme_day;
    printf("[ui_styles] theme -> DAY\n");
    /* TODO M038: 遍历所有注册控件, 更新 style */
}

void ui_theme_set_night(void)
{
    s_current_theme = &theme_night;
    printf("[ui_styles] theme -> NIGHT\n");
    /* TODO M038: 遍历所有注册控件, 更新 style */
}

int ui_theme_is_night(void)
{
    return (s_current_theme == &theme_night) ? 1 : 0;
}

/* ======================================================================
 * Part 3: i18n 字符串表
 *
 * 从 strings.xml 提取, 支持 zh_CN / en_US
 * hcn_tr(key) 返回当前语言的翻译字符串
 * ====================================================================== */

typedef struct {
    const char *key;
    const char *zh_CN;
    const char *en_US;
} hcn_i18n_entry_t;

static const hcn_i18n_entry_t s_i18n_table[] = {
    /* 设置菜单 */
    { "tmps",          "胎压检测",     "Tmps" },
    { "ride_ele",      "骑行能耗",     "Energy" },
    { "connect",       "连接手机",     "Connect" },
    { "language",      "语言",         "Language" },
    { "brightness",    "亮度",         "Brightness" },
    { "unit",          "单位",         "Unit" },
    { "clock",         "时间",         "Time" },
    { "display",       "显示设置",     "Display" },
    { "device",        "设备信息",     "Device" },

    /* 骑行能耗 */
    { "power",         "电耗 (Wh/hm)", "power consumption (Wh/hm)" },
    { "last_ride",     "上次骑行",     "Last Ride" },
    { "near_5km",      "近5km",        "Nearly 5" },
    { "near_20km",     "近20km",       "Nearly 20" },
    { "Last_electricity", "上次骑行电耗", "Last energy" },
    { "Single_energy", "单次电耗",     "Single energy" },
    { "average_power", "平均电耗",     "Average energy" },

    /* 蓝牙 */
    { "bt",            "蓝牙",         "Bluetooth" },
    { "bt_open",       "开",           "Open" },
    { "bt_close",      "关",           "Close" },

    /* 设置选项 */
    { "auto",          "自动",         "Atuo" },
    { "kilometer",     "公制",         "Kilometer" },
    { "mile",          "英制",         "Mile" },
    { "day",           "白天",         "Day" },
    { "night",         "黑夜",         "Night" },
    { "sn",            "序列号",       "SN" },
    { "version",       "版本号",       "Version" },

    /* 来电/通话 */
    { "incoming_tips", "短按确认键接听 返回键挂断",     "Press Confirm Answer Back Hang up" },
    { "outgoing_tips", "短按返回键挂断",               "Press Back Hang up" },
    { "calling_tips",  "接听中 短按返回键挂断",         "On The Call Press Back Hang up" },
    { "call_pop_incoming", "来电中 确认接听 返回挂断",  "IncomingCall Enter-Answer Back-Hang up" },
    { "call_pop_output",   "去电中 返回挂断",          "OutgoingCall Enter-Answer Back-Hang up" },
    { "call_pop_answing",  "通话中 返回挂断",          "Answering Back-Hang up" },

    /* 导航/音乐/连接 */
    { "scan_qr",       "扫码连接",     "Scan To Connect" },
    { "press_up_down", "确认后按上下键切换", "UP or Down Switch" },
    { "no_music",      "暂无曲目",     "No Tracks" },
    { "Enter the call page to view details", "电话页面查看详情", "Enter the call page to view details" },
    { "navi_connect",  "未发起导航 点击确认键进入地图", "Click confirm Enter the map without navigation" },

    /* OTA 升级 */
    { "updating",         "升级中...",     "Upgrading..." },
    { "update_success",   "更新成功",     "Update Successful" },
    { "update_error",     "更新失败",     "Update Failed" },
    { "update_timeout",   "更新超时",     "Update Timeout" },
    { "error_crc",        "CRC校验错误",  "Error : CRC check " },
    { "error_flash",      "Flash写入错误", "Error : Flash write " },
    { "error_file_type",  "文件类型错误",  "Error : File type " },
    { "null",             " ",            " " },

    /* 设备页标签 */
    { "Bluetooth Name：",  "蓝牙名称：",  "Bluetooth Name：" },
    { "SN：",              "设备系列号：", "SN：" },
    { "Version：",         "版本信息：",   "Version：" },
    { "CarBit：",          "亿连骑行：",   "CarBit：" },
    { "Bluetooth Ver:",    "蓝牙版本：",   "Bluetooth Ver:" },
    { "Inactive",          "未激活",       "Inactive" },
    { "Activated",         "已激活",       "Activated" },
    { "OTA_state",         "OTA状态：",    "OTA State" },
    { "OTA_no_start",      "关闭",         "Close" },
    { "OTA_starting",      "启动中",       "Starting" },
    { "OTA_started",       "启动成功",     "Startup Successful" },

    { NULL, NULL, NULL } /* sentinel */
};

#define HCN_LANG_ZH_CN  0
#define HCN_LANG_EN_US  1

static uint8_t s_current_lang = HCN_LANG_EN_US; /* 默认英文 */

void hcn_i18n_set_lang(uint8_t lang)
{
    s_current_lang = (lang <= HCN_LANG_EN_US) ? lang : HCN_LANG_EN_US;
    printf("[ui_styles] language -> %s\n",
           s_current_lang == HCN_LANG_ZH_CN ? "zh_CN" : "en_US");
}

uint8_t hcn_i18n_get_lang(void)
{
    return s_current_lang;
}

/**
 * hcn_tr - 翻译函数 (替代 AWTK 的 tr_text / locale_info_tr)
 *
 * @param key  字符串键名 (对应 strings.xml 中的 name 属性)
 * @return 当前语言的翻译字符串, 找不到则返回 key 本身
 */
const char *hcn_tr(const char *key)
{
    if (key == NULL) return "";

    for (const hcn_i18n_entry_t *e = s_i18n_table; e->key != NULL; e++) {
        if (strcmp(e->key, key) == 0) {
            return (s_current_lang == HCN_LANG_ZH_CN) ? e->zh_CN : e->en_US;
        }
    }

    /* 找不到 → 返回 key 本身 (与 AWTK locale_info_tr 行为一致) */
    return key;
}

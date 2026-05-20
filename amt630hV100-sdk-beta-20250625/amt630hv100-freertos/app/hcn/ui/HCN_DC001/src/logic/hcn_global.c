/**
 * @file hcn_global.c
 * @brief 全局设置: 语言/单位/主题/里程 — LVGL 版本
 *
 * M029: 重写 AWTK 资源管理器依赖
 * - locale_info_change → 自建 i18n (桩, M028 完善)
 * - assets_set_global_theme → lv_theme 切换 (桩, M028 完善)
 * - assets_manager/image_manager → 移除 (LVGL 用 lv_img_dsc_t)
 * - gloabl_load_image → LVGL 不支持运行时动态加载图片到资源系统
 *   蓝牙封面改为 lv_img_set_src + 内存 buffer
 */

#include "hcn_global.h"
#include "view/set_view/set_view_interface.h"
#include "view/home_view/home_view_interface.h"
#include "proxy/vehicle_argument.h"
#include "proxy/vehicle_data.h"
#include "proxy/vehicle_mile.h"
#include "hcn_logic.h"
#include <string.h>

/* ======================================================================
 * 主题/语言 桩实现 (M028 完善)
 * ====================================================================== */

static uint8_t s_current_theme = 0; /* 0=day, 1=night */
static uint8_t s_current_lang  = 0; /* 0=zh_CN, 1=en_US */

static const char* country_language_str[LANGUAGE_OPTION_MAX] = {
    "zh_CN" , "en_US"
};

ret_t global_refresh_unit(uint8_t unit)
{
    int32_t value ;
    value = vehicle_get_data_speed() ;
    if (MPH == vehicle_get_param_unit())
            value *= KM_CONVERT_MILE ;
    home_refresh_speed(value) ;
    home_refresh_unit(unit);

    global_refresh_mileage();
    home_refresh_mileage_unit(unit) ;

    home_refresh_electrical_unit(unit);

    return RET_OK ;
}

ret_t global_refresh_language(uint8_t value)
{
    if (value > LANGUAGE_OPTION_MAX)
        return RET_FAIL ;

    s_current_lang = value;

    /**
     * AWTK: locale_info_change(locale_info(), language, country)
     * LVGL: M028 实现 — 调用 hcn_i18n_set_lang()
     */
    extern void hcn_i18n_set_lang(uint8_t lang);
    hcn_i18n_set_lang(value);

    printf("[hcn_global] language set to: %s\n", country_language_str[value]);

    return RET_OK ;
}

ret_t global_refresh_display(uint8_t value)
{
    if (DIAPLAY_AUTO_OPTION == value)
        return RET_OK ;

    if (value == s_current_theme) {
        printf("[hcn_global] theme unchanged (%d)\n", value);
        return RET_OK;
    }

    s_current_theme = value;

    /**
     * AWTK: assets_set_global_theme("night" / "default")
     * LVGL: M028 实现 — 调用 ui_theme_set_day/night
     */
    extern void ui_theme_set_day(void);
    extern void ui_theme_set_night(void);

    if (value == DIAPLAY_NIGHT_OPTION) {
        ui_theme_set_night();
    } else {
        ui_theme_set_day();
    }

    printf("[hcn_global] theme -> %s\n",
           value == DIAPLAY_NIGHT_OPTION ? "night" : "day");

    return RET_OK;
}

ret_t global_data_init(const timer_info_t *info)
{
    (void)info ;

    static bool is_init_usr_param  = false ;
    static bool is_init_mile_param = false ;

    if (!is_init_usr_param && (true == vehicle_get_param_recovery()))
    {
        uint8_t value  ;
        value = vehicle_get_param_language();
        global_refresh_language(value) ;

        value = vehicle_get_param_unit() ;
        global_refresh_unit(value);

        value = vehicle_get_param_display();
        if (0 == value)
            global_refresh_display(DIAPLAY_DAY_OPTION);
        else if(1 == value)
            global_refresh_display(DIAPLAY_NIGHT_OPTION);
        else
            global_refresh_display(vehicle_get_data_current_display()) ;

        value =  vehicle_get_data_gear() ;
        home_refresh_gear(value) ;

        value = vehicle_get_data_drv_mode() ;
        home_refresh_drv_mode(value) ;

        refresh_ver(veicle_get_data_version());

        is_init_usr_param = true ;
        printf("vehicle_get_param_recovery successed %s : %d\n" ,__FUNCTION__ , __LINE__);
    }

    if (!is_init_mile_param  && (true == vehicle_get_mile_recovery()) )
    {
        global_refresh_mileage();
        is_init_mile_param = true ;
        printf("vehicle_get_mile_recovery successed %s : %d\n" ,__FUNCTION__ , __LINE__);
    }

    if (is_init_usr_param && is_init_mile_param)
    {
        return RET_REMOVE ;
    }

    return RET_REPEAT ;
}


void global_refresh_mileage()
{
    uint32_t u32_odo   = vehicle_get_mile_odo()  ;
    uint32_t u32_tripA = vehicle_get_mile_tripA();
    uint32_t u32_tripB = vehicle_get_mile_tripB();
    uint32_t u32_once  = vehicle_get_mile_once() ;
    if (MPH == vehicle_get_param_unit())
    {
        u32_odo   *= KM_CONVERT_MILE ;
        u32_tripA *= KM_CONVERT_MILE ;
        u32_tripB *= KM_CONVERT_MILE ;
        u32_once  *= KM_CONVERT_MILE ;
    }
    home_refresh_odo ((double)u32_odo) ;
    home_refresh_trip((double)u32_tripA) ;
    home_refresh_info_distance(u32_once) ;

    return ;
}


/* UTF-8 辅助函数 — 纯算法, 无 AWTK 依赖 */
static int is_utf8_head(char c)
{
    return ((c & 0xE0) == 0xC0) || ((c & 0xF0) == 0xE0) || ((c & 0xF8) == 0xF0);
}

static int utf8_char_len(char c)
{
    if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;
    else return 1;
}

bool truncate_utf8_string(char* str , int intercept_length)
{
    int len = strlen(str);
    int i;
    int byte_count = 0;

    if((len > intercept_length)
        && (len < APP_MESSAGE_CONTENT_INFO_LEN))
    {
        for (i = 0; i < len && byte_count < intercept_length; i += utf8_char_len(str[i]))
        {
            if ((is_utf8_head(str[i]))
                && (byte_count + utf8_char_len(str[i]) > intercept_length))
            {
                break;
            }
            byte_count += utf8_char_len(str[i]);
        }
        str[byte_count ]    = '.';
        str[byte_count + 1] = '.';
        str[byte_count + 2] = '.';
        str[byte_count + 3] = '\0';
        return true;
    }
    else if(len <= intercept_length)
    {
        return true;
    }

    return false;
}

/**
 * gloabl_load_image — 蓝牙音乐封面动态加载
 *
 * AWTK: assets_manager_add_data → image_manager → widget刷新
 * LVGL: 不支持运行时往资源系统添加图片
 *
 * LVGL 方案: 将 PNG buffer 解码到 lv_img_dsc_t, 然后 lv_img_set_src
 * 需要 lv_png 解码支持 (已在 main_hcn_lvgl.c 中 lv_png_init())
 *
 * TODO M032: 实现 music_view 中的封面显示
 */
ret_t gloabl_load_image(uint8_t *buff, uint32_t length)
{
    if (NULL == buff || 0 == length)
        return RET_FAIL ;

    printf("[hcn_global] gloabl_load_image len=%u (TODO M032)\n", length);

    /* TODO M032: decode PNG buffer → lv_img_dsc_t → lv_img_set_src */

    return RET_OK ;
}

#include "hcn_global.h"
#include "view/set_view/set_view_interface.h"
#include "view/home_view/home_view_interface.h"

static const char* country_language_str[LANGUAGE_OPTION_MAX] = {
    "zh_CN" , "en_US" 
};

ret_t global_refresh_unit(uint8_t unit)
{
    //速度
    // home_refresh_speed(uint32_t speed) ;
    home_refresh_unit(unit);

    // 小计
    // home_refresh_trip(double trip) ;
    // home_refresh_odo(double odo) ;

    home_refresh_mileage_unit(unit) ;

    //电量
    home_refresh_electrical_unit(unit);

    //info窗口
    home_refresh_info_distance(0);


    return RET_OK ;
}

ret_t global_refresh_language(uint8_t value) 
{
    if (value > LANGUAGE_OPTION_MAX)
        return RET_FAIL ;
    
    char country [3] = {0};
    char language[3] = {0};
    strncpy(language, country_language_str[value] , 2);
    strncpy(country , country_language_str[value] + 3, 2);

    locale_info_change(locale_info(), language, country) ;

    return RET_OK ;
}
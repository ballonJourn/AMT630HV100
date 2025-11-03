#include "hcn_global.h"
#include "view/set_view/set_view_interface.h"
#include "view/home_view/home_view_interface.h"
#include "proxy/vehicle_argument.h"
#include "proxy/vehicle_data.h"
#include "proxy/vehicle_mile.h"
#include "hcn_logic.h"

static const char* country_language_str[LANGUAGE_OPTION_MAX] = {
    "zh_CN" , "en_US" 
};

ret_t global_refresh_unit(uint8_t unit)
{
    //速度
    int32_t value ;
    value = vehicle_get_data_speed() ;
    if (MPH == vehicle_get_param_unit())
            value *= KM_CONVERT_MILE ; 
    home_refresh_speed(value) ;
    home_refresh_unit(unit);

    // 里程
    global_refresh_mileage();
    home_refresh_mileage_unit(unit) ;

    //电量
    // home_refresh_electrical(uint32_t mileage) 
    home_refresh_electrical_unit(unit);

    //info窗口 参数需改
    // uint32_t u32_vlaue =  vehicle_get_mile_once();
    // if (MPH == vehicle_get_param_unit())
    //         u32_vlaue *= KM_CONVERT_MILE ; 
    // home_refresh_info_distance(u32_vlaue);


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

ret_t global_data_init(const timer_info_t *info)
{
    // printf("===================");
    (void)info ;
    
    static bool is_init_usr_param  = false ;
    static bool is_init_mile_param = false ;
    
    if (!is_init_usr_param && (true == vehicle_get_param_recovery()))
    {
        // 设置语言
        uint8_t value  ;
        value = vehicle_get_param_language();
        global_refresh_language(value) ;
        
        // 设置单位 // 设置里程程息 、 剩余里程
        value = vehicle_get_param_unit() ;
        global_refresh_unit(value);
        
        // 档位 
        home_refresh_gear(GEAR_N) ;
        
        // 驾驶模式
        home_refresh_drv_mode(DRV_MODE_E) ;
        
        is_init_usr_param = true ; 

        printf("vehicle_get_param_recovery successed %s : %d\n" ,__FUNCTION__ , __LINE__);
    }


    if (!is_init_mile_param  && (true == vehicle_get_mile_recovery()) )
    {
        // 里程
        global_refresh_mileage();

        is_init_mile_param = true ;

        printf("vehicle_get_mile_recovery successed %s : %d\n" ,__FUNCTION__ , __LINE__);
    }


    if (is_init_usr_param && is_init_mile_param)
    {
        // info->ctx = 0 ;  //(timerID)
        // clean_timer_ID(REFRESH_TIMER_100_MS);
        return RET_REMOVE ;
    }
    
    return RET_REPEAT ;

}


void global_refresh_mileage()
{
    // 里程
    uint32_t u32_odo   = vehicle_get_mile_odo()  ;
    uint32_t u32_tripA = vehicle_get_mile_tripA();
    uint32_t u32_tripB = vehicle_get_mile_tripB();
    uint32_t u32_once = vehicle_get_mile_once() ;
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

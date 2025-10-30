#include "vehicle_argument.h"
#include "vehicle_data.h"
#include "storage_param1/hcn_usr_param.h"
#include "backlight/hcn_backlight.h"

//语言

uint8_t vehicle_get_param_language()
{
    uint8_t value ;
#if !ON_PC_CACLE
    if (get_hcn_usr_param(HCN_PARAM_LANGUAGE, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_param_language(uint8_t value)
{
#if !ON_PC_CACLE
    set_hcn_usr_param(HCN_PARAM_LANGUAGE, (void*)&value) ;
#endif
    return ;
}

//单位
uint8_t vehicle_get_param_unit()
{
    uint8_t value ;
#if !ON_PC_CACLE
    if (get_hcn_usr_param(HCN_PARAM_MILE_FORMAT, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_param_unit(uint8_t value)
{
#if !ON_PC_CACLE
    set_hcn_usr_param(HCN_PARAM_MILE_FORMAT, (void*)&value) ;
#endif
    return ;
}

//亮度等级
uint8_t vehicle_get_param_brightness()
{
    uint8_t value ;
#if !ON_PC_CACLE
    if (get_hcn_usr_param(HCN_PARAM_BRIGHTNESS_LEVEL, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_param_brightness(uint8_t value)
{
#if !ON_PC_CACLE
    set_hcn_usr_param(HCN_PARAM_BRIGHTNESS_LEVEL, (void*)&value) ;
    set_backlight_level(value);
#endif
    
    return ;
}

//主题
uint8_t vehicle_get_param_display()
{
    uint8_t value ;
#if !ON_PC_CACLE
    if (get_hcn_usr_param(HCN_PARAM_THEME, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_param_display(uint8_t value)
{
#if !ON_PC_CACLE
    set_hcn_usr_param(HCN_PARAM_THEME, (void*)&value) ;
#endif
    return ;
}

//蓝牙开关
uint8_t vehicle_get_param_bluetooth()
{
    uint8_t value ;
#if !ON_PC_CACLE
    if (get_hcn_usr_param(HCN_PARAM_BT_SWITCH, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_param_bluetooth(uint8_t value)
{
#if !ON_PC_CACLE
    set_hcn_usr_param(HCN_PARAM_BT_SWITCH, (void*)&value) ;
#endif
    return ;
}

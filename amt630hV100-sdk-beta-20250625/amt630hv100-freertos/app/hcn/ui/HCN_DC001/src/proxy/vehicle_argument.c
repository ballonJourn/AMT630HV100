#include "vehicle_argument.h"
#include "vehicle_data.h"
#include "storage_param1/hcn_usr_param.h"
#include "backlight/hcn_backlight.h"

///< false:参数未准备好 true:已准备好
bool vehicle_get_param_recovery()
{
#if !ON_PC_CACLE
    return get_recovery_usr_param() ;
#endif

    return true ;
}

///< 语言 0:中文 1:英文,预留位
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

///< 里程格式 0:公制 1：英制
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

//毫米波雷达开关
uint8_t vehicle_get_param_radar()
{
    uint8_t value ;
#if !ON_PC_CACLE
    if (get_hcn_usr_param(HCN_PARAM_RADAR_SWITCH, &value) )
        return value ;
#endif
    return 1 ; ///< 默认开启
}

void vehicle_set_param_radar(uint8_t value)
{
#if !ON_PC_CACLE
    set_hcn_usr_param(HCN_PARAM_RADAR_SWITCH, (void*)&value) ;
#endif

    ///< 同步控制雷达数据上传
#ifdef HCN_MMWAVE_RADAR_ENABLE
    extern int mmwave_radar_data_switch(uint8_t enable);
    mmwave_radar_data_switch(value);
#endif

    return ;
}

//手机互联类型 0:CarPlay 1:亿连
uint8_t vehicle_get_param_carlink_type()
{
    uint8_t value ;
#if !ON_PC_CACLE
    if (get_hcn_usr_param(HCN_PARAM_CARLINK_TYPE, &value) )
        return value ;
#endif
    return 0 ; ///< 默认CarPlay
}

void vehicle_set_param_carlink_type(uint8_t value)
{
#if !ON_PC_CACLE
    set_hcn_usr_param(HCN_PARAM_CARLINK_TYPE, (void*)&value) ;
#endif
    return ;
}

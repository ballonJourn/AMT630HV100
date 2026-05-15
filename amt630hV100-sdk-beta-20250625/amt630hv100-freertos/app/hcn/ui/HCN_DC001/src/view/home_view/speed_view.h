#ifndef HOME_SPEED_VIEW_H
#define HOME_SPEED_VIEW_H
#include "./common.h"

enum home_speed_com{
    SPEED_HALO,
    SPEED_VALUE,
    SPEED_CRICLE,
    SPEED_POINTER,
    SPEED_UNIT,
    DRVING_MODE,
    GEAR_VIEW ,
    SPEED_NUM_MAX,
};


ret_t home_speed_view_init(widget_t* parent); 


//设置速度
ret_t home_refresh_speed(uint32_t speed);


//设置转速
ret_t home_refresh_rpm(uint32_t rpm) ;


//设置驾驶模式
ret_t home_refresh_drv_mode(drv_mode_e mode) ;


//设置档位
ret_t home_refresh_gear(gear_e gear);


//设置单位
ret_t home_refresh_unit(unit_e unit);

#endif
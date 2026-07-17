#ifndef LINK_VIEW_H__
#define LINK_VIEW_H__
#include "awtk.h"
#include "view/home_view/common.h"

enum link_view_com
{
    LINK_VIEW_SPEED         ,
    LINK_VIEW_UNIT          ,
    LINK_VIEW_RIDE_MODE     ,
    LINK_VIEW_GEAR_VIEW     ,
    LINK_VIEW_POEWR_LABEL   ,
    LINK_VIEW_POWER_BAR     ,
    LINK_VIEW_ELEC_LABEL    ,
    LINK_VIEW_ELEC_BAR      ,
    LINK_VIEW_ELEC_UNIT     ,
    LINK_VIEW_NUM_MAX       ,
};

ret_t link_view_init(widget_t* parent) ;

ret_t link_refresh_speed(uint32_t speed) ;

ret_t link_refresh_unit(unit_e unit) ;

ret_t link_refresh_drv_mode(drv_mode_e mode) ;

ret_t link_refresh_gear(gear_e gear) ;

ret_t link_refresh_power(int power) ;

ret_t link_refresh_electricalret_unit(unit_e unit) ;

ret_t link_refresh_electrical(uint32_t mileage) ;

ret_t link_refresh_qr(int state) ;

ret_t rest_data();

#endif

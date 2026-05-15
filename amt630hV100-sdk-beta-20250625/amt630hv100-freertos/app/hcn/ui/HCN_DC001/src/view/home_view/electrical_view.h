#ifndef HOME_ELECTRICAL_VIEW_H
#define HOME_ELECTRICAL_VIEW_H
#include "common.h"

enum home_elect_com{
    ELECT_BAR        ,
    ELECT_PERCENTAGE ,
    ELECT_VALUE      ,
    ELECT_UNIT       ,
    ELECT_NUM_MAX    ,
};

typedef enum  {
    NONE ,
    RED ,
    YELLOW ,
    GREEN ,
}fg_color;

ret_t home_elec_view_init(widget_t* parent);

ret_t home_refresh_electrical(uint32_t mileage);

ret_t home_refresh_electrical_unit(unit_e unit);


#endif
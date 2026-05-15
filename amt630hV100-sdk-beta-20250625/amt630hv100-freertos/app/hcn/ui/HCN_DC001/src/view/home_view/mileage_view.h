#ifndef HOME_MILEAGE_VIEW_H
#define HOME_MILEAGE_VIEW_H
#include "common.h"

enum home_mileage_com{
    TRIP_LABEL      ,
    TRIP_UNIT       ,
         
    ODO_LABEL       ,
    ODO_UNIT        ,

    MILEAGE_NUM_MAX ,
};

ret_t home_mileage_view_init(widget_t* parent);


ret_t home_refresh_trip(double trip) ;


ret_t home_refresh_odo(double odo) ;


ret_t home_refresh_mileage_unit(unit_e unit) ;


#endif //HOME_MILEAGE_VIEW_H
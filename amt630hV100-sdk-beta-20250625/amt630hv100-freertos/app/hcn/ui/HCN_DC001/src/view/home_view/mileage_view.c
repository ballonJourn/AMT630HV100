#include "mileage_view.h"

const char* home_mileage_widget_name[MILEAGE_NUM_MAX] = {
    "trip_label" , "trip_unit" , "odo_label", "odo_unit" 
} ;

static widget_t* home_mileage_widget[MILEAGE_NUM_MAX] = { NULL };

ret_t home_mileage_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < MILEAGE_NUM_MAX; i++){
        home_mileage_widget[i] = widget_lookup(parent, home_mileage_widget_name[i], TRUE);
    }
    return RET_OK ;
}

ret_t home_refresh_trip(double trip) 
{
    char mileage_value_str[16] = " " ;           
    tk_snprintf(mileage_value_str, sizeof(mileage_value_str) , "%.1f" , (float)(((int)((trip / 1000.0f) * 10 ) ) / 10.0f ) );

    if (home_mileage_widget[TRIP_LABEL])
    {
       widget_set_text_utf8(home_mileage_widget[TRIP_LABEL] , mileage_value_str);
    }
    
    return RET_OK ;
}

ret_t home_refresh_odo(double odo) 
{
    char mileage_value_str[16] = " " ;
    tk_snprintf(mileage_value_str, sizeof(mileage_value_str) , "%d" ,  (int)(odo / 1000) );     
    
    if (home_mileage_widget[ODO_LABEL])
    {
       widget_set_text_utf8(home_mileage_widget[ODO_LABEL] , mileage_value_str);
    }
    
    return RET_OK ;
}

ret_t home_refresh_mileage_unit(unit_e unit) 
{
    if (home_mileage_widget[TRIP_UNIT])
    {
       widget_set_text_utf8(home_mileage_widget[TRIP_UNIT] , (unit == KM_H) ? "km" : "mile" );
    }

    if (home_mileage_widget[ODO_UNIT])
    {
       widget_set_text_utf8(home_mileage_widget[ODO_UNIT] , (unit == KM_H) ? "km" : "mile" );
    }

    return RET_OK ;
}
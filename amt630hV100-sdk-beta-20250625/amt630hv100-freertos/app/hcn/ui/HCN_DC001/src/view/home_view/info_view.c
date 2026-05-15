#include "info_view.h"
#include "proxy/vehicle_argument.h"

const char* home_info_widget_name[INFO_NUM_MAX] = {
    "ride_time" , "ride_distance" 
} ;

static widget_t* home_info_widget[INFO_NUM_MAX] = { NULL };;


ret_t home_info_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;

    for (size_t i = 0; i < INFO_NUM_MAX; i++){
        home_info_widget[i] = widget_lookup(parent, home_info_widget_name[i], TRUE);
    }
    return RET_OK ;
}


ret_t home_refresh_info_time(uint64_t total_seconds) 
{
    uint32_t hours   = total_seconds / 3600 ;
    uint32_t minutes = (total_seconds % 3600) / 60 ;
    //uint32_t seconds = total_seconds % 60;

    char buff[64]  = { 0 };

    tk_snprintf(buff , sizeof(buff) - 1, "%dh %dmin" , hours , minutes) ;
    if (home_info_widget[INFO_TIME])
        widget_set_text_utf8(home_info_widget[INFO_TIME] , buff);
    
    return RET_OK ;
}

ret_t home_refresh_info_distance(uint32_t distance)
{
    char buff[16] = {0} ;

    tk_snprintf(buff , sizeof(buff) - 1, "%d %s" , (distance / 1000 ) , vehicle_get_param_unit() ? "mile" : "km" ) ;
    if (home_info_widget[INFO_DISTANCE])
        widget_set_text_utf8(home_info_widget[INFO_DISTANCE] , buff);

    return RET_OK ;
}
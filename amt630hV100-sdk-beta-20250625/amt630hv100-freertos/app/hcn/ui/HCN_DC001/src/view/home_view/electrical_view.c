#include "electrical_view.h"

const char* home_elec_widget_name[ELECT_NUM_MAX] = {
    "electrical_bar" , "electrical_percentage" , "electrical_value" , "electrical_unit" 
} ;

static widget_t* home_elec_widget[ELECT_NUM_MAX] = { NULL };


ret_t home_elec_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < ELECT_NUM_MAX; i++){
        home_elec_widget[i] = widget_lookup(parent, home_elec_widget_name[i], TRUE);
    }
    
    return RET_OK ;
}

ret_t home_refresh_electrical(uint32_t mileage) 
{
    float step = 100.0f / ELECTRI_MAX  ;
    int perent = (int)(step * mileage) ;
    char format[8] = " " ;
    tk_snprintf(format , sizeof(format) , "%d" , perent) ;

    if (home_elec_widget[ELECT_BAR]){
        progress_bar_set_value(home_elec_widget[ELECT_BAR] , perent) ;
    }

    if (home_elec_widget[ELECT_PERCENTAGE]){
        widget_set_text_utf8(home_elec_widget[ELECT_PERCENTAGE] , format);
    }
    
    tk_snprintf(format , sizeof(format) , "%d" , mileage) ;
    if (home_elec_widget[ELECT_VALUE]){
        widget_set_text_utf8(home_elec_widget[ELECT_VALUE] , format);
    }
    
    return RET_OK ;
}

ret_t home_refresh_electrical_unit(unit_e unit) 
{
    if (home_elec_widget[ELECT_UNIT]){
        widget_set_text_utf8(home_elec_widget[ELECT_UNIT] , (unit == KM_H) ? "km" : "mile" );
    }
    
    return RET_OK ;
}

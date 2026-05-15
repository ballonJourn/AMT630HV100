#include "electrical_view.h"
#include "proxy/vehicle_argument.h"
#include "logic/hcn_global.h"
#include "view/home_view/common.h"

const char* home_elec_widget_name[ELECT_NUM_MAX] = {
    "electrical_bar" , "electrical_percentage" , "electrical_value" , "electrical_unit" 
} ;

static widget_t* home_elec_widget[ELECT_NUM_MAX] = { NULL };

static uint32_t current_value = 0 ;

static const char* color_buff[] = {"#00000000" , "#FF0000" ,"#FFFF00" , "#00FF00"} ;

static fg_color current_color = NONE ;


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
    mileage = tk_min(mileage , ELECTRI_MAX) ;
    current_value = mileage ;
    float step = 100.0f / ELECTRI_MAX  ;
    int perent = (int)(step * mileage) ;
    char format[8] = " " ;
    tk_snprintf(format , sizeof(format) , "%d" , perent) ;

    fg_color _color = NONE ;
    if ( 0 <= perent && perent <= 10 )
        _color = RED ;
    else if( 10 < perent && perent <= 20 )
        _color = YELLOW ;
    else 
        _color = GREEN ;

    if (home_elec_widget[ELECT_BAR]){
        progress_bar_set_value(home_elec_widget[ELECT_BAR] , perent) ;

        if (current_color != _color)
        {
            widget_set_style_str(home_elec_widget[ELECT_BAR] , STYLE_ID_FG_COLOR , color_buff[_color]) ;
            current_color = _color ;
            // printf("elelctrical color changed\n") ;
        }
    
    }

    if (home_elec_widget[ELECT_PERCENTAGE]){
        widget_set_text_utf8(home_elec_widget[ELECT_PERCENTAGE] , format);
    }
    

    uint32_t temp_value = mileage ;
    if (home_elec_widget[ELECT_VALUE])
    {
        if (MPH == vehicle_get_param_unit())
            temp_value  *= KM_CONVERT_MILE ; 
        
        widget_set_value_int(home_elec_widget[ELECT_VALUE] , temp_value) ;
    
    }
    
    return RET_OK ;
}

ret_t home_refresh_electrical_unit(unit_e unit) 
{
    if (home_elec_widget[ELECT_UNIT]){
        widget_set_text_utf8(home_elec_widget[ELECT_UNIT] , (unit == KM_H) ? "km" : "mile" );
    }
    
    uint32_t temp_value = current_value ;
    if (home_elec_widget[ELECT_VALUE])
    {
        if (MPH == vehicle_get_param_unit())
        {
            temp_value  *= KM_CONVERT_MILE ; 
        }
        widget_set_value_int(home_elec_widget[ELECT_VALUE] , temp_value) ;

    }

    return RET_OK ;
}

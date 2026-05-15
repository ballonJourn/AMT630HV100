
#include "signal_view.h"

const char* home_signal_widget_name[ICON_SIGNAL_NUM_MAX] = {
    "icon_GMS" , "icon_gps" , "icon_bt" , "icon_high_beam" ,"icon_left" , "icon_ready" , "icon_right" , "icon_near_beam" ,
    "icon_abs" , "icon_ecu" , "icon_tcs", "icon_engine" 
} ;

static widget_t* home_signal_widget[ICON_SIGNAL_NUM_MAX] = { NULL };

ret_t home_signal_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < ICON_SIGNAL_NUM_MAX; i++){
        home_signal_widget[i] = widget_lookup(parent, home_signal_widget_name[i], TRUE);
    }
    return RET_OK ;
}


ret_t home_refresh_signal(signal_e icon , bool_t visible) 
{
    if (home_signal_widget[icon])
    {
        widget_set_visible(home_signal_widget[icon] , visible ? TRUE : FALSE) ;
    }
    
    return RET_OK ;
}


ret_t home_refresh_GMS_level(gms_level_e level )
{
    char format[64] ;
    level = tk_min_int(level , GMS_LEVEL_3) ;
    tk_snprintf(format , sizeof(format) , "top_signal_%d_n" , (int)level);

    if (home_signal_widget[ICON_GMS])
    {
        image_set_image(home_signal_widget[ICON_GMS] , format) ;
    }
    
    return RET_OK ;
}


ret_t home_refresh_signal_visible(bool_t visible )
{
    for (size_t i = 0; i < ICON_SIGNAL_NUM_MAX; i++){
        // if (i == ICON_GMS || i == ICON_GPS )   //不参与自检图表
        //     continue;
        
        if (home_signal_widget[i])
            widget_set_visible(home_signal_widget[i] , visible ? TRUE : FALSE) ;
    }
    return RET_OK ;
}
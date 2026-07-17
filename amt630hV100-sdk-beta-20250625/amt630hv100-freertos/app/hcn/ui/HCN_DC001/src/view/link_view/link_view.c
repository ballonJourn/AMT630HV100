#include "link_view.h"
#include "proxy/vehicle_argument.h"
#include "logic/hcn_global.h"
#include "view/home_view/electrical_view.h"


const char* link_view_widget_name[LINK_VIEW_NUM_MAX] = {
    "speed_lab" , "unit_lab"  , "ride_mode_img" , "gear_view" , 
    "power_lab" , "power_bar" , "elec_lab"      , "elec_bar"  , "elec_unit"
} ;

static char* unit_str[UNIT_MAX] = {"km/h" , "mph"} ;

static widget_t* link_view_widget[LINK_VIEW_NUM_MAX] = { NULL };

static widget_t* widget_qr = NULL ;

static const char* color_buff[] = {"#00000000" , "#FF0000" ,"#FFFF00" , "#00FF00"} ;

static fg_color current_color = NONE ;

static uint32_t current_value = 0 ;

ret_t link_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;

    for (size_t i = 0; i < LINK_VIEW_NUM_MAX; i++){
        link_view_widget[i] = widget_lookup(parent, link_view_widget_name[i], TRUE);
    }

    widget_qr = widget_lookup( parent, "link_qr", TRUE);

    return RET_OK;
}

ret_t link_refresh_speed(uint32_t speed)
{
    speed = tk_min(speed , SPEED_MAX) ;
    
    if(link_view_widget[LINK_VIEW_SPEED] ){
        widget_set_value_int(link_view_widget[LINK_VIEW_SPEED] , speed );
    }

    return RET_OK ;
}

ret_t link_refresh_unit(unit_e unit)
{
    unit = tk_min(unit , MPH) ;

    if(link_view_widget[LINK_VIEW_UNIT] ){
        widget_set_text_utf8(link_view_widget[LINK_VIEW_UNIT] , unit_str[unit]) ;
    }

    return RET_OK ;
}


ret_t link_refresh_drv_mode(drv_mode_e mode)
{
    char format_buff[64] = { 0 };
    mode = tk_min(mode , DRV_MODE_S) ;
    tk_snprintf(format_buff , sizeof(format_buff) , "drv_mode_%d", (int)mode);

    if(link_view_widget[LINK_VIEW_RIDE_MODE] ){
        image_set_image(link_view_widget[LINK_VIEW_RIDE_MODE] , format_buff );
    }

    return RET_OK ;
}

ret_t link_refresh_gear(gear_e gear)
{
   if (link_view_widget[LINK_VIEW_GEAR_VIEW] == NULL || (gear > GEAR_R)) 
        return RET_FAIL ;

   widget_t *gearWid = link_view_widget[LINK_VIEW_GEAR_VIEW] ;

   int count = widget_count_children(gearWid);

   widget_t *children = NULL ;
   for (size_t i = 0; i < count; i++)
   {
      children = widget_get_child(gearWid,i);
      if (gear == i ){
         widget_set_state(children , STATE_SELECTE);
      }else{
         widget_set_state(children , STATE_NORMAL);
      }
   }

   slide_menu_set_value(gearWid, gear);
   // slide_menu_scroll_to_next(gearWid);

   return RET_OK ;
}


ret_t link_refresh_power(int power) 
{
    if(link_view_widget[LINK_VIEW_POEWR_LABEL]){
       widget_set_value_int(link_view_widget[LINK_VIEW_POEWR_LABEL] , power) ;
    }

    if(link_view_widget[LINK_VIEW_POWER_BAR]){
        slider_set_value(link_view_widget[LINK_VIEW_POWER_BAR] , power) ;
    }

    return RET_OK ;
}


ret_t link_refresh_electricalret_unit(unit_e unit) 
{
    if (link_view_widget[LINK_VIEW_ELEC_UNIT]){
        widget_set_text_utf8(link_view_widget[LINK_VIEW_ELEC_UNIT] , (unit == KM_H) ? "km" : "mile" );
    }
    
    uint32_t temp_value = current_value ;
    if (link_view_widget[LINK_VIEW_ELEC_LABEL])
    {
        if (MPH == vehicle_get_param_unit())
            temp_value  *= KM_CONVERT_MILE ; 

        widget_set_value_int(link_view_widget[LINK_VIEW_ELEC_LABEL] , temp_value);
    }


    return RET_OK ;
}

ret_t link_refresh_electrical(uint32_t mileage) 
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

    if (link_view_widget[LINK_VIEW_ELEC_BAR])
    {
        
        if (current_color != _color)
        {
            widget_set_style_str(link_view_widget[LINK_VIEW_ELEC_BAR] , STYLE_ID_FG_COLOR , color_buff[_color]) ;
            current_color = _color ;
            // printf("elelctrical color changed\n") ;
        }
        progress_bar_set_value(link_view_widget[LINK_VIEW_ELEC_BAR] , perent) ;
    
    }

    uint32_t temp_value = mileage ;
    if (link_view_widget[LINK_VIEW_ELEC_LABEL])
    {
        if (MPH == vehicle_get_param_unit())
            temp_value  *= KM_CONVERT_MILE ; 

        widget_set_value_int(link_view_widget[LINK_VIEW_ELEC_LABEL] , temp_value);
    }
    
    
    return RET_OK ;
}

ret_t rest_data()
{
    current_color = NONE ;
    current_value = 0 ;
    
    return RET_OK ;
}

ret_t link_refresh_qr(int state)
{
    if (widget_qr)
        widget_set_visible(widget_qr , state ? true : false) ;
    
    return RET_OK ;
}

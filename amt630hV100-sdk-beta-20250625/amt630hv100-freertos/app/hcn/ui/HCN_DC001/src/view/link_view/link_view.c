#include "link_view.h"
#include "../home_view/common.h"

typedef enum 
{
    NONE  ,
    GREEN ,
    RED   ,
}bar_fg_color ;

static  bar_fg_color cur_color = NONE ;

const char* link_view_widget_name[LINK_VIEW_NUM_MAX] = {
    "speed_lab" , "unit_lab"  , "ride_mode_img" , "gear_view" , 
    "power_lab" , "power_bar" , "elec_lab"      , "elec_bar"  
} ;

static char* unit_str[UNIT_MAX] = {"km/h" , "mph"} ;

static widget_t* link_view_widget[LINK_VIEW_NUM_MAX] = { NULL };

static widget_t* widget_qr = NULL ;

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
    unit = tk_min(unit , UNIT_MAX) ;

    if(link_view_widget[LINK_VIEW_UNIT] ){
        widget_set_text_utf8(link_view_widget[LINK_VIEW_UNIT] , unit_str[unit]) ;
    }

    return RET_OK ;
}


ret_t link_refresh_drv_mode(drv_mode_e mode)
{
    char format_buff[64] = { 0 };
    mode = tk_min(mode , DRV_MODE_MAX) ;
    tk_snprintf(format_buff , sizeof(format_buff) , "drv_mode_%d", (int)mode);

    if(link_view_widget[LINK_VIEW_RIDE_MODE] ){
        image_set_image(link_view_widget[LINK_VIEW_RIDE_MODE] , format_buff );
    }

    return RET_OK ;
}

ret_t link_refresh_gear(gear_e gear)
{
   if (link_view_widget[LINK_VIEW_GEAR_VIEW] == NULL) return RET_FAIL ;

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


ret_t link_refresh_electrical(uint32_t mileage) 
{
    // static bar_color = 
    float step = 100.0f / ELECTRI_MAX  ;
    int perent = (int)(step * mileage) ;

    bar_fg_color _color = ( perent > 20 ) ? (GREEN) : (RED) ;
    if (cur_color != _color)
    {
        widget_set_style_str(link_view_widget[LINK_VIEW_ELEC_BAR] , STYLE_ID_FG_COLOR , _color ==  GREEN ? "#00FF55" : "#FF0000") ;
        cur_color = _color ;
    }
    
    if (link_view_widget[LINK_VIEW_ELEC_BAR]){
        progress_bar_set_value(link_view_widget[LINK_VIEW_ELEC_BAR] , perent) ;
    }

    if (link_view_widget[LINK_VIEW_ELEC_LABEL]){
        widget_set_value_int(link_view_widget[LINK_VIEW_ELEC_LABEL] , mileage);
    }


    return RET_OK ;
}

ret_t link_refresh_qr(int state)
{
    if (widget_qr)
        widget_set_visible(widget_qr , state ? true : false) ;
    
    return RET_OK ;
}

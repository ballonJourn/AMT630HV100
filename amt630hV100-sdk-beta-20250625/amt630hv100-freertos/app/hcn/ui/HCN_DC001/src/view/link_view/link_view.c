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
    tk_snprintf(format_buff , sizeof(format_buff) , "link_drv_mode_%d", (int)mode);

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

   /* gear_view is a plain view (not slide_menu) so all 3 gears are always
    * visible simultaneously.  widget_set_state() above handles highlighting. */

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


/* ═══════════════════════════════════════════════════════════════════════
 * Top bar: clock + signal icons
 * Widget names prefixed with "link_" to avoid collision with home_page
 * ═══════════════════════════════════════════════════════════════════════ */

enum link_clock_com { LK_CLK_MIN, LK_CLK_COLON, LK_CLK_SEC, LK_CLK_AMPM, LK_CLK_MAX };
static const char* lk_clock_names[LK_CLK_MAX] = {
    "link_time_min", "link_time_colon", "link_time_sec", "link_time_ampm"
};
static widget_t* lk_clock_w[LK_CLK_MAX] = { NULL };

enum link_signal_com {
    LK_SIG_GMS, LK_SIG_GPS, LK_SIG_BT, LK_SIG_HIGH_BEAM,
    LK_SIG_LEFT, LK_SIG_READY, LK_SIG_RIGHT, LK_SIG_NEAR_BEAM,
    LK_SIG_ABS, LK_SIG_ECU, LK_SIG_TCS, LK_SIG_ENGINE, LK_SIG_RADAR,
    LK_SIG_MAX
};
static const char* lk_signal_names[LK_SIG_MAX] = {
    "link_icon_GMS", "link_icon_gps", "link_icon_bt", "link_icon_high_beam",
    "link_icon_left", "link_icon_ready", "link_icon_right", "link_icon_near_beam",
    "link_icon_abs", "link_icon_ecu", "link_icon_tcs", "link_icon_engine", "link_icon_radar"
};
static widget_t* lk_signal_w[LK_SIG_MAX] = { NULL };

ret_t link_topbar_init(widget_t* parent)
{
    if (parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < LK_CLK_MAX; i++)
        lk_clock_w[i] = widget_lookup(parent, lk_clock_names[i], TRUE);
    for (size_t i = 0; i < LK_SIG_MAX; i++)
        lk_signal_w[i] = widget_lookup(parent, lk_signal_names[i], TRUE);
    return RET_OK;
}

ret_t link_topbar_refresh_clock(int hour, int min, bool_t colon_visible, uint8_t time_fmt)
{
    char buff[4];
    int display_hour = hour;

    /* 12-hour conversion */
    if (time_fmt == 1) {
        if (hour == 0) display_hour = 12;
        else if (hour > 12) display_hour = hour - 12;
    }

    tk_snprintf(buff, sizeof(buff), "%02d", display_hour);
    if (lk_clock_w[LK_CLK_MIN])
        widget_set_text_utf8(lk_clock_w[LK_CLK_MIN], buff);

    tk_snprintf(buff, sizeof(buff), "%02d", min);
    if (lk_clock_w[LK_CLK_SEC])
        widget_set_text_utf8(lk_clock_w[LK_CLK_SEC], buff);

    if (lk_clock_w[LK_CLK_COLON])
        widget_set_visible(lk_clock_w[LK_CLK_COLON], colon_visible);

    if (lk_clock_w[LK_CLK_AMPM]) {
        if (time_fmt == 1) {
            widget_set_visible(lk_clock_w[LK_CLK_AMPM], TRUE);
            widget_set_text_utf8(lk_clock_w[LK_CLK_AMPM], (hour < 12) ? "AM" : "PM");
        } else {
            widget_set_visible(lk_clock_w[LK_CLK_AMPM], FALSE);
        }
    }

    return RET_OK;
}

ret_t link_topbar_refresh_signal(int index, bool_t visible)
{
    if (index >= 0 && index < LK_SIG_MAX && lk_signal_w[index])
        widget_set_visible(lk_signal_w[index], visible ? TRUE : FALSE);
    return RET_OK;
}


/* ═══════════════════════════════════════════════════════════════════════
 * Bottom bar: TRIP / ODO / electrical percentage + progress + range
 * ═══════════════════════════════════════════════════════════════════════ */

enum link_bottom_com {
    LK_BOT_TRIP_LABEL, LK_BOT_TRIP_UNIT,
    LK_BOT_ODO_LABEL, LK_BOT_ODO_UNIT,
    LK_BOT_ELEC_BAR, LK_BOT_ELEC_PERCENT, LK_BOT_ELEC_RANGE, LK_BOT_ELEC_RANGE_UNIT,
    LK_BOT_MAX
};
static const char* lk_bottom_names[LK_BOT_MAX] = {
    "link_trip_label", "link_trip_unit",
    "link_odo_label", "link_odo_unit",
    "link_electrical_bar", "link_elec_percentage", "link_elec_range_value", "link_elec_range_unit"
};
static widget_t* lk_bottom_w[LK_BOT_MAX] = { NULL };

static uint32_t lk_bot_elec_value = 0;

ret_t link_bottombar_init(widget_t* parent)
{
    if (parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < LK_BOT_MAX; i++)
        lk_bottom_w[i] = widget_lookup(parent, lk_bottom_names[i], TRUE);
    return RET_OK;
}

ret_t link_bottombar_refresh_trip(double trip)
{
    if (lk_bottom_w[LK_BOT_TRIP_LABEL]) {
        char buf[16];
        tk_snprintf(buf, sizeof(buf), "%.1f", trip);
        widget_set_text_utf8(lk_bottom_w[LK_BOT_TRIP_LABEL], buf);
    }
    return RET_OK;
}

ret_t link_bottombar_refresh_odo(double odo)
{
    if (lk_bottom_w[LK_BOT_ODO_LABEL]) {
        char buf[16];
        tk_snprintf(buf, sizeof(buf), "%d", (int)odo);
        widget_set_text_utf8(lk_bottom_w[LK_BOT_ODO_LABEL], buf);
    }
    return RET_OK;
}

ret_t link_bottombar_refresh_mileage_unit(unit_e unit)
{
    const char* u = (unit == KM_H) ? "km" : "mile";
    if (lk_bottom_w[LK_BOT_TRIP_UNIT])
        widget_set_text_utf8(lk_bottom_w[LK_BOT_TRIP_UNIT], u);
    if (lk_bottom_w[LK_BOT_ODO_UNIT])
        widget_set_text_utf8(lk_bottom_w[LK_BOT_ODO_UNIT], u);
    return RET_OK;
}

ret_t link_bottombar_refresh_electrical(uint32_t mileage)
{
    mileage = tk_min(mileage, ELECTRI_MAX);
    lk_bot_elec_value = mileage;
    float step = 100.0f / ELECTRI_MAX;
    int percent = (int)(step * mileage);
    char buf[8];

    if (lk_bottom_w[LK_BOT_ELEC_BAR])
        progress_bar_set_value(lk_bottom_w[LK_BOT_ELEC_BAR], percent);

    if (lk_bottom_w[LK_BOT_ELEC_PERCENT]) {
        tk_snprintf(buf, sizeof(buf), "%d", percent);
        widget_set_text_utf8(lk_bottom_w[LK_BOT_ELEC_PERCENT], buf);
    }

    if (lk_bottom_w[LK_BOT_ELEC_RANGE]) {
        uint32_t temp = mileage;
        if (MPH == vehicle_get_param_unit())
            temp *= KM_CONVERT_MILE;
        widget_set_value_int(lk_bottom_w[LK_BOT_ELEC_RANGE], temp);
    }

    return RET_OK;
}

ret_t link_bottombar_refresh_electrical_unit(unit_e unit)
{
    if (lk_bottom_w[LK_BOT_ELEC_RANGE_UNIT])
        widget_set_text_utf8(lk_bottom_w[LK_BOT_ELEC_RANGE_UNIT], (unit == KM_H) ? "km" : "mile");

    /* re-apply mileage with new unit */
    if (lk_bottom_w[LK_BOT_ELEC_RANGE]) {
        uint32_t temp = lk_bot_elec_value;
        if (MPH == vehicle_get_param_unit())
            temp *= KM_CONVERT_MILE;
        widget_set_value_int(lk_bottom_w[LK_BOT_ELEC_RANGE], temp);
    }

    return RET_OK;
}
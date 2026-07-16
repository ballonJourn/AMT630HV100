#include "speed_view.h"
#include "../../logic/hcn_selfcheck.h"

const char* home_speed_widget_name[SPEED_NUM_MAX] = {
   "bg_halo" , "speed_value" , "progress_circle" , "dashboard_pointer" ,"speed_unit" , "driving_mode" , "gear_view"
} ;

static widget_t* home_speed_widget[SPEED_NUM_MAX] = { NULL };

ret_t home_speed_view_init(widget_t* parent)
{
   if(parent == NULL) return RET_FAIL;
   for (size_t i = 0; i < SPEED_NUM_MAX; i++){
      home_speed_widget[i] = widget_lookup(parent, home_speed_widget_name[i], TRUE);
   }
   return RET_OK ;
}


ret_t home_refresh_speed(uint32_t speed)
{
   speed = tk_min(speed , SPEED_MAX) ;
   
   uint32_t duration = 300 ;

   if (checkself_get_state() == CHECK_STATE_CHECKING){
      duration = 30 ;
   }

   if(home_speed_widget[SPEED_VALUE] ){
      widget_animate_value_to(home_speed_widget[SPEED_VALUE], speed  , duration);
   }

   return RET_OK ;
}


ret_t home_refresh_rpm(uint32_t rpm)
{
   rpm = tk_min(rpm , RPM_MAX) ;

   float step = (float)(ANGLE_MAX * (1.0f)) / RPM_MAX  ;
   uint32_t duration = 300 ;

   if (checkself_get_state() == CHECK_STATE_CHECKING){
      duration = 30 ;
   }
   
   int startAngle = -135 ;
   if(home_speed_widget[SPEED_CRICLE] ){
      widget_animate_value_to(home_speed_widget[SPEED_CRICLE] ,  step * rpm , duration );
   }

   if(home_speed_widget[SPEED_POINTER] ){
      widget_animate_value_to(home_speed_widget[SPEED_POINTER] ,  step * rpm + startAngle , duration );
   }

   return RET_OK ;
}


ret_t home_refresh_drv_mode(drv_mode_e mode)
{
   mode = tk_min(mode , DRV_MODE_S);
   
   char format_buff[64] = { 0 };
   tk_snprintf(format_buff , sizeof(format_buff) , "bg_halo_%d", (int)mode);

   if(home_speed_widget[SPEED_HALO] ){
      image_set_image(home_speed_widget[SPEED_HALO] , format_buff );
   }

   memset(format_buff , 0x0 , sizeof(format_buff)) ;
   tk_snprintf(format_buff , sizeof(format_buff) , "drv_mode_%d", (int)mode);

   if(home_speed_widget[DRVING_MODE] ){
      image_set_image(home_speed_widget[DRVING_MODE] , format_buff );
   }

   memset(format_buff , 0x0 , sizeof(format_buff)) ;
   tk_snprintf(format_buff , sizeof(format_buff) , "dashboard_progress_%d", (int)mode);

   if(home_speed_widget[SPEED_CRICLE] ){
      widget_set_style_str(home_speed_widget[SPEED_CRICLE], "normal:fg_image" , format_buff);
      widget_invalidate_force(home_speed_widget[SPEED_CRICLE], NULL);
   }

   memset(format_buff , 0x0 , sizeof(format_buff)) ;
   tk_snprintf(format_buff , sizeof(format_buff) , "pointer_%d", (int)mode);

   if(home_speed_widget[SPEED_POINTER] ){
      gauge_pointer_set_image(home_speed_widget[SPEED_POINTER],  format_buff);
   }

   
   return RET_OK ;
}


ret_t home_refresh_unit(unit_e unit)
{
   char format_buff[128] = { 0 };
   tk_snprintf(format_buff , sizeof(format_buff) , "%s", (unit == KM_H) ? "unit_km_h" : "unit_mph");

   if(home_speed_widget[SPEED_UNIT] ){
      image_set_image(home_speed_widget[SPEED_UNIT] , format_buff );
   }

   return RET_OK ;
}


ret_t home_refresh_gear(gear_e gear)
{
   if (home_speed_widget[GEAR_VIEW] == NULL || (gear > GEAR_R))
      return RET_FAIL ;

   widget_t *gearWid = home_speed_widget[GEAR_VIEW] ;

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


ret_t home_drv_mode_set_visible(bool_t visible)
{
   if(home_speed_widget[DRVING_MODE] ){
      widget_set_visible(home_speed_widget[DRVING_MODE] , visible) ;
   }
   return RET_OK ;
}
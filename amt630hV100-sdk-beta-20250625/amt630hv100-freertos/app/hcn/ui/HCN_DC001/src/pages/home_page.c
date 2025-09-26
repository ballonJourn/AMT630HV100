#include "awtk.h"
#include "../common/navigator.h"
#include "../logic/hcn_logic.h"
#include "../view/view_manager.h"
/**
 * 初始化窗口的子控件
 */
// typedef ret_t (*timer_func_t)(const timer_info_t* timer);

ret_t refesh_ui(const timer_info_t* timer){
  static int count = 0 ;
  count++;
  home_refresh_drv_mode(count % 3) ;

  home_refresh_dock_item(count % 5) ;

  home_refresh_gear(count % 3) ;

  home_refresh_signal_visible((count % 2 == 0)) ;

  home_refresh_trip(1888.6) ;
  home_refresh_odo(1888.6) ;
  home_refresh_mileage_unit(MPH) ;

  
  deal_key_down_short_press() ;

  // home_refresh_speed(count);

  // if (count % 2 == 0)
  // {
  //   animation_play_out() ;
  // }else
  // {
  //   animation_play_in() ;
  // }
  

  return RET_REPEAT;
}

static ret_t visit_init_child(void* ctx, const void* iter) {
  widget_t* widget = WIDGET(iter);
  (void)ctx;
  const char* name = widget->name;

  // 初始化指定名称的控件（设置属性或注册事件），请保证控件名称在窗口上唯一
  if (name != NULL && *name != '\0') {

  }

  return RET_OK;
}

/**
 * 初始化窗口
 */
ret_t home_page_init(widget_t* win, void* ctx) {
  (void)ctx;
  return_value_if_fail(win != NULL, RET_BAD_PARAMS);

  widget_foreach(win, visit_init_child, win);
#if 0
  home_speed_view_init(win);
  home_dock_view_init(win) ;
  home_signal_view_init(win);
  home_mileage_view_init(win) ;
  home_elec_view_init(win) ;
  view_manager_init(win) ;
  home_animation_init(win) ;

#endif 
  demonstration_stop() ;
  home_view_init(win) ;

  timer_add(refesh_ui,NULL , 2000);

  return RET_OK;
}







#include "awtk.h"
#include "../common/navigator.h"
#include "../view/home_view/speed_view.h"
#include "../view/home_view/dock_view.h"
#include "../view/home_view/mileage_view.h"
/**
 * 初始化窗口的子控件
 */
// typedef ret_t (*timer_func_t)(const timer_info_t* timer);
ret_t home_speed_view_init(widget_t* parent); 

ret_t home_refresh_speed(uint32_t speed);

ret_t home_refresh_rpm(uint32_t rpm) ;
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


  home_refresh_electrical(90);

  home_refresh_electrical_unit(MPH);
  return RET_REPEAT;
}

static ret_t visit_init_child(void* ctx, const void* iter) {
  widget_t* win = WIDGET(ctx);
  widget_t* widget = WIDGET(iter);
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

  home_speed_view_init(win);
  home_dock_view_init(win) ;
  home_signal_view_init(win);
  home_mileage_view_init(win) ;
  home_elec_view_init(win) ;

  timer_add(refesh_ui,NULL , 2000);

  return RET_OK;
}







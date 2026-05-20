#include "view/home_view/common.h"
#include "../common/navigator.h"
#include "../logic/hcn_logic.h"
#include "../view/view_manager.h"
/**
 * 初始化窗口的子控件
 */


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

  demonstration_stop() ;
  home_view_init(win) ;
  set_view_init(win) ;


  return RET_OK;
}







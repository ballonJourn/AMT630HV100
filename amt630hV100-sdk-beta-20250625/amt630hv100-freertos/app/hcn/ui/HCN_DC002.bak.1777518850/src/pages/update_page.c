#include "awtk.h"
#include "../common/navigator.h"
#include "view/update_view/update_logic.h"

static ret_t visit_init_child(void* ctx, const void* iter) {
  (void)ctx;
  widget_t* widget = WIDGET(iter);
  const char* name = widget->name;

  // 初始化指定名称的控件（设置属性或注册事件），请保证控件名称在窗口上唯一
  if (name != NULL && *name != '\0') {

  }

  return RET_OK;
}

/**
 * 初始化窗口
 * @note
 * 使用 common/navigator.h 中的 API 打开窗口时，会自动调用该初始化函数
 * 使用 AWTK 原生的 window_open 函数打开窗口时，须手动调用该初始化函数
 */
ret_t update_page_init(widget_t* win, void* ctx) {
  (void)ctx;
  return_value_if_fail(win != NULL, RET_BAD_PARAMS);

  widget_foreach(win, visit_init_child, win);
  update_init(win) ;

  return RET_OK;
}

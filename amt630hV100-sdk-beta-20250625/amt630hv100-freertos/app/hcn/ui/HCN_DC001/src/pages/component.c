#include "awtk.h"
#include "../common/navigator.h"

/**
 * 初始化窗口的子控件
 */
static ret_t visit_init_child(void* ctx, const void* iter) {
  widget_t* component = WIDGET(ctx);
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
ret_t component_init(widget_t* component, void* ctx) {
  (void)ctx;
  return_value_if_fail(component != NULL, RET_BAD_PARAMS);

  widget_foreach(component, visit_init_child, component);
  printf(" component_init \n");

  return RET_OK;
}

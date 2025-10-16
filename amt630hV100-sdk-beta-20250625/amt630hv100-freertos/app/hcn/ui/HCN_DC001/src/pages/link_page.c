#include "awtk.h"
#include "../common/navigator.h"
#include "proxy/vehicle_data.h"
#include "../../3rd/awtk-widget-qr/src/qr/qr.h"
#include "vehicle_param/vehicle_param.h"

/**
 * 初始化窗口的子控件
 */
#if ON_PC_CACLE == 0
extern void clear_rect(float x, float y, float w, float h, float a, float r, float g, float b);
ret_t onClearBg(void *ctx, event_t *e)
{
  clear_rect(0, 0, 1024, 600, 0, 0, 0, 0);//453  
  return RET_OK;
}
#endif

static ret_t visit_init_child(void* ctx, const void* iter) {
  (void)ctx;
  widget_t* widget = WIDGET(iter);
  const char* name = widget->name;

  // 初始化指定名称的控件（设置属性或注册事件），请保证控件名称在窗口上唯一
  if (name != NULL && *name != '\0') {

  }

  return RET_OK;
}



#if ON_PC_CACLE == 0
extern int get_qr_text_buf(char *buf, int len) ;
widget_t* qr ;
int timerId = 0 ;
ret_t refresh_ui(const timer_info_t* timer)
{
  char buff[256 ] ;
  get_qr_text_buf(buff , sizeof(buff)) ;

  qr = widget_lookup((widget_t *)timer->ctx,"link_qr", TRUE);

  static int state = 0 ;
  int _state = vehicle_get_data(VEH_CARLINK_CONNECTED) ;

  if (qr)
  {
    qr_set_value(qr , buff) ;
    if (state !=  _state)
    {
      widget_set_visible(qr , _state ? false : true);
      state = _state ;
    }
    
  }
  return RET_REPEAT;
}

ret_t onWindowChanged(void *ctx, event_t *e)
{
  if (e->type == EVT_WINDOW_CLOSE)
  {
    if(timerId != 0 && timer_find(timerId))
    {
      timer_remove(timerId) ;
      timerId = 0 ;
    }
  }
  return RET_OK ;
}
#endif

/**
 * 初始化窗口
 */
ret_t link_page_init(widget_t* win, void* ctx) {
  (void)ctx;
  return_value_if_fail(win != NULL, RET_BAD_PARAMS);

  widget_foreach(win, visit_init_child, win);

#if ON_PC_CACLE == 0
  widget_on(win, EVT_BEFORE_PAINT, onClearBg, win);
  timerId =  timer_add(refresh_ui , win , 1000) ;
  widget_on(win , EVT_WINDOW_CLOSE , onWindowChanged ,NULL);
#endif

  
  return RET_OK;
}

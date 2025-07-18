/**
 * File:   main_loop_xm.c
 * Author: ZhuoYongHong
 * Brief:  XM implemented main_loop interface
 *
 * Copyright (c) 2021 - 2025  ShenZhen ExceedSpace Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2021-04-17 ZhuoYongHong created
 *
 */

#include "native_window_xm.h"
#include "main_loop/main_loop_simple.h"
#include "main_loop_xm.h"
#include "base/window_manager.h"
#include "base/font_manager.h"
//#include "lcd/lcd_xm.h"
#include "base/idle.h"
#include "base/events.h"
#include "base/timer.h"
#include "base/system_info.h"

#include <xm.h>


#include <stdio.h>
#include "awtk_global.h"
#include "tkc/time_now.h"
#include "base/input_method.h"
#include <XM_event.h>



static ret_t main_loop_xm_dispatch_tp_event(main_loop_simple_t* loop, XM_EVENT* xm_event) {
  pointer_event_t event;
  int type = xm_event->type;
  widget_t* widget = loop->base.wm;

  memset(&event, 0x00, sizeof(event));
  switch (type) {
    case XM_EVENT_TOUCHDOWN: {
      {
        loop->pressed = 1;
        pointer_event_init(&event, EVT_POINTER_DOWN, widget, xm_event->tp.x,
                           xm_event->tp.y);
        event.button = 0;
        event.pressed = loop->pressed;
        event.e.native_window_handle = 0;//SDL_GetWindowFromID(xm_event->button.windowID);
		if (xm_event->tp.timestamp)
			event.e.time = xm_event->tp.timestamp;

        //SDL_CaptureMouse(TRUE);
        window_manager_dispatch_input_event(widget, (event_t*)&event);
      }
      break;
    }
    case XM_EVENT_TOUCHUP: {
       {
        //SDL_CaptureMouse(FALSE);
        pointer_event_init(&event, EVT_POINTER_UP, widget, xm_event->tp.x,
                           xm_event->tp.y);
        event.button = 0;
        event.pressed = loop->pressed;
        event.e.native_window_handle = 0;//SDL_GetWindowFromID(xm_event->button.windowID);
		if (xm_event->tp.timestamp)
			event.e.time = xm_event->tp.timestamp;

#ifdef ENABLE_TOUCH_UP_LONG_PERIOD_RESPONSE
		// 当快速触摸时,因为按下/释放的间隔时间过短, 按下的效果无法在LCD显示出来. 
		// 使能ENABLE_TOUCH_UP_LONG_PERIOD_RESPONSE可以显示明显的Touch Down按下响应
		sleep_ms(30);
#endif
        window_manager_dispatch_input_event(widget, (event_t*)&event);
        loop->pressed = 0;
      }
      break;
    }
    case XM_EVENT_TOUCHMOVE: {
      pointer_event_init(&event, EVT_POINTER_MOVE, widget, xm_event->tp.x,
                         xm_event->tp.y);
      event.button = 0;
      event.pressed = loop->pressed;
      event.e.native_window_handle = 0;//SDL_GetWindowFromID(xm_event->button.windowID);
	  if (xm_event->tp.timestamp)
		  event.e.time = xm_event->tp.timestamp;

      window_manager_dispatch_input_event(widget, (event_t*)&event);
      break;
    }
    default:
      break;
  }

  return RET_OK;
}

static ret_t main_loop_xm_dispatch_key_event(main_loop_simple_t* loop, XM_EVENT* xm_event) {
	key_event_t event;
	int type = xm_event->type;
	widget_t* widget = loop->base.wm;

	memset(&event, 0x00, sizeof(event));
	switch (type) {
	case XM_EVENT_KEYDOWN: {
		{
			//if(xm_event->key.repeat)
			//	key_event_init(&event, EVT_KEY_REPEAT, widget, xm_event->key.scancode);
			//else
				key_event_init(&event, EVT_KEY_DOWN, widget, xm_event->key.scancode);
			event.e.native_window_handle = 0;
			if (xm_event->tp.timestamp)
				event.e.time = xm_event->tp.timestamp;

			window_manager_dispatch_input_event(widget, (event_t*)&event);
		}
		break;
	}
	case XM_EVENT_KEYUP: {
		{
			key_event_init(&event, EVT_KEY_UP, widget, xm_event->key.scancode);
			event.e.native_window_handle = 0;
			if (xm_event->tp.timestamp)
				event.e.time = xm_event->tp.timestamp;

			window_manager_dispatch_input_event(widget, (event_t*)&event);
		}
		break;
	}

	default:
		break;
	}

	return RET_OK;
}

static ret_t main_loop_xm_dispatch_text_input(main_loop_simple_t* loop, XM_EVENT* xm_event) {
	im_commit_event_t event;
	XM_TextInputEvent* text_input_event = (XM_TextInputEvent*)&xm_event->text;

	memset(&event, 0x00, sizeof(event));
	event.e = event_init(EVT_IM_COMMIT, NULL);
	event.text = text_input_event->text;

	return input_method_dispatch_to_widget(input_method(), &(event.e));
}

#ifdef NINE
#include <nine\nine_event.h>
static ret_t main_loop_xm_dispatch_nine_event(main_loop_simple_t* loop, XM_EVENT* xm_event) {
  event_t nine_event;
  widget_t* widget = loop->base.wm;
  nine_event = event_init(EVT_NINE_EVT, NULL);
  memcpy(&xm_event->nine.event.e, &nine_event, sizeof(event_t));
  return widget_dispatch_recursive(window_manager(), &xm_event->nine.event.e);
  //return widget_dispatch(window_manager(), &nine_event);
  //window_manager_dispatch_input_event(widget, (event_t*)&xm_event->nine.event.e);
}
#endif

static ret_t main_loop_xm_dispatch(main_loop_simple_t* loop) {
  XM_EVENT event;
  ret_t ret = RET_OK;
  
  while(XM_WaitEvent(&event, 1) && loop->base.running) {
    switch (event.type) 
    {	
      case XM_EVENT_TOUCHDOWN:
      case XM_EVENT_TOUCHUP:
      case XM_EVENT_TOUCHMOVE:
      	ret = main_loop_xm_dispatch_tp_event(loop, &event);
			// 改善触摸事件的响应速度
			if (ret == RET_OK)
				return ret;
      	break;
		case XM_EVENT_KEYDOWN:
		case XM_EVENT_KEYUP:
			ret = main_loop_xm_dispatch_key_event(loop, &event);
			break;
		case XM_EVENT_TEXTINPUT:
			ret = main_loop_xm_dispatch_text_input(loop, &event);
			break;
#ifdef NINE
    case XM_EVENT_NINE:
      ret = main_loop_xm_dispatch_nine_event(loop, &event);
      if (ret == RET_OK)
        return ret;
      break;
#endif
	  }
  }

  return ret;
}

static ret_t main_loop_xm_destroy(main_loop_t* l) {
  main_loop_simple_t* loop = (main_loop_simple_t*)l;
  main_loop_simple_reset(loop);
  native_window_xm_deinit();

  return RET_OK;
}

static ret_t main_loop_xm_init_canvas(uint32_t w, uint32_t h) {
  //lcd_t* lcd = platform_create_lcd(w, h);

 // return_value_if_fail(lcd != NULL, RET_OOM);
  native_window_xm_init(TRUE, w, h);

  return RET_OK;
}

main_loop_t* main_loop_init(int w, int h) {
  main_loop_simple_t* loop = NULL;
  return_value_if_fail(main_loop_xm_init_canvas(w, h) == RET_OK, NULL);
  
  loop = main_loop_simple_init(w, h, NULL, NULL);
  return_value_if_fail(loop != NULL, NULL);
  
  loop->base.destroy = main_loop_xm_destroy;
  loop->dispatch_input = main_loop_xm_dispatch;

  return (main_loop_t*)loop;
}

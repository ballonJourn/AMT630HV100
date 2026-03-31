/**
 * File:   native_window_xm.c
 * Author: zhuoyonghong
 * Brief:  native window xm
 *
 * Copyright (c) 2019 - 2021  zhuoyonghong
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2021-04-10 zhuoyonghong created
 *
 */

#ifdef XM_WINDOWS_HOST
#include "xm_windows_host.h"
#elif defined(XM_HMI_HOST)
#include "xm_hmi_host.h"
#endif

#include "base/system_info.h"
#include "base/window_manager.h"

#include "lcd/lcd_nanovg.h"
#include "base/native_window.h"
#include "openvg.h"
#include "vgext.h"

typedef struct _native_window_xm_t {
  native_window_t native_window;
  void *vg_context;
  void *vg_surface;
  //void *handle;		

  void *window;		// win32´°¿Ú¾ä±ú

  void *lcd;

  canvas_t canvas;
} native_window_xm_t;

static native_window_t* s_shared_win = NULL;

#define NATIVE_WINDOW_XM(win) ((native_window_xm_t*)(win))

static ret_t native_window_xm_move(native_window_t* win, xy_t x, xy_t y) {
  return RET_OK;
}

static ret_t native_window_xm_resize(native_window_t* win, wh_t w, wh_t h) {
  win->rect.w = w;
  win->rect.h = h;

  return RET_OK;
}


static canvas_t* native_window_xm_get_canvas(native_window_t* win) {
  native_window_xm_t* xm = NATIVE_WINDOW_XM(win);

  return &(xm->canvas);
}

#ifdef WITH_VGCANVAS
void *vgcanvas_get_current_surface(void);

static ret_t native_window_xm_swap_buffer(native_window_t* win) 
{
	native_window_xm_t* xm = NATIVE_WINDOW_XM(win);
	//lcd_vgcanvas_t* lcd = (lcd_vgcanvas_t*)xm->lcd;
	(void)(xm);
	void *surface = vgcanvas_get_current_surface();
	void* surfacePixels = (void*)vgGetSurfacePixels(surface);
#ifdef XM_WINDOWS_HOST
	XM_WinHost_WindowBuffersSwap(surfacePixels, win->rect.w, win->rect.h, 32);
#elif defined(XM_HMI_HOST)
	//vgFinish();
	XM_HmiHost_WindowBuffersSwap(surfacePixels, win->rect.w, win->rect.h, 32);
#endif
  
  return RET_OK;
}
#else
#include "board.h"
#include "pxp.h"
#include "lcd.h"
#include "cp15/cp15.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_bgra8888.h"

static ret_t lcd_mem_swap(lcd_t* lcd)
{
	lcd_mem_t* mem = (lcd_mem_t*)lcd;
	uint8_t* offline_fb = mem->offline_fb;
	uint32_t cur_addr;
	CP15_clean_dcache_for_dma((uint32_t)offline_fb, (uint32_t)offline_fb + FB_SIZE);
	if (ark_lcd_get_fb_addr(2) != NULL) {
		LcdOsdInfo info = {0};
		uint32_t width, height;
		ark_lcs_get_osd_area(&width,&height);
		ark_lcd_get_osd_yaddr(LCD_UI_LAYER, &cur_addr);
		if (!ark_lcd_get_osd_info_atomic_isactive(LCD_UI_LAYER) && cur_addr == (uint32_t)mem->next_fb)
			ark_lcd_wait_for_vsync();

		info.width = width;
		info.height = height;
#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
		int ret;
		uint32_t src_format,dst_format;
#if LCD_BPP == 16
		src_format = PXP_SRC_FMT_RGB565;
		dst_format = PXP_OUT_FMT_RGB565;
#else
		src_format = PXP_SRC_FMT_RGB888;
		dst_format = PXP_OUT_FMT_ARGB8888;
#endif

#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_180
		info.width = height;
		info.height = width;
#endif
		ret = pxp_scaler_rotate((uint32_t)offline_fb, 0, 0, src_format, width, height,
						 (uint32_t)mem->next_fb, 0, dst_format, info.width, info.height, LCD_ROTATE_ANGLE);
		if(ret < 0){
			printf("%s pxp_scaler_rotate failed\n", __func__);
			//...
		}
#else
		mem->next_fb  = (uint8_t*)offline_fb;
#endif
		info.yaddr = (uint32_t)mem->next_fb;
#if LCD_BPP == 16
		info.format = LCD_OSD_FORAMT_RGB565;
#else
		info.format = LCD_OSD_FORAMT_ARGB888;
#endif
		ark_lcd_set_osd_info_atomic(LCD_UI_LAYER,&info);
		lcd_mem_set_offline_fb(mem, mem->online_fb);
		lcd_mem_set_online_fb(mem, offline_fb);
  } else {
	ark_lcd_set_osd_yaddr(LCD_UI_LAYER, (unsigned int)offline_fb);
	ark_lcd_set_osd_sync(LCD_UI_LAYER);
	/* wait vsync */
	ark_lcd_wait_for_vsync();
	lcd_mem_set_offline_fb(mem, mem->online_fb);
	lcd_mem_set_online_fb(mem, offline_fb);
  }

  return RET_OK;	
}

lcd_t* platform_create_lcd(wh_t w, wh_t h) {
  lcd_t* lcd = NULL;

#if LCD_BPP == 16
  if (ark_lcd_get_fb_addr(2) != NULL) {
    lcd = lcd_mem_bgr565_create_three_fb(w, h, ark_lcd_get_fb_addr(0),
      ark_lcd_get_fb_addr(1), ark_lcd_get_fb_addr(2));
  }
  else
    lcd = lcd_mem_bgr565_create_double_fb(w, h, ark_lcd_get_fb_addr(0),
      ark_lcd_get_fb_addr(1));
#elif LCD_BPP == 32
  if (ark_lcd_get_fb_addr(2) != NULL) {
    lcd = lcd_mem_bgra8888_create_three_fb(w, h, ark_lcd_get_fb_addr(0),
      ark_lcd_get_fb_addr(1), ark_lcd_get_fb_addr(2));
  	}
  else
    lcd = lcd_mem_bgra8888_create_double_fb(w, h, ark_lcd_get_fb_addr(0),
      ark_lcd_get_fb_addr(1));
#endif

  lcd->swap = lcd_mem_swap;
  lcd->support_dirty_rect = 0;

  return lcd;
}
#endif

extern ret_t tk_quit();



static ret_t native_window_xm_get_info(native_window_t* win, native_window_info_t* info) {

  native_window_xm_t* xm = NATIVE_WINDOW_XM(win);
  int w, h;
  (void)(xm);

  info->x = 0;
  info->y = 0;
  info->ratio = 1;
  //info->ratio = xm->canvas.lcd->ratio;
  XM_GetWidowSize(&w, &h);
  info->w = w;
  info->h = h;

  win->rect.x = 0;
  win->rect.y = 0;
  win->rect.w = w;
  win->rect.h = h;
  win->ratio = info->ratio;

  log_debug("ratio=%f %d %d\n", info->ratio, info->w, info->h);

  return RET_OK;
}



static const native_window_vtable_t s_native_window_vtable = {
    .type = "native_window_xm",
    .resize = native_window_xm_resize,
    .get_info = native_window_xm_get_info,
#ifdef WITH_VGCANVAS    
    .swap_buffer = native_window_xm_swap_buffer,
#endif
    .get_canvas = native_window_xm_get_canvas,
};

static ret_t native_window_xm_on_destroy(object_t* obj) {
  log_debug("Close native window.\n");
  //native_window_sdl_close(NATIVE_WINDOW(obj));

  return RET_OK;
}

static ret_t native_window_xm_exec(object_t* obj, const char* cmd, const char* args) {

  return RET_NOT_FOUND;
}

static const object_vtable_t s_native_window_xm_vtable = {
    .type = "native_window_xm",
    .desc = "native_window_xm",
    .size = sizeof(native_window_xm_t),
    .exec = native_window_xm_exec,
    .on_destroy = native_window_xm_on_destroy};

static native_window_t* native_window_create_internal(uint32_t w, uint32_t h) {
  lcd_t* lcd = NULL;
  object_t* obj = object_create(&s_native_window_xm_vtable);
  native_window_t* win = NATIVE_WINDOW(obj);
  native_window_xm_t* xm = NATIVE_WINDOW_XM(win);
  return_value_if_fail(xm != NULL, NULL);

#ifdef XM_WINDOWS_HOST
  xm->window = XM_WinHost_WindowCreate("XM", w, h);
#elif defined(XM_HMI_HOST)
  xm->window = XM_HmiHost_WindowCreate("XM", w, h);
#endif

  canvas_t* c = &(xm->canvas);

  win->shared = TRUE;
  win->handle = xm->window;
  win->vt = &s_native_window_vtable;
  win->rect = rect_init(0, 0, w, h);

#ifdef WITH_VGCANVAS  
  lcd = lcd_nanovg_init(win);
#else
  lcd = platform_create_lcd(w, h);
#endif
  xm->lcd = lcd;

  canvas_init(c, lcd, font_manager());
  

  return win;
}

native_window_t* native_window_create(widget_t* widget) {
  native_window_t* nw = s_shared_win;
  return_value_if_fail(nw != NULL, NULL);

  widget_set_prop_pointer(widget, WIDGET_PROP_NATIVE_WINDOW, nw);

  return nw;
}



ret_t native_window_xm_init(bool_t shared, uint32_t w, uint32_t h) {

	(void)(shared);
	
#ifdef XM_WINDOWS_HOST
	//if(!XM_WinHost_WindowCreate ("VG", w, h))
	//	return RET_FAIL;
#endif


  s_shared_win = native_window_create_internal(w, h);
  return RET_OK;
}

ret_t native_window_xm_deinit(void) {
  if (s_shared_win != NULL) {
    object_unref(OBJECT(s_shared_win));
    s_shared_win = NULL;
  }
  
#ifdef XM_WINDOWS_HOST
	XM_WinHost_WindowDestroy();
#elif defined(XM_HMI_HOST)
  	XM_HmiHost_WindowDestroy();
#endif  

  return RET_OK;
}

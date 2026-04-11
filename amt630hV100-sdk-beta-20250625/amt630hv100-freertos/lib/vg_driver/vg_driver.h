#ifndef _VG_DRIVER_H
#define _VG_DRIVER_H

#include "board.h"

#define	VG_CENTER_X		514
#define	VG_CENTER_Y		306

#if LCD_ROTATE_ANGLE==LCD_ROTATE_ANGLE_90 || LCD_ROTATE_ANGLE==LCD_ROTATE_ANGLE_270
#define LCD_SRC_W	LCD_HEIGHT
#define LCD_SRC_H	LCD_WIDTH
#else
#define LCD_SRC_W	LCD_WIDTH
#define LCD_SRC_H	LCD_HEIGHT
#endif

#ifdef LVGL_VG_GPU
#define VG_W	LCD_SRC_W
#define VG_H	LCD_SRC_H
#define	VG_X	0
#define	VG_Y	0
#define VG_OSD_W	LCD_SRC_W
#define VG_OSD_H	LCD_SRC_H
#define VG_BPP		LCD_BPP

#elif defined(VG_ONLY)

#define VG_W	LCD_SRC_W
#define VG_H	LCD_SRC_H
#define	VG_X	(0)
#define	VG_Y	(0)
#define VG_OSD_W	LCD_SRC_W
#define VG_OSD_H	LCD_SRC_H
#define VG_BPP		LCD_BPP

#elif defined(AWTK)
#ifndef VG_OSD_W
#define VG_OSD_W	OSD_WIDTH
#endif
#ifndef VG_OSD_H
#define VG_OSD_H	OSD_HEIGHT
#endif
#define VG_W	OSD_WIDTH
#define VG_H	OSD_HEIGHT
#define	VG_X	0
#define	VG_Y	0
#define VG_BPP		LCD_BPP

#else
#ifdef REVERSE_TRACK
#define VG_W	704
#define VG_H	440
#define	VG_X	0
#define	VG_Y	0
#define VG_OSD_W	704
#define VG_OSD_H	440
#define VG_BPP		16
#else
#define VG_W	496
#define VG_H	496
#define	VG_X	((VG_CENTER_X) - (VG_W)/2)
#define	VG_Y	((VG_CENTER_Y) - (VG_H)/2)
#define VG_OSD_W	LCD_SRC_W
#define VG_OSD_H	LCD_SRC_H
#define VG_BPP		LCD_BPP
#endif

#endif

extern unsigned int xm_vg_get_width (void);

extern unsigned int xm_vg_get_height (void);

extern unsigned int xm_vg_get_osd_stride(void);

extern unsigned int xm_vg_get_stride (void);

extern unsigned int xm_vg_get_bpp (void);

extern int xm_vg_is_valid_gpu_fb (unsigned int base);

extern unsigned int xm_vg_require_gpu_fb (void);

extern int xm_vg_get_offset_x (void);

extern int xm_vg_get_offset_y (void);

extern void xm_vg_get_osd_window (	unsigned int* x,
							unsigned int* y,
							unsigned int* w,
							unsigned int* h,
							unsigned int* stride);

extern void* xm_vg_get_gpu_background_image (void);

extern unsigned int vg_get_framebuffer (void);

extern void xm_vg_draw_prepare(void *para);

extern void xm_vg_draw_start(void);

#ifdef REVERSE_TRACK
extern void xm_vg_set_gpu_fb_addr(unsigned int addr);
#endif

#endif

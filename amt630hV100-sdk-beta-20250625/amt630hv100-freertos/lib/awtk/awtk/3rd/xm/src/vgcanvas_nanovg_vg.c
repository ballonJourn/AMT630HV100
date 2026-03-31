/**
 * File:   vgcanvas_nanovg_gl.c
 * Author: AWTK Develop Team
 * Brief:  vector graphics canvas base on nanovg-gl
 *
 * Copyright (c) 2018 - 2021  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2018-04-14 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifdef WITH_VGCANVAS

#include "base/system_info.h"



#include "nanovg.h"
#include "tkc/utf8.h"
#include "tkc/mem.h"
#include "base/vgcanvas.h"
#include "base/image_manager.h"
#include "base/native_window.h"
#include "base/font_manager.h"
#include "base/assets_manager.h"
#include "base/vgcanvas_asset_manager.h"
//#include "nanovg_vg.h"
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "VG/vgext.h"
#include "canvasvg.h"
#include "nanovg_openvg.h"



//#define	CREATE_FB_SURFACE

#define	USE_OFFLINE_SURFACE


// 允许offline_fb与online_fb具有不同的尺寸与位置, 
// 比如 当前帧画面全屏显示, 下一帧画面与摄像头画面分割显示
typedef struct _vgcanvas_nanovg_offline_fb_t {
	
#ifdef USE_OFFLINE_SURFACE
  void *surface;			// 帧缓冲
#endif
 
  uint32_t width;			// VG开窗大小
  uint32_t height;
  uint32_t bpp;			// VG使用的分辨率
  
  uint32_t osd_x;			// OSD偏移
  uint32_t osd_y;

  int dirty_rects[4 * TK_MAX_DIRTY_RECT_NR];

} vgcanvas_nanovg_offline_fb_t;

typedef struct _vgcanvas_nanovg_t {
  vgcanvas_t base;

  int font_id;
  NVGcontext* vg;
  uint32_t text_align_v;
  uint32_t text_align_h;
  
  void *	vg_context;		
  void * vg_surface;		// 标记当前VG正在使用的surface	

  vgcanvas_nanovg_offline_fb_t* offline_fb;		// 系统定义的framebuffer

  native_window_t* window;
  

  
} vgcanvas_nanovg_t;


static void *current_surface;

#ifdef ENABLE_FB_DIRTY_RECTS_COPY

#define	SURFACE_COUNT	3
#ifndef NVG_MAX_DIRTY_RECTS
#define	NVG_MAX_DIRTY_RECTS	32
#endif
// 脏矩形自动管理与刷新
static int fbo_mode = 0;

static int surface_fifo_count = 0;
static int surface_fifo_index = -1;
static dirty_rects_t surface_dirty_rects[SURFACE_COUNT] = { 0 };	// 最多3帧脏矩形管理

static unsigned int surface_fifo_address[SURFACE_COUNT] = { 0 };
static unsigned int surface_fifo_bpp = 0;
static unsigned int surface_fifo_stride = 0;
static int surface_fifo_width = 0;
static int surface_fifo_height = 0;

// 设置当前FB的脏矩形区域
static void vg_set_dirty_rects(const dirty_rects_t *dirty_rects)
{
	if(fbo_mode)
		return;
	
	if (surface_fifo_count == 0)
		return;
	
	surface_fifo_index ++;
	if(surface_fifo_index >= surface_fifo_count)
		surface_fifo_index = 0;

	memcpy(&surface_dirty_rects[surface_fifo_index], dirty_rects, sizeof(dirty_rects_t));
}

// surface_addr 保存LCD帧显示缓存的数组
// surface_size LCD帧显示缓存个数
// surface_width LCD帧像素宽度
// surface_height LCD帧像素高度
// surface_bpp LCD帧像素位宽
// surface_stride LCD帧缓存每行字节长度
int vg_set_surface(unsigned int *surface_addr, int surface_size,
	unsigned int surface_width,
	unsigned int surface_height,
	unsigned int surface_bpp, unsigned int surface_stride)
{
	surface_fifo_count = 0;
	surface_fifo_index = -1;
	memset(surface_fifo_address, 0, sizeof(surface_fifo_address));
	if (surface_size <= 0 || surface_size > SURFACE_COUNT)
	{
		printf("vg_set_surface failed, invalid parameter\n");
		return 0;
	}
	for (int i = 0; i < surface_size; i++)
	{
		if (surface_addr[i] == 0)
		{
			printf("vg_set_surface failed, invalid parameter\n");
			return 0;
		}
		//surface_fifo_address[i] = surface_addr[i];
	}
	surface_fifo_index = -1;
	surface_fifo_count = surface_size;
	memset(surface_dirty_rects, 0, sizeof(surface_dirty_rects));
	surface_fifo_bpp = surface_bpp;
	surface_fifo_stride = surface_stride;
	surface_fifo_width = surface_width;
	surface_fifo_height = surface_height;
	return 1;
}

extern void dma_flush_range(unsigned int ulStart, unsigned int ulEnd);
extern void dma_inv_range(unsigned int base, unsigned int last);

// The cache line length is eight words (32 bytes) 
// 按照cache line长度读写, 最大DDR访问效率
static void _surface_copy_block32 (unsigned int * dst, unsigned int* src, unsigned int size_block32, unsigned int stride, int h)
{
#if defined(AMT630H) || defined(AMT630HV100)
	asm (	"PUSH     {R4-R11, LR}\n"
		  	"LDR      R12, [SP, #+36]\n"
			"SUB      R3, R3, R2, LSL #+5\n"	
			"B        __surface_copy_block32_0\n"
			"__surface_copy_block32_1:\n"
			"LDM		R1!,	{R4, R5, R6, R7, R8, R9, R10, R11}\n"
			"SUB      LR, LR, #+1\n"
			"STM		R0!,	{R4, R5, R6, R7, R8, R9, R10, R11}\n"
			"__surface_copy_block32_2:\n"
			"CMP      LR, #+1\n"
			"BGE      __surface_copy_block32_1\n"
			"SUB      R12, R12, #+1\n"
			"ADD      R1, R1, R3\n"
			"ADD      R0, R0, R3\n"
			"__surface_copy_block32_0:\n"
			"CMP      R12, #+1\n"
			"BLT      __surface_copy_block32_3\n"
			"MOV      LR, R2\n"
			"B        __surface_copy_block32_2\n"
			"__surface_copy_block32_3:\n"
			"POP      {R4-R11, PC} \n"
		  );
#else
	int count;
	
	stride -= size_block32 * 32;
	while(h > 0)
	{
		count = size_block32;
		while(count > 0)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
			dst[4] = src[4];
			dst[5] = src[5];
			dst[6] = src[6];
			dst[7] = src[7];
			dst += 8;
			src += 8;
			count --;
		}
		h --;
		src = (unsigned int *)(((char *)src) + stride);
		dst = (unsigned int *)(((char *)dst) + stride);
	}
#endif
}

static void _surface_flush_block32(unsigned int  dst, unsigned int size, unsigned int stride, int h)
{
	while (h > 0)
	{
		dma_flush_range((unsigned int)dst, size + (unsigned int)dst);
		dst += stride;
		h--;
	}
}

static void vg_surface_dirty_rects_copy(unsigned int dst_surface_fifo, unsigned int src_surface_fifo, int x, int y, int w, int h)
{
	char *dst = (char *)dst_surface_fifo;
	char *src = (char *)src_surface_fifo;
	unsigned int off_1, off_2, size;
	assert(y >= 0 && y < surface_fifo_height);
	
	// 必须32字节对齐
#ifdef AMT630H
	assert (!(dst_surface_fifo % 32));
	assert (!(src_surface_fifo % 32));
#endif

	off_1 = (x * surface_fifo_bpp / 8 );
	off_2 = ((x + w) * surface_fifo_bpp / 8);
	
#ifdef AMT630H
	// 32字节对齐
	off_1 = (off_1 ) & (~31);
	off_2 = (off_2 + 31) & (~31);
#endif
	
	size = off_2 - off_1;
	
	dst = (char *)dst_surface_fifo + y * surface_fifo_stride + off_1;
	src = (char *)src_surface_fifo + y * surface_fifo_stride + off_1;
	
#if 1
	_surface_copy_block32 ((unsigned int *)dst, (unsigned int *)src, (off_2 - off_1)/32, surface_fifo_stride, h);
	_surface_flush_block32((unsigned int)dst, off_2 - off_1,  surface_fifo_stride, h);
#else
	while (h > 0)
	{
		memcpy(dst, src,size);
		dma_flush_range((unsigned int)dst, size + (unsigned int)dst);
		src += surface_fifo_stride;
		dst += surface_fifo_stride;
		h--;
	}
#endif
}

// 判断脏矩形dr是否完整包含矩形r
static int is_dirty_rects_include(const dirty_rects_t *dr, const rect_t* r)
{
	int i;
	for (i = 0; i < dr->nr; i++)
	{
		if (dr->rects[i].x <= r->x
			&& (dr->rects[i].x + dr->rects[i].w) >= (r->x + r->w)
			&& dr->rects[i].y <= r->y
			&& (dr->rects[i].y + dr->rects[i].h) >= (r->y + r->h)
			)
		{
			return 1;
		}
	}
	return 0;
}

// 把脏矩形src1中完全被脏矩形2包含的区域排除后得到的区域保存到dst
// 这个算法仅仅将完全包含的区域排除, 并没有考虑区域相交时的情况
static void dirty_rects_exclude(const dirty_rects_t *src1, const dirty_rects_t *src2, dirty_rects_t *dst)
{
	int i;

	dirty_rects_reset(dst);
	for (i = 0; i < src1->nr; i++)
	{
		if (is_dirty_rects_include(src2, src1->rects + i))
		{
			// 区域完全被包含
		}
		else
		{
			dirty_rects_add(dst, src1->rects + i);
		}
	}
}

// 计算2个脏矩形集的交集
static void dirty_rects_intersect_dirty_rects(const dirty_rects_t *src1, const dirty_rects_t *src2, dirty_rects_t *dst)
{
	int i;
	dirty_rects_t dirty_rects;
	dirty_rects_reset(dst);
	for (i = 0; i < src1->nr; i++)
	{
		dirty_rects_reset(&dirty_rects);
		rect_intersect_dirty_rects(src2, &src1->rects[i], &dirty_rects);
		dirty_rects_combine(&dirty_rects, dst);
	}
}

// 更新当前FB尚未更新的脏区域(位于其他的FrameBuffer之中)
// 从前面已更新的FrameBuffer中将脏区域复制到当前FB(需要排除当前的区域)
static int vg_fb_dirty_rects_copy (void)
{
	unsigned int surface_src, surface_dst;
	if (surface_fifo_count <= 0)
		return 0;
	if (surface_fifo_count == 1)
		return 1;

	{
		int sync_count = surface_fifo_count - 1;
		int sync_index = surface_fifo_index;
		// 按照顺序拷贝脏矩形 (当前FB之前的第二帧(-2) 当前FB之前的第一帧(-1) )
		while (sync_count > 0)
		{
			int index = sync_index - sync_count;
			if (index < 0)
				index = index + surface_fifo_count;

			surface_dst = surface_fifo_address[surface_fifo_index];
			surface_src = surface_fifo_address[index];
			// 检查FB帧基址是否已设置
			if (surface_dst && surface_src)
			{
				dirty_rects_t dr;
				dirty_rects_reset(&dr);

				// 计算有效的脏区域
				if (sync_count == 2)
				{
					// 从当前FB之前的第二帧拷贝脏数据到当前FB
					// 当前FB之前的第二帧(-2) 当前FB之前的第一帧(-1) 当前FB
					// 计算当前FB之前的第二帧的有效脏矩形, 需要将当前FB之前的的第一帧及当前FB的脏区域排除
					dirty_rects_t dr2;
					int index1 = sync_index - 1;
					if (index1 < 0)
						index1 = index1 + surface_fifo_count;
					dirty_rects_reset(&dr2);
					// 排除第一帧的脏区域
					dirty_rects_exclude(surface_dirty_rects + index, surface_dirty_rects + index1, &dr2);
					// 排除当前帧的脏区域
					dirty_rects_exclude(&dr2, surface_dirty_rects + surface_fifo_index, &dr);
				}
				else if (sync_count == 1)
				{
					// 从当前FB之前的第一帧拷贝脏数据到当前FB
					// 排除当前帧的脏区域
					dirty_rects_exclude(surface_dirty_rects + index, surface_dirty_rects + surface_fifo_index, &dr);
				}

				// 拷贝有效的脏区域
				for (int i = 0; i < dr.nr; i++)
				{
					VGint x = dr.rects[i].x;
					VGint y = dr.rects[i].y;
					VGint w = dr.rects[i].w;
					VGint h = dr.rects[i].h;
					if (w == 0 || h == 0)
						break;
					
					vg_surface_dirty_rects_copy(surface_dst, surface_src, x, y, w, h);
				}
			}

			sync_count--;
		}
	}
	return 1;

}

// 将所有脏矩形区域的内容同步到LCD显示FB
int vg_surface_sync(unsigned int lcd_buffer, unsigned int width, unsigned int height)
{
	(void)(width);
	(void)(height);
	if (surface_fifo_count == 0)
		return 0;

	surface_fifo_address[surface_fifo_index] = lcd_buffer;
	
	// 从过去的FB帧(2帧或1帧)中复制脏区域
	vg_fb_dirty_rects_copy();

	// 从VG surface复制脏区域
	extern  VGubyte *vgGetCurrentSurfacePixels (void);
	// 获取当前surface的基址. 
	//   该函数为扩展函数, 不支持sub-image
	VGubyte *surface_pixels = vgGetCurrentSurfacePixels();
	for (int i = 0; i < surface_dirty_rects[surface_fifo_index].nr; i ++)
	{
		VGint x = surface_dirty_rects[surface_fifo_index].rects[i].x;
		VGint y = surface_dirty_rects[surface_fifo_index].rects[i].y;
		VGint w = surface_dirty_rects[surface_fifo_index].rects[i].w;
		VGint h = surface_dirty_rects[surface_fifo_index].rects[i].h;
		if (w == 0 || h == 0)
			break;
		vg_surface_dirty_rects_copy (lcd_buffer, (unsigned int)surface_pixels, x, y, w, h);
	}

	return 1;
}
#else

int vg_set_surface(unsigned int *surface_addr, int surface_size,
	unsigned int surface_width,
	unsigned int surface_height,
	unsigned int surface_bpp, unsigned int surface_stride)
{
	(void)(surface_addr);
	(void)(surface_size);
	(void)(surface_width);
	(void)(surface_height);
	(void)(surface_bpp);
	(void)(surface_stride);
	return 0;
}

#endif // ENABLE_FB_DIRTY_RECTS_COPY

void *vgcanvas_get_current_surface(void)
{
	return current_surface;
}
// 释放NanoVG framebuffer的资源
static void vgcanvas_destroy_offline_fb(vgcanvas_nanovg_offline_fb_t* offline_fb) 
{
	if(offline_fb)
	{
#ifdef USE_OFFLINE_SURFACE		
		if(offline_fb->surface)
			vgSurfaceDestroy (offline_fb->surface);
		offline_fb->surface = NULL;	
#endif
		TKMEM_FREE(offline_fb);
	}
}

// 创建Nanovg framebuffer
static vgcanvas_nanovg_offline_fb_t* vgcanvas_create_offline_fb(uint32_t width, uint32_t height) 
{
	vgcanvas_nanovg_offline_fb_t* offline_fb;
	do
	{
		offline_fb = (vgcanvas_nanovg_offline_fb_t*)TKMEM_ZALLOC(vgcanvas_nanovg_offline_fb_t);
		if(offline_fb == NULL)
		{
			log_debug("Cannot create offline_fb! \r\n");	
			break;
		}

#ifdef USE_OFFLINE_SURFACE		
		// create surface
		offline_fb->surface = vgSurfaceCreate(width, height, VG_FALSE, VG_FALSE, VG_TRUE);
		if(!offline_fb->surface)
		{
			log_debug("Cannot create vg surface 0! \r\n");	
			break;	
		}
#endif
		offline_fb->osd_x = 0;
		offline_fb->osd_y = 0;
		
		offline_fb->width = width;
		offline_fb->height = height;
					
		return offline_fb; 
	} while (0);

	vgcanvas_destroy_offline_fb (offline_fb);

	return NULL;
}



#include "nanovg_openvg.h"

static ret_t __vgcanvas_destroy (vgcanvas_t* vg)
{
	vgcanvas_nanovg_t *nanovg = (vgcanvas_nanovg_t*)vg;
	if(nanovg->vg)
	{
		nvgDeleteOpenVG (nanovg->vg);
		nanovg->vg = NULL;	
	}		
	vgMakeCurrent (NULL, NULL);
	if(nanovg->offline_fb)
	{
		vgcanvas_destroy_offline_fb (nanovg->offline_fb);
		nanovg->offline_fb = NULL;
	}
	
	vgContextDestroy (nanovg->vg_context);
	nanovg->vg_context = NULL;
	nanovg->vg_surface = NULL;
	current_surface = NULL;

	return RET_OK;
}




static ret_t vgcanvas_nanovg_reinit(vgcanvas_t* vg, uint32_t w, uint32_t h, uint32_t stride,
                                    bitmap_format_t format, void* data) {
  (void)vg;
  (void)w;
  (void)h;
  (void)format;
  (void)data;
  return RET_OK;
}

#if 0
static inline rect_t  vgcanvas_nanovg_get_dirty_rect(const rect_t* dirty_rect) {
  rect_t r;
  uint32_t w, h;
  system_info_t* info = system_info();
  return_value_if_fail(info != NULL, rect_init(0, 0, 0, 0));
  w = info->lcd_w * info->device_pixel_ratio;
  h = info->lcd_h * info->device_pixel_ratio;
  if (dirty_rect == NULL) {
    switch (info->lcd_orientation) {
      case LCD_ORIENTATION_0:
        r = rect_init(0, 0, w, h);
        break;
      case LCD_ORIENTATION_90:
        r = rect_init(w - h, h - w, h, w);
        break;
      case LCD_ORIENTATION_180:
        r = rect_init(0, 0, w, h);
        break;
      case LCD_ORIENTATION_270:
        r = rect_init(0, 0, h, w);
        break;
    }
  } else {
    switch (info->lcd_orientation) {
      case LCD_ORIENTATION_0:
        r = rect_init(dirty_rect->x, h - dirty_rect->h - dirty_rect->y, dirty_rect->w, dirty_rect->h);
        break;
      case LCD_ORIENTATION_90:
        r = rect_init(w - dirty_rect->h - dirty_rect->y, h - dirty_rect->w - dirty_rect->x, dirty_rect->h, dirty_rect->w);
        break;
      case LCD_ORIENTATION_180:
        r = rect_init(w - dirty_rect->w - dirty_rect->x, dirty_rect->y, dirty_rect->w, dirty_rect->h);
        break;
      case LCD_ORIENTATION_270:
        r = rect_init(dirty_rect->y, dirty_rect->x, dirty_rect->h, dirty_rect->w);
        break;
    }
  }
  return r;
}
#endif


// 设置当前渲染的framebuffer
static inline void vgcanvas_nanovg_set_offline_fb(vgcanvas_nanovg_t* canvas, uint32_t w,
													uint32_t h, const dirty_rects_t* dirty_rects) 
{
	vgcanvas_nanovg_offline_fb_t* offline_fb = canvas->offline_fb;
	if(offline_fb != NULL)
	{
		// 检查fb帧尺寸是否一致.若不相同, 释放并重新创建
		if(offline_fb->width != w || offline_fb->height != h)
		{
			vgcanvas_destroy_offline_fb(canvas->offline_fb);
			canvas->offline_fb = vgcanvas_create_offline_fb(w, h);
			offline_fb = canvas->offline_fb;
		}
		
#ifdef USE_OFFLINE_SURFACE		
		if(offline_fb != NULL && canvas->vg_surface != offline_fb->surface) 
		{
			// 设置当前帧为VG渲染帧
			canvas->vg_surface = offline_fb->surface;
			current_surface = canvas->vg_surface;
			vgMakeCurrent (canvas->vg_context, canvas->vg_surface);
			(void)dirty_rects;
		}
#endif

		
	}	

#ifdef ENABLE_FB_DIRTY_RECTS_COPY
	if(dirty_rects && dirty_rects->nr)
		vg_set_dirty_rects(dirty_rects);
#endif
}


// 等待VG完成所有的指令
static inline void vgcanvas_nanovg_offline_fb_flush(vgcanvas_nanovg_t* canvas) 
{
	(void)(canvas);
	vgFinish ();
}

static ret_t vgcanvas_nanovg_begin_frame(vgcanvas_t* vgcanvas, const dirty_rects_t* dirty_rects) {
  float_t angle = 0.0f;
  float_t anchor_x = 0.0f;
  float_t anchor_y = 0.0f;

  system_info_t* info = system_info();
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  const rect_t* dirty_rect = dirty_rects != NULL ? &(dirty_rects->max) : NULL;

  if (dirty_rect != NULL) {
    canvas->base.dirty_rect = rect_init(dirty_rect->x, dirty_rect->y, dirty_rect->w, dirty_rect->h);
#if defined(VGCANVAS_DIRTY_RECTS_SUPPORT) && defined(DIRTY_RECTS_CLIP_SUPPORT)
	 memcpy(&canvas->base.dirty_rects, dirty_rects, sizeof(dirty_rects_t));
#endif
  } else {
    if (info->lcd_orientation == LCD_ORIENTATION_90 ||
        info->lcd_orientation == LCD_ORIENTATION_270) {
      canvas->base.dirty_rect = rect_init(0, 0, info->lcd_h, info->lcd_w);
    } else {
      canvas->base.dirty_rect = rect_init(0, 0, info->lcd_w, info->lcd_h);
    }
#if defined(VGCANVAS_DIRTY_RECTS_SUPPORT) && defined(DIRTY_RECTS_CLIP_SUPPORT)
	 dirty_rects_reset(&canvas->base.dirty_rects);
	 dirty_rects_add(&canvas->base.dirty_rects, &canvas->base.dirty_rect);
#ifdef ENABLE_FB_DIRTY_RECTS_COPY	 
	 // 20220522 脏矩形区域为空时设置为整个LCD区域
	 dirty_rects = &canvas->base.dirty_rects;
#endif
#endif
  }

  vgcanvas_nanovg_set_offline_fb(canvas, info->lcd_w * info->device_pixel_ratio,
                                 info->lcd_h * info->device_pixel_ratio, dirty_rects);

  nvgBeginFrame(canvas->vg, info->lcd_w, info->lcd_h, info->device_pixel_ratio);

  switch (info->lcd_orientation) {
    case LCD_ORIENTATION_0:
      angle = 0.0f;
      break;
    case LCD_ORIENTATION_90:
      angle = TK_D2R(90);
      break;
    case LCD_ORIENTATION_180:
      angle = TK_D2R(180);
      break;
    case LCD_ORIENTATION_270:
      angle = TK_D2R(270);
      break;
  }

  anchor_x = info->lcd_w / 2.0f;
  anchor_y = info->lcd_h / 2.0f;

  nvgSave(canvas->vg);

  if (info->lcd_orientation == LCD_ORIENTATION_90 || info->lcd_orientation == LCD_ORIENTATION_270) {
    nvgTranslate(canvas->vg, anchor_x, anchor_y);
    nvgRotate(canvas->vg, angle);
    nvgTranslate(canvas->vg, -anchor_y, -anchor_x);
  } else if (info->lcd_orientation == LCD_ORIENTATION_180) {
    nvgTranslate(canvas->vg, anchor_x, anchor_y);
    nvgRotate(canvas->vg, angle);
    nvgTranslate(canvas->vg, -anchor_x, -anchor_y);
  }

  return RET_OK;
}

// 将当前渲染帧释放, 该帧可用于LCD显示用途
static ret_t vgcanvas_nanovg_end_frame(vgcanvas_t* vgcanvas) 
{
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  NVGcontext* vg = canvas->vg;

  nvgRestore(vg);
  nvgEndFrame(vg);

  vgcanvas_nanovg_offline_fb_flush(canvas);

  // 调用本地framebuffer切换函数实现显示帧物理切换过程
  native_window_swap_buffer(canvas->window);

  return RET_OK;
}

// 创建一个离线的framebuffer对象, 非LCD实时显示使用. 
static ret_t vgcanvas_nanovg_create_fbo(vgcanvas_t* vgcanvas, uint32_t w, uint32_t h, bool_t custom_draw_model, framebuffer_object_t* fbo)
{
	VGImage vgImage = VG_INVALID_HANDLE;
	void *vgSurface = VG_INVALID_HANDLE;
	//NVGcontext* vg = NULL;
	//vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
	// 20210921 加入vgFinish可以消除视窗平滑过渡时出现的白色矩形区域
	vgFinish();	
	

#ifdef CREATE_FB_SURFACE
	// 仅支持2种固定 32位 ARGB8888 及 16位 RGB565
#if WITH_FB_BGR565 == 1
	//vgImage = vgCreateImage(VG_sRGB_565, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);
	vgImage = vgCreateImage(VG_sRGB_565, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_NONANTIALIASED);
#else
	vgImage = vgCreateImage(VG_sARGB_8888, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);
#endif
	/*
	if(vgcanvas->format == BITMAP_FMT_RGBA8888)
		vgImage = vgCreateImage(VG_sARGB_8888, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);
	else if(vgcanvas->format == BITMAP_FMT_RGB565)
		vgImage = vgCreateImage(VG_sRGB_565, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);
	else
		//vgImage = vgCreateImage(VG_sRGBA_8888, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);
		//vgImage = vgCreateImage(VG_sARGB_8888, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);
		//vgImage = vgCreateImage(VG_sABGR_8888, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);	// VG不支持ABGR格式的Surface
		vgImage = vgCreateImage(VG_sRGB_565, w * vgcanvas->ratio, h * vgcanvas->ratio, VG_IMAGE_QUALITY_BETTER);
	*/	
	if(vgImage == VG_INVALID_HANDLE)
	{
		return RET_FAIL;	
	}	
	
	// 
	vgSurface = vgSurfaceCreateFromImage (vgImage, VG_TRUE);
	if(vgSurface == VG_INVALID_HANDLE)
	{
		vgDestroyImage (vgImage);
		return RET_FAIL;		
	}
#endif
		
	//vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
	fbo->w = w;
	fbo->h = h;
	fbo->init = FALSE;
	fbo->handle = vgSurface;
	fbo->id = vgImage;
	fbo->ratio = vgcanvas->ratio;
	fbo->offline_fbo = (int)vgSurface;
  fbo->custom_draw_model = custom_draw_model;

  return RET_OK;
}

static ret_t vgcanvas_nanovg_destroy_fbo(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo) 
{
#ifdef CREATE_FB_SURFACE
  	if(fbo->id)
  	{
  		vgDestroyImage((VGImage)fbo->id);
  		fbo->id = VG_INVALID_HANDLE;
  	}
  	if(fbo->handle)
  	{
  		vgSurfaceDestroy (fbo->handle);
  		fbo->handle = VG_INVALID_HANDLE;
  	}
#endif
	(void)vgcanvas;

	return RET_OK;
}

#define	CREATE_SURFACE		// PC版本的surface在执行vgMakeCurrent后, surface对象的内容会保留
                           // HW版本的surface在执行vgMakeCurrent后, surface对象的内容会自动释放.因此每次surface切换后均需重新创建surface对象

static ret_t vgcanvas_nanovg_bind_fbo(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo) 
{
	NVGcontext* vg = NULL;
	vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
	
#ifdef ENABLE_FB_DIRTY_RECTS_COPY
	fbo_mode = 1;
#endif
    
	vg = canvas->vg;

#ifdef CREATE_SURFACE
	// 重绘整个区域
	//fbo->online_dirty_rect = rect_init(0, 0, canvas->offline_fb->width, canvas->offline_fb->height);
	fbo->online_dirty_rect = rect_init(vgcanvas->dirty_rect.x, vgcanvas->dirty_rect.y, vgcanvas->dirty_rect.w, vgcanvas->dirty_rect.h);
#else
	fbo->online_dirty_rect = rect_init(vgcanvas->dirty_rect.x, vgcanvas->dirty_rect.y, vgcanvas->dirty_rect.w, vgcanvas->dirty_rect.h);
#endif

	vgcanvas->dirty_rect = rect_init(0, 0, fbo->w, fbo->h);
#if defined(VGCANVAS_DIRTY_RECTS_SUPPORT) && defined(DIRTY_RECTS_CLIP_SUPPORT)
	dirty_rects_reset(&vgcanvas->dirty_rects);
	dirty_rects_add(&vgcanvas->dirty_rects, &vgcanvas->dirty_rect);
#endif	
#ifdef CREATE_FB_SURFACE	
	// 将离线fb置为VG当前active surface
	canvas->vg_surface = fbo->handle;
	current_surface = canvas->vg_surface;
	vgMakeCurrent (canvas->vg_context, canvas->vg_surface);
	
#ifdef CREATE_SURFACE
	vgSurfaceDestroy (canvas->offline_fb->surface);
	canvas->offline_fb->surface = NULL;
#endif
#endif
 
	if (!fbo->init) 
	{
		fbo->init = TRUE;
	}

	nvgBeginFrameEx(vg, fbo->w, fbo->h, fbo->ratio, FALSE);
	nvgSave(vg);
	nvgReset(vg);
  
	return RET_OK;
}

// 恢复
static ret_t vgcanvas_nanovg_unbind_fbo(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo) 
{
	NVGcontext* vg = NULL;
	vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;

#ifdef ENABLE_FB_DIRTY_RECTS_COPY
	fbo_mode = 0;
#endif
	
	vg = canvas->vg;

	nvgRestore(vg);
	nvgEndFrame(vg);
	
#ifdef CREATE_FB_SURFACE	
#ifdef CREATE_SURFACE
	// 恢复创建离线FB之前的显示FB
	// 20210821 重新创建全新的surface, 原有的surface在eglMakeCurrent调用里被摧毁
	canvas->offline_fb->surface = vgSurfaceCreate(canvas->offline_fb->width, canvas->offline_fb->height, VG_FALSE, VG_FALSE, VG_TRUE);
	if (canvas->offline_fb->surface == NULL)
		return RET_OOM;
#endif
	canvas->vg_surface = canvas->offline_fb->surface;
	current_surface = canvas->vg_surface;
	vgMakeCurrent (canvas->vg_context, canvas->vg_surface);
#endif

	vgcanvas->dirty_rect = rect_init(fbo->online_dirty_rect.x, fbo->online_dirty_rect.y, fbo->online_dirty_rect.w, fbo->online_dirty_rect.h);	
#if defined(VGCANVAS_DIRTY_RECTS_SUPPORT) && defined(DIRTY_RECTS_CLIP_SUPPORT)
	dirty_rects_reset(&vgcanvas->dirty_rects);
	dirty_rects_add(&vgcanvas->dirty_rects, &vgcanvas->dirty_rect);
#endif	
	nvgBeginFrameEx(vg, vgcanvas->w, vgcanvas->h, fbo->ratio, FALSE);

	return RET_OK;
}



static ret_t vgcanvas_nanovg_fbo_to_bitmap(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo,
                                           bitmap_t* img, const rect_t* r) 
{
	uint32_t x = 0;		// 指定fb中取景窗的偏移
	uint32_t y = 0;
	uint32_t height = 0;
	uint8_t* img_data = NULL;
	VGImage vgImage = fbo->id;
	ret_t ret = RET_OK;
		
	vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
	(void)(canvas);

	vgFinish();

	// 保存当前使用的FB
	img_data = (uint8_t*)bitmap_lock_buffer_for_write(img);
	height = (uint32_t)(fbo->h * fbo->ratio);
	(void)(height);
	(void)(vgImage);
	// 检查是否存在偏移设置
	if (r != NULL) {
		x = r->x;
		y = r->y;
	} 

#ifdef CREATE_FB_SURFACE
	// VG的坐标自底向上, IMG的坐标是自顶向下
	// AWTK的BITMAP_FMT_BGRA8888对应VG的VG_sARGB_8888
	if (img->format == BITMAP_FMT_BGRA8888)
		vgGetImageSubData(vgImage, img_data, img->line_length, VG_sARGB_8888, x, y, img->w, img->h);
	else if (img->format == BITMAP_FMT_RGB565)
		vgGetImageSubData(vgImage, img_data, img->line_length, VG_sRGB_565, x, y, img->w, img->h);
	else
		ret = RET_BAD_PARAMS;
#else
	// VG的坐标自底向上, IMG的坐标是自顶向下
	// AWTK的BITMAP_FMT_BGRA8888对应VG的VG_sARGB_8888
	if (img->format == BITMAP_FMT_BGRA8888)
		vgReadPixels( img_data, img->line_length, VG_sARGB_8888, x, y, img->w, img->h);
	else if (img->format == BITMAP_FMT_RGB565)
		vgReadPixels( img_data, img->line_length, VG_sRGB_565, x, y, img->w, img->h);
	else
		ret = RET_BAD_PARAMS;
#endif

	bitmap_unlock_buffer(img);
	// 恢复之前的VG FB
	return ret;
}

#include "vgcanvas_nanovg.inc"

static ret_t vgcanvas_nanovg_destroy(vgcanvas_t* vgcanvas) 
{
	//vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
	
  vgcanvas_asset_manager_remove_vg(vgcanvas_asset_manager(), vgcanvas);
  vgcanvas_nanovg_deinit(vgcanvas);
	
	__vgcanvas_destroy (vgcanvas);

	return RET_OK;
}
static int vgcanvas_nanovg_ensure_image(vgcanvas_nanovg_t* canvas, bitmap_t* img);


static ret_t vgcanvas_asset_manager_nanovg_font_destroy(void* vg, const char* font_name,
	void* specific) {
	int32_t id = tk_pointer_to_int(specific);
	vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vg;
	if (canvas != NULL && canvas->vg != NULL && id >= 0) {
		nvgDeleteFontByName(canvas->vg, font_name);
	}
	return RET_OK;
}

static ret_t vgcanvas_asset_manager_nanovg_bitmap_destroy(void* vg, void* specific) {
	int32_t id = tk_pointer_to_int(specific);
	vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vg;
	if (canvas != NULL && canvas->vg != NULL && id >= 0) {
		nvgDeleteImage(canvas->vg, id);
	}
	return RET_OK;
}

vgcanvas_t* vgcanvas_create(uint32_t w, uint32_t h, uint32_t stride, bitmap_format_t format,
	void* win)
{
	native_window_info_t info;
	native_window_t* window = NATIVE_WINDOW(win);
	return_value_if_fail(native_window_get_info(win, &info) == RET_OK, NULL);
	vgcanvas_nanovg_t* nanovg = (vgcanvas_nanovg_t*)TKMEM_ZALLOC(vgcanvas_nanovg_t);
	return_value_if_fail(nanovg != NULL, NULL);
	(void)format;
	do {
		nanovg->base.w = w;
		nanovg->base.h = h;
		nanovg->base.vt = &vt;
		nanovg->window = window;
		nanovg->base.ratio = info.ratio;
		nanovg->vg_surface = NULL;
		current_surface = nanovg->vg_surface;
		nanovg->base.format = format;
		
		vgcanvas_nanovg_init((vgcanvas_t*)nanovg);
		
#ifdef _WINDOWS
		CanvasvgInit();
#endif
		// 创建VG上下文
		nanovg->vg_context = vgContextCreate();
		assert(nanovg->vg_context);
		vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
		vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);

	
		nanovg->offline_fb = vgcanvas_create_offline_fb(w * info.ratio, h * info.ratio);
		if (nanovg->offline_fb == NULL)
			break;
		nanovg->vg = nvgCreateOpenVG(NVG_ANTIALIAS | NVG_STENCIL_STROKES, w, h);
		if (nanovg->vg == NULL) {
			assert(!"nvgCreateOpenVG is not supported!");
			break;
		}
		
		vgcanvas_asset_manager_add_vg(vgcanvas_asset_manager(), &(nanovg->base),
			vgcanvas_asset_manager_nanovg_bitmap_destroy,
			vgcanvas_asset_manager_nanovg_font_destroy);

		return &(nanovg->base);

	} while (0);

	__vgcanvas_destroy(&nanovg->base);

	return NULL;

}



static ret_t vgcanvas_nanovg_on_bitmap_destroy(bitmap_t* img) {
	VGImage image = (VGImage)img->specific;
	NVGcontext* vg = (NVGcontext*)(img->specific_ctx);

	if (vg != NULL && image != VG_INVALID_HANDLE) {
		vgDestroyImage(image);
	}

	img->specific = NULL;
	img->specific_ctx = NULL;
	img->specific_destroy = NULL;
	img->flags = 0;

	return RET_OK;
}

// 检查并创建VGImage
static int vgcanvas_nanovg_ensure_image(vgcanvas_nanovg_t* canvas, bitmap_t* img) 
{
	//int flag = 0;
	int32_t i = 0;
	int32_t f = 0;
	uint8_t* img_data = NULL;
	VGImage image;
	img_data = bitmap_lock_buffer_for_read(img);

	if (img->flags & BITMAP_FLAG_TEXTURE) 
	{
		i = tk_pointer_to_int(img->specific);
		if (img->flags & BITMAP_FLAG_CHANGED)
		{
			img->flags &= (~(BITMAP_FLAG_CHANGED));
			nvgUpdateImage(canvas->vg, i, img_data, img->line_length);
		}
		bitmap_unlock_buffer(img);
		//if (img->specific != VG_INVALID_HANDLE)
		{
			return (int)img->specific;
		}
	}

	//flag = (int)vgcanvas_nanovg_bitmap_flag_to_NVGimageFlags((bitmap_flag_t)(img->flags));

	switch (img->format) {
	case BITMAP_FMT_RGBA8888: {
		f = VG_sABGR_8888;
		break;
	}

	case BITMAP_FMT_BGRA8888: {
		f = VG_sARGB_8888;
		break;
	}
	case BITMAP_FMT_BGR565: {
		f = VG_sRGB_565;
		//f = VG_sBGR_565;
		break;
	}
	case BITMAP_FMT_RGB565: {
		//f = VG_sBGR_565;
		f = VG_sRGB_565;
		break;
	}
							/*case BITMAP_FMT_RGB888: {
		f = NVG_TEXTURE_RGB;
		break;
	}*/
	default: { assert(!"not supported format"); }
	}


	// canvas 仅支持RGB565及ARGB8888两种模式
#if 1
	image = vgCreateImage(f, img->w, img->h, VG_IMAGE_QUALITY_BETTER);
#else
	if (canvas->base.format == BITMAP_FMT_RGB565)
		image = vgCreateImage (VG_sRGB_565, img->w, img->h, VG_IMAGE_QUALITY_BETTER);
	else if(canvas->base.format == BITMAP_FMT_RGBA8888)
		image = vgCreateImage(VG_sABGR_8888, img->w, img->h, VG_IMAGE_QUALITY_BETTER);
	else
		//image = vgCreateImage (VG_sRGBA_8888, img->w, img->h, VG_IMAGE_QUALITY_BETTER);
		image = vgCreateImage (VG_sARGB_8888, img->w, img->h, VG_IMAGE_QUALITY_BETTER);
#endif

	if (image != VG_INVALID_HANDLE)
	{
		vgImageSubData(image, img_data, img->line_length, f, 0, 0, img->w, img->h);
		//vgFinish();
		img->specific = (void *)image;
		img->specific_ctx = canvas->vg;
		img->flags |= BITMAP_FLAG_TEXTURE;
		img->specific_destroy = vgcanvas_nanovg_on_bitmap_destroy;

		image_manager_update_specific(img->image_manager, img);
	}

	bitmap_unlock_buffer(img);

	return (int)image;
}

// 检查并创建VGImage
static int vgcanvas_nanovg_ensure_sub_image(vgcanvas_nanovg_t* canvas, bitmap_t* img, int x, int y, int w, int h)
{
	//int flag = 0;
	//int32_t i = 0;
	int32_t f = 0;
	uint8_t* img_data;
	VGImage image;
	img_data = bitmap_lock_buffer_for_read(img);

	switch (img->format) {
	case BITMAP_FMT_RGBA8888: {
		f = VG_sABGR_8888;
		break;
	}

	case BITMAP_FMT_BGRA8888: {
		f = VG_sARGB_8888;
		break;
	}
	case BITMAP_FMT_BGR565: {
		f = VG_sRGB_565;
		//f = VG_sBGR_565;
		break;
	}
	case BITMAP_FMT_RGB565: {
		//f = VG_sBGR_565;
		f = VG_sRGB_565;
		break;
	}
	default: 
		{ assert(!"not supported format"); }
	}
	
#if 0
  unsigned int TotalFreeBytes,  MaximumAllocateBytes;
  TotalFreeBytes = 0;
  MaximumAllocateBytes = 0;
  vgScanVideoMemoryUsage (&TotalFreeBytes, &MaximumAllocateBytes);
  printf ("TotalFreeBytes = 0x%x, MaximumAllocateBytes=0x%x\n", TotalFreeBytes, MaximumAllocateBytes);
#endif
	// canvas 仅支持RGB565及ARGB8888两种模式
	image = vgCreateImage(f, w, h, VG_IMAGE_QUALITY_BETTER);
	if (image != VG_INVALID_HANDLE)
	{
		img_data += img->line_length * y;
		if (f == VG_sABGR_8888 || f == VG_sARGB_8888)
			img_data += x * 4;
		else
			img_data += x * 2;
		//img_data += img->line_length * (h - 1);
		vgImageSubData(image, img_data, img->line_length, f, 0, 0, w, h);
	}

	bitmap_unlock_buffer(img);

	return (int)image;
}

static void vgcanvas_nanovg_free_sub_image(vgcanvas_nanovg_t* canvas, int image)
{
	vgDestroyImage((VGImage)image);
}


void vgcanvas_nanovg_free_image(vgcanvas_nanovg_t* canvas, bitmap_t* img) 
{
	uint8_t* img_data;
	VGImage image = (VGImage)img->specific;
	(void)(image);
	NVGcontext* vg = (NVGcontext*)(img->specific_ctx);
	(void)(vg);
	img_data = bitmap_lock_buffer_for_read(img);
	(void)(img_data);
	if (img->flags & BITMAP_FLAG_TEXTURE) 
	{
	//	vgDestroyImage(image);
		vgcanvas_nanovg_on_bitmap_destroy (img);
	//	img->flags &= ~BITMAP_FLAG_TEXTURE;
	}
	
	bitmap_unlock_buffer(img);
}

void clear_rect(float x, float y, float w, float h, float a, float r, float g, float b) {
	VGfloat m[9];
	// save current VG_MATRIX_MODE
	VGint matrix_mode = vgGeti(VG_MATRIX_MODE);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	// save the current matrix
	vgGetMatrix(m);
	vgLoadIdentity();

	VGfloat clear_color[4] = {r, g, b, a};
	vgSetfv(VG_CLEAR_COLOR, 4, clear_color);
	vgClear(x, y, w, h);

	vgFinish();
	// restore the old matrix
	vgLoadMatrix(m);
	// restore the old VG_MATRIX_MODE
	vgSeti(VG_MATRIX_MODE, matrix_mode);
}

#endif
#include <stdarg.h>

#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"

#include "rtos.h"
#include "romfile.h"
#include "vg_driver.h"
#ifdef AWTK
#include "tkc/utils.h"
#endif
#if !defined(VG_ONLY) && !defined(AWTK)
#include "lvgl/lvgl.h"
#endif

#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
#define VG_USE_LCD_TMP_BUFFER
#endif

#ifdef VG_USE_LCD_TMP_BUFFER
static void *xm_vg_tmp_lcd_buf = NULL;
static uint32_t xm_vg_get_tmp_lcd_buffer (void)
{
	if(!xm_vg_tmp_lcd_buf) {
		u32 width = (VG_OSD_W + 0xF) & (~0xF);
		u32 height = (VG_OSD_H + 0xF) & (~0xF);
		xm_vg_tmp_lcd_buf  = (void *)pvPortMalloc(width * height *	VG_BPP / 8);
	}
	if(!xm_vg_tmp_lcd_buf) {
		printf("%s, pvPortMalloc failed\n", __FUNCTION__);
		return 0;
	}

	return (uint32_t)xm_vg_tmp_lcd_buf;
}

static void xm_vg_release_tmp_lcd_buffer (void)
{
	if(xm_vg_tmp_lcd_buf)
		vPortFree(xm_vg_tmp_lcd_buf);
	xm_vg_tmp_lcd_buf = NULL;
}
#endif

#ifdef VG_DRIVER
#include "xm_base.h"

uint32_t FS_FWrite(const void * pData, uint32_t Size, uint32_t N, void * pFile)
{
	return 0;
}

int32_t FS_FTell(void * pFile)
{
	return 0;
}

unsigned int xm_vg_get_width (void)
{
	return VG_W;
}

unsigned int xm_vg_get_height (void)
{
	return VG_H;
}

unsigned int xm_vg_get_stride (void)
{
	return VG_OSD_W * VG_BPP / 8;
}

unsigned int xm_vg_get_bpp (void)
{
	return VG_BPP;
}

#if defined(REVERSE_TRACK)
int xm_vg_is_valid_gpu_fb (unsigned int base)
{
	int i;

	for (i = 0; i < VIDEO_DISPLAY_BUF_NUM; i++) {
		unsigned fb_base = ulVideoDisplayGetBufferAddr(i);
		unsigned vg_base = fb_base + VG_Y * VG_OSD_W * VG_BPP / 8 + VG_X *  VG_BPP / 8;

		if (base == vg_base)
			return 1;
	}

	return 0;
}
#else
int xm_vg_is_valid_gpu_fb (unsigned int base)
{
	unsigned fb_base = ark_lcd_get_virt_addr();
	unsigned vg_base = fb_base + VG_Y * VG_OSD_W * VG_BPP / 8 + VG_X *  VG_BPP / 8;

	if (base == vg_base || base == vg_base + xm_vg_get_stride() * VG_OSD_H)
		return 1;

	return 0;
}
#endif

void* XM_RomAddress (const char *src)
{
	if (!src || src[0] == '\0') return NULL;

	void *ptr;
	RomFile *file = RomFileOpen(src);

	if (!file) {
		printf("open romfile %s fail.\n", src);
		return NULL;
	}
	RomFileRead(file, NULL, file->size);
	ptr = file->buf;
#ifndef ROMFILE_USE_SMALL_MEM
	RomFileClose(file);
#endif

	return ptr;
}


#ifdef LVGL_VG_GPU
unsigned int xm_vg_require_gpu_fb (void)
{
    lv_disp_t * disp = lv_disp_get_default();
	unsigned int vg_frame_base = 0;

	if (disp) {
	    lv_disp_buf_t * vdb = lv_disp_get_buf(disp);
	    lv_color_t * disp_buf = vdb->buf_act;
		vg_frame_base = (unsigned int)disp_buf;
	} else {
		vg_frame_base = ark_lcd_get_virt_addr();
	}

	vg_frame_base += VG_Y * VG_OSD_W * VG_BPP / 8 + VG_X *  VG_BPP / 8;
	return 	vg_frame_base;
}

#else

#if defined(VG_ONLY)

static int fb_index = 0;
unsigned int xm_vg_require_gpu_fb (void)
{
#ifdef VG_USE_LCD_TMP_BUFFER
	return xm_vg_get_tmp_lcd_buffer();
#else
	unsigned fb_base = ark_lcd_get_virt_addr();
	fb_base += fb_index * VG_OSD_W * VG_OSD_H *  VG_BPP / 8;

	return fb_base;
#endif
}

void xm_vg_release_gpu_fb (void)
{
#ifdef VG_USE_LCD_TMP_BUFFER

#else
	unsigned fb_base = xm_vg_require_gpu_fb();
	CP15_clean_dcache_for_dma((uint32_t)fb_base, VG_OSD_W * VG_OSD_H *  VG_BPP / 8 + (uint32_t)fb_base);
	ark_lcd_set_osd_yaddr(LCD_UI_LAYER, (unsigned int)fb_base);
	ark_lcd_set_osd_sync(LCD_UI_LAYER);

	ark_lcd_wait_for_vsync();

	fb_index = (fb_index + 1) & 1;
#endif
}

// 获取当前正在渲染的OSD framebuffer
//   *no == -1 表示获取当前正在渲染的OSD framebuffer
// 返回值
//		当前正在渲染的OSD framebuffer基址
unsigned int xm_vg_get_osd_fb (int *no)
{
	unsigned fb_base = ark_lcd_get_virt_addr();
	fb_base += fb_index * VG_OSD_W * VG_OSD_H *  VG_BPP / 8;
	if(*no == (-1))
	{
		*no = fb_index;
		return fb_base;
	}
	else
	{
		if(*no >= 2)
			return 0;
		else
			return fb_base;
	}
}

#elif defined(AWTK)

#include "fb_queue.h"

static fb_queue_s *curr_fb = NULL;
unsigned int xm_vg_require_gpu_fb (void)
{
	fb_queue_s *fb_unit = NULL;
	while (!fb_unit)
	{
		fb_unit = fb_queue_get_free_unit();
		if(fb_unit)
			break;
		OS_Delay (1);
	}
	curr_fb = fb_unit;
	//XM_lock();
	//printf ("req %x\n", fb_unit->fb_base);
	//XM_unlock();
	return fb_unit->fb_base;
}

void xm_vg_release_gpu_fb (void)
{
	fb_queue_s *fb_unit = curr_fb;
	if(fb_unit)
	{
		curr_fb = NULL;
		fb_queue_set_ready (fb_unit);
		//XM_lock();
		//printf ("rdy %x\n", fb_unit->fb_base);
		//XM_unlock();
	}
}


#elif defined(REVERSE_TRACK)

static unsigned int gpu_fb_addr = 0;

void xm_vg_set_gpu_fb_addr(unsigned int addr)
{
	gpu_fb_addr = addr;
}

unsigned int xm_vg_require_gpu_fb (void)
{
	unsigned int vg_frame_base;

	vg_frame_base = gpu_fb_addr ? gpu_fb_addr : ulVideoDisplayGetBufferAddr(0);

	vg_frame_base += VG_Y * VG_OSD_W * VG_BPP / 8 + VG_X *  VG_BPP / 8;
	return 	vg_frame_base;
}

#else
unsigned int xm_vg_require_gpu_fb (void)
{
    lv_disp_t * disp = lv_disp_get_default();
    lv_disp_buf_t * vdb = lv_disp_get_buf(disp);
    lv_color_t * disp_buf = vdb->buf_act;

	unsigned int vg_frame_base = (unsigned int)disp_buf;

	vg_frame_base += VG_Y * VG_OSD_W * VG_BPP / 8 + VG_X *  VG_BPP / 8;
	return 	vg_frame_base;
}
#endif
#endif



int xm_vg_get_offset_x (void)
{
	return VG_X;
}

int xm_vg_get_offset_y (void)
{
	return VG_Y;
}

unsigned int xm_vg_get_osd_stride (void)
{
	return VG_OSD_W * VG_BPP / 8;
}

void xm_vg_get_osd_window (	unsigned int* x,
										unsigned int* y,
										unsigned int* w,
										unsigned int* h,
										unsigned int* stride
									)
{
	*x = VG_X;
	*y = VG_Y;
	*w = VG_W;
	*h = VG_H;
	*stride = xm_vg_get_stride();
}

void* xm_vg_get_gpu_background_image (void)
{
	static RomFile *bgfile = NULL;

	if(!bgfile) {
		bgfile = RomFileOpen("vg_bg.rgb");
		if (!bgfile)
			return NULL;
		RomFileRead(bgfile, NULL, bgfile->size);
	}
	char *bk = (char*)bgfile->buf;
	if(bk)
		bk += (VG_H - 1) * VG_W * VG_BPP / 8;
	return (void *)bk;
}

unsigned int vg_get_framebuffer (void)
{
	return xm_vg_require_gpu_fb();	
}

#ifndef LVGL_VG_GPU

#if defined(VG_ONLY) || defined(AWTK)
#if	defined(DOUBLE_POINTER_HALO)
extern int double_pointer_halo_init (int width, int height);
extern int double_pointer_halo_draw (void);
#elif defined(SINGLE_POINTER_HALO)
extern int single_pointer_halo_init (int width, int height);
extern int single_pointer_halo_draw (void);
#elif defined(AWTK)
extern int gui_app_start(int lcd_w, int lcd_h);
#endif
#include <stdlib.h>

int xm_vg_loop (void *context)
{
#ifdef AWTK
	// 修改AWTK UI线程名称
	char*strName = pcTaskGetName(NULL);
	tk_snprintf(strName, configMAX_TASK_NAME_LEN-1, "awtk_ui");

	// 修改AWTK线程优先级为16
	vTaskPrioritySet(NULL, 3);

#ifdef WITH_VGCANVAS
	gui_app_start (xm_vg_get_width(), xm_vg_get_height());
#else
	gui_app_start (OSD_WIDTH, OSD_HEIGHT);
#endif
	return 0;
#else
	int frame_count = 0;
	int vg_draw_tickets = 0;	// 累加VG时间
	int vsync_delay_tickets = 0;	// 累加LCD帧同步等待时间

#if	defined(DOUBLE_POINTER_HALO)
	double_pointer_halo_init(xm_vg_get_width(), xm_vg_get_height());
#elif defined(SINGLE_POINTER_HALO)
	single_pointer_halo_init(xm_vg_get_width(), xm_vg_get_height());
#endif
	unsigned int start_ticket = XM_GetTickCount();

	while(1)
	{
		// 获取当前正在渲染使用的OSD framebuffer基址及序号
		//int fb_no = -1;
		//unsigned int fb_base = xm_vg_get_osd_fb(&fb_no);
		int t1 = get_timer(0);

#if	defined(DOUBLE_POINTER_HALO)
		double_pointer_halo_draw();
#elif defined(SINGLE_POINTER_HALO)
		single_pointer_halo_draw();
#endif
		int t2 = get_timer(0);
		vg_draw_tickets += abs(t2 - t1);

#ifdef VG_USE_LCD_TMP_BUFFER
			static int fb_index = 0;
			unsigned int fb_base = ark_lcd_get_virt_addr();
			fb_base += fb_index * VG_OSD_W * VG_OSD_H *  VG_BPP / 8;
			uint32_t src_addr = xm_vg_get_tmp_lcd_buffer();

			CP15_invalidate_dcache_for_dma((uint32_t)fb_base, VG_OSD_W * VG_OSD_H *	VG_BPP / 8 + (uint32_t)fb_base);
			pxp_scaler_rotate(src_addr, 0, 0,
					PXP_SRC_FMT_RGB565, VG_OSD_W, VG_OSD_H,
					(uint32_t)fb_base, 0, PXP_OUT_FMT_RGB565, LCD_WIDTH, LCD_HEIGHT, LCD_ROTATE_ANGLE);

			ark_lcd_set_osd_yaddr(LCD_UI_LAYER, (unsigned int)fb_base);
			ark_lcd_set_osd_sync(LCD_UI_LAYER);
			ark_lcd_wait_for_vsync();
			fb_index != fb_index;
#else
		// 释放当前渲染的GPU帧, 并等待LCD刷新该帧
		xm_vg_release_gpu_fb ();
#endif

		int t3 = get_timer(0);
		vsync_delay_tickets += abs(t3 - t2);
		
		frame_count ++;
		
		if(frame_count == 1000)
		{
			unsigned int t = XM_GetTickCount() - start_ticket;
			printf ("fps = %4.1f , vg_ticket = %d us, vsync_delay_ticket = %d us\r\n", 1000.0*1000.0 / t,  vg_draw_tickets / 1000, vsync_delay_tickets / 1000);
			vg_draw_tickets = 0;
			vsync_delay_tickets = 0;
			frame_count = 0;
			start_ticket = XM_GetTickCount();
		}
		//OS_Delay(1);

	}

	//double_pointer_halo_exit();
	//vgFinish();

#ifdef VG_USE_LCD_TMP_BUFFER
	xm_vg_release_tmp_lcd_buffer();
#endif

	return 0;
#endif
}
#else
#ifdef REVERSE_TRACK
static int track_index = 0;
#else
static int pointer_halo_speed = 0;
#endif
static TaskHandle_t vg_task = NULL;
QueueHandle_t vg_done;

void xm_vg_draw_prepare(void *para)
{
	if (para == NULL)
		return;

#ifdef REVERSE_TRACK
	track_index = *(int*)para;
#else
	pointer_halo_speed = *(int*)para;
#endif
}

void xm_vg_draw_start(void)
{
	if (vg_done)
		xQueueReset(vg_done);
	
	if (vg_task)
		xTaskNotify(vg_task, 1, eSetValueWithOverwrite);

	if (vg_done)
		xQueueReceive(vg_done, NULL, portMAX_DELAY);
}

#ifdef REVERSE_TRACK
extern int reversing_auxiliary_line_init (int width, int height);
extern int reversing_auxiliary_line_draw (int index);
#else
extern int pointer_halo_init (int width, int height);
extern int pointer_halo_draw (int speed);
#endif

int xm_vg_loop (void *context)
{
	uint32_t ulNotifiedValue;
	//uint32_t stick = xTaskGetTickCount();
	//uint32_t framecount = 0;

	vg_task = xTaskGetCurrentTaskHandle();
	
	vg_done = xQueueCreate(1, 0);
	
#ifdef REVERSE_TRACK
	reversing_auxiliary_line_init(xm_vg_get_width(), xm_vg_get_height());
#else
	pointer_halo_init(xm_vg_get_width(), xm_vg_get_height());
#endif

	while(1)
	{
		xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
				0xffffffff, /* Reset the notification value to 0 on exit. */
				&ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
				portMAX_DELAY);			

#ifdef REVERSE_TRACK
		reversing_auxiliary_line_draw(track_index);
#else
		pointer_halo_draw(pointer_halo_speed);
#endif

		/* if (++framecount == 1000) {
			uint32_t tick = xTaskGetTickCount();
			printf("fps %d.\n", 1000 * 1000 / (tick - stick));
			framecount = 0;
			stick = xTaskGetTickCount();
		} */

		xQueueSend(vg_done, NULL, 0);
	}

#ifdef REVERSE_TRACK
	//reversing_auxiliary_line_exit();
#else
	//pointer_halo_exit();
#endif
	//vgFinish();

	return 0;
}
#endif
#endif

#ifdef LVGL_VG_GPU

extern void lvgl_thread(void *data);

int xm_vg_loop (void *context)
{
	lvgl_thread(NULL);

	return 0;
}

#endif

#endif

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "carlink_video.h"
#include "mfcapi.h"
#include "lcd.h"
#include "pxp.h"
#include "cp15/cp15.h"
#include "timer.h"
#include "task.h"
#include "semphr.h"
#include "os_adapt.h"
#include "ff_stdio.h"
#include "vehicle_param/vehicle_param.h"

#if !DISABLE_CARLINK_H264_FRAME_BUF
static List_t								frame_free_list;
static List_t								frame_ready_list;
static video_frame_s						h264_frame_fifos[H264_FRAME_FIFO_COUNT];
static char*								h264_buf = NULL;
static SemaphoreHandle_t					frame_list_mutex = NULL;
static TaskHandle_t							dec_h264_handle;
static QueueHandle_t						h264_frame_queue;
static uint8_t								g_init_flag = 0;
static uint8_t								g_video_start = 0;
static int g_disp_x = 0, g_disp_y = 0, g_disp_width = LCD_WIDTH, g_disp_height = LCD_HEIGHT;
static int g_video_width = LCD_WIDTH, g_video_height = LCD_HEIGHT, g_video_fps = 30;
static int g_hide_carlink_flag = 0;

static int g_active_video_x = 0, g_active_video_y = 0;

void set_carlink_active_video_info(int x, int y)//for android auto
{
	g_active_video_x = x;
	g_active_video_y = y;
}


void set_carlink_video_info(int w, int h, int fps)
{
	g_video_width	= w;
	g_video_height	= h;
	g_video_fps		= fps;
}

int get_carlink_video_width()
{
	return g_video_width;
}

int get_carlink_video_height()
{
	return g_video_height;
}

int get_carlink_video_fps()
{
	return g_video_fps;
}

void set_carlink_display_info(int x, int y, int w, int h)
{
	g_disp_x		= x;
	g_disp_y		= y;
	g_disp_width	= w;
	g_disp_height	= h;
}

static void h264_frame_list_reset()
{
	video_frame_s *buffer;
	int i;

	buffer = (video_frame_s *)h264_frame_fifos;
	vListInitialise(&frame_free_list);
	vListInitialise(&frame_ready_list);
	for(i = 0; i < H264_FRAME_FIFO_COUNT; i++) {
		buffer->buf = (char *)(((unsigned int)h264_buf + i * H264_FRAME_BUF_SIZE));
		buffer->cur = buffer->buf;
		buffer->frame_id = i;
		buffer->len = 0;
		vListInitialiseItem(&buffer->entry);
		vListInsertEnd(&frame_free_list, &buffer->entry);
		listSET_LIST_ITEM_OWNER(&buffer->entry, buffer);
		buffer++;
	}
}

static int h264_frame_buf_init(void)
{
	h264_buf = pvPortMalloc(H264_FRAME_FIFO_COUNT * H264_FRAME_BUF_SIZE );
	if (h264_buf == NULL) {
		printf("malloc jpeg buf failed\r\n");
		return -1;
	}
	h264_frame_list_reset();

	return 0;
}

void h264_dec_ctx_init()
{
	g_video_start = 0;
	xQueueSend(h264_frame_queue, NULL, 0);
}

void set_carlink_display_state(int on)
{
	g_hide_carlink_flag = on;
	xQueueSend(h264_frame_queue, NULL, 0);
}


void set_carlink_display_state_irq(int on)
{
	g_hide_carlink_flag = on;

	xQueueSendFromISR(h264_frame_queue, NULL, 0);
}

video_frame_s* get_h264_frame_buf()
{
	video_frame_s* frame = NULL;
	ListItem_t* item = NULL;
	xSemaphoreTake(frame_list_mutex, portMAX_DELAY);
	if (g_video_start && !listLIST_IS_EMPTY(&frame_free_list)) {
		frame = list_first_entry(&frame_free_list);
		item = listGET_HEAD_ENTRY(&frame_free_list);
		uxListRemove(item);
	}
	xSemaphoreGive(frame_list_mutex);
	return frame;
}


void notify_h264_frame_ready(video_frame_s** frame)
{
	//ListItem_t* item = NULL;
	video_frame_s * tmp = *frame;
	xSemaphoreTake(frame_list_mutex, portMAX_DELAY);
	if (NULL != tmp) {
		vListInsertEnd(&frame_ready_list, &tmp->entry);
	}
	xSemaphoreGive(frame_list_mutex);

	xQueueSend(h264_frame_queue, NULL, 0);
}

void set_h264_frame_free(video_frame_s* frame)
{
	xSemaphoreTake(frame_list_mutex, portMAX_DELAY);
	if (g_video_start) {
		frame->cur = frame->buf;
		frame->len = 0;
		vListInsertEnd(&frame_free_list, &frame->entry);
	}
	xSemaphoreGive(frame_list_mutex);
}

static void h264_decode_proc(void *pvParameters)
{
	MFCHandle *handle = NULL;
	DWLLinearMem_t inBuf;
	OutFrameBuffer outBuf = {0};
	int ret = -1, i;
	uint32_t align_width, align_height;
	uint32_t yaddr, uaddr, vaddr, dstaddr;
	video_frame_s* frame = NULL;
	uint32_t ts = get_timer(0), show_ts = 0, video_ts = 0;
	int32_t delta_ts = 0;;
	int carlink_lcd_take = 0;
	ListItem_t* item = NULL;
	//FF_FILE *fp = ff_fopen("/usb/ey.h264", "w+");

	handle = mfc_init(RAW_STRM_TYPE_H264_NOREORDER);
	if(!handle) {
		printf("%s, mfc_init failed.\r\n", __func__);
		goto exit;
	}
	g_video_start = 1;
	while(1) {
	 if (xQueueReceive(h264_frame_queue, NULL, portMAX_DELAY) != pdPASS) {
		printf("%s xQueueReceive err!\r\n", __func__);
		continue;
		}
		if (g_video_start == 0) {
			printf("dec uninit\r\n");
			if (carlink_lcd_take) {
				ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
				ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
				vVideoDisplayBufGive();
				ark_lcd_osd_enable(LCD_UI_LAYER, 1);
				ark_lcd_set_osd_sync(LCD_UI_LAYER);
				carlink_lcd_take = 0;
			}
			mfc_uninit(handle);
			xSemaphoreTake(frame_list_mutex, portMAX_DELAY);
			h264_frame_list_reset();
			xSemaphoreGive(frame_list_mutex);
			xQueueReset(h264_frame_queue);
			handle = mfc_init(RAW_STRM_TYPE_H264_NOREORDER);
			if(!handle) {
				printf("%s, mfc_reinit failed.\r\n", __func__);
				continue;
			}
			g_video_start = 1;
			continue;
		}

		if (g_hide_carlink_flag == 0) {
			ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
			ark_lcd_osd_enable(LCD_UI_LAYER, 1);
			ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
			ark_lcd_set_osd_sync(LCD_UI_LAYER);
		}
		xSemaphoreTake(frame_list_mutex, portMAX_DELAY);
		if (!listLIST_IS_EMPTY(&frame_ready_list)) {
			frame = list_first_entry(&frame_ready_list);
			item = listGET_HEAD_ENTRY(&frame_ready_list);
			uxListRemove(item);
		} else {
			xSemaphoreGive(frame_list_mutex);
			continue;
		}
		xSemaphoreGive(frame_list_mutex);

		memset(&outBuf, 0, sizeof(OutFrameBuffer));
		inBuf.busAddress		= (u32)frame->cur;
		inBuf.virtualAddress	= (u32 *)inBuf.busAddress;
		inBuf.size				= frame->len;
		CP15_clean_dcache_for_dma(inBuf.busAddress, inBuf.busAddress + inBuf.size);
		ts = get_timer(0);
		//printf("start dec...\r\n");
		ret = mfc_decode(handle, &inBuf, &outBuf);
		//ret = 0;
		//if (NULL != fp)
		//	ret = ff_fwrite((void*)inBuf.virtualAddress, 1, inBuf.size, fp);

		xSemaphoreTake(frame_list_mutex, portMAX_DELAY);
		frame->cur = frame->buf;
		frame->len = 0;
		vListInsertEnd(&frame_free_list, &frame->entry);
		xSemaphoreGive(frame_list_mutex);

		if (ret < 0) {
			printf("h264 decode fail.\n");
			continue;
		}//printf("dec num:%d use:%d ms\r\n", outBuf.num, (get_timer(0) - ts) / 1000);
		//continue;

		for(i = 0; i < outBuf.num; i++) {
			int active_offset = 0;
			align_width = outBuf.frameWidth;
			align_height = outBuf.frameHeight;

			if(!(align_width && align_height))
				continue;
			active_offset = g_active_video_y * align_width;
			yaddr = outBuf.buffer[i].yBusAddress + active_offset + g_active_video_x;
			uaddr = outBuf.buffer[i].yBusAddress + align_width * align_height + active_offset / 2 + g_active_video_x;
			vaddr = 0;//yaddr + align_width * align_height * 5/4;

			if (carlink_lcd_take == 0) {
				carlink_lcd_take = 1;
				xVideoDisplayBufTake(portMAX_DELAY);
			}
			if (carlink_lcd_take) {
				LcdOsdInfo info = {0};

				if (0 == g_hide_carlink_flag)
					continue;

				show_ts = get_timer(0);

				info.format = LCD_OSD_FORAMT_Y_UV420;	//set default format.
				if ((info.format == LCD_OSD_FORAMT_RGB565) || \
					(g_disp_width != g_video_width) || (g_disp_height != g_video_height)) {
					uint32_t pxp_width, pxp_height;
					int pxp_out_fmt = PXP_OUT_FMT_YUV2P420;

					if(info.format == LCD_OSD_FORAMT_RGB565)
						pxp_out_fmt = PXP_OUT_FMT_RGB565;

					//align_width = ((outBuf.frameWidth + 0xf) & (~0xf));
					if (outBuf.frameWidth != g_video_width)
						align_width = g_video_width;
					else
						align_width = outBuf.frameWidth ;
					if (outBuf.frameHeight != g_video_height)
						align_height = g_video_height;
					else
						align_height = outBuf.frameHeight;

					pxp_width = g_disp_width;
					pxp_height = g_disp_height;
					dstaddr = ulVideoDisplayBufGet();

					pxp_scaler_rotate(yaddr, uaddr, vaddr, PXP_SRC_FMT_YUV2P420, align_width, align_height,
						dstaddr, dstaddr + pxp_width * pxp_height, pxp_out_fmt, pxp_width, pxp_height, LCD_ROTATE_ANGLE);
					info.yaddr = dstaddr;
					info.uaddr = dstaddr + pxp_width * pxp_height;
					info.vaddr = 0;
				} else {
					info.yaddr = yaddr;
					info.uaddr = uaddr;
					info.vaddr = vaddr;
				}
				info.x = g_disp_x;
				info.y = g_disp_y;
				info.width = g_disp_width;
				info.height = g_disp_height;
				if (g_active_video_x != 0)
					info.stride = outBuf.frameWidth;

				if (!ark_lcd_get_osd_info_atomic_isactive(LCD_VIDEO_LAYER)) {
					ark_lcd_wait_for_vsync();
				}

				ark_lcd_set_osd_info_atomic(LCD_VIDEO_LAYER, &info);
				ark_lcd_osd_enable(LCD_VIDEO_LAYER, 1);
				ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
				ark_lcd_osd_enable(LCD_UI_LAYER, 1);
				ark_lcd_set_osd_sync(LCD_UI_LAYER);
				vVideoDisplayBufRender(dstaddr);
				if (outBuf.num > 1) {// for apple h264
					video_ts = (1000 / g_video_fps);
					show_ts = (get_timer(0) - show_ts) / 1000;
					delta_ts =  ((int32_t)video_ts - (int32_t)show_ts) -3;
					//printf("delay %d ms\r\n", delta_ts);
					if (delta_ts > 0)
						vTaskDelay(pdMS_TO_TICKS(delta_ts));
				}
			}
		}
		printf("dec num:%d use:%d ms\r\n", outBuf.num, (get_timer(0) - ts) / 1000);
		//printf("all use:%d ms dec:%ld ms all ts:%ld ms\r\n",  (get_timer(0) - ts) / 1000, dec_ts, (get_timer(0) - all_ts) / 1000);
		//all_ts = get_timer(0);
	}

exit:
	if (NULL != handle)
		mfc_uninit(handle);

	vTaskDelete(NULL);
}

int carlink_ey_video_init()
{
	int ret = -1;

	if (g_init_flag ==1) {
		return 0;
	}
	g_init_flag = 1;
	ret = h264_frame_buf_init();
	frame_list_mutex = xSemaphoreCreateMutex();
	//h264_frame_queue = xQueueCreate(H264_FRAME_FIFO_COUNT, sizeof(uint32_t));
	h264_frame_queue = xQueueCreate(1, 0);
	ret = xTaskCreate(h264_decode_proc, "h264_decode_proc", 2048 * 4, NULL, configMAX_PRIORITIES - 3, &dec_h264_handle);
	return ret;
}

#else
static int g_disp_x = 0, g_disp_y = 0, g_disp_width = LCD_WIDTH, g_disp_height = LCD_HEIGHT;
static int g_video_width = LCD_WIDTH, g_video_height = LCD_HEIGHT, g_video_fps = 30;
static int g_hide_carlink_flag = 0;
//static int g_carlink_take_video = 0;
static int g_active_video_x = 0, g_active_video_y = 0;

void set_carlink_active_video_info(int x, int y)//for android auto
{
	g_active_video_x = x;
	g_active_video_y = y;
}


void set_carlink_video_info(int w, int h, int fps)
{
	g_video_width	= w;
	g_video_height	= h;
	g_video_fps		= fps;
}

int get_carlink_video_width()
{
	return g_video_width;
}

int get_carlink_video_height()
{
	return g_video_height;
}

int get_carlink_video_fps()
{
	return g_video_fps;
}

void set_carlink_display_info(int x, int y, int w, int h)
{
	g_disp_x		= x;
	g_disp_y		= y;
	g_disp_width	= w;
	g_disp_height	= h;
}


void h264_dec_ctx_init()
{
}

void set_carlink_display_state(int on)
{
	g_hide_carlink_flag = on;

	if (!on) {
		ark_lcd_osd_enable(LCD_OSD0, 0);
		ark_lcd_set_osd_sync(LCD_OSD0);
		ark_lcd_osd_enable(LCD_OSD1, 0);
		ark_lcd_set_osd_sync(LCD_OSD1);
	}
}


void set_carlink_display_state_irq(int on)
{
	set_carlink_display_state(on);
}


void* h264_video_player_init()
{
	MFCHandle *handle = NULL;
	
	handle = mfc_init(RAW_STRM_TYPE_H264_NOREORDER);
	if(!handle) {
		printf("%s, mfc_init failed.\r\n", __func__);
		return NULL;
	}

	g_hide_carlink_flag = 1;

	return (void*)handle;
}

void h264_video_player_uninit(void* h264_Handle)
{
	MFCHandle *handle = (MFCHandle*)h264_Handle;
			
	if (NULL != handle) {
		mfc_uninit(handle);
	}
	g_hide_carlink_flag = 0;

	ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
	ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
	ark_lcd_osd_enable(LCD_UI_LAYER, 1);
	ark_lcd_set_osd_sync(LCD_UI_LAYER);
}

int h264_video_player_proc(void* h264_Handle, const char *h264_buf, int h264_buf_len)
{
	MFCHandle *handle = (MFCHandle*)h264_Handle;
	DWLLinearMem_t inBuf;
	OutFrameBuffer outBuf = {0};
	int ret = -1, i;
	uint32_t align_width, align_height;
	uint32_t yaddr, uaddr, vaddr, dstaddr;
	uint32_t ts = get_timer(0), show_ts = 0, video_ts = 0;
	int32_t delta_ts = 0;

	if (NULL == h264_Handle) {
		return -1;
	}

	inBuf.busAddress		= (u32)h264_buf;
	inBuf.virtualAddress	= (u32 *)h264_buf;
	inBuf.size				= h264_buf_len;
	CP15_clean_dcache_for_dma(inBuf.busAddress, inBuf.busAddress + inBuf.size);
	ts = get_timer(0);
	ret = mfc_decode(handle, &inBuf, &outBuf);
	if (ret != 0) {
		printf("mfc_decode err.\n\n");
		return -1;
	}

	if (0 == g_hide_carlink_flag)
		return 0;

	/* Only render to LCD_VIDEO_LAYER when link_page is in foreground.
	 * On home_page the video layer must stay dark — otherwise the 30fps
	 * VIDEO/UI layer toggling causes visible flicker.  H264 decode above
	 * still runs, keeping the carlink session alive for nav data. */
	if (!link_api_get_preview_enable())
		return 0;

	/* Render gate: re-check the volatile gate AFTER decode completes but
	 * BEFORE any LCD layer operation.  link_api_set_preview_enable(0) closes
	 * this gate as the very first step, so even a frame that passed the
	 * link_api_get_preview_enable() check above will be caught here if the
	 * disable happened between that check and now.  This closes the race
	 * window that caused sporadic flicker on home_page. */
	if (!g_link_render_gate)
		return 0;

	for(i = 0; i < outBuf.num; i++) {
		int active_offset = 0;
		align_width = outBuf.frameWidth;
		align_height = outBuf.frameHeight;

		if(!(align_width && align_height))
			continue;

		/* Per-frame re-check: if gate closed mid-loop (multi-frame Apple
		 * H264 packets where outBuf.num > 1), abort immediately. */
		if (!g_link_render_gate)
			break;

		active_offset = g_active_video_y * align_width;
		yaddr = outBuf.buffer[i].yBusAddress + active_offset + g_active_video_x;
		uaddr = outBuf.buffer[i].yBusAddress + align_width * align_height + active_offset / 2 + g_active_video_x;
		vaddr = 0;

		LcdOsdInfo info = {0};
		show_ts = get_timer(0);

		info.format = LCD_OSD_FORAMT_Y_UV420;	//set default format.
		if ((info.format == LCD_OSD_FORAMT_RGB565) || \
			(g_disp_width != g_video_width) || (g_disp_height != g_video_height)) {
			uint32_t pxp_width, pxp_height;
			int pxp_out_fmt = PXP_OUT_FMT_YUV2P420;

			if(info.format == LCD_OSD_FORAMT_RGB565)
				pxp_out_fmt = PXP_OUT_FMT_RGB565;

			//align_width = ((outBuf.frameWidth + 0xf) & (~0xf));
			if (outBuf.frameWidth != g_video_width)
				align_width = g_video_width;
			else
				align_width = outBuf.frameWidth ;
			if (outBuf.frameHeight != g_video_height)
				align_height = g_video_height;
			else
				align_height = outBuf.frameHeight;

			pxp_width = g_disp_width;
			pxp_height = g_disp_height;
			dstaddr = ulVideoDisplayBufGet();

			pxp_scaler_rotate(yaddr, uaddr, vaddr, PXP_SRC_FMT_YUV2P420, align_width, align_height,
				dstaddr, dstaddr + pxp_width * pxp_height, pxp_out_fmt, pxp_width, pxp_height, LCD_ROTATE_ANGLE);
			info.yaddr = dstaddr;
			info.uaddr = dstaddr + pxp_width * pxp_height;
			info.vaddr = 0;
		} else {
			info.yaddr = yaddr;
			info.uaddr = uaddr;
			info.vaddr = vaddr;
		}
		info.x = g_disp_x;
		info.y = g_disp_y;
		info.width = g_disp_width;
		info.height = g_disp_height;
		if (g_active_video_x != 0)
			info.stride = outBuf.frameWidth;

		if (!ark_lcd_get_osd_info_atomic_isactive(LCD_VIDEO_LAYER)) {
			ark_lcd_wait_for_vsync();
		}

		ark_lcd_set_osd_info_atomic(LCD_VIDEO_LAYER, &info);
		ark_lcd_osd_enable(LCD_VIDEO_LAYER, 1);
		ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);

		if ((vehicle_get_data(VEH_CARLINK_CP_STATUS) == 1)  
		|| (vehicle_get_data(VEH_CARLINK_AA_STATUS) == 1)) {
			ark_lcd_osd_enable(LCD_UI_LAYER, 0);
			ark_lcd_set_osd_sync(LCD_UI_LAYER);
		} else {
			ark_lcd_osd_enable(LCD_UI_LAYER, 1);
			ark_lcd_set_osd_sync(LCD_UI_LAYER);
		}
		vVideoDisplayBufRender(dstaddr);
		if (outBuf.num > 1) {// for apple h264
			video_ts = (1000 / g_video_fps);
			show_ts = (get_timer(0) - show_ts) / 1000;
			delta_ts =  ((int32_t)video_ts - (int32_t)show_ts) -3;
			//printf("delay %d ms\r\n", delta_ts);
			if (delta_ts > 0)
				vTaskDelay(pdMS_TO_TICKS(delta_ts));
		}
	}
	//vVideoDisplayBufGive();
	return 0;
}

int carlink_ey_video_init()
{
	return 0;
}

#endif
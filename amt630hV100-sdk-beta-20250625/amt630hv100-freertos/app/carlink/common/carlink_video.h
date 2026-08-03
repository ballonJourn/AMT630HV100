#ifndef __CARLINK_VIDEO_H
#define __CARLINK_VIDEO_H
#include <FreeRTOS.h>
#include "board.h"
#include "list.h"

#define DISABLE_CARLINK_H264_FRAME_BUF       1

#define H264DEC_INBUF_SIZE			 (LCD_WIDTH * LCD_HEIGHT * 2)
#define H264DEC_DISP_COUNTS 		 (2)
#define H264_FRAME_FIFO_COUNT	        (12)
#define H264_FRAME_BUF_SIZE 		 (0x80000)

typedef struct h264_frame_s {
	char*		             buf;			
	char*                    cur;
	unsigned int             len;			
	unsigned int             frame_id;
	ListItem_t               entry;
} video_frame_s;

video_frame_s* get_h264_frame_buf(void);
void notify_h264_frame_ready(video_frame_s** frame);
void set_h264_frame_free(video_frame_s* frame);
void h264_dec_ctx_init();
int carlink_ey_video_init();
int get_carlink_video_width(void);
int get_carlink_video_height(void);
int get_carlink_video_fps(void);

void set_carlink_video_info(int w, int h, int fps);//set h264 video stream info from phone
void set_carlink_display_info(int x, int y, int w, int h);//set carlink show area in lcd
void set_carlink_display_state(int on); // on: 1.display carlink;  0. display native ui
void set_carlink_active_video_info(int x, int y);//for android auto

/* Query whether link_page preview mode is active. When link_page is not
 * visible, video frames should not be pushed to LCD_VIDEO_LAYER even if
 * g_hide_carlink_flag is 1 (carlink connected). Implemented in main_awtk.c. */
extern uint8_t link_api_get_preview_enable(void);

/* Render gate: volatile flag checked by h264_video_player_proc BEFORE any
 * LCD layer operation.  Closed (0) by link_api_set_preview_enable(0) as the
 * very first step, so even an in-flight frame that already read
 * g_hide_carlink_flag==1 will abort before touching LCD_VIDEO_LAYER.
 * Defined in main_awtk.c alongside link_preview_enable. */
extern volatile uint8_t g_link_render_gate;


void* h264_video_player_init();
void h264_video_player_uninit(void* h264_Handle);
int h264_video_player_proc(void* h264_Handle, const char *h264_buf, int h264_buf_len);

#define WRITE_BE32(ptr, val) \
do { \
    uint8_t* __ptr = (uint8_t*)(ptr); \
    *__ptr++ = (val) >> 24; \
    *__ptr++ = ((val) & 0x00FF0000) >> 16; \
    *__ptr++ = ((val) & 0x0000FF00) >> 8; \
    *__ptr = ((val) & 0x000000FF); \
} while (0)

#define WRITE_BE16(ptr, val) \
do { \
    uint8_t* __ptr = (uint8_t*)(ptr); \
    *__ptr++ = (val) >> 8; \
    *__ptr = (val) & 0x00FF; \
} while (0)


#define READ_BE32(ptr, dest) \
do { \
    uint8_t* __ptr = (uint8_t*)(ptr); \
    (dest) = (*__ptr++) << 24; \
    (dest) |= (*__ptr++) << 16; \
    (dest) |= (*__ptr++) << 8; \
    (dest) |= *__ptr; \
} while (0)

#define READ_BE16(ptr, dest) \
do { \
    uint8_t* __ptr = (uint8_t*)(ptr); \
    (dest) = (*__ptr++) << 8; \
    (dest) |= *__ptr; \
} while (0)

#define WRITE_LE32(ptr, val) \
	do { \
		uint8_t* __ptr = (uint8_t*)(ptr); \
		*__ptr++ = ((val) & 0x000000FF); \
		*__ptr++ = ((val) & 0x0000FF00) >> 8; \
		*__ptr++ = ((val) & 0x00FF0000) >> 16; \
		*__ptr = (val) >> 24; \
	} while (0)
	
#define WRITE_LE16(ptr, val) \
	do { \
		uint8_t* __ptr = (uint8_t*)(ptr); \
		*__ptr++ = (val) & 0x00FF; \
		*__ptr = (val) >> 8; \
	} while (0)
	
	
#define READ_LE32(ptr, dest) \
	do { \
		uint8_t* __ptr = (uint8_t*)(ptr); \
		(dest) = *__ptr; \
		(dest) |= (*__ptr++) << 8; \
		(dest) |= (*__ptr++) << 16; \
		(dest) |= (*__ptr++) << 24; \
	} while (0)
	
#define READ_LE16(ptr, dest) \
	do { \
		uint8_t* __ptr = (uint8_t*)(ptr); \
		(dest) = *__ptr; \
		(dest) |= (*__ptr++) << 8; \
	} while (0)
#endif
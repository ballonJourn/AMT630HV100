/*
 * dvr_api.h - DVR backend API declarations (v2 - CLI-based interface)
 *
 * These functions are implemented in main_awtk.c under ENABLE_BD_USB_DVR_FUNC.
 * DVR task auto-starts on USB insertion when elene file is detected.
 * dvr_view.c calls dvr_api_*() to control DVR from AWTK UI.
 */

#ifndef DVR_API_H
#define DVR_API_H

#include <stdint.h>

/* ---- DVR Control API ---- */
void dvr_api_get_id(void);
void dvr_api_rec_start(void);
void dvr_api_rec_stop(void);
void dvr_api_snap(void);
void dvr_api_sos(void);
void dvr_api_get_list(uint8_t mode);         /* mode: 0=video, 1=photo */
void dvr_api_pb_start(uint8_t mode, uint16_t index);
void dvr_api_pb_pause(void);
void dvr_api_pb_stop(void);
void dvr_api_get_status(void);
void dvr_api_view_switch(uint8_t mode);      /* 0:front 1:rear 2:f+r 3:r+f 4:hzh */
void dvr_api_set_res(uint8_t res);           /* 0:1080P 1:720P */
void dvr_api_set_loop_time(uint8_t time);    /* 1/2/3 min, 0=get current */
void dvr_api_set_mic(uint8_t onoff);
void dvr_api_set_stamp(uint8_t onoff);
void dvr_api_format(void);
void dvr_api_restore_default(void);
void dvr_api_del_file(uint16_t index);
void dvr_api_lock_file(uint16_t index);
void dvr_api_unlock_file(uint16_t index);
void dvr_api_get_total_time(uint16_t index);
void dvr_api_pb_ff(void);
void dvr_api_pb_fb(void);

/* ---- Preview / Display ---- */
void dvr_api_set_preview_enable(uint8_t enable);  /* 0:skip jpeg 1:decode */
uint8_t dvr_api_get_preview_enable(void);
void dvr_api_set_display_window(int32_t x, int32_t y, int32_t width, int32_t height);
void dvr_api_get_display_window(int32_t *x, int32_t *y, int32_t *width, int32_t *height);
void dvr_api_reset_display_window(void);

/* ---- Status Query ---- */
int  dvr_api_check_exists(void);   /* elene file exists? */
int  dvr_api_is_running(void);     /* DVR task running? */
uint8_t dvr_get_sd_status(void);
uint8_t dvr_get_rec_status(void);
uint8_t dvr_get_lock_status(void);
uint8_t dvr_get_mic_status(void);
uint8_t dvr_get_sd_error_status(void);
uint8_t dvr_get_sd_full_status(void);
uint16_t dvr_get_video_list_count(void);
uint16_t dvr_get_photo_list_count(void);
uint8_t dvr_get_view_mode(void);

/* ---- Sensor Switch ---- */
void dvr_set_sensor_switch_enable(uint8_t enable);

#endif /* DVR_API_H */

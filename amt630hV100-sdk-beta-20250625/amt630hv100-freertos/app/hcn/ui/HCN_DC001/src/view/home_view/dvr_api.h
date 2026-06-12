/*
 * dvr_api.h - DVR backend API declarations
 *
 * These functions are implemented in main_awtk.c (commit 6e9b20e9).
 * dvr_view.c calls them to bridge AWTK UI actions to the USB DVR hardware.
 *
 * IMPORTANT: The DVR code in main_awtk.c was reverted by commit d923f206.
 * You must re-apply the DVR additions from e62395f5 into main_awtk.c,
 * AND change the following functions from "static" to non-static:
 *   - dvr_send_normal_cmd()
 *   - dvr_get_file_list()
 *   - dvr_get_status()
 */

#ifndef DVR_API_H
#define DVR_API_H

#include <stdint.h>

/* Command IDs - must match main_awtk.c defines */
#define BD_CTRL_REC_START       0x01
#define BD_CTRL_REC_STOP        0x02
#define BD_CTRL_SNAP            0x03
#define BD_CTRL_GET_LIST        0x05
#define BD_CTRL_PB_START        0x06
#define BD_CTRL_PB_PAUSE        0x07
#define BD_CTRL_PB_STOP         0x08
#define BD_CTRL_GET_STS         0x09
#define BD_CTRL_SET_REC_TIME    0x0C
#define BD_CTRL_MIC_ON          0x0D
#define BD_CTRL_FORMAT          0x11
#define BD_CTRL_DEL_FILE        0x14
#define BD_CTRL_SENSOR_SEL      0x22

/* Send a command to DVR via USB elene protocol.
 * Implemented in main_awtk.c - must be non-static. */
extern void dvr_send_normal_cmd(unsigned short cmd_id, unsigned short cmd_par);

/* Request file list:
 *   mode 0: front-camera video
 *   mode 1: rear-camera  video
 *   mode 2: front-camera photo
 *   mode 3: rear-camera  photo
 * Implemented in main_awtk.c - must be non-static. */
extern void dvr_get_file_list(uint8_t mode);

/* Request DVR status update.
 * Implemented in main_awtk.c - must be non-static. */
extern void dvr_get_status(void);

/* Status getters - already non-static in main_awtk.c */
extern uint8_t dvr_get_sd_status(void);
extern uint8_t dvr_get_rec_status(void);
extern uint8_t dvr_get_lock_status(void);
extern uint8_t dvr_get_mic_status(void);
extern uint8_t dvr_get_sd_error_status(void);
extern uint8_t dvr_get_sd_full_status(void);

extern uint16_t dvr_get_video_list_count(void);
extern uint16_t dvr_get_photo_list_count(void);

extern void dvr_set_sensor_switch_enable(uint8_t enable);

/* DVR preview lifecycle - called by dvr_view.c on page enter/exit */
extern void dvr_start_preview(void);
extern void dvr_stop_preview(void);
extern uint8_t dvr_is_device_online(void);

/* DVR preview enable control — manages alpha-clear hook + VIDEO layer.
 * enable=1: register post-render hook (alpha punch), VIDEO layer active.
 * enable=0: unregister hook, disable VIDEO layer, alpha restored by AWTK.
 * Used by dvr_view.c to pause/resume preview when entering overlay pages
 * (file list, settings) that would otherwise have alpha cleared by hook. */
extern void dvr_api_set_preview_enable(uint8_t enable);
extern uint8_t dvr_api_get_preview_enable(void);

/* DVR display window control */
extern void dvr_api_set_display_window(int32_t x, int32_t y, int32_t width, int32_t height);

/* View switch: 0=front, 1=rear, 2=f+r, 3=r+f, 4=hzh */
extern void dvr_api_view_switch(uint8_t mode);
extern uint8_t dvr_get_view_mode(void);

/* Recording control */
extern void dvr_api_rec_start(void);
extern void dvr_api_rec_stop(void);

/* Loop recording time: 1=1min, 2=2min, 3=3min */
extern void dvr_api_set_loop_time(uint8_t time);

/* Format SD card */
extern void dvr_api_format(void);

/* ---------- DVR playback / file APIs (commit 6e9b20e9) ---------- */

/* Get DVR firmware version ID */
extern void dvr_api_get_id(void);

/* Take a snapshot (photo) */
extern void dvr_api_snap(void);

/* Emergency/SOS recording (lock current file) */
extern void dvr_api_sos(void);

/* Get file list (mode: 0-3, see dvr_get_file_list) */
extern void dvr_api_get_list(uint8_t mode);

/* Start playback
 *   mode 0: play front-camera video
 *   mode 1: play rear-camera  video
 *   mode 2: play front-camera photo
 *   mode 3: play rear-camera  photo
 *   index: file index within the bucket */
extern void dvr_api_pb_start(uint8_t mode, uint16_t index);

/* Pause / resume playback (toggle) */
extern void dvr_api_pb_pause(void);

/* Stop playback */
extern void dvr_api_pb_stop(void);

/* Playback fast-forward */
extern void dvr_api_pb_ff(void);

/* Playback fast-backward / rewind */
extern void dvr_api_pb_fb(void);

/* Delete file by index */
extern void dvr_api_del_file(uint16_t index);

/* Lock / unlock file by index */
extern void dvr_api_lock_file(uint16_t index);
extern void dvr_api_unlock_file(uint16_t index);

/* Get total time of a video file */
extern void dvr_api_get_total_time(uint16_t index);

/* DVR status query */
extern void dvr_api_get_status(void);

/* ---------- Per-bucket filename cache (commit 6e9b20e9) ---------- */

#define DVR_NAME_MAX  16

extern uint16_t dvr_api_get_video_list_f(char *out, uint16_t max_entries);
extern uint16_t dvr_api_get_video_list_r(char *out, uint16_t max_entries);
extern uint16_t dvr_api_get_photo_list_f(char *out, uint16_t max_entries);
extern uint16_t dvr_api_get_photo_list_r(char *out, uint16_t max_entries);
extern uint16_t dvr_api_get_video_list_f_count(void);
extern uint16_t dvr_api_get_video_list_r_count(void);
extern uint16_t dvr_api_get_photo_list_f_count(void);
extern uint16_t dvr_api_get_photo_list_r_count(void);

/* FPS print toggle (CLI-controllable, default ON) */
extern void dvr_api_set_fps_print(uint8_t enable);
extern uint8_t dvr_api_get_fps_print(void);

/* File-list ready flag (async bridge).
 * After calling dvr_api_get_list(), poll dvr_api_is_filelist_ready()
 * until it returns 1, then call dvr_api_clear_filelist_ready() and
 * read the bucket data. Set by USB-task, cleared by UI-task. */
extern uint8_t dvr_api_is_filelist_ready(void);
extern void    dvr_api_clear_filelist_ready(void);

#endif /* DVR_API_H */
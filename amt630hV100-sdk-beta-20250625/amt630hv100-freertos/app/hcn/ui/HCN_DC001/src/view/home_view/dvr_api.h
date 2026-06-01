/*
 * dvr_api.h - DVR backend API declarations
 *
 * These functions are implemented in main_awtk.c (commit e62395f5).
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
#define BD_CTRL_GET_STS         0x09
#define BD_CTRL_SET_REC_TIME    0x0C
#define BD_CTRL_MIC_ON          0x0D
#define BD_CTRL_FORMAT          0x11
#define BD_CTRL_DEL_FILE        0x14
#define BD_CTRL_SENSOR_SEL      0x22

/* Send a command to DVR via USB elene protocol.
 * Implemented in main_awtk.c - must be non-static. */
extern void dvr_send_normal_cmd(unsigned short cmd_id, unsigned short cmd_par);

/* Request file list: mode=0 for video, mode=1 for photo.
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

#endif /* DVR_API_H */
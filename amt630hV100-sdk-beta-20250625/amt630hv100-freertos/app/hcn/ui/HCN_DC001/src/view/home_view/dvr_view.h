#ifndef DVR_VIEW_H
#define DVR_VIEW_H

#include "common.h"

typedef enum dvr_sub_page {
    DVR_SUB_MAIN      = 0,  /* Preview + 4-button dock */
    DVR_SUB_CAM_SW    = 1,  /* Preview + cam sub-dock (Front/Rear/Snap) */
    DVR_SUB_LIST      = 2,  /* File list overlay (2 tabs: front/rear) */
    DVR_SUB_SETTING   = 3,  /* Settings overlay */
    DVR_SUB_POPUP     = 4,  /* Confirm popup */
    DVR_SUB_PLAYBACK  = 5,  /* Video/photo playback */
    DVR_SUB_SET_EDIT  = 6,  /* Setting sub-option editing */
    DVR_SUB_LIST_IDLE = 7,  /* dvr_bg + dock, no file list (after playback) */
    DVR_SUB_MAX       ,
} dvr_sub_page_e;

typedef enum dvr_dock_btn {
    DVR_DOCK_PREVIEW  = 0,
    DVR_DOCK_VIDEO_PB = 1,
    DVR_DOCK_PHOTO_PB = 2,
    DVR_DOCK_SETTINGS = 3,
    DVR_DOCK_BTN_MAX  ,
} dvr_dock_btn_e;

typedef enum dvr_cam_item {
    DVR_CAM_FRONT    = 0,
    DVR_CAM_REAR     = 1,
    DVR_CAM_SNAPSHOT = 2,
    DVR_CAM_ITEM_MAX ,
} dvr_cam_item_e;

typedef enum dvr_list_tab {
    DVR_TAB_FRONT = 0,
    DVR_TAB_REAR  = 1,
    DVR_TAB_MAX   ,
} dvr_list_tab_e;

typedef enum dvr_setting_row {
    DVR_SET_CAMERA  = 0,
    DVR_SET_LOOP    = 1,
    DVR_SET_FORMAT  = 2,
    DVR_SET_ABOUT   = 3,
    DVR_SET_ROW_MAX ,
} dvr_setting_row_e;

#define DVR_FILE_ITEM_MAX  6

ret_t home_dvr_view_init(widget_t* parent);
void dvr_page_deal_key_set(void);
void dvr_page_deal_key_back(void);
void dvr_page_deal_key_up(void);
void dvr_page_deal_key_down(void);
dvr_sub_page_e dvr_get_current_sub(void);
void dvr_set_current_sub(dvr_sub_page_e sub);
void dvr_file_list_set_name(int index, const char* name);
void dvr_file_list_clear(void);
void dvr_file_list_populate(void);
void dvr_setting_update_storage(int used_gb, int total_gb);

#endif
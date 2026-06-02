#ifndef DVR_VIEW_H
#define DVR_VIEW_H

#include "common.h"

/* DVR sub-page states */
typedef enum dvr_sub_page {
    DVR_SUB_MAIN     = 0,  /* 6.1 - DVR main preview */
    DVR_SUB_LIST     = 1,  /* 6.3 - File list (video/photo) */
    DVR_SUB_SETTING  = 2,  /* 6.5 - DVR settings */
    DVR_SUB_POPUP    = 3,  /* 6.3-2 - Delete confirm popup */
    DVR_SUB_MAX      ,
} dvr_sub_page_e;

/* DVR main dock buttons */
typedef enum dvr_dock_btn {
    DVR_DOCK_CAMERA   = 0,
    DVR_DOCK_PLAYBACK = 1,
    DVR_DOCK_PHOTO    = 2,
    DVR_DOCK_SETTINGS = 3,
    DVR_DOCK_BTN_MAX  ,
} dvr_dock_btn_e;

/* DVR list tab */
typedef enum dvr_list_tab {
    DVR_TAB_FRONT_VIDEO = 0,
    DVR_TAB_REAR_VIDEO  = 1,
    DVR_TAB_FRONT_PHOTO = 2,
    DVR_TAB_REAR_PHOTO  = 3,
} dvr_list_tab_e;

/* DVR setting rows */
typedef enum dvr_setting_row {
    DVR_SET_CAMERA  = 0,
    DVR_SET_LOOP    = 1,
    DVR_SET_FORMAT  = 2,
    DVR_SET_ABOUT   = 3,
    DVR_SET_ROW_MAX ,
} dvr_setting_row_e;

#define DVR_FILE_ITEM_MAX  6

ret_t home_dvr_view_init(widget_t* parent);

/* DVR preview lifecycle (manages UI bg transparency + VIDEO layer) */
void dvr_view_enter_preview(void);
void dvr_view_exit_preview(void);

/* DVR key handlers */
void dvr_page_deal_key_set(void);
void dvr_page_deal_key_back(void);
void dvr_page_deal_key_up(void);
void dvr_page_deal_key_down(void);

/* DVR sub-page control */
dvr_sub_page_e dvr_get_current_sub(void);
void dvr_set_current_sub(dvr_sub_page_e sub);

/* File list operations */
void dvr_file_list_set_name(int index, const char* name);
void dvr_file_list_clear(void);

/* Setting updates */
void dvr_setting_update_storage(int used_gb, int total_gb);

#endif
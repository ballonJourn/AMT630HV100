#include "dvr_view.h"
#include "../view_manager.h"

/* ---- Widget pointers ---- */
static widget_t* dvr_main_view   = NULL;
static widget_t* dvr_list_view   = NULL;
static widget_t* dvr_setting_view= NULL;
static widget_t* dvr_popup_view  = NULL;

/* DVR main dock buttons */
static const char* dvr_dock_btn_names[DVR_DOCK_BTN_MAX] = {
    "dvr_btn_camera", "dvr_btn_playback", "dvr_btn_photo", "dvr_btn_settings"
};
static widget_t* dvr_dock_btn[DVR_DOCK_BTN_MAX] = { NULL };

/* DVR file list items */
static widget_t* dvr_file_name[DVR_FILE_ITEM_MAX] = { NULL };
static widget_t* dvr_file_icon[DVR_FILE_ITEM_MAX] = { NULL };
static widget_t* dvr_file_view_btn[DVR_FILE_ITEM_MAX] = { NULL };
static widget_t* dvr_file_del_btn[DVR_FILE_ITEM_MAX] = { NULL };

/* DVR setting widgets */
static widget_t* dvr_storage_bar  = NULL;
static widget_t* dvr_storage_text = NULL;

/* State */
static dvr_sub_page_e current_dvr_sub = DVR_SUB_MAIN;
static int dvr_dock_focus = DVR_DOCK_CAMERA;
static int dvr_list_focus = 0;
static int dvr_set_focus  = DVR_SET_CAMERA;
static int dvr_popup_focus = 0; /* 0=confirm, 1=cancel */

/* ---- Internal helpers ---- */
static void dvr_show_sub(dvr_sub_page_e sub)
{
    current_dvr_sub = sub;

    if (dvr_main_view)    widget_set_visible(dvr_main_view,    sub == DVR_SUB_MAIN);
    if (dvr_list_view)    widget_set_visible(dvr_list_view,    sub == DVR_SUB_LIST);
    if (dvr_setting_view) widget_set_visible(dvr_setting_view, sub == DVR_SUB_SETTING);
    if (dvr_popup_view)   widget_set_visible(dvr_popup_view,   sub == DVR_SUB_POPUP);
}

static void dvr_refresh_dock_highlight(int index)
{
    for (int i = 0; i < DVR_DOCK_BTN_MAX; i++) {
        if (dvr_dock_btn[i]) {
            widget_set_state(dvr_dock_btn[i], (i == index) ? STATE_SELECTE : STATE_NORMAL);
        }
    }
    dvr_dock_focus = index;
}

/* ---- Init ---- */
ret_t home_dvr_view_init(widget_t* parent)
{
    char name_buf[32];

    if (parent == NULL) return RET_FAIL;

    dvr_main_view    = widget_lookup(parent, "dvr_main_view",    TRUE);
    dvr_list_view    = widget_lookup(parent, "dvr_list_view",    TRUE);
    dvr_setting_view = widget_lookup(parent, "dvr_setting_view", TRUE);
    dvr_popup_view   = widget_lookup(parent, "dvr_popup_view",   TRUE);

    /* Dock buttons */
    for (int i = 0; i < DVR_DOCK_BTN_MAX; i++) {
        dvr_dock_btn[i] = widget_lookup(parent, dvr_dock_btn_names[i], TRUE);
    }

    /* File list items */
    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++) {
        snprintf(name_buf, sizeof(name_buf), "dvr_file_name_%d", i);
        dvr_file_name[i] = widget_lookup(parent, name_buf, TRUE);

        snprintf(name_buf, sizeof(name_buf), "dvr_file_icon_%d", i);
        dvr_file_icon[i] = widget_lookup(parent, name_buf, TRUE);

        snprintf(name_buf, sizeof(name_buf), "dvr_file_view_%d", i);
        dvr_file_view_btn[i] = widget_lookup(parent, name_buf, TRUE);

        snprintf(name_buf, sizeof(name_buf), "dvr_file_del_%d", i);
        dvr_file_del_btn[i] = widget_lookup(parent, name_buf, TRUE);
    }

    /* Setting storage */
    dvr_storage_bar  = widget_lookup(parent, "dvr_storage_bar",  TRUE);
    dvr_storage_text = widget_lookup(parent, "dvr_storage_text", TRUE);

    /* Default: show main view */
    dvr_show_sub(DVR_SUB_MAIN);
    dvr_refresh_dock_highlight(DVR_DOCK_CAMERA);

    return RET_OK;
}

/* ---- Key handlers ---- */
void dvr_page_deal_key_set(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN:
        switch (dvr_dock_focus) {
            case DVR_DOCK_CAMERA:
                /* Trigger photo/recording - handled by uart_dvr module */
                break;
            case DVR_DOCK_PLAYBACK:
                dvr_show_sub(DVR_SUB_LIST);
                dvr_list_focus = 0;
                set_current_level(MENU_LEVEL_2);
                break;
            case DVR_DOCK_PHOTO:
                /* Switch to photo list tab */
                dvr_show_sub(DVR_SUB_LIST);
                dvr_list_focus = 0;
                set_current_level(MENU_LEVEL_2);
                break;
            case DVR_DOCK_SETTINGS:
                dvr_show_sub(DVR_SUB_SETTING);
                dvr_set_focus = DVR_SET_CAMERA;
                set_current_level(MENU_LEVEL_2);
                break;
        }
        break;

    case DVR_SUB_LIST:
        /* Pressing SET on a file item: show popup for delete confirm */
        dvr_show_sub(DVR_SUB_POPUP);
        dvr_popup_focus = 0;
        set_current_level(MENU_LEVEL_3);
        break;

    case DVR_SUB_SETTING:
        /* Execute setting action based on current row */
        break;

    case DVR_SUB_POPUP:
        if (dvr_popup_focus == 0) {
            /* Confirm delete */
            /* TODO: call bd_del_file or similar */
        }
        /* Return to list either way */
        dvr_show_sub(DVR_SUB_LIST);
        set_current_level(MENU_LEVEL_2);
        break;

    default:
        break;
    }
}

void dvr_page_deal_key_back(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN:
        /* Back from DVR main -> return to LEVEL_0 dock navigation */
        set_current_level(MENU_LEVEL_0);
        break;

    case DVR_SUB_LIST:
        dvr_show_sub(DVR_SUB_MAIN);
        set_current_level(MENU_LEVEL_1);
        break;

    case DVR_SUB_SETTING:
        dvr_show_sub(DVR_SUB_MAIN);
        set_current_level(MENU_LEVEL_1);
        break;

    case DVR_SUB_POPUP:
        dvr_show_sub(DVR_SUB_LIST);
        set_current_level(MENU_LEVEL_2);
        break;

    default:
        break;
    }
}

void dvr_page_deal_key_up(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN: {
        int idx = (dvr_dock_focus - 1 + DVR_DOCK_BTN_MAX) % DVR_DOCK_BTN_MAX;
        dvr_refresh_dock_highlight(idx);
        break;
    }
    case DVR_SUB_LIST:
        if (dvr_list_focus > 0) dvr_list_focus--;
        break;

    case DVR_SUB_SETTING:
        if (dvr_set_focus > 0) dvr_set_focus--;
        break;

    case DVR_SUB_POPUP:
        dvr_popup_focus = 0;
        break;

    default:
        break;
    }
}

void dvr_page_deal_key_down(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN: {
        int idx = (dvr_dock_focus + 1) % DVR_DOCK_BTN_MAX;
        dvr_refresh_dock_highlight(idx);
        break;
    }
    case DVR_SUB_LIST:
        if (dvr_list_focus < DVR_FILE_ITEM_MAX - 1) dvr_list_focus++;
        break;

    case DVR_SUB_SETTING:
        if (dvr_set_focus < DVR_SET_ROW_MAX - 1) dvr_set_focus++;
        break;

    case DVR_SUB_POPUP:
        dvr_popup_focus = 1;
        break;

    default:
        break;
    }
}

/* ---- Sub-page state ---- */
dvr_sub_page_e dvr_get_current_sub(void)
{
    return current_dvr_sub;
}

void dvr_set_current_sub(dvr_sub_page_e sub)
{
    dvr_show_sub(sub);
}

/* ---- File list operations ---- */
void dvr_file_list_set_name(int index, const char* name)
{
    if (index < 0 || index >= DVR_FILE_ITEM_MAX) return;
    if (dvr_file_name[index] && name) {
        widget_set_text_utf8(dvr_file_name[index], name);
    }
}

void dvr_file_list_clear(void)
{
    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++) {
        if (dvr_file_name[i]) {
            widget_set_text_utf8(dvr_file_name[i], "");
        }
    }
}

/* ---- Setting updates ---- */
void dvr_setting_update_storage(int used_gb, int total_gb)
{
    char buf[32];

    if (dvr_storage_bar && total_gb > 0) {
        progress_bar_set_value(dvr_storage_bar, (used_gb * 100) / total_gb);
    }
    if (dvr_storage_text) {
        snprintf(buf, sizeof(buf), "%dG/%dG", used_gb, total_gb);
        widget_set_text_utf8(dvr_storage_text, buf);
    }
}

/*
 * dvr_view.c — DVR UI view layer for independent dvr_page window
 *
 * Key differences from alpha-punch version:
 *   - dvr_page_deal_key_back() calls navigator_back() to close window
 *   - No dvr_clear_ui_alpha() / dvr_restore_ui_alpha_all() needed
 *   - dvr_stop_preview() only disables VIDEO layer, no framebuffer writes
 */

#include "dvr_view.h"
#include "dvr_api.h"
#include "../view_manager.h"
#include "common/navigator.h"

/* ---- Widget pointers ---- */
static widget_t* dvr_main_view   = NULL;
static widget_t* dvr_list_view   = NULL;
static widget_t* dvr_setting_view= NULL;
static widget_t* dvr_popup_view  = NULL;

static const char* dvr_dock_btn_names[DVR_DOCK_BTN_MAX] = {
    "dvr_btn_camera", "dvr_btn_playback", "dvr_btn_photo", "dvr_btn_settings"
};
static widget_t* dvr_dock_btn[DVR_DOCK_BTN_MAX] = { NULL };

static widget_t* dvr_file_name[DVR_FILE_ITEM_MAX] = { NULL };
static widget_t* dvr_file_view_btn[DVR_FILE_ITEM_MAX] = { NULL };
static widget_t* dvr_file_del_btn[DVR_FILE_ITEM_MAX] = { NULL };

static widget_t* dvr_storage_bar  = NULL;
static widget_t* dvr_storage_text = NULL;

/* State */
static dvr_sub_page_e current_dvr_sub = DVR_SUB_MAIN;
static int dvr_dock_focus  = DVR_DOCK_CAMERA;
static int dvr_list_focus  = 0;
static int dvr_set_focus   = DVR_SET_CAMERA;
static int dvr_popup_focus = 0;
static int dvr_list_mode   = 0; /* 0=video, 1=photo */

typedef enum {
    POPUP_SRC_FILE_DELETE = 0,
    POPUP_SRC_CARD_FORMAT = 1,
} popup_source_e;
static popup_source_e dvr_popup_source = POPUP_SRC_FILE_DELETE;

/* ---- Helpers ---- */
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
        if (dvr_dock_btn[i])
            widget_set_state(dvr_dock_btn[i], (i == index) ? STATE_SELECTE : STATE_NORMAL);
    }
    dvr_dock_focus = index;
}

static void dvr_refresh_list_highlight(int index)
{
    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++) {
        if (dvr_file_name[i])
            widget_set_state(dvr_file_name[i], (i == index) ? STATE_SELECTE : STATE_NORMAL);
    }
}

static const char* set_row_bg_names[DVR_SET_ROW_MAX] = {
    "dvr_set_cam_label_bg", "dvr_set_loop_label_bg",
    "dvr_set_fmt_label_bg", "dvr_set_about_label_bg"
};

static void dvr_refresh_setting_highlight(int row)
{
    if (!dvr_setting_view) return;
    for (int i = 0; i < DVR_SET_ROW_MAX; i++) {
        widget_t* w = widget_lookup(dvr_setting_view, set_row_bg_names[i], TRUE);
        if (w) widget_set_state(w, (i == row) ? STATE_SELECTE : STATE_NORMAL);
    }
}

static void dvr_refresh_popup_highlight(int focus)
{
    if (!dvr_popup_view) return;
    widget_t* c = widget_lookup(dvr_popup_view, "dvr_popup_confirm_bg", TRUE);
    widget_t* x = widget_lookup(dvr_popup_view, "dvr_popup_cancel_bg",  TRUE);
    if (c) widget_set_state(c, (focus == 0) ? STATE_SELECTE : STATE_NORMAL);
    if (x) widget_set_state(x, (focus == 1) ? STATE_SELECTE : STATE_NORMAL);
}

/* ---- Init ---- */
ret_t home_dvr_view_init(widget_t* parent)
{
    char buf[32];
    if (!parent) return RET_FAIL;

    dvr_main_view    = widget_lookup(parent, "dvr_main_view",    TRUE);
    dvr_list_view    = widget_lookup(parent, "dvr_list_view",    TRUE);
    dvr_setting_view = widget_lookup(parent, "dvr_setting_view", TRUE);
    dvr_popup_view   = widget_lookup(parent, "dvr_popup_view",   TRUE);

    for (int i = 0; i < DVR_DOCK_BTN_MAX; i++)
        dvr_dock_btn[i] = widget_lookup(parent, dvr_dock_btn_names[i], TRUE);

    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++) {
        snprintf(buf, sizeof(buf), "dvr_file_name_%d", i);
        dvr_file_name[i] = widget_lookup(parent, buf, TRUE);
        snprintf(buf, sizeof(buf), "dvr_file_view_%d", i);
        dvr_file_view_btn[i] = widget_lookup(parent, buf, TRUE);
        snprintf(buf, sizeof(buf), "dvr_file_del_%d", i);
        dvr_file_del_btn[i] = widget_lookup(parent, buf, TRUE);
    }

    dvr_storage_bar  = widget_lookup(parent, "dvr_storage_bar",  TRUE);
    dvr_storage_text = widget_lookup(parent, "dvr_storage_text", TRUE);

    /* Reset ALL state for clean re-entry (window is destroyed on close,
     * but these statics survive across open/close cycles) */
    dvr_dock_focus  = DVR_DOCK_CAMERA;
    dvr_list_focus  = 0;
    dvr_set_focus   = DVR_SET_CAMERA;
    dvr_popup_focus = 0;
    dvr_list_mode   = 0;
    dvr_popup_source = POPUP_SRC_FILE_DELETE;

    dvr_show_sub(DVR_SUB_MAIN);
    dvr_refresh_dock_highlight(DVR_DOCK_CAMERA);
    return RET_OK;
}

/* ---- SET key ---- */
void dvr_page_deal_key_set(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN:
        /* Dock SET: camera=toggle rec, playback=file list, photo=snap, settings=enter */
        switch (dvr_dock_focus)
        {
        case DVR_DOCK_CAMERA:
            /* Toggle recording */
            if (dvr_get_rec_status())
                dvr_send_normal_cmd(BD_CTRL_REC_STOP, 0);
            else
                dvr_send_normal_cmd(BD_CTRL_REC_START, 0);
            break;
        case DVR_DOCK_PLAYBACK:
            /* Enter file list (video mode) */
            dvr_list_mode = 0;
            dvr_get_file_list(0);
            dvr_show_sub(DVR_SUB_LIST);
            dvr_list_focus = 0;
            dvr_refresh_list_highlight(0);
            break;
        case DVR_DOCK_PHOTO:
            /* Enter file list (photo mode) */
            dvr_list_mode = 1;
            dvr_get_file_list(1);
            dvr_show_sub(DVR_SUB_LIST);
            dvr_list_focus = 0;
            dvr_refresh_list_highlight(0);
            break;
        case DVR_DOCK_SETTINGS:
            /* Enter DVR settings */
            dvr_show_sub(DVR_SUB_SETTING);
            dvr_set_focus = DVR_SET_CAMERA;
            dvr_refresh_setting_highlight(DVR_SET_CAMERA);
            break;
        default:
            break;
        }
        break;

    case DVR_SUB_LIST:
        /* In file list, SET could trigger playback or view details */
        break;

    case DVR_SUB_SETTING:
        /* Handle setting sub-options */
        break;

    case DVR_SUB_POPUP:
        if (dvr_popup_focus == 0) {
            /* Confirm */
            if (dvr_popup_source == POPUP_SRC_FILE_DELETE)
                dvr_send_normal_cmd(BD_CTRL_DEL_FILE, dvr_list_focus);
            else
                dvr_send_normal_cmd(BD_CTRL_FORMAT, 0);
        }
        /* Both confirm and cancel return to previous view */
        dvr_show_sub(dvr_popup_source == POPUP_SRC_CARD_FORMAT ? DVR_SUB_SETTING : DVR_SUB_LIST);
        break;

    default:
        break;
    }
}

/* ---- BACK key ---- */
void dvr_page_deal_key_back(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN:
        /*
         * Exit DVR page entirely.
         * dvr_stop_preview() disables VIDEO layer.
         * navigator_back() closes dvr_page, returns to home_page
         * (which is still alive underneath — never destroyed).
         * on_dvr_page_close callback handles dock reset.
         */
        dvr_stop_preview();
        navigator_back();
        break;

    case DVR_SUB_LIST:
        /* Return from file list to main preview */
        dvr_show_sub(DVR_SUB_MAIN);
        dvr_refresh_dock_highlight(dvr_dock_focus);
        break;

    case DVR_SUB_SETTING:
        /* Return from settings to main preview */
        dvr_show_sub(DVR_SUB_MAIN);
        dvr_refresh_dock_highlight(dvr_dock_focus);
        break;

    case DVR_SUB_POPUP:
        /* Return from popup to list */
        dvr_show_sub(DVR_SUB_LIST);
        break;

    default:
        break;
    }
}

/* ---- UP key ---- */
void dvr_page_deal_key_up(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN: {
        /* Switch to previous preview mode (front/rear/f+r/r+f/hzh) */
        uint8_t mode = dvr_get_view_mode();
        mode = (mode + 4) % 5;   /* -1 mod 5 */
        dvr_api_view_switch(mode);
        break;
    }
    case DVR_SUB_LIST:
        if (dvr_list_focus > 0) {
            dvr_list_focus--;
            dvr_refresh_list_highlight(dvr_list_focus);
        }
        break;
    case DVR_SUB_SETTING:
        if (dvr_set_focus > 0) {
            dvr_set_focus--;
            dvr_refresh_setting_highlight(dvr_set_focus);
        }
        break;
    case DVR_SUB_POPUP:
        dvr_popup_focus = 0;
        dvr_refresh_popup_highlight(0);
        break;
    default:
        break;
    }
}

/* ---- DOWN key ---- */
void dvr_page_deal_key_down(void)
{
    switch (current_dvr_sub)
    {
    case DVR_SUB_MAIN: {
        /* Switch to next preview mode (front/rear/f+r/r+f/hzh) */
        uint8_t mode = dvr_get_view_mode();
        mode = (mode + 1) % 5;
        dvr_api_view_switch(mode);
        break;
    }
    case DVR_SUB_LIST:
        if (dvr_list_focus < DVR_FILE_ITEM_MAX - 1) {
            dvr_list_focus++;
            dvr_refresh_list_highlight(dvr_list_focus);
        }
        break;
    case DVR_SUB_SETTING:
        if (dvr_set_focus < DVR_SET_ROW_MAX - 1) {
            dvr_set_focus++;
            dvr_refresh_setting_highlight(dvr_set_focus);
        }
        break;
    case DVR_SUB_POPUP:
        dvr_popup_focus = 1;
        dvr_refresh_popup_highlight(1);
        break;
    default:
        break;
    }
}

/* ---- State accessors ---- */
dvr_sub_page_e dvr_get_current_sub(void) { return current_dvr_sub; }
void dvr_set_current_sub(dvr_sub_page_e sub) { dvr_show_sub(sub); }

void dvr_file_list_set_name(int index, const char* name)
{
    if (index >= 0 && index < DVR_FILE_ITEM_MAX && dvr_file_name[index] && name)
        widget_set_text_utf8(dvr_file_name[index], name);
}

void dvr_file_list_clear(void)
{
    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++)
        if (dvr_file_name[i]) widget_set_text_utf8(dvr_file_name[i], "");
}

void dvr_setting_update_storage(int used_gb, int total_gb)
{
    char buf[32];
    if (dvr_storage_bar && total_gb > 0)
        progress_bar_set_value(dvr_storage_bar, (used_gb * 100) / total_gb);
    if (dvr_storage_text) {
        snprintf(buf, sizeof(buf), "%dG/%dG", used_gb, total_gb);
        widget_set_text_utf8(dvr_storage_text, buf);
    }
}
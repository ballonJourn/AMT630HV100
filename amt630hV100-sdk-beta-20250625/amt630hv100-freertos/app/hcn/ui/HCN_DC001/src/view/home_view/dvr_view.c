#include "dvr_view.h"
#include "dvr_api.h"
#include "../view_manager.h"

/*
 * DVR View Layer — bridges AWTK UI with dvr_api_*() in main_awtk.c
 *
 * Key action -> dvr_api.h function mapping:
 *   Camera btn SET   -> dvr_api_rec_start() / dvr_api_rec_stop()
 *   Camera btn LONG  -> dvr_api_snap()
 *   Playback btn SET -> dvr_api_get_list(0)  (video)
 *   Photo btn SET    -> dvr_api_get_list(1)  (photo)
 *   Settings btn SET -> enter setting sub-page
 *   Setting format   -> dvr_api_format()
 *   Setting loop 1/2/3 -> dvr_api_set_loop_time(n)
 *   Setting mic      -> dvr_api_set_mic(0/1)
 *   File list delete -> dvr_api_del_file(file_index)
 *   Back from list   -> dvr_api_rec_start()
 *   Exit DVR page    -> dvr_api_set_preview_enable(0)
 */

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
        switch (dvr_dock_focus) {
        case DVR_DOCK_CAMERA:
            /* Toggle recording based on real hardware status */
            if (dvr_get_rec_status()) {
                dvr_api_rec_stop();
            } else {
                dvr_api_rec_start();
            }
            break;
        case DVR_DOCK_PLAYBACK:
            /* Stop recording first, then request video file list */
            if (dvr_get_rec_status()) {
                dvr_api_rec_stop();
            }
            dvr_list_mode = 0;
            dvr_api_get_list(0); /* video */
            dvr_show_sub(DVR_SUB_LIST);
            dvr_list_focus = 0;
            dvr_refresh_list_highlight(0);
            set_current_level(MENU_LEVEL_2);
            break;
        case DVR_DOCK_PHOTO:
            if (dvr_get_rec_status()) {
                dvr_api_rec_stop();
            }
            dvr_list_mode = 1;
            dvr_api_get_list(1); /* photo */
            dvr_show_sub(DVR_SUB_LIST);
            dvr_list_focus = 0;
            dvr_refresh_list_highlight(0);
            set_current_level(MENU_LEVEL_2);
            break;
        case DVR_DOCK_SETTINGS:
            dvr_api_get_status(); /* refresh status before entering settings */
            dvr_show_sub(DVR_SUB_SETTING);
            dvr_set_focus = DVR_SET_CAMERA;
            dvr_refresh_setting_highlight(DVR_SET_CAMERA);
            set_current_level(MENU_LEVEL_2);
            break;
        }
        break;

    case DVR_SUB_LIST:
        /* SET on file item -> delete confirmation popup */
        dvr_popup_source = POPUP_SRC_FILE_DELETE;
        dvr_show_sub(DVR_SUB_POPUP);
        dvr_popup_focus = 0;
        dvr_refresh_popup_highlight(0);
        set_current_level(MENU_LEVEL_3);
        break;

    case DVR_SUB_SETTING:
        switch (dvr_set_focus) {
        case DVR_SET_CAMERA:
            /* Toggle mic on/off */
            dvr_api_set_mic(dvr_get_mic_status() ? 0 : 1);
            break;
        case DVR_SET_LOOP:
            /* Cycle loop time: 1->2->3->1 */
            {
                static uint8_t loop_val = 1;
                loop_val = (loop_val % 3) + 1;
                dvr_api_set_loop_time(loop_val);
            }
            break;
        case DVR_SET_FORMAT:
            dvr_popup_source = POPUP_SRC_CARD_FORMAT;
            dvr_show_sub(DVR_SUB_POPUP);
            dvr_popup_focus = 0;
            dvr_refresh_popup_highlight(0);
            set_current_level(MENU_LEVEL_3);
            break;
        case DVR_SET_ABOUT:
            /* No action, display only */
            break;
        }
        break;

    case DVR_SUB_POPUP:
        if (dvr_popup_focus == 0) {
            /* Confirm */
            if (dvr_popup_source == POPUP_SRC_CARD_FORMAT) {
                /* Format: must stop recording first */
                if (dvr_get_rec_status()) {
                    dvr_api_rec_stop();
                }
                dvr_api_format();
            } else {
                /* Delete file at current list index */
                dvr_api_del_file(dvr_list_focus);
            }
        }
        /* Return to caller */
        if (dvr_popup_source == POPUP_SRC_CARD_FORMAT) {
            dvr_show_sub(DVR_SUB_SETTING);
            dvr_refresh_setting_highlight(dvr_set_focus);
        } else {
            dvr_show_sub(DVR_SUB_LIST);
            dvr_refresh_list_highlight(dvr_list_focus);
        }
        set_current_level(MENU_LEVEL_2);
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
        /* Exit DVR -> disable preview, restore home page */
        dvr_api_set_preview_enable(0);
        set_dock_view(ICON_INFO); /* switch back to instrument page */
        set_current_level(MENU_LEVEL_0);
        break;

    case DVR_SUB_LIST:
        /* Back to preview, resume recording */
        dvr_api_rec_start();
        dvr_show_sub(DVR_SUB_MAIN);
        dvr_refresh_dock_highlight(dvr_dock_focus);
        set_current_level(MENU_LEVEL_1);
        break;

    case DVR_SUB_SETTING:
        dvr_show_sub(DVR_SUB_MAIN);
        dvr_refresh_dock_highlight(dvr_dock_focus);
        set_current_level(MENU_LEVEL_1);
        break;

    case DVR_SUB_POPUP:
        if (dvr_popup_source == POPUP_SRC_CARD_FORMAT) {
            dvr_show_sub(DVR_SUB_SETTING);
            dvr_refresh_setting_highlight(dvr_set_focus);
        } else {
            dvr_show_sub(DVR_SUB_LIST);
            dvr_refresh_list_highlight(dvr_list_focus);
        }
        set_current_level(MENU_LEVEL_2);
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
        int idx = (dvr_dock_focus - 1 + DVR_DOCK_BTN_MAX) % DVR_DOCK_BTN_MAX;
        dvr_refresh_dock_highlight(idx);
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
        int idx = (dvr_dock_focus + 1) % DVR_DOCK_BTN_MAX;
        dvr_refresh_dock_highlight(idx);
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
/*
 * dvr_view.c — DVR UI view layer for independent dvr_page window
 *
 * Sub-page state machine:
 *   DVR_SUB_MAIN   — Preview with function dock (Camera/Playback/Photo/Settings)
 *                     UP/DOWN navigates dock buttons, SET activates focused button
 *   DVR_SUB_CAM_SW — Preview with front/rear camera dock
 *                     UP/DOWN toggles front/rear, SET confirms switch, BACK returns
 *   DVR_SUB_LIST   — File list overlay (video/photo)
 *                     UP/DOWN scrolls files, SET plays/views, BACK returns to MAIN
 *   DVR_SUB_SETTING— Settings overlay (camera on/off, loop time, format, about)
 *                     UP/DOWN navigates rows, SET activates row action, BACK returns
 *   DVR_SUB_POPUP  — Delete/format confirmation popup
 *                     UP/DOWN toggles confirm/cancel, SET confirms, BACK cancels
 *
 * Preview management:
 *   alpha-clear hook runs every frame and punches transparent holes in FB.
 *   When entering overlay pages (LIST/SETTING/POPUP), preview is paused to
 *   prevent the hook from clearing overlay alpha. Resumed on return.
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

/* Function dock (4 buttons) */
static widget_t* dvr_dock_bar    = NULL;

/* Camera switch dock (front/rear) */
static widget_t* dvr_cam_dock    = NULL;
static widget_t* dvr_cam_tab_sel = NULL;

static const char* dvr_dock_btn_names[DVR_DOCK_BTN_MAX] = {
    "dvr_btn_camera", "dvr_btn_playback", "dvr_btn_photo", "dvr_btn_settings"
};
static widget_t* dvr_dock_btn[DVR_DOCK_BTN_MAX] = { NULL };

static widget_t* dvr_file_name[DVR_FILE_ITEM_MAX] = { NULL };
static widget_t* dvr_file_view_btn[DVR_FILE_ITEM_MAX] = { NULL };
static widget_t* dvr_file_del_btn[DVR_FILE_ITEM_MAX] = { NULL };

/* File list bottom tab */
static widget_t* dvr_list_tab_sel = NULL;

/* Setting selection indicators */
static widget_t* dvr_set_cam_on_sel  = NULL;
static widget_t* dvr_set_loop_sel    = NULL;

static widget_t* dvr_storage_bar  = NULL;
static widget_t* dvr_storage_text = NULL;

/* State */
static dvr_sub_page_e current_dvr_sub = DVR_SUB_MAIN;
static int dvr_dock_focus  = DVR_DOCK_CAMERA;
static int dvr_list_focus  = 0;
static int dvr_list_tab    = 0; /* 0=front video, 1=rear video */
static int dvr_set_focus   = DVR_SET_CAMERA;
static int dvr_popup_focus = 0;
static int dvr_list_mode   = 0; /* 0=video, 1=photo */
static int dvr_cam_focus   = 0; /* 0=front, 1=rear */

/* Setting sub-option state */
static int dvr_set_cam_onoff = 1;   /* 0=off, 1=on (default on) */
static int dvr_set_loop_val  = 0;   /* 0=1min, 1=2min, 2=3min */

typedef enum {
    POPUP_SRC_FILE_DELETE = 0,
    POPUP_SRC_CARD_FORMAT = 1,
} popup_source_e;
static popup_source_e dvr_popup_source = POPUP_SRC_FILE_DELETE;

/* ---- Helpers ---- */

/*
 * dvr_show_sub — Manages visibility of all overlay views + preview state.
 */
static void dvr_show_sub(dvr_sub_page_e sub)
{
    dvr_sub_page_e prev_sub = current_dvr_sub;
    current_dvr_sub = sub;

    int prev_needs_preview = (prev_sub == DVR_SUB_MAIN || prev_sub == DVR_SUB_CAM_SW);
    int next_needs_preview = (sub == DVR_SUB_MAIN || sub == DVR_SUB_CAM_SW);

    /* Pause preview when entering overlay to prevent alpha-clear hook
     * from punching holes in opaque overlay content */
    if (prev_needs_preview && !next_needs_preview) {
        dvr_api_set_preview_enable(0);
        printf("DVR: preview paused (entering sub=%d)\n", sub);
    }

    /* dvr_main_view visible for both MAIN and CAM_SW */
    int main_visible = (sub == DVR_SUB_MAIN || sub == DVR_SUB_CAM_SW);
    if (dvr_main_view)    widget_set_visible(dvr_main_view,    main_visible);
    if (dvr_list_view)    widget_set_visible(dvr_list_view,    sub == DVR_SUB_LIST);
    if (dvr_setting_view) widget_set_visible(dvr_setting_view, sub == DVR_SUB_SETTING);
    if (dvr_popup_view)   widget_set_visible(dvr_popup_view,   sub == DVR_SUB_POPUP);

    /* Toggle between the two dock bars within dvr_main_view */
    if (dvr_dock_bar)     widget_set_visible(dvr_dock_bar,     sub == DVR_SUB_MAIN);
    if (dvr_cam_dock)     widget_set_visible(dvr_cam_dock,     sub == DVR_SUB_CAM_SW);

    /* Resume preview when returning to preview pages */
    if (!prev_needs_preview && next_needs_preview) {
        dvr_api_set_preview_enable(1);
        printf("DVR: preview resumed (entering sub=%d)\n", sub);
    }
}

static void dvr_refresh_dock_highlight(int index)
{
    for (int i = 0; i < DVR_DOCK_BTN_MAX; i++) {
        if (dvr_dock_btn[i])
            widget_set_state(dvr_dock_btn[i], (i == index) ? STATE_SELECTE : STATE_NORMAL);
    }
    dvr_dock_focus = index;
}

/*
 * dvr_refresh_cam_tab — Moves the dock_selected highlight image.
 * Left (front): x=0, Right (rear): x=602
 */
static void dvr_refresh_cam_tab(int focus)
{
    dvr_cam_focus = focus;
    if (dvr_cam_tab_sel) {
        int32_t x_pos = (focus == 0) ? 0 : 602;
        widget_move(dvr_cam_tab_sel, x_pos, 0);
    }
}

static void dvr_refresh_list_highlight(int index)
{
    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++) {
        if (dvr_file_name[i])
            widget_set_state(dvr_file_name[i], (i == index) ? STATE_SELECTE : STATE_NORMAL);
    }
}

/*
 * dvr_refresh_list_tab — Moves the file list bottom tab highlight.
 * Left (front): x=0, Right (rear): x=602
 */
static void dvr_refresh_list_tab(int tab)
{
    dvr_list_tab = tab;
    if (dvr_list_tab_sel) {
        int32_t x_pos = (tab == 0) ? 0 : 602;
        widget_move(dvr_list_tab_sel, x_pos, 0);
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

/*
 * dvr_refresh_cam_onoff_indicator — Move the camera on/off selection dot.
 * ON position: x=278, OFF position: x=498
 */
static void dvr_refresh_cam_onoff_indicator(int onoff)
{
    dvr_set_cam_onoff = onoff;
    if (dvr_set_cam_on_sel) {
        int32_t x_pos = (onoff == 1) ? 278 : 498;
        widget_move(dvr_set_cam_on_sel, x_pos, 28);
    }
}

/*
 * dvr_refresh_loop_indicator — Move the loop time selection dot.
 * 1min(index=0): x=278, 2min(index=1): x=498, 3min(index=2): x=718
 */
static void dvr_refresh_loop_indicator(int index)
{
    dvr_set_loop_val = index;
    if (dvr_set_loop_sel) {
        int32_t x_pos = 278 + index * 220;
        widget_move(dvr_set_loop_sel, x_pos, 28);
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

    /* Function dock bar (4 buttons) */
    dvr_dock_bar     = widget_lookup(parent, "dvr_dock_bar",     TRUE);

    /* Camera switch dock (front/rear tabs) */
    dvr_cam_dock     = widget_lookup(parent, "dvr_cam_dock",     TRUE);
    dvr_cam_tab_sel  = widget_lookup(parent, "dvr_cam_tab_sel",  TRUE);

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

    /* File list bottom tab highlight */
    dvr_list_tab_sel = widget_lookup(parent, "dvr_list_tab_sel", TRUE);

    /* Setting selection indicators */
    dvr_set_cam_on_sel = widget_lookup(parent, "dvr_set_cam_on_sel", TRUE);
    dvr_set_loop_sel   = widget_lookup(parent, "dvr_set_loop_sel",   TRUE);

    dvr_storage_bar  = widget_lookup(parent, "dvr_storage_bar",  TRUE);
    dvr_storage_text = widget_lookup(parent, "dvr_storage_text", TRUE);

    /* Reset ALL state for clean re-entry */
    dvr_dock_focus  = DVR_DOCK_CAMERA;
    dvr_list_focus  = 0;
    dvr_list_tab    = 0;
    dvr_set_focus   = DVR_SET_CAMERA;
    dvr_popup_focus = 0;
    dvr_cam_focus   = 0;
    dvr_list_mode   = 0;
    dvr_set_cam_onoff = 1;
    dvr_set_loop_val  = 0;
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
        /* Function dock SET: activate focused button */
        switch (dvr_dock_focus)
        {
        case DVR_DOCK_CAMERA:
            /* Enter camera switch mode: show front/rear dock */
            dvr_cam_focus = (dvr_get_view_mode() == 1) ? 1 : 0;
            dvr_show_sub(DVR_SUB_CAM_SW);
            dvr_refresh_cam_tab(dvr_cam_focus);
            printf("DVR: enter cam-switch, focus=%d\n", dvr_cam_focus);
            break;
        case DVR_DOCK_PLAYBACK:
            /* Enter file list (video mode) */
            dvr_list_mode = 0;
            dvr_list_tab = 0;
            dvr_get_file_list(0);
            dvr_show_sub(DVR_SUB_LIST);
            dvr_list_focus = 0;
            dvr_refresh_list_highlight(0);
            dvr_refresh_list_tab(0);
            break;
        case DVR_DOCK_PHOTO:
            /* Take a snapshot */
            dvr_send_normal_cmd(BD_CTRL_SNAP, 0);
            printf("DVR: snapshot taken\n");
            break;
        case DVR_DOCK_SETTINGS:
            /* Enter DVR settings */
            dvr_show_sub(DVR_SUB_SETTING);
            dvr_set_focus = DVR_SET_CAMERA;
            dvr_refresh_setting_highlight(DVR_SET_CAMERA);
            /* Sync indicators with current state */
            dvr_refresh_cam_onoff_indicator(dvr_set_cam_onoff);
            dvr_refresh_loop_indicator(dvr_set_loop_val);
            break;
        default:
            break;
        }
        break;

    case DVR_SUB_CAM_SW:
        /* Confirm front/rear camera selection and send command */
        dvr_api_view_switch(dvr_cam_focus);  /* 0=front, 1=rear */
        printf("DVR: cam-switch confirmed, mode=%d\n", dvr_cam_focus);
        break;

    case DVR_SUB_LIST:
        /* SET in file list — switch between front/rear video tab */
        dvr_list_tab = dvr_list_tab ? 0 : 1;
        dvr_refresh_list_tab(dvr_list_tab);
        /* Request file list for the new tab's camera */
        dvr_get_file_list(dvr_list_mode);
        dvr_list_focus = 0;
        dvr_refresh_list_highlight(0);
        printf("DVR: list tab switched to %s\n", dvr_list_tab ? "rear" : "front");
        break;

    case DVR_SUB_SETTING:
        /* Handle setting sub-options */
        switch (dvr_set_focus)
        {
        case DVR_SET_CAMERA:
            /* Toggle camera recording on/off */
            dvr_set_cam_onoff = dvr_set_cam_onoff ? 0 : 1;
            if (dvr_set_cam_onoff)
                dvr_api_rec_start();
            else
                dvr_api_rec_stop();
            dvr_refresh_cam_onoff_indicator(dvr_set_cam_onoff);
            printf("DVR: camera rec %s\n", dvr_set_cam_onoff ? "ON" : "OFF");
            break;
        case DVR_SET_LOOP:
            /* Cycle loop recording time: 0->1->2->0 (1min->2min->3min->1min) */
            dvr_set_loop_val = (dvr_set_loop_val + 1) % 3;
            dvr_api_set_loop_time(dvr_set_loop_val + 1);  /* API expects 1/2/3 */
            dvr_refresh_loop_indicator(dvr_set_loop_val);
            printf("DVR: loop time = %d min\n", dvr_set_loop_val + 1);
            break;
        case DVR_SET_FORMAT:
            /* Format SD card — show confirmation popup */
            dvr_popup_source = POPUP_SRC_CARD_FORMAT;
            dvr_popup_focus = 1;  /* default to Cancel for safety */
            dvr_show_sub(DVR_SUB_POPUP);
            dvr_refresh_popup_highlight(dvr_popup_focus);
            break;
        case DVR_SET_ABOUT:
            /* About / storage info — no action */
            break;
        default:
            break;
        }
        break;

    case DVR_SUB_POPUP:
        if (dvr_popup_focus == 0) {
            /* Confirm */
            if (dvr_popup_source == POPUP_SRC_FILE_DELETE)
                dvr_send_normal_cmd(BD_CTRL_DEL_FILE, dvr_list_focus);
            else
                dvr_api_format();
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
        /* Hide DVR UI first to avoid visual glitch during alpha restore */
        if (dvr_main_view) widget_set_visible(dvr_main_view, FALSE);
        dvr_stop_preview();
        navigator_back();
        break;

    case DVR_SUB_CAM_SW:
        /* Return from camera switch dock to function dock */
        dvr_show_sub(DVR_SUB_MAIN);
        dvr_refresh_dock_highlight(dvr_dock_focus);
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
        /* Return from popup to list or setting */
        dvr_show_sub(dvr_popup_source == POPUP_SRC_CARD_FORMAT ? DVR_SUB_SETTING : DVR_SUB_LIST);
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
    case DVR_SUB_MAIN:
        /* Navigate dock buttons upward */
        if (dvr_dock_focus > 0) {
            dvr_dock_focus--;
            dvr_refresh_dock_highlight(dvr_dock_focus);
        }
        break;
    case DVR_SUB_CAM_SW:
        /* Switch to front camera */
        dvr_cam_focus = 0;
        dvr_refresh_cam_tab(0);
        break;
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
    case DVR_SUB_MAIN:
        /* Navigate dock buttons downward */
        if (dvr_dock_focus < DVR_DOCK_BTN_MAX - 1) {
            dvr_dock_focus++;
            dvr_refresh_dock_highlight(dvr_dock_focus);
        }
        break;
    case DVR_SUB_CAM_SW:
        /* Switch to rear camera */
        dvr_cam_focus = 1;
        dvr_refresh_cam_tab(1);
        break;
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
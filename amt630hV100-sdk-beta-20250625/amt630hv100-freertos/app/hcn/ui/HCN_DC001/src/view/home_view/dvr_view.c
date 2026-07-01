/*
 * dvr_view.c — DVR UI state machine
 *
 * Flow:
 *   MAIN (dvr_bg+dock) -> SET -> CAM_SW / LIST_SEL / LOADING -> SETTING
 *   CAM_SW: SET on Front/Rear switches camera and STAYS in CAM_SW (preview)
 *   LIST_SEL (front/rear select) -> SET -> LIST (file list)
 *   LIST -> SET on file -> LIST_ACT (action: UP=Play, DOWN=Delete)
 *   LIST_ACT -> SET(Play) -> PLAYBACK | SET(Delete) -> POPUP
 *   LIST_ACT -> BACK -> LIST
 *   PLAYBACK -> BACK -> LIST (pick another clip)
 *   LIST -> BACK -> LIST_SEL
 *   LIST_SEL -> BACK -> MAIN
 *
 * LOADING: entered from DOCK_SETTINGS. Shows the popup with a spinning
 * dvr_loading_0..7 animation while querying DVR version (GET_ID) and
 * TF capacity (GET_TF_CAPACITY). Once BOTH replies arrive (or 6s timeout),
 * the popup auto-closes and transitions to SETTING with data filled in.
 *
 * Recording: stopped on ENTERING the file area (the dock SET handlers) and
 * restarted on the LIST_SEL -> MAIN exit. The whole file area (front/rear
 * select, list, playback) thus runs with recording OFF.
 *
 * Highlight: file list browsing (UP/DOWN in LIST) uses green text color
 * for the focused item. No font size change — color only.
 *
 * Preview (alpha-clear hook + VIDEO layer) is active in CAM_SW and PLAYBACK.
 * CRITICAL ORDERING: BD_CTRL_PB_START is sent BEFORE preview is enabled.
 */

#include "dvr_view.h"
#include "dvr_api.h"
#include "../view_manager.h"
#include "common/navigator.h"

/* Forward declarations */
static void show_sub(dvr_sub_page_e sub);
static void del_poll_stop(void);
static void dvr_request_file_list(void);

/* ---- Widgets ---- */
static widget_t* dvr_main_view    = NULL;
static widget_t* dvr_idle_view    = NULL;
static widget_t* dvr_list_view    = NULL;
static widget_t* dvr_setting_view = NULL;
static widget_t* dvr_popup_view   = NULL;
static widget_t* dvr_dock_bar     = NULL;
static widget_t* dvr_cam_dock     = NULL;
static widget_t* dvr_cam_tab_sel  = NULL;
static widget_t* dvr_main_bg     = NULL;

static const char* dock_btn_names[DVR_DOCK_BTN_MAX] = {
    "dvr_btn_preview", "dvr_btn_video_pb", "dvr_btn_photo_pb", "dvr_btn_settings"
};
static widget_t* dock_btn[DVR_DOCK_BTN_MAX] = {0};

static widget_t* file_name_w[DVR_FILE_ITEM_MAX] = {0};
static widget_t* file_icon_w[DVR_FILE_ITEM_MAX] = {0};
static widget_t* file_view_w[DVR_FILE_ITEM_MAX] = {0};
static widget_t* file_del_w[DVR_FILE_ITEM_MAX]  = {0};

static widget_t* list_tab_sel     = NULL;
static widget_t* tab_label_left   = NULL;
static widget_t* tab_label_right  = NULL;
/* CHANGED: removed set_cam_on_sel; added set_fmt_sel, set_ver_text, popup_loading_w */
static widget_t* set_fmt_sel      = NULL;
static widget_t* set_ver_text     = NULL;
static widget_t* set_loop_sel     = NULL;
static widget_t* storage_bar      = NULL;
static widget_t* storage_text     = NULL;
static widget_t* file_scroll_w    = NULL;
static widget_t* popup_loading_w  = NULL;   /* loading spinner image inside popup */
static widget_t* list_no_sd_label = NULL;

/* Idle view widgets */
static widget_t* idle_tab_sel     = NULL;
static widget_t* idle_tab_left    = NULL;
static widget_t* idle_tab_right   = NULL;

/* ---- State ---- */
static dvr_sub_page_e cur_sub = DVR_SUB_MAIN;
static int dock_focus   = DVR_DOCK_PREVIEW;
static int cam_focus    = DVR_CAM_FRONT;
static int list_focus   = 0;
static int list_tab     = DVR_TAB_FRONT;
static int list_mode    = 0;               /* 0=video, 1=photo */
/* CHANGED: DVR_SET_VERSION instead of DVR_SET_CAMERA */
static int set_focus    = DVR_SET_VERSION;
static int popup_focus  = 0;
/* CHANGED: removed set_cam_onoff; added set_fmt_focus */
static int set_loop_val  = 0;
static int set_edit_val  = 0;
static int set_fmt_focus = DVR_FMT_SD_FORMAT;
static int list_count    = 0;
static int list_offset   = 0;

/* CHANGED: added POP_FACTORY_RST */
typedef enum { POP_FILE_DEL=0, POP_FORMAT=1, POP_NO_SD=2, POP_FACTORY_RST=3 } pop_src_e;
static pop_src_e popup_src = POP_FILE_DEL;

static int pb_paused = 0, pb_idx = 0, pb_mode = 0;

/* Action sub-selection within file list item (Play / Delete) */
#define LIST_ACT_PLAY   0
#define LIST_ACT_DELETE  1
static int list_act_focus = LIST_ACT_PLAY;

static int list_fetch_pending = 0;
static uint32_t list_poll_timer_id = TK_INVALID_ID;
#define LIST_POLL_INTERVAL_MS  100
#define LIST_POLL_MAX_RETRIES  30
static int list_poll_retries = 0;

/* CHANGED: loading poll timer for settings (version + TF capacity) */
static uint32_t loading_timer_id = TK_INVALID_ID;
#define LOADING_POLL_INTERVAL_MS  200
#define LOADING_POLL_MAX_RETRIES 30    /* 200ms × 30 = 6s timeout */
static int loading_poll_retries = 0;
static int loading_anim_frame   = 0;

/* Delete-then-refresh poll timer: waits for DEL_FILE ACK before refreshing list */
static uint32_t del_poll_timer_id = TK_INVALID_ID;
#define DEL_POLL_INTERVAL_MS  200
#define DEL_POLL_MAX_RETRIES  25    /* 200ms × 25 = 5s timeout */
static int del_poll_retries = 0;

static inline uint8_t list_api_mode(void) { return (uint8_t)(list_mode * 2 + list_tab); }

/* ---- Visibility ---- */
static void show_sub(dvr_sub_page_e sub)
{
    dvr_sub_page_e prev = cur_sub;
    cur_sub = sub;

    if ((prev == DVR_SUB_LIST || prev == DVR_SUB_LIST_ACT) && sub != DVR_SUB_LIST && sub != DVR_SUB_LIST_ACT) {
        if (list_poll_timer_id != TK_INVALID_ID) {
            timer_remove(list_poll_timer_id);
            list_poll_timer_id = TK_INVALID_ID;
            list_fetch_pending = 0;
        }
        del_poll_stop();
    }

    int pv_prev = (prev == DVR_SUB_MAIN || prev == DVR_SUB_CAM_SW || prev == DVR_SUB_PLAYBACK);
    int pv_next = (sub  == DVR_SUB_MAIN || sub  == DVR_SUB_CAM_SW || sub  == DVR_SUB_PLAYBACK);
    if (pv_prev && !pv_next) { dvr_api_set_preview_enable(0); printf("DVR: preview off (sub=%d)\n", sub); }

    int mv = (sub == DVR_SUB_MAIN || sub == DVR_SUB_CAM_SW || sub == DVR_SUB_PLAYBACK);
    if (dvr_main_view)    widget_set_visible(dvr_main_view,    mv);
    if (dvr_main_bg)      widget_set_visible(dvr_main_bg,      sub == DVR_SUB_MAIN);
    if (dvr_idle_view)    widget_set_visible(dvr_idle_view,    sub == DVR_SUB_LIST_IDLE || sub == DVR_SUB_LIST_SEL);
    if (dvr_list_view)    widget_set_visible(dvr_list_view,    sub == DVR_SUB_LIST || sub == DVR_SUB_LIST_ACT);
    if (dvr_setting_view) widget_set_visible(dvr_setting_view, sub == DVR_SUB_SETTING || sub == DVR_SUB_SET_EDIT);
    /* CHANGED: popup also visible in LOADING state */
    if (dvr_popup_view)   widget_set_visible(dvr_popup_view,   sub == DVR_SUB_POPUP || sub == DVR_SUB_LOADING);
    if (dvr_dock_bar)     widget_set_visible(dvr_dock_bar,     sub == DVR_SUB_MAIN);
    if (dvr_cam_dock)     widget_set_visible(dvr_cam_dock,     sub == DVR_SUB_CAM_SW);

    if (!pv_prev && pv_next) { dvr_api_set_preview_enable(1); printf("DVR: preview on (sub=%d)\n", sub); }
}

/* ---- Highlight helpers ---- */
static void hl_dock(int i) {
    for (int n=0;n<DVR_DOCK_BTN_MAX;n++) if(dock_btn[n]) widget_set_state(dock_btn[n],(n==i)?STATE_SELECTE:STATE_NORMAL);
    dock_focus = i;
}
static void hl_cam(int i) {
    cam_focus = i;
    if (dvr_cam_tab_sel) widget_move(dvr_cam_tab_sel, i*341, 0);
}
static void hl_list(int i) {
    for(int n=0;n<DVR_FILE_ITEM_MAX;n++) {
        if(file_name_w[n]) {
            if(n==i)
                widget_set_style_color(file_name_w[n], "normal:text_color", 0xFF00FF00);
            else
                widget_set_style_color(file_name_w[n], "normal:text_color", 0xFF083557);
        }
        if(file_view_w[n]) widget_set_state(file_view_w[n], STATE_NORMAL);
        if(file_del_w[n])  widget_set_state(file_del_w[n],  STATE_NORMAL);
    }
}
static void hl_list_act(int act) {
    list_act_focus = act;
    int i = list_focus;
    if(i>=0 && i<DVR_FILE_ITEM_MAX) {
        if(file_view_w[i]) widget_set_state(file_view_w[i], (act==LIST_ACT_PLAY)  ? STATE_SELECTE : STATE_NORMAL);
        if(file_del_w[i])  widget_set_state(file_del_w[i],  (act==LIST_ACT_DELETE) ? STATE_SELECTE : STATE_NORMAL);
    }
}
static void hl_tab(int t) {
    list_tab = t;
    if (list_tab_sel) widget_move(list_tab_sel, t?512:0, 0);
    if (tab_label_left && tab_label_right) {
        if (list_mode == 0) { widget_set_text_utf8(tab_label_left,"Front Video"); widget_set_text_utf8(tab_label_right,"Rear Video"); }
        else                { widget_set_text_utf8(tab_label_left,"Front Photo"); widget_set_text_utf8(tab_label_right,"Rear Photo"); }
    }
}
static void hl_idle_tab(int t) {
    if (idle_tab_sel) widget_move(idle_tab_sel, t?512:0, 0);
    if (idle_tab_left && idle_tab_right) {
        if (list_mode == 0) { widget_set_text_utf8(idle_tab_left,"Front Video"); widget_set_text_utf8(idle_tab_right,"Rear Video"); }
        else                { widget_set_text_utf8(idle_tab_left,"Front Photo"); widget_set_text_utf8(idle_tab_right,"Rear Photo"); }
    }
}
/* CHANGED: set_bg_names[0] from dvr_set_cam_label_bg to dvr_set_ver_label_bg */
static const char* set_bg_names[DVR_SET_ROW_MAX]={"dvr_set_ver_label_bg","dvr_set_loop_label_bg","dvr_set_fmt_label_bg","dvr_set_about_label_bg"};
static void hl_set(int r) {
    if(!dvr_setting_view)return;
    for(int i=0;i<DVR_SET_ROW_MAX;i++){widget_t*w=widget_lookup(dvr_setting_view,set_bg_names[i],TRUE);if(w)widget_set_state(w,(i==r)?STATE_SELECTE:STATE_NORMAL);}
}
/* CHANGED: removed hl_cam_onoff; added hl_fmt */
static void hl_fmt(int f) {
    if(set_fmt_sel) widget_move(set_fmt_sel, (f==0)?285:615, 28);
    set_fmt_focus = f;
}
static void hl_loop(int v)      { if(set_loop_sel) widget_move(set_loop_sel,278+v*220,28); }
static void hl_popup(int f) {
    if(!dvr_popup_view)return;
    widget_t*c=widget_lookup(dvr_popup_view,"dvr_popup_confirm_bg",TRUE);
    widget_t*x=widget_lookup(dvr_popup_view,"dvr_popup_cancel_bg",TRUE);
    widget_t*cl=widget_lookup(dvr_popup_view,"dvr_popup_confirm",TRUE);
    widget_t*xl=widget_lookup(dvr_popup_view,"dvr_popup_cancel",TRUE);
    /* CHANGED: hide loading spinner for normal popup modes */
    if(popup_loading_w) widget_set_visible(popup_loading_w, FALSE);
    if (popup_src == POP_NO_SD) {
        if(c) { widget_set_visible(c, TRUE); widget_set_state(c, STATE_SELECTE); }
        if(cl) { widget_set_visible(cl, TRUE); widget_set_text_utf8(cl, "OK"); }
        if(x) widget_set_visible(x, FALSE);
        if(xl) widget_set_visible(xl, FALSE);
    } else {
        if(c) { widget_set_visible(c, TRUE); widget_set_state(c,(f==0)?STATE_SELECTE:STATE_NORMAL); }
        if(x) { widget_set_visible(x, TRUE); widget_set_state(x,(f==1)?STATE_SELECTE:STATE_NORMAL); }
        if(cl) { widget_set_visible(cl, TRUE); widget_set_text_utf8(cl, "OK"); }
        if(xl) { widget_set_visible(xl, TRUE); widget_set_text_utf8(xl, "Cancel"); }
    }
}

/* CHANGED: configure popup for loading mode — hide buttons, show spinner */
static void popup_show_loading(void) {
    if(!dvr_popup_view) return;
    widget_t* title = widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE);
    widget_t*c=widget_lookup(dvr_popup_view,"dvr_popup_confirm_bg",TRUE);
    widget_t*x=widget_lookup(dvr_popup_view,"dvr_popup_cancel_bg",TRUE);
    widget_t*cl=widget_lookup(dvr_popup_view,"dvr_popup_confirm",TRUE);
    widget_t*xl=widget_lookup(dvr_popup_view,"dvr_popup_cancel",TRUE);
    if(title) widget_set_text_utf8(title, "Loading...");
    if(c)  widget_set_visible(c, FALSE);
    if(x)  widget_set_visible(x, FALSE);
    if(cl) widget_set_visible(cl, FALSE);
    if(xl) widget_set_visible(xl, FALSE);
    if(popup_loading_w) {
        widget_set_visible(popup_loading_w, TRUE);
        widget_set_prop_str(popup_loading_w, WIDGET_PROP_IMAGE, "dvr_loading_0");
    }
}

static dvr_sub_page_e no_sd_return_sub = DVR_SUB_MAIN;
static void show_no_sd_popup(dvr_sub_page_e return_to) {
    widget_t* title = dvr_popup_view ? widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE) : NULL;
    if(title) widget_set_text_utf8(title, "No SD Card");
    popup_src = POP_NO_SD; no_sd_return_sub = return_to; popup_focus = 0;
    show_sub(DVR_SUB_POPUP); hl_popup(0);
    printf("DVR: no SD card alert\n");
}

/* CHANGED: loading poll timer — spins animation, checks version+TF, auto-transitions to SETTING */
static void loading_stop(void) {
    if (loading_timer_id != TK_INVALID_ID) { timer_remove(loading_timer_id); loading_timer_id = TK_INVALID_ID; }
}
static void loading_fill_settings(void) {
    /* Fill version text */
    char vbuf[32];
    if (dvr_api_get_version(vbuf, sizeof(vbuf))) {
        if(set_ver_text) widget_set_text_utf8(set_ver_text, vbuf);
    } else {
        if(set_ver_text) widget_set_text_utf8(set_ver_text, "N/A");
    }
    /* Fill storage from KiB */
    uint32_t total_kib=0, free_kib=0;
    if (dvr_api_get_tf_capacity(&total_kib, &free_kib)) {
        uint32_t used_kib = (total_kib > free_kib) ? (total_kib - free_kib) : 0;
        dvr_setting_update_storage_kib(used_kib, total_kib);
    } else {
        if(storage_text) widget_set_text_utf8(storage_text, "N/A");
        if(storage_bar) progress_bar_set_value(storage_bar, 0);
    }
}
static ret_t on_loading_poll_timer(const timer_info_t* info) {
    (void)info;
    /* Animate spinner */
    loading_anim_frame = (loading_anim_frame + 1) & 7;
    if (popup_loading_w) {
        char img[24];
        snprintf(img, sizeof(img), "dvr_loading_%d", loading_anim_frame);
        widget_set_prop_str(popup_loading_w, WIDGET_PROP_IMAGE, img);
    }
    /* Only wait for TF capacity — version may never come (DVR firmware
     * dependent) and should not block settings entry.  TF capacity is
     * the data that actually matters to the user on the settings page. */
    int tf_ok = dvr_api_get_tf_capacity(NULL, NULL);
    if (tf_ok || ++loading_poll_retries >= LOADING_POLL_MAX_RETRIES) {
        loading_stop();
        loading_fill_settings();
        show_sub(DVR_SUB_SETTING);
        set_focus = DVR_SET_VERSION;
        hl_set(DVR_SET_VERSION); hl_loop(set_loop_val); hl_fmt(set_fmt_focus);
        printf("DVR: loading done (tf=%d retries=%d) -> settings\n", tf_ok, loading_poll_retries);
        return RET_REMOVE;
    }
    return RET_REPEAT;
}

/* Enter settings: if TF capacity cache is valid (pre-fetched at
 * dvr_page_init), go directly — no loading popup needed.  Version is
 * best-effort: displayed if cached, "N/A" otherwise. */
static void enter_settings(void) {
    int tf_ok = dvr_api_get_tf_capacity(NULL, NULL);
    if (tf_ok) {
        /* TF data available — go straight to settings */
        loading_fill_settings();
        show_sub(DVR_SUB_SETTING);
        set_focus = DVR_SET_VERSION;
        hl_set(DVR_SET_VERSION); hl_loop(set_loop_val); hl_fmt(set_fmt_focus);
        printf("DVR: enter settings (cache hit, no loading)\n");
        return;
    }
    /* No TF data yet — show loading popup, stop rec, re-query */
    loading_stop();
    loading_poll_retries = 0;
    loading_anim_frame = 0;
    dvr_api_rec_stop();
    dvr_api_get_id();
    dvr_api_get_tf_capacity_query();
    popup_show_loading();
    show_sub(DVR_SUB_LOADING);
    loading_timer_id = timer_add(on_loading_poll_timer, NULL, LOADING_POLL_INTERVAL_MS);
    printf("DVR: enter settings (cache miss, loading started)\n");
}

/* ---- Delete-then-refresh timer ---- */
static void del_poll_stop(void) {
    if (del_poll_timer_id != TK_INVALID_ID) { timer_remove(del_poll_timer_id); del_poll_timer_id = TK_INVALID_ID; }
}
static ret_t on_del_poll_timer(const timer_info_t* info) {
    (void)info;
    /* Guard: if user navigated away from list, abort silently */
    if (cur_sub != DVR_SUB_LIST && cur_sub != DVR_SUB_LIST_ACT) {
        del_poll_stop();
        printf("DVR: del refresh aborted (left list, sub=%d)\n", cur_sub);
        return RET_REMOVE;
    }
    del_poll_retries++;
    /* Phase 1 (tick 1~3): let the DEL command get physically sent over USB.
     * dvr_send_normal_cmd only sets a flag; the actual ff_fwrite happens in
     * the DVR capture task's next loop iteration (2~50ms).  We give it 3
     * ticks (600ms) so the DVR has time to process the delete too. */
    if (del_poll_retries <= 3) {
        return RET_REPEAT;
    }
    /* Phase 2 (tick 4): send GET_LIST to refresh */
    if (del_poll_retries == 4) {
        printf("DVR: del wait done, requesting fresh list\n");
        dvr_file_list_clear();
        dvr_request_file_list();
        /* list_focus adjustment happens after list arrives (in filelist poll) */
        return RET_REPEAT;
    }
    /* Phase 3 (tick 5+): wait for list response (already handled by list_poll_timer) */
    del_poll_stop();
    if(list_focus >= list_count && list_focus > 0) list_focus--;
    hl_list(list_focus);
    return RET_REMOVE;
}
static void del_then_refresh(void) {
    del_poll_stop();
    del_poll_retries = 0;
    del_poll_timer_id = timer_add(on_del_poll_timer, NULL, DEL_POLL_INTERVAL_MS);
    printf("DVR: del sent, waiting before list refresh\n");
}

/* ---- File list data ---- */
static ret_t on_filelist_poll_timer(const timer_info_t* info) {
    (void)info;
    if (dvr_api_is_filelist_ready()) {
        dvr_api_clear_filelist_ready(); list_fetch_pending = 0;
        list_poll_timer_id = TK_INVALID_ID;
        dvr_file_list_populate(); hl_list(list_focus);
        printf("DVR: filelist ready, populated\n"); return RET_REMOVE;
    }
    if (++list_poll_retries >= LIST_POLL_MAX_RETRIES) {
        list_fetch_pending = 0; list_poll_timer_id = TK_INVALID_ID;
        dvr_file_list_populate(); hl_list(list_focus);
        printf("DVR: filelist poll timeout (%d retries)\n", LIST_POLL_MAX_RETRIES); return RET_REMOVE;
    }
    return RET_REPEAT;
}
static void dvr_request_file_list(void) {
    if (list_poll_timer_id != TK_INVALID_ID) { timer_remove(list_poll_timer_id); list_poll_timer_id = TK_INVALID_ID; }
    dvr_api_clear_filelist_ready(); list_fetch_pending = 1; list_poll_retries = 0;
    dvr_api_get_list(list_api_mode());
    list_poll_timer_id = timer_add(on_filelist_poll_timer, NULL, LIST_POLL_INTERVAL_MS);
    printf("DVR: file list requested mode=%d, poll started\n", list_api_mode());
}
static uint16_t get_count(void) {
    switch(list_api_mode()) {
    case 0: return dvr_api_get_video_list_f_count();
    case 1: return dvr_api_get_video_list_r_count();
    case 2: return dvr_api_get_photo_list_f_count();
    case 3: return dvr_api_get_photo_list_r_count();
    default:return 0;
    }
}
#define DVR_FETCH_MAX 128
static uint8_t get_fname(int idx,char*out,int sz) {
    static char buf[DVR_FETCH_MAX*DVR_NAME_MAX]; uint16_t c=0;
    switch(list_api_mode()){
    case 0:c=dvr_api_get_video_list_f(buf,DVR_FETCH_MAX);break;
    case 1:c=dvr_api_get_video_list_r(buf,DVR_FETCH_MAX);break;
    case 2:c=dvr_api_get_photo_list_f(buf,DVR_FETCH_MAX);break;
    case 3:c=dvr_api_get_photo_list_r(buf,DVR_FETCH_MAX);break;
    default:return 0;
    }
    if(c>DVR_FETCH_MAX) c=DVR_FETCH_MAX;
    if(idx<0||idx>=(int)c)return 0;
    const char*s=buf+(size_t)idx*DVR_NAME_MAX; int l=(int)strlen(s); if(l>=sz)l=sz-1;
    memcpy(out,s,l); out[l]='\0';
    if (list_tab == DVR_TAB_REAR) {
        char *dot = strrchr(out, '.');
        if (dot != NULL && (int)strlen(out) + 2 < sz) {
            memmove(dot + 2, dot, strlen(dot) + 1);
            dot[0] = '_'; dot[1] = 'R';
        }
    }
    return 1;
}
void dvr_file_list_populate(void) {
    char fn[DVR_NAME_MAX]; uint16_t tot=get_count();
    if(list_offset>(int)tot-DVR_FILE_ITEM_MAX) list_offset=(int)tot-DVR_FILE_ITEM_MAX;
    if(list_offset<0) list_offset=0;
    list_count=(int)tot;
    for(int i=0;i<DVR_FILE_ITEM_MAX;i++){
        int fi=list_offset+i;
        if(fi<(int)tot && get_fname(fi,fn,sizeof(fn))) dvr_file_list_set_name(i,fn);
        else dvr_file_list_set_name(i,"");
    }
    printf("DVR: populate mode=%d tab=%d cnt=%d off=%d\n",list_mode,list_tab,tot,list_offset);
}

/* ---- Init ---- */
ret_t home_dvr_view_init(widget_t* parent) {
    char buf[32]; if(!parent)return RET_FAIL;
    dvr_main_view    = widget_lookup(parent,"dvr_main_view",TRUE);
    dvr_idle_view    = widget_lookup(parent,"dvr_idle_view",TRUE);
    dvr_list_view    = widget_lookup(parent,"dvr_list_view",TRUE);
    dvr_setting_view = widget_lookup(parent,"dvr_setting_view",TRUE);
    dvr_popup_view   = widget_lookup(parent,"dvr_popup_view",TRUE);
    dvr_dock_bar     = widget_lookup(parent,"dvr_dock_bar",TRUE);
    dvr_cam_dock     = widget_lookup(parent,"dvr_cam_dock",TRUE);
    dvr_cam_tab_sel  = widget_lookup(parent,"dvr_cam_tab_sel",TRUE);
    dvr_main_bg      = widget_lookup(parent,"dvr_main_bg",TRUE);
    for(int i=0;i<DVR_DOCK_BTN_MAX;i++) dock_btn[i]=widget_lookup(parent,dock_btn_names[i],TRUE);
    for(int i=0;i<DVR_FILE_ITEM_MAX;i++){
        snprintf(buf,sizeof(buf),"dvr_file_name_%d",i);  file_name_w[i]=widget_lookup(parent,buf,TRUE);
        snprintf(buf,sizeof(buf),"dvr_file_icon_%d",i);  file_icon_w[i]=widget_lookup(parent,buf,TRUE);
        snprintf(buf,sizeof(buf),"dvr_file_view_%d",i);  file_view_w[i]=widget_lookup(parent,buf,TRUE);
        snprintf(buf,sizeof(buf),"dvr_file_del_%d",i);   file_del_w[i]=widget_lookup(parent,buf,TRUE);
    }
    list_tab_sel   = widget_lookup(parent,"dvr_list_tab_sel",TRUE);
    tab_label_left = widget_lookup(parent,"dvr_tab_label_left",TRUE);
    tab_label_right= widget_lookup(parent,"dvr_tab_label_right",TRUE);
    /* CHANGED: lookup new widgets instead of set_cam_on_sel */
    set_fmt_sel    = widget_lookup(parent,"dvr_set_fmt_sel",TRUE);
    set_ver_text   = widget_lookup(parent,"dvr_set_ver_text",TRUE);
    popup_loading_w= widget_lookup(parent,"dvr_popup_loading",TRUE);
    set_loop_sel   = widget_lookup(parent,"dvr_set_loop_sel",TRUE);
    storage_bar    = widget_lookup(parent,"dvr_storage_bar",TRUE);
    storage_text   = widget_lookup(parent,"dvr_storage_text",TRUE);
    file_scroll_w  = widget_lookup(parent,"dvr_file_scroll",TRUE);
    list_no_sd_label = widget_lookup(parent,"dvr_list_no_sd",TRUE);
    idle_tab_sel   = widget_lookup(parent,"dvr_idle_tab_sel",TRUE);
    idle_tab_left  = widget_lookup(parent,"dvr_idle_tab_left",TRUE);
    idle_tab_right = widget_lookup(parent,"dvr_idle_tab_right",TRUE);

    dock_focus=DVR_DOCK_PREVIEW; cam_focus=DVR_CAM_FRONT;
    list_focus=0; list_tab=0; list_mode=0; list_count=0; list_offset=0;
    /* CHANGED: DVR_SET_VERSION, removed set_cam_onoff, added set_fmt_focus */
    set_focus=DVR_SET_VERSION; popup_focus=0;
    set_loop_val=0; set_edit_val=0; set_fmt_focus=DVR_FMT_SD_FORMAT;
    popup_src=POP_FILE_DEL; pb_paused=0; pb_idx=0; pb_mode=0;
    list_act_focus=LIST_ACT_PLAY;

    show_sub(DVR_SUB_MAIN); hl_dock(DVR_DOCK_PREVIEW);
    return RET_OK;
}

/* ==== SET ==== */
void dvr_page_deal_key_set(void) {
    switch(cur_sub) {
    case DVR_SUB_MAIN:
        switch(dock_focus) {
        case DVR_DOCK_PREVIEW:
            cam_focus=(dvr_get_view_mode()==1)?DVR_CAM_REAR:DVR_CAM_FRONT;
            show_sub(DVR_SUB_CAM_SW); hl_cam(cam_focus);
            printf("DVR: enter cam dock\n"); break;
        case DVR_DOCK_VIDEO_PB:
            if (!dvr_get_sd_status()) { show_no_sd_popup(DVR_SUB_MAIN); break; }
            dvr_api_rec_stop(); printf("DVR: rec stopped (enter file area)\n");
            list_mode=0; list_tab=DVR_TAB_FRONT;
            show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
            printf("DVR: enter video cam select\n"); break;
        case DVR_DOCK_PHOTO_PB:
            if (!dvr_get_sd_status()) { show_no_sd_popup(DVR_SUB_MAIN); break; }
            dvr_api_rec_stop(); printf("DVR: rec stopped (enter file area)\n");
            list_mode=1; list_tab=DVR_TAB_FRONT;
            show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
            printf("DVR: enter photo cam select\n"); break;
        /* CHANGED: DOCK_SETTINGS now enters LOADING state (popup with spinner) */
        case DVR_DOCK_SETTINGS:
            enter_settings();
            break;
        default: break;
        } break;

    case DVR_SUB_CAM_SW:
        switch(cam_focus) {
        case DVR_CAM_FRONT:  dvr_api_view_switch(0); printf("DVR: cam->front (stay in preview)\n"); break;
        case DVR_CAM_REAR:   dvr_api_view_switch(1); printf("DVR: cam->rear (stay in preview)\n");  break;
        case DVR_CAM_SNAPSHOT:
            if (!dvr_get_sd_status()) { show_no_sd_popup(DVR_SUB_CAM_SW); }
            else { dvr_api_snap(); printf("DVR: snap!\n"); }
            break;
        default: break;
        } break;

    case DVR_SUB_LIST: {
        if(list_fetch_pending){printf("DVR: list loading, ignoring SET\n");break;}
        int fi=list_offset+list_focus;
        if(fi<list_count){
            list_act_focus = LIST_ACT_PLAY;
            cur_sub = DVR_SUB_LIST_ACT;
            hl_list_act(LIST_ACT_PLAY);
            printf("DVR: list -> action select (row=%d)\n", fi);
        } else { printf("DVR: no file row=%d cnt=%d\n", fi, list_count); }
        } break;

    case DVR_SUB_LIST_ACT: {
        int fi=list_offset+list_focus;
        if(list_act_focus == LIST_ACT_PLAY) {
            pb_mode=list_api_mode(); pb_idx=fi; pb_paused=0;
            uint16_t pb_file_no = (uint16_t)fi;
            { char fn[DVR_NAME_MAX];
              if (get_fname(fi, fn, sizeof(fn)) && strlen(fn) >= 8)
                  pb_file_no = (uint16_t)((fn[4]-'0')*1000 + (fn[5]-'0')*100
                                          + (fn[6]-'0')*10 + (fn[7]-'0')); }
            dvr_api_pb_start((uint8_t)pb_mode, pb_file_no);
            show_sub(DVR_SUB_PLAYBACK);
            printf("DVR: pb start mode=%d pos=%d file_no=%d\n", pb_mode, fi, pb_file_no);
        } else {
            popup_src = POP_FILE_DEL; popup_focus = 1;
            show_sub(DVR_SUB_POPUP); hl_popup(1);
            printf("DVR: action delete -> popup (row=%d)\n", fi);
        }
        } break;

    /* CHANGED: SETTING handler — Version=no action, Format=enter sub-edit with two options */
    case DVR_SUB_SETTING:
        switch(set_focus){
        case DVR_SET_VERSION: /* display-only, no action */ break;
        case DVR_SET_LOOP:   set_edit_val=set_loop_val;  hl_loop(set_edit_val);      show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_FORMAT:
            set_fmt_focus = DVR_FMT_SD_FORMAT; hl_fmt(set_fmt_focus);
            show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_SD_CARD: /* display-only, no action */ break;
        default: break;
        } break;

    /* CHANGED: SET_EDIT — removed camera, added format sub-option confirm */
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_LOOP){set_loop_val=set_edit_val; dvr_api_set_loop_time(set_loop_val+1); hl_loop(set_loop_val);}
        else if(set_focus==DVR_SET_FORMAT){
            if(set_fmt_focus == DVR_FMT_SD_FORMAT) { popup_src=POP_FORMAT; }
            else                                    { popup_src=POP_FACTORY_RST; }
            { widget_t* title = dvr_popup_view ? widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE) : NULL;
              if(title) widget_set_text_utf8(title,
                  (popup_src==POP_FORMAT) ? "Format SD?" : "Factory Reset?"); }
            popup_focus=1; show_sub(DVR_SUB_POPUP); hl_popup(1);
            break;
        }
        show_sub(DVR_SUB_SETTING); hl_set(set_focus); break;

    case DVR_SUB_POPUP:
        if (popup_src == POP_NO_SD) {
            widget_t* title = dvr_popup_view ? widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE) : NULL;
            if(title) widget_set_text_utf8(title, "Confirm?");
            show_sub(no_sd_return_sub);
            if(no_sd_return_sub==DVR_SUB_MAIN) hl_dock(dock_focus);
            else if(no_sd_return_sub==DVR_SUB_CAM_SW) hl_cam(cam_focus);
            break;
        }
        if(popup_focus==0){
            if(popup_src==POP_FILE_DEL){
                int fi=list_offset+list_focus;
                uint16_t par=(uint16_t)fi;
                { char fn[DVR_NAME_MAX];
                  if (get_fname(fi, fn, sizeof(fn)) && strlen(fn) >= 8)
                      par = (uint16_t)((fn[4]-'0')*1000 + (fn[5]-'0')*100
                                       + (fn[6]-'0')*10 + (fn[7]-'0')); }
                if(list_mode==1) par|=0x8000;
                if(list_tab==1)  par|=0x4000;
                dvr_api_clear_del_ack();
                dvr_send_normal_cmd(BD_CTRL_DEL_FILE,par);
                printf("DVR: del file_no=%d (fi=%d)\n", par & 0x3FFF, fi);
            /* CHANGED: split format vs factory reset */
            } else if(popup_src==POP_FORMAT) {
                if (dvr_get_rec_status()) { dvr_api_rec_stop(); printf("DVR: rec stopped for format\n"); }
                dvr_api_format();
            } else if(popup_src==POP_FACTORY_RST) {
                if (dvr_get_rec_status()) { dvr_api_rec_stop(); printf("DVR: rec stopped for factory reset\n"); }
                dvr_api_restore_default();
                printf("DVR: factory reset executed\n");
            }
        }
        /* CHANGED: both FORMAT and FACTORY_RST return to settings */
        if(popup_src==POP_FORMAT || popup_src==POP_FACTORY_RST){show_sub(DVR_SUB_SETTING);hl_set(set_focus);}
        else{show_sub(DVR_SUB_LIST);hl_list(list_focus);del_then_refresh();}
        break;

    case DVR_SUB_PLAYBACK:
        pb_paused=pb_paused?0:1; dvr_api_pb_pause();
        printf("DVR: pb %s\n",pb_paused?"paused":"resumed"); break;

    case DVR_SUB_LIST_IDLE:
        break;

    case DVR_SUB_LIST_SEL:
        list_offset=0; list_focus=0;
        show_sub(DVR_SUB_LIST); dvr_file_list_clear();
        hl_list(0); hl_tab(list_tab); dvr_request_file_list();
        printf("DVR: cam select -> list (mode=%d tab=%d)\n", list_mode, list_tab);
        break;

    /* CHANGED: ignore keys during loading */
    case DVR_SUB_LOADING:
        break;

    default: break;
    }
}

/* ==== BACK ==== */
void dvr_page_deal_key_back(void) {
    switch(cur_sub) {
    case DVR_SUB_MAIN:
        if(dvr_main_view) widget_set_visible(dvr_main_view,FALSE);
        dvr_stop_preview(); navigator_back(); break;
    case DVR_SUB_CAM_SW:
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
    case DVR_SUB_LIST:
        show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab); break;
    case DVR_SUB_LIST_ACT:
        cur_sub = DVR_SUB_LIST;
        hl_list(list_focus);
        printf("DVR: action select -> list\n"); break;
    /* CHANGED: stop loading timer when leaving settings */
    case DVR_SUB_SETTING:
        loading_stop();
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus);
        dvr_api_rec_start(); printf("DVR: rec restarted on exit settings -> main\n"); break;
    /* CHANGED: removed camera, added format back */
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_LOOP) hl_loop(set_loop_val);
        else if(set_focus==DVR_SET_FORMAT) hl_fmt(set_fmt_focus);
        show_sub(DVR_SUB_SETTING); hl_set(set_focus); break;
    case DVR_SUB_POPUP:
        if (popup_src == POP_NO_SD) {
            widget_t* title = dvr_popup_view ? widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE) : NULL;
            if(title) widget_set_text_utf8(title, "Confirm?");
            show_sub(no_sd_return_sub);
            if(no_sd_return_sub==DVR_SUB_MAIN) hl_dock(dock_focus);
            else if(no_sd_return_sub==DVR_SUB_CAM_SW) hl_cam(cam_focus);
            break;
        }
        if(popup_src==POP_FORMAT || popup_src==POP_FACTORY_RST){show_sub(DVR_SUB_SETTING);hl_set(set_focus);}
        else{show_sub(DVR_SUB_LIST);hl_list(list_focus);} break;
    case DVR_SUB_PLAYBACK:
        dvr_api_pb_stop(); pb_paused=0;
        show_sub(DVR_SUB_LIST); dvr_file_list_populate(); hl_list(list_focus);
        printf("DVR: pb stopped -> list\n"); break;
    case DVR_SUB_LIST_IDLE:
        show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
        printf("DVR: idle -> cam select\n"); break;
    case DVR_SUB_LIST_SEL:
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus);
        dvr_api_rec_start(); printf("DVR: rec restarted on exit -> main\n"); break;
    /* CHANGED: BACK during loading cancels and returns to MAIN */
    case DVR_SUB_LOADING:
        loading_stop();
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus);
        dvr_api_rec_start(); printf("DVR: loading cancelled, rec restarted -> main\n"); break;
    default: break;
    }
}

/* ==== UP ==== */
void dvr_page_deal_key_up(void) {
    switch(cur_sub) {
    case DVR_SUB_MAIN:     if(dock_focus>0) hl_dock(dock_focus-1); break;
    case DVR_SUB_CAM_SW:   if(cam_focus>0) hl_cam(cam_focus-1); break;
    case DVR_SUB_LIST:
        if(list_focus>0){list_focus--;hl_list(list_focus);}
        else if(list_offset>0){list_offset--;dvr_file_list_populate();hl_list(0);}
        else if(list_tab>0){
            list_tab--; list_offset=0;
            dvr_file_list_clear(); dvr_request_file_list();
            list_focus=0; hl_list(0); hl_tab(list_tab);
        }
        break;
    case DVR_SUB_SETTING:  if(set_focus>0){set_focus--;hl_set(set_focus);} break;
    /* CHANGED: removed camera, added format sub-option toggle */
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_LOOP){set_edit_val=(set_edit_val>0)?set_edit_val-1:2;hl_loop(set_edit_val);}
        else if(set_focus==DVR_SET_FORMAT){set_fmt_focus=set_fmt_focus?0:1;hl_fmt(set_fmt_focus);}
        break;
    case DVR_SUB_POPUP: popup_focus=0;hl_popup(0); break;
    case DVR_SUB_LIST_ACT:
        if(list_act_focus != LIST_ACT_PLAY) { list_act_focus = LIST_ACT_PLAY; hl_list_act(LIST_ACT_PLAY); }
        break;
    case DVR_SUB_LIST_SEL:
        if(list_tab>0){ list_tab--; hl_idle_tab(list_tab); } break;
    case DVR_SUB_LOADING: break;  /* CHANGED: ignore keys during loading */
    default: break;
    }
}

/* ==== DOWN ==== */
void dvr_page_deal_key_down(void) {
    switch(cur_sub) {
    case DVR_SUB_MAIN:     if(dock_focus<DVR_DOCK_BTN_MAX-1) hl_dock(dock_focus+1); break;
    case DVR_SUB_CAM_SW:   if(cam_focus<DVR_CAM_ITEM_MAX-1) hl_cam(cam_focus+1); break;
    case DVR_SUB_LIST:
        if(list_focus<DVR_FILE_ITEM_MAX-1 && (list_offset+list_focus+1)<list_count){list_focus++;hl_list(list_focus);}
        else if((list_offset+DVR_FILE_ITEM_MAX)<list_count){list_offset++;dvr_file_list_populate();hl_list(DVR_FILE_ITEM_MAX-1);}
        else if(list_tab<DVR_TAB_MAX-1){
            list_tab++; list_offset=0;
            dvr_file_list_clear(); dvr_request_file_list();
            list_focus=0; hl_list(0); hl_tab(list_tab);
        }
        break;
    case DVR_SUB_SETTING:  if(set_focus<DVR_SET_ROW_MAX-1){set_focus++;hl_set(set_focus);} break;
    /* CHANGED: removed camera, added format sub-option toggle */
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_LOOP){set_edit_val=(set_edit_val+1)%3;hl_loop(set_edit_val);}
        else if(set_focus==DVR_SET_FORMAT){set_fmt_focus=set_fmt_focus?0:1;hl_fmt(set_fmt_focus);}
        break;
    case DVR_SUB_POPUP: popup_focus=1;hl_popup(1); break;
    case DVR_SUB_LIST_ACT:
        if(list_act_focus != LIST_ACT_DELETE) { list_act_focus = LIST_ACT_DELETE; hl_list_act(LIST_ACT_DELETE); }
        break;
    case DVR_SUB_LIST_SEL:
        if(list_tab<DVR_TAB_MAX-1){ list_tab++; hl_idle_tab(list_tab); } break;
    case DVR_SUB_LOADING: break;  /* CHANGED: ignore keys during loading */
    default: break;
    }
}

/* ---- Accessors ---- */
dvr_sub_page_e dvr_get_current_sub(void) { return cur_sub; }
void dvr_set_current_sub(dvr_sub_page_e sub) { show_sub(sub); }
void dvr_file_list_set_name(int i,const char*n) {
    if(i>=0&&i<DVR_FILE_ITEM_MAX&&file_name_w[i]&&n) widget_set_text_utf8(file_name_w[i],n);
}
void dvr_file_list_clear(void) {
    for(int i=0;i<DVR_FILE_ITEM_MAX;i++) if(file_name_w[i]) widget_set_text_utf8(file_name_w[i],"");
}
void dvr_setting_update_storage(int used,int total) {
    char buf[32];
    if(storage_bar&&total>0) progress_bar_set_value(storage_bar,(used*100)/total);
    if(storage_text){snprintf(buf,sizeof(buf),"%dG/%dG",used,total);widget_set_text_utf8(storage_text,buf);}
}
/* CHANGED: new function — display storage from KiB values (no overflow) */
void dvr_setting_update_storage_kib(uint32_t used_kib, uint32_t total_kib) {
    char buf[48];
    if (total_kib == 0) {
        if(storage_bar) progress_bar_set_value(storage_bar, 0);
        if(storage_text) widget_set_text_utf8(storage_text, "No SD");
        return;
    }
    double used_gib  = (double)used_kib  / (1024.0 * 1024.0);
    double total_gib = (double)total_kib / (1024.0 * 1024.0);
    int pct = (int)((uint64_t)used_kib * 100 / total_kib);
    if(pct > 100) pct = 100;
    if(storage_bar) progress_bar_set_value(storage_bar, pct);
    snprintf(buf, sizeof(buf), "%.1fG/%.1fG", used_gib, total_gib);
    if(storage_text) widget_set_text_utf8(storage_text, buf);
}
/* CHANGED: new function */
void dvr_setting_update_version(const char *ver) {
    if (set_ver_text && ver) widget_set_text_utf8(set_ver_text, ver);
}
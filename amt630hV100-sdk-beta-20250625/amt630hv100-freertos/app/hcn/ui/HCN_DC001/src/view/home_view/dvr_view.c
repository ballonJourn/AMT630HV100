/*
 * dvr_view.c — DVR UI state machine
 *
 * Flow:
 *   MAIN (dvr_bg+dock) -> SET -> CAM_SW / LIST_SEL / SETTING
 *   CAM_SW: SET on Front/Rear switches camera and STAYS in CAM_SW (preview)
 *   LIST_SEL (front/rear select) -> SET -> LIST (file list)
 *   LIST -> SET on file -> LIST_ACT (action: UP=Play, DOWN=Delete)
 *   LIST_ACT -> SET(Play) -> PLAYBACK | SET(Delete) -> POPUP
 *   LIST_ACT -> BACK -> LIST
 *   PLAYBACK -> BACK -> LIST (pick another clip)
 *   LIST -> BACK -> LIST_SEL
 *   LIST_SEL -> BACK -> MAIN
 *
 * Recording: stopped on ENTERING the file area (the dock SET handlers) and
 * restarted on the LIST_SEL -> MAIN exit. The whole file area (front/rear
 * select, list, playback) thus runs with recording OFF - so pb_start never
 * has to stop recording itself (which used to race and show live preview
 * instead of the clip), and the live preview is never left black on exit.
 *
 * Highlight: file list browsing (UP/DOWN in LIST) uses green text color
 * for the focused item. No font size change — color only.
 *
 * Preview (alpha-clear hook + VIDEO layer) is active in CAM_SW and PLAYBACK.
 * CRITICAL ORDERING: BD_CTRL_PB_START is sent BEFORE preview is enabled, so the
 * DVR is already streaming when the JPEG-decode gate opens. Enabling the gate
 * first (the removed 500ms "warmup") left it open over an idle USB stream,
 * desynchronising frame alignment and blocking all subsequent playback frames.
 */

#include "dvr_view.h"
#include "dvr_api.h"
#include "../view_manager.h"
#include "common/navigator.h"

/* Forward declarations */
static void show_sub(dvr_sub_page_e sub);

/* ---- Widgets ---- */
static widget_t* dvr_main_view    = NULL;
static widget_t* dvr_idle_view    = NULL;
static widget_t* dvr_list_view    = NULL;
static widget_t* dvr_setting_view = NULL;
static widget_t* dvr_popup_view   = NULL;
static widget_t* dvr_dock_bar     = NULL;
static widget_t* dvr_cam_dock     = NULL;
static widget_t* dvr_cam_tab_sel  = NULL;
static widget_t* dvr_main_bg     = NULL;  /* background image in main_view, hidden during preview */

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
static widget_t* set_cam_on_sel   = NULL;
static widget_t* set_loop_sel     = NULL;
static widget_t* storage_bar      = NULL;
static widget_t* storage_text     = NULL;
static widget_t* file_scroll_w    = NULL;
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
static int set_focus    = DVR_SET_CAMERA;
static int popup_focus  = 0;
static int set_cam_onoff = 1;
static int set_loop_val  = 0;
static int set_edit_val  = 0;
static int list_count    = 0;
static int list_offset   = 0;

typedef enum { POP_FILE_DEL=0, POP_FORMAT=1, POP_NO_SD=2 } pop_src_e;
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

static inline uint8_t list_api_mode(void) { return (uint8_t)(list_mode * 2 + list_tab); }

/* ---- Visibility ---- */
static void show_sub(dvr_sub_page_e sub)
{
    dvr_sub_page_e prev = cur_sub;
    cur_sub = sub;

    if ((prev == DVR_SUB_LIST || prev == DVR_SUB_LIST_ACT) && sub != DVR_SUB_LIST && sub != DVR_SUB_LIST_ACT && list_poll_timer_id != TK_INVALID_ID) {
        timer_remove(list_poll_timer_id);
        list_poll_timer_id = TK_INVALID_ID;
        list_fetch_pending = 0;
    }

    /* MAIN included so preview stays on across the DVR session (5b67ca2
     * behavior): keeps the USB video pipe drained and the hole punched.
     * LIST/LIST_ACT/LIST_SEL/LIST_IDLE/SETTING excluded so the file list is never
     * punched through. */
    int pv_prev = (prev == DVR_SUB_MAIN || prev == DVR_SUB_CAM_SW || prev == DVR_SUB_PLAYBACK);
    int pv_next = (sub  == DVR_SUB_MAIN || sub  == DVR_SUB_CAM_SW || sub  == DVR_SUB_PLAYBACK);
    if (pv_prev && !pv_next) { dvr_api_set_preview_enable(0); printf("DVR: preview off (sub=%d)\n", sub); }

    int mv = (sub == DVR_SUB_MAIN || sub == DVR_SUB_CAM_SW || sub == DVR_SUB_PLAYBACK);
    if (dvr_main_view)    widget_set_visible(dvr_main_view,    mv);
    /* Show dvr_bg in MAIN (no preview), hide in CAM_SW/PLAYBACK (preview on) */
    if (dvr_main_bg)      widget_set_visible(dvr_main_bg,      sub == DVR_SUB_MAIN);
    if (dvr_idle_view)    widget_set_visible(dvr_idle_view,    sub == DVR_SUB_LIST_IDLE || sub == DVR_SUB_LIST_SEL);
    if (dvr_list_view)    widget_set_visible(dvr_list_view,    sub == DVR_SUB_LIST || sub == DVR_SUB_LIST_ACT);
    if (dvr_setting_view) widget_set_visible(dvr_setting_view, sub == DVR_SUB_SETTING || sub == DVR_SUB_SET_EDIT);
    if (dvr_popup_view)   widget_set_visible(dvr_popup_view,   sub == DVR_SUB_POPUP);
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
    /* DIAG: bump a monotonic tag so the alpha hook and the video-commit path
     * each log the next few frames; lets us see what AWTK does to the preview
     * region on a pure highlight move (NO camera switch). */
    // extern volatile uint32_t g_dvr_diag_nav_tag;
    // g_dvr_diag_nav_tag++;
    // printf("DVR-DIAG hl_cam focus=%d tag=%u (highlight moved, NO switch)\n",
    //        i, (unsigned)g_dvr_diag_nav_tag);
}
static void hl_list(int i) {
    /* Requirement: browsing highlight uses green text color, not font size change */
    for(int n=0;n<DVR_FILE_ITEM_MAX;n++) {
        if(file_name_w[n]) {
            if(n==i)
                widget_set_style_color(file_name_w[n], "normal:text_color", 0xFF00FF00);  /* highlight green */
            else
                widget_set_style_color(file_name_w[n], "normal:text_color", 0xFF083557);  /* restore default */
        }
        /* Reset action icons to normal when changing file selection */
        if(file_view_w[n]) widget_set_state(file_view_w[n], STATE_NORMAL);
        if(file_del_w[n])  widget_set_state(file_del_w[n],  STATE_NORMAL);
    }
}
/* Highlight play/delete action icon within the focused file row */
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
static const char* set_bg_names[DVR_SET_ROW_MAX]={"dvr_set_cam_label_bg","dvr_set_loop_label_bg","dvr_set_fmt_label_bg","dvr_set_about_label_bg"};
static void hl_set(int r) {
    if(!dvr_setting_view)return;
    for(int i=0;i<DVR_SET_ROW_MAX;i++){widget_t*w=widget_lookup(dvr_setting_view,set_bg_names[i],TRUE);if(w)widget_set_state(w,(i==r)?STATE_SELECTE:STATE_NORMAL);}
}
static void hl_cam_onoff(int v) { if(set_cam_on_sel) widget_move(set_cam_on_sel,(v==1)?278:498,28); }
static void hl_loop(int v)      { if(set_loop_sel) widget_move(set_loop_sel,278+v*220,28); }
static void hl_popup(int f) {
    if(!dvr_popup_view)return;
    widget_t*c=widget_lookup(dvr_popup_view,"dvr_popup_confirm_bg",TRUE);
    widget_t*x=widget_lookup(dvr_popup_view,"dvr_popup_cancel_bg",TRUE);
    widget_t*cl=widget_lookup(dvr_popup_view,"dvr_popup_confirm",TRUE);
    widget_t*xl=widget_lookup(dvr_popup_view,"dvr_popup_cancel",TRUE);
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

static dvr_sub_page_e no_sd_return_sub = DVR_SUB_MAIN;
static void show_no_sd_popup(dvr_sub_page_e return_to) {
    widget_t* title = dvr_popup_view ? widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE) : NULL;
    if(title) widget_set_text_utf8(title, "No SD Card");
    popup_src = POP_NO_SD; no_sd_return_sub = return_to; popup_focus = 0;
    show_sub(DVR_SUB_POPUP); hl_popup(0);
    printf("DVR: no SD card alert\n");
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
/* get_fname must return ANY row the user scrolls to -- any index up to the full
 * stored list count, NOT just the 6 visible rows. The buckets in main_awtk.c
 * hold up to DVR_VIDEO_LIST_MAX(20) / DVR_PHOTO_LIST_MAX(30) names, so buf must
 * cover the larger of those. (Bug: buf was sized for only DVR_FILE_ITEM_MAX=6,
 * but dvr_api_get_*_list returns the FULL count, so indices 6..N read past the
 * 96-byte buf -> stack garbage shown as the on-screen file names.) */
#define DVR_FETCH_MAX 128  /* must be >= max(DVR_VIDEO_LIST_MAX, DVR_PHOTO_LIST_MAX) in main_awtk.c */
static uint8_t get_fname(int idx,char*out,int sz) {
    static char buf[DVR_FETCH_MAX*DVR_NAME_MAX]; uint16_t c=0;   /* static: 2KB off-stack; get_fname runs only in the UI thread, sequentially (no reentrancy) */
    switch(list_api_mode()){
    case 0:c=dvr_api_get_video_list_f(buf,DVR_FETCH_MAX);break;
    case 1:c=dvr_api_get_video_list_r(buf,DVR_FETCH_MAX);break;
    case 2:c=dvr_api_get_photo_list_f(buf,DVR_FETCH_MAX);break;
    case 3:c=dvr_api_get_photo_list_r(buf,DVR_FETCH_MAX);break;
    default:return 0;
    }
    if(c>DVR_FETCH_MAX) c=DVR_FETCH_MAX;   /* API returns full count; clamp to what fit in buf */
    if(idx<0||idx>=(int)c)return 0;
    const char*s=buf+(size_t)idx*DVR_NAME_MAX; int l=(int)strlen(s); if(l>=sz)l=sz-1;
    memcpy(out,s,l); out[l]='\0';
    /* Front and rear are two camera streams of the SAME event, so the DVR
     * assigns them the SAME file number -> identical names. Tag the rear
     * name with "_R" (e.g. MOVI0301_R.avi) so the two lists are
     * distinguishable on screen. Inserted before the extension; the 4-digit
     * field stays at out[4..7], so the playback file-number parse is intact. */
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
    set_cam_on_sel = widget_lookup(parent,"dvr_set_cam_on_sel",TRUE);
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
    set_focus=DVR_SET_CAMERA; popup_focus=0;
    set_cam_onoff=1; set_loop_val=0; set_edit_val=0;
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
            /* Symmetric with the exit (LIST_SEL -> MAIN restarts recording):
             * stop recording HERE, on entering the file area. Doing it now -
             * not back-to-back with pb_start - gives the DVR time to close the
             * active recording while the user browses, so a later pb_start
             * switches to playback cleanly instead of racing (DVR still
             * recording -> live preview shown instead of the clip). */
            dvr_api_rec_stop(); printf("DVR: rec stopped (enter file area)\n");
            list_mode=0; list_tab=DVR_TAB_FRONT;
            show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
            printf("DVR: enter video cam select\n"); break;
        case DVR_DOCK_PHOTO_PB:
            if (!dvr_get_sd_status()) { show_no_sd_popup(DVR_SUB_MAIN); break; }
            /* Stop recording on entering the file area (see DVR_DOCK_VIDEO_PB). */
            dvr_api_rec_stop(); printf("DVR: rec stopped (enter file area)\n");
            list_mode=1; list_tab=DVR_TAB_FRONT;
            show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
            printf("DVR: enter photo cam select\n"); break;
        case DVR_DOCK_SETTINGS:
            show_sub(DVR_SUB_SETTING); set_focus=DVR_SET_CAMERA;
            hl_set(DVR_SET_CAMERA); hl_cam_onoff(set_cam_onoff); hl_loop(set_loop_val);
            printf("DVR: enter settings\n"); break;
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
        /* SET on a file item enters action sub-mode: UP/DOWN selects Play or Delete */
        if(list_fetch_pending){printf("DVR: list loading, ignoring SET\n");break;}
        int fi=list_offset+list_focus;
        if(fi<list_count){
            list_act_focus = LIST_ACT_PLAY;
            cur_sub = DVR_SUB_LIST_ACT;   /* no visibility change, same view */
            hl_list_act(LIST_ACT_PLAY);
            printf("DVR: list -> action select (row=%d)\n", fi);
        } else { printf("DVR: no file row=%d cnt=%d\n", fi, list_count); }
        } break;

    case DVR_SUB_LIST_ACT: {
        /* Confirm the selected action: Play or Delete */
        int fi=list_offset+list_focus;
        if(list_act_focus == LIST_ACT_PLAY) {
            /* Start playback — same logic as the old direct-play path */
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
            /* Delete — show confirm popup */
            popup_src = POP_FILE_DEL; popup_focus = 1;
            show_sub(DVR_SUB_POPUP); hl_popup(1);
            printf("DVR: action delete -> popup (row=%d)\n", fi);
        }
        } break;

    case DVR_SUB_SETTING:
        switch(set_focus){
        case DVR_SET_CAMERA: set_edit_val=set_cam_onoff; hl_cam_onoff(set_edit_val); show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_LOOP:   set_edit_val=set_loop_val;  hl_loop(set_edit_val);      show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_FORMAT: popup_src=POP_FORMAT; popup_focus=1; show_sub(DVR_SUB_POPUP); hl_popup(1); break;
        default: break;
        } break;

    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_CAMERA){set_cam_onoff=set_edit_val; if(set_cam_onoff)dvr_api_rec_start();else dvr_api_rec_stop(); hl_cam_onoff(set_cam_onoff);}
        else if(set_focus==DVR_SET_LOOP){set_loop_val=set_edit_val; dvr_api_set_loop_time(set_loop_val+1); hl_loop(set_loop_val);}
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
                /* DVR identifies files by NUMBER (the %04d in MOVIxxxx/PICTxxxx),
                 * NOT by list position. Parse the number from the filename —
                 * same logic as playback (pb_start). Sending raw position made
                 * the DVR delete file #<position> which is usually wrong. */
                uint16_t par=(uint16_t)fi;   /* fallback to position */
                { char fn[DVR_NAME_MAX];
                  if (get_fname(fi, fn, sizeof(fn)) && strlen(fn) >= 8)
                      par = (uint16_t)((fn[4]-'0')*1000 + (fn[5]-'0')*100
                                       + (fn[6]-'0')*10 + (fn[7]-'0')); }
                if(list_mode==1) par|=0x8000;
                if(list_tab==1)  par|=0x4000;
                dvr_send_normal_cmd(BD_CTRL_DEL_FILE,par);
                printf("DVR: del file_no=%d (fi=%d)\n", par & 0x3FFF, fi);
            } else {
                /* Stop recording before formatting SD card */
                if (dvr_get_rec_status()) { dvr_api_rec_stop(); printf("DVR: rec stopped for format\n"); }
                dvr_api_format();
            }
        }
        if(popup_src==POP_FORMAT){show_sub(DVR_SUB_SETTING);hl_set(set_focus);}
        else{show_sub(DVR_SUB_LIST);dvr_file_list_clear();dvr_request_file_list();if(list_focus>=list_count&&list_focus>0)list_focus--;hl_list(list_focus);}
        break;

    case DVR_SUB_PLAYBACK:
        pb_paused=pb_paused?0:1; dvr_api_pb_pause();
        printf("DVR: pb %s\n",pb_paused?"paused":"resumed"); break;

    case DVR_SUB_LIST_IDLE:
        /* SET does nothing in idle state */
        break;

    case DVR_SUB_LIST_SEL:
        /* User confirmed front/rear selection — enter file list */
        list_offset=0; list_focus=0;
        show_sub(DVR_SUB_LIST); dvr_file_list_clear();
        hl_list(0); hl_tab(list_tab); dvr_request_file_list();
        printf("DVR: cam select -> list (mode=%d tab=%d)\n", list_mode, list_tab);
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
        /* Back from file list -> front/rear select. Still inside the file area
         * (recording stays stopped); rec_start happens on LIST_SEL -> MAIN. */
        show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab); break;
    case DVR_SUB_LIST_ACT:
        /* Back from action select -> return to file list browsing */
        cur_sub = DVR_SUB_LIST;  /* no visibility change needed, same view */
        hl_list(list_focus);     /* reset action icon highlights */
        printf("DVR: action select -> list\n"); break;
    case DVR_SUB_SETTING:
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_CAMERA) hl_cam_onoff(set_cam_onoff);
        else if(set_focus==DVR_SET_LOOP) hl_loop(set_loop_val);
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
        if(popup_src==POP_FORMAT){show_sub(DVR_SUB_SETTING);hl_set(set_focus);}
        else{show_sub(DVR_SUB_LIST);hl_list(list_focus);} break;
    case DVR_SUB_PLAYBACK:
        /* Back from playback -> the file LIST (pick another clip). Recording
         * stays stopped through the whole file area; it is restarted once, on
         * the exit back to live preview (LIST_SEL -> MAIN, below). */
        dvr_api_pb_stop(); pb_paused=0;
        show_sub(DVR_SUB_LIST); dvr_file_list_populate(); hl_list(list_focus);
        printf("DVR: pb stopped -> list\n"); break;
    case DVR_SUB_LIST_IDLE:
        /* Back from idle -> LIST_SEL (cam selection) */
        show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
        printf("DVR: idle -> cam select\n"); break;
    case DVR_SUB_LIST_SEL:
        /* Symmetric exit: recording was stopped on ENTERING the file area
         * (the dock handlers), so restart it HERE, on the back to live preview.
         * Unconditional - rec_start while already recording is a harmless no-op
         * and avoids trusting the cached status, which lags the DVR. */
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus);
        dvr_api_rec_start(); printf("DVR: rec restarted on exit -> main\n"); break;
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
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_CAMERA){set_edit_val=set_edit_val?0:1;hl_cam_onoff(set_edit_val);}
        else if(set_focus==DVR_SET_LOOP){set_edit_val=(set_edit_val>0)?set_edit_val-1:2;hl_loop(set_edit_val);}
        break;
    case DVR_SUB_POPUP: popup_focus=0;hl_popup(0); break;
    case DVR_SUB_LIST_ACT:
        /* UP in action select: switch to Play */
        if(list_act_focus != LIST_ACT_PLAY) { list_act_focus = LIST_ACT_PLAY; hl_list_act(LIST_ACT_PLAY); }
        break;
    case DVR_SUB_LIST_SEL:
        if(list_tab>0){ list_tab--; hl_idle_tab(list_tab); } break;
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
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_CAMERA){set_edit_val=set_edit_val?0:1;hl_cam_onoff(set_edit_val);}
        else if(set_focus==DVR_SET_LOOP){set_edit_val=(set_edit_val+1)%3;hl_loop(set_edit_val);}
        break;
    case DVR_SUB_POPUP: popup_focus=1;hl_popup(1); break;
    case DVR_SUB_LIST_ACT:
        /* DOWN in action select: switch to Delete */
        if(list_act_focus != LIST_ACT_DELETE) { list_act_focus = LIST_ACT_DELETE; hl_list_act(LIST_ACT_DELETE); }
        break;
    case DVR_SUB_LIST_SEL:
        if(list_tab<DVR_TAB_MAX-1){ list_tab++; hl_idle_tab(list_tab); } break;
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
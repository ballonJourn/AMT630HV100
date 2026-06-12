/*
 * dvr_view.c — DVR UI: 4-button dock + cam sub-dock + file list + settings
 *
 * Main dock (4 buttons):
 *   [DVR Preview] [Video Playback] [Photo Playback] [Settings]
 *
 * DVR Preview → cam sub-dock (Front / Rear / Snapshot)
 *   Front SET → switch to front cam, back to MAIN
 *   Rear  SET → switch to rear  cam, back to MAIN
 *   Snap  SET → take photo, stay (can snap again)
 *
 * Video Playback → file list, 2 tabs (Front Video / Rear Video)
 * Photo Playback → file list, 2 tabs (Front Photo / Rear Photo)
 *   UP/DOWN scrolls. SET on file → PLAYBACK. BACK → MAIN.
 *   UP at top / DOWN at bottom → switches front↔rear tab.
 *
 * Settings → row list. SET enters edit. UP/DOWN changes. SET commits.
 */

#include "dvr_view.h"
#include "dvr_api.h"
#include "../view_manager.h"
#include "common/navigator.h"

/* ---- Widgets ---- */
static widget_t* dvr_main_view    = NULL;
static widget_t* dvr_list_view    = NULL;
static widget_t* dvr_setting_view = NULL;
static widget_t* dvr_popup_view   = NULL;
static widget_t* dvr_dock_bar     = NULL;
static widget_t* dvr_cam_dock     = NULL;
static widget_t* dvr_cam_tab_sel  = NULL;

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
static widget_t* file_scroll_w    = NULL;  /* scroll_view containing file rows */
static widget_t* list_no_sd_label = NULL;  /* "No SD Card" label in list view */

/* ---- State ---- */
static dvr_sub_page_e cur_sub = DVR_SUB_MAIN;
static int dock_focus   = DVR_DOCK_PREVIEW;
static int cam_focus    = DVR_CAM_FRONT;
static int list_focus   = 0;
static int list_tab     = DVR_TAB_FRONT;   /* 0=front, 1=rear */
static int list_mode    = 0;               /* 0=video, 1=photo (set by dock button) */
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

/* Async file-list fetch: after dvr_api_get_list() we poll
 * dvr_api_is_filelist_ready() via an AWTK timer until the USB
 * task fills the cache, then populate the UI list. */
static int list_fetch_pending = 0;
static uint32_t list_poll_timer_id = TK_INVALID_ID;
#define LIST_POLL_INTERVAL_MS  100
#define LIST_POLL_MAX_RETRIES  30   /* 3 seconds timeout */
static int list_poll_retries = 0;

/* ---- dvr_get_file_list mode encoding ----
 * mode 0: front video, 1: rear video, 2: front photo, 3: rear photo
 * = list_mode * 2 + list_tab
 */
static inline uint8_t list_api_mode(void) { return (uint8_t)(list_mode * 2 + list_tab); }

/* ---- Visibility ---- */
static void show_sub(dvr_sub_page_e sub)
{
    dvr_sub_page_e prev = cur_sub;
    cur_sub = sub;

    /* Cancel any pending file-list poll timer when leaving LIST sub-page */
    if (prev == DVR_SUB_LIST && sub != DVR_SUB_LIST && list_poll_timer_id != TK_INVALID_ID) {
        timer_remove(list_poll_timer_id);
        list_poll_timer_id = TK_INVALID_ID;
        list_fetch_pending = 0;
    }

    int pv_prev = (prev == DVR_SUB_MAIN || prev == DVR_SUB_CAM_SW || prev == DVR_SUB_PLAYBACK);
    int pv_next = (sub  == DVR_SUB_MAIN || sub  == DVR_SUB_CAM_SW || sub  == DVR_SUB_PLAYBACK);
    if (pv_prev && !pv_next) { dvr_api_set_preview_enable(0); printf("DVR: preview off (sub=%d)\n", sub); }

    int mv = (sub == DVR_SUB_MAIN || sub == DVR_SUB_CAM_SW || sub == DVR_SUB_PLAYBACK);
    if (dvr_main_view)    widget_set_visible(dvr_main_view,    mv);
    if (dvr_list_view)    widget_set_visible(dvr_list_view,    sub == DVR_SUB_LIST);
    if (dvr_setting_view) widget_set_visible(dvr_setting_view, sub == DVR_SUB_SETTING || sub == DVR_SUB_SET_EDIT);
    if (dvr_popup_view)   widget_set_visible(dvr_popup_view,   sub == DVR_SUB_POPUP);
    if (dvr_dock_bar)     widget_set_visible(dvr_dock_bar,     sub == DVR_SUB_MAIN);
    if (dvr_cam_dock)     widget_set_visible(dvr_cam_dock,     sub == DVR_SUB_CAM_SW);

    if (!pv_prev && pv_next) { dvr_api_set_preview_enable(1); printf("DVR: preview on (sub=%d)\n", sub); }
}

/* ---- Refresh helpers ---- */
static void hl_dock(int i) {
    for (int n=0;n<DVR_DOCK_BTN_MAX;n++) if(dock_btn[n]) widget_set_state(dock_btn[n],(n==i)?STATE_SELECTE:STATE_NORMAL);
    dock_focus = i;
}
static void hl_cam(int i) {
    cam_focus = i;
    if (dvr_cam_tab_sel) widget_move(dvr_cam_tab_sel, i*324, 0);
}
static void hl_list(int i) {
    for(int n=0;n<DVR_FILE_ITEM_MAX;n++) if(file_name_w[n]) widget_set_state(file_name_w[n],(n==i)?STATE_SELECTE:STATE_NORMAL);
}
static void hl_tab(int t) {
    list_tab = t;
    if (list_tab_sel) widget_move(list_tab_sel, t?486:0, 0);
    if (tab_label_left && tab_label_right) {
        if (list_mode == 0) { widget_set_text_utf8(tab_label_left,"Front Video"); widget_set_text_utf8(tab_label_right,"Rear Video"); }
        else                { widget_set_text_utf8(tab_label_left,"Front Photo"); widget_set_text_utf8(tab_label_right,"Rear Photo"); }
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
        /* Alert mode: single centered OK button */
        if(c) { widget_set_visible(c, TRUE); widget_set_state(c, STATE_SELECTE); }
        if(cl) { widget_set_visible(cl, TRUE); widget_set_text_utf8(cl, "OK"); }
        if(x) widget_set_visible(x, FALSE);
        if(xl) widget_set_visible(xl, FALSE);
    } else {
        /* Normal confirm/cancel mode */
        if(c) { widget_set_visible(c, TRUE); widget_set_state(c,(f==0)?STATE_SELECTE:STATE_NORMAL); }
        if(x) { widget_set_visible(x, TRUE); widget_set_state(x,(f==1)?STATE_SELECTE:STATE_NORMAL); }
        if(cl) { widget_set_visible(cl, TRUE); widget_set_text_utf8(cl, "OK"); }
        if(xl) { widget_set_visible(xl, TRUE); widget_set_text_utf8(xl, "Cancel"); }
    }
}

/* Show "No SD Card" alert popup. Returns to `return_sub` on SET/BACK. */
static dvr_sub_page_e no_sd_return_sub = DVR_SUB_MAIN;

static void show_no_sd_popup(dvr_sub_page_e return_to) {
    widget_t* title = dvr_popup_view ? widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE) : NULL;
    if(title) widget_set_text_utf8(title, "No SD Card");
    popup_src = POP_NO_SD;
    no_sd_return_sub = return_to;
    popup_focus = 0;
    show_sub(DVR_SUB_POPUP);
    hl_popup(0);
    printf("DVR: no SD card alert\n");
}

/* ---- File list data ---- */

/* Timer callback: polls dvr_api_is_filelist_ready() until the USB task
 * signals that the file-list response has been parsed, then populates
 * the UI list.  Self-cancels on success or timeout. */
static ret_t on_filelist_poll_timer(const timer_info_t* info)
{
    (void)info;
    if (dvr_api_is_filelist_ready()) {
        dvr_api_clear_filelist_ready();
        list_fetch_pending = 0;
        list_poll_timer_id = TK_INVALID_ID;
        dvr_file_list_populate();
        hl_list(list_focus);
        printf("DVR: filelist ready, populated\n");
        return RET_REMOVE;
    }
    if (++list_poll_retries >= LIST_POLL_MAX_RETRIES) {
        list_fetch_pending = 0;
        list_poll_timer_id = TK_INVALID_ID;
        dvr_file_list_populate(); /* show whatever is cached */
        hl_list(list_focus);
        printf("DVR: filelist poll timeout (%d retries)\n", LIST_POLL_MAX_RETRIES);
        return RET_REMOVE;
    }
    return RET_REPEAT;
}

/* Fire dvr_api_get_list() and start a timer to poll for the result.
 * Safe to call multiple times; cancels any previous pending poll. */
static void dvr_request_file_list(void)
{
    if (list_poll_timer_id != TK_INVALID_ID) {
        timer_remove(list_poll_timer_id);
        list_poll_timer_id = TK_INVALID_ID;
    }
    dvr_api_clear_filelist_ready();
    list_fetch_pending = 1;
    list_poll_retries = 0;
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
static uint8_t get_fname(int idx,char*out,int sz) {
    char buf[DVR_FILE_ITEM_MAX*DVR_NAME_MAX]; uint16_t c=0;
    switch(list_api_mode()){
    case 0:c=dvr_api_get_video_list_f(buf,DVR_FILE_ITEM_MAX);break;
    case 1:c=dvr_api_get_video_list_r(buf,DVR_FILE_ITEM_MAX);break;
    case 2:c=dvr_api_get_photo_list_f(buf,DVR_FILE_ITEM_MAX);break;
    case 3:c=dvr_api_get_photo_list_r(buf,DVR_FILE_ITEM_MAX);break;
    default:return 0;
    }
    if(idx<0||idx>=(int)c)return 0;
    const char*s=buf+(size_t)idx*DVR_NAME_MAX; int l=(int)strlen(s); if(l>=sz)l=sz-1;
    memcpy(out,s,l); out[l]='\0'; return 1;
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
        /* Update row icon: video or photo */
        if(file_icon_w[i]) {
            if(list_mode==0) { widget_use_style(file_icon_w[i],"default"); /* keep video icon */ }
            /* Note: icon swap via style would need per-mode styles in XML.
             * For now the XML has video icons; photo mode will also show them.
             * TODO: swap icon image name via widget_set_prop_str if needed. */
        }
    }
    printf("DVR: populate mode=%d tab=%d cnt=%d off=%d\n",list_mode,list_tab,tot,list_offset);
}

/* ---- Init ---- */
ret_t home_dvr_view_init(widget_t* parent) {
    char buf[32]; if(!parent)return RET_FAIL;
    dvr_main_view    = widget_lookup(parent,"dvr_main_view",TRUE);
    dvr_list_view    = widget_lookup(parent,"dvr_list_view",TRUE);
    dvr_setting_view = widget_lookup(parent,"dvr_setting_view",TRUE);
    dvr_popup_view   = widget_lookup(parent,"dvr_popup_view",TRUE);
    dvr_dock_bar     = widget_lookup(parent,"dvr_dock_bar",TRUE);
    dvr_cam_dock     = widget_lookup(parent,"dvr_cam_dock",TRUE);
    dvr_cam_tab_sel  = widget_lookup(parent,"dvr_cam_tab_sel",TRUE);
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

    dock_focus=DVR_DOCK_PREVIEW; cam_focus=DVR_CAM_FRONT;
    list_focus=0; list_tab=0; list_mode=0; list_count=0; list_offset=0;
    set_focus=DVR_SET_CAMERA; popup_focus=0;
    set_cam_onoff=1; set_loop_val=0; set_edit_val=0;
    popup_src=POP_FILE_DEL; pb_paused=0; pb_idx=0; pb_mode=0;

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
            if (!dvr_get_sd_status()) {
                show_no_sd_popup(DVR_SUB_MAIN);
                printf("DVR: no SD, cannot enter video list\n"); break;
            }
            list_mode=0; list_tab=DVR_TAB_FRONT; list_offset=0; list_focus=0;
            show_sub(DVR_SUB_LIST);
            dvr_file_list_clear();
            hl_list(0); hl_tab(list_tab);
            dvr_request_file_list();
            printf("DVR: enter video list\n"); break;
        case DVR_DOCK_PHOTO_PB:
            if (!dvr_get_sd_status()) {
                show_no_sd_popup(DVR_SUB_MAIN);
                printf("DVR: no SD, cannot enter photo list\n"); break;
            }
            list_mode=1; list_tab=DVR_TAB_FRONT; list_offset=0; list_focus=0;
            show_sub(DVR_SUB_LIST);
            dvr_file_list_clear();
            hl_list(0); hl_tab(list_tab);
            dvr_request_file_list();
            printf("DVR: enter photo list\n"); break;
        case DVR_DOCK_SETTINGS:
            show_sub(DVR_SUB_SETTING); set_focus=DVR_SET_CAMERA;
            hl_set(DVR_SET_CAMERA); hl_cam_onoff(set_cam_onoff); hl_loop(set_loop_val);
            printf("DVR: enter settings\n"); break;
        default: break;
        }
        break;

    case DVR_SUB_CAM_SW:
        switch(cam_focus) {
        case DVR_CAM_FRONT:  dvr_api_view_switch(0); printf("DVR: cam→front\n"); show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
        case DVR_CAM_REAR:   dvr_api_view_switch(1); printf("DVR: cam→rear\n");  show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
        case DVR_CAM_SNAPSHOT:
            if (!dvr_get_sd_status()) {
                show_no_sd_popup(DVR_SUB_CAM_SW);
            } else {
                dvr_api_snap(); printf("DVR: snap!\n");
            }
            break;
        default: break;
        }
        break;

    case DVR_SUB_LIST: {
        if(list_fetch_pending){printf("DVR: list still loading, ignoring SET\n");break;}
        int fi=list_offset+list_focus;
        if(fi<list_count){
            if(dvr_get_rec_status()){dvr_api_rec_stop();printf("DVR: rec stopped\n");}
            pb_mode=list_api_mode(); pb_idx=fi; pb_paused=0;
            dvr_api_pb_start((uint8_t)pb_mode,(uint16_t)fi);
            show_sub(DVR_SUB_PLAYBACK);
            printf("DVR: pb start mode=%d idx=%d\n",pb_mode,fi);
        } else { printf("DVR: no file row=%d cnt=%d\n",fi,list_count); }
        } break;

    case DVR_SUB_SETTING:
        switch(set_focus){
        case DVR_SET_CAMERA: set_edit_val=set_cam_onoff; hl_cam_onoff(set_edit_val); show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_LOOP:   set_edit_val=set_loop_val;  hl_loop(set_edit_val);      show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_FORMAT: popup_src=POP_FORMAT; popup_focus=1; show_sub(DVR_SUB_POPUP); hl_popup(1); break;
        default: break;
        }
        break;

    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_CAMERA){set_cam_onoff=set_edit_val; if(set_cam_onoff)dvr_api_rec_start();else dvr_api_rec_stop(); hl_cam_onoff(set_cam_onoff); printf("DVR: cam %s\n",set_cam_onoff?"ON":"OFF");}
        else if(set_focus==DVR_SET_LOOP){set_loop_val=set_edit_val; dvr_api_set_loop_time(set_loop_val+1); hl_loop(set_loop_val); printf("DVR: loop=%dmin\n",set_loop_val+1);}
        show_sub(DVR_SUB_SETTING); hl_set(set_focus);
        break;

    case DVR_SUB_POPUP:
        if (popup_src == POP_NO_SD) {
            /* Alert dismissed — restore popup title for next use */
            widget_t* title = dvr_popup_view ? widget_lookup(dvr_popup_view,"dvr_popup_title",TRUE) : NULL;
            if(title) widget_set_text_utf8(title, "Confirm?");
            show_sub(no_sd_return_sub);
            if(no_sd_return_sub==DVR_SUB_MAIN) hl_dock(dock_focus);
            else if(no_sd_return_sub==DVR_SUB_CAM_SW) hl_cam(cam_focus);
            break;
        }
        if(popup_focus==0){
            if(popup_src==POP_FILE_DEL){
                int fi=list_offset+list_focus; uint16_t par=(uint16_t)fi;
                if(list_mode==1) par|=0x8000; /* photo bit */
                if(list_tab==1)  par|=0x4000; /* rear bit */
                dvr_send_normal_cmd(BD_CTRL_DEL_FILE,par);
                printf("DVR: del par=0x%04X\n",par);
            } else { dvr_api_format(); }
        }
        if(popup_src==POP_FORMAT){show_sub(DVR_SUB_SETTING);hl_set(set_focus);}
        else{show_sub(DVR_SUB_LIST);dvr_file_list_clear();dvr_request_file_list();if(list_focus>=list_count&&list_focus>0)list_focus--;hl_list(list_focus);}
        break;

    case DVR_SUB_PLAYBACK:
        pb_paused=pb_paused?0:1; dvr_api_pb_pause();
        printf("DVR: pb %s\n",pb_paused?"paused":"resumed"); break;

    default: break;
    }
}

/* ==== BACK ==== */
void dvr_page_deal_key_back(void) {
    switch(cur_sub) {
    case DVR_SUB_MAIN:
        if(dvr_main_view) widget_set_visible(dvr_main_view,FALSE);
        dvr_stop_preview(); navigator_back(); break;
    case DVR_SUB_CAM_SW:   show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
    case DVR_SUB_LIST:     show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
    case DVR_SUB_SETTING:  show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
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
        dvr_api_pb_stop(); pb_paused=0; printf("DVR: pb stopped\n");
        show_sub(DVR_SUB_LIST); dvr_file_list_populate(); hl_list(list_focus); hl_tab(list_tab); break;
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
            dvr_file_list_clear();
            dvr_request_file_list();
            list_focus=0; hl_list(0); hl_tab(list_tab);
        }
        break;
    case DVR_SUB_SETTING:  if(set_focus>0){set_focus--;hl_set(set_focus);} break;
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_CAMERA){set_edit_val=set_edit_val?0:1;hl_cam_onoff(set_edit_val);}
        else if(set_focus==DVR_SET_LOOP){set_edit_val=(set_edit_val>0)?set_edit_val-1:2;hl_loop(set_edit_val);}
        break;
    case DVR_SUB_POPUP: popup_focus=0;hl_popup(0); break;
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
            dvr_file_list_clear();
            dvr_request_file_list();
            list_focus=0; hl_list(0); hl_tab(list_tab);
        }
        break;
    case DVR_SUB_SETTING:  if(set_focus<DVR_SET_ROW_MAX-1){set_focus++;hl_set(set_focus);} break;
    case DVR_SUB_SET_EDIT:
        if(set_focus==DVR_SET_CAMERA){set_edit_val=set_edit_val?0:1;hl_cam_onoff(set_edit_val);}
        else if(set_focus==DVR_SET_LOOP){set_edit_val=(set_edit_val+1)%3;hl_loop(set_edit_val);}
        break;
    case DVR_SUB_POPUP: popup_focus=1;hl_popup(1); break;
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
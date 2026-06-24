/*
 * dvr_page.c — Init for the independent DVR window (dvr_page.xml)
 *
 * On entry: shows MAIN state (dvr_bg image + 4-button dock).
 * Preview is NOT started here — only when user enters CAM_SW.
 */

#include "awtk.h"
#include "../view/home_view/dvr_view.h"
#include "../view/home_view/dvr_api.h"
#include "../view/view_manager.h"

#define DVR_PREVIEW_MODE_FRONT_REAR  2

static ret_t on_dvr_page_close(void* ctx, event_t* e)
{
    (void)ctx; (void)e;
    printf("DVR-EXIT: on_dvr_page_close enter, preview_enable=%d\n",
           dvr_api_get_preview_enable());
    if (dvr_api_get_preview_enable()) { dvr_stop_preview(); }
    dvr_set_sensor_switch_enable(1);
    set_dock_view(ICON_DVR);
    set_current_level(MENU_LEVEL_0);
    printf("DVR-EXIT: on_dvr_page_close done\n");
    return RET_OK;
}

ret_t dvr_page_init(widget_t* win, void* ctx)
{
    (void)ctx;
    return_value_if_fail(win != NULL, RET_BAD_PARAMS);
    home_dvr_view_init(win);
    widget_on(win, EVT_WINDOW_CLOSE, on_dvr_page_close, NULL);

    /* Display window: full-width (0,0,1024,500). Both the
     * VIDEO (OSD0) layer geometry and the alpha-clear hole derive from this. */
    dvr_api_set_display_window(0, 0, 1024, 500);

    /* Start preview on page entry (restore 5b67ca2 behavior). This keeps the
     * DVR USB video stream drained and the alpha-clear hook active for the
     * whole session. Without it the pipe backs up while at MAIN and the DVR
     * fails to deliver sustained playback frames (single-frame photo still
     * worked, but continuous video playback went black). */
    dvr_api_set_preview_enable(1);

    /* Lock preview to front+rear dual-camera mode */
    dvr_api_view_switch(DVR_PREVIEW_MODE_FRONT_REAR);

    /* Query DVR status (SD card state, etc.) */
    dvr_api_get_status();

    /* Pre-fetch version and TF capacity into cache while user is on the
     * main DVR screen.  By the time the user navigates to Settings, the
     * async replies will (usually) have arrived, so the loading popup
     * can be skipped entirely. */
    dvr_api_get_id();
    dvr_api_get_tf_capacity_query();

    dvr_set_sensor_switch_enable(0);
    printf("DVR: dvr_page_init, preview on, window=(0,0,1024,500)\n");
    return RET_OK;
}
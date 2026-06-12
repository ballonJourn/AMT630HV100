/*
 * dvr_page.c — Init for the independent DVR window (dvr_page.xml)
 *
 * This window has bg_color="#00000000" (fully transparent).
 * The VIDEO layer (OSD0) shows through the transparent regions.
 * Only the dock bar and overlay views (list/settings/popup) are opaque.
 *
 * No alpha-punch needed. No per-frame framebuffer manipulation.
 */

#include "awtk.h"
#include "../view/home_view/dvr_view.h"
#include "../view/home_view/dvr_api.h"
#include "../view/view_manager.h"

/* View mode: 0=front, 1=rear, 2=front+rear, 3=rear+front, 4=hzh */
#define DVR_PREVIEW_MODE_FRONT_REAR  2

/*
 * Called when dvr_page window is closed (BACK from DVR).
 * Ensures video layer is stopped and home_page dock resets.
 */
static ret_t on_dvr_page_close(void* ctx, event_t* e)
{
    (void)ctx;
    (void)e;

    printf("DVR-EXIT: on_dvr_page_close enter, preview_enable=%d\n",
           dvr_api_get_preview_enable());

    if (dvr_api_get_preview_enable()) {
        dvr_stop_preview();
    }

    dvr_set_sensor_switch_enable(1);
    set_dock_view(ICON_DVR);
    set_current_level(MENU_LEVEL_0);

    printf("DVR-EXIT: on_dvr_page_close done\n");
    return RET_OK;
}

/*
 * dvr_page_init — Called by navigator when dvr_page window opens.
 * Initializes DVR UI widgets and starts video preview.
 *
 * Preview is locked to front+rear dual-camera mode (mode 2).
 * UP/DOWN keys are freed for dock-button navigation within the DVR page.
 */
ret_t dvr_page_init(widget_t* win, void* ctx)
{
    (void)ctx;
    return_value_if_fail(win != NULL, RET_BAD_PARAMS);

    /* Initialize DVR view widgets (dock buttons, file list, settings, popup) */
    home_dvr_view_init(win);

    /* Register close event to clean up video layer */
    widget_on(win, EVT_WINDOW_CLOSE, on_dvr_page_close, NULL);

    /* Start video preview — VIDEO layer renders underneath this window */
    dvr_api_set_display_window(52, 0, 972, 500);
    dvr_api_set_preview_enable(1);

    /* Lock preview to front+rear dual-camera mode */
    dvr_api_view_switch(DVR_PREVIEW_MODE_FRONT_REAR);

    /* Query current DVR status (SD card, recording, etc.) so UI
     * has accurate state for snap/recording controls.
     * Note: auto-rec is handled by dvr_usb_task itself — it queries
     * status after init and starts recording when SD=1 is confirmed.
     * This UI-side getsts is supplementary (may arrive before task
     * is ready, which is fine — the task handles its own retry). */
    dvr_api_get_status();

    /* Disable auto view-mode cycling; preview is fixed to dual-cam */
    dvr_set_sensor_switch_enable(0);

    printf("DVR: dvr_page_init, preview started (52,0,972,500) mode=%d\n",
           DVR_PREVIEW_MODE_FRONT_REAR);
    return RET_OK;
}
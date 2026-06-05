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

    /* Disable auto view-mode cycling; user switches manually with UP/DOWN */
    dvr_set_sensor_switch_enable(0);

    printf("DVR: dvr_page_init, preview started (52,0,972,500)\n");
    return RET_OK;
}
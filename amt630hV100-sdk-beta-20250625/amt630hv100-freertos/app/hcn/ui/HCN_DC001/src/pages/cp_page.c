#include "awtk.h"
#include "../common/navigator.h"
#include "view/cp_view/cp_view.h"
#include "proxy/bluetooth_data.h"

static ret_t on_cp_page_will_open(void* ctx, event_t* e)
{
    (void)ctx ;

    if (e->type == EVT_WINDOW_WILL_OPEN) {
#if !ON_PC_CACLE
        const char* bt_name = vehicle_get_bluetooth_name() ;
        cp_view_refresh_tip(bt_name) ;
#else
        cp_view_refresh_tip("HCN-XXXX") ;
#endif
    }

    return RET_OK ;
}

/**
 * 初始化窗口
 */
ret_t cp_page_init(widget_t* win, void* ctx)
{
    (void)ctx ;
    return_value_if_fail(win != NULL, RET_BAD_PARAMS) ;

    cp_view_init(win) ;

    widget_on(win, EVT_WINDOW_WILL_OPEN, on_cp_page_will_open, win) ;

    return RET_OK ;
}

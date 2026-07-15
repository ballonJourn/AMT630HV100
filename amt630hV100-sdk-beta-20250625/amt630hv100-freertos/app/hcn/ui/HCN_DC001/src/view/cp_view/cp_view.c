#include "cp_view.h"
#include <stdio.h>

static widget_t* cp_tip_label = NULL ;

ret_t cp_view_init(widget_t* parent)
{
    if (parent == NULL) return RET_FAIL ;

    cp_tip_label = widget_lookup(parent, "cp_tip_label", TRUE) ;

    return RET_OK ;
}

void cp_view_refresh_tip(const char* bt_name)
{
    if (cp_tip_label && bt_name) {
        char buf[128] ;
        tk_snprintf(buf, sizeof(buf),
            "请通过蓝牙连接\n%s\n以使用CarPlay", bt_name) ;
        widget_set_text_utf8(cp_tip_label, buf) ;
    }
}

/**
*
* @file hcn_msg_manage.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/22 16:36
* @author och
*
*/

#include "mw/msg_manage/hcn_msg_manage.h"

static usb_status_t g_usb_status = USB_STATUS_REMOVED;

void hcn_usb_status_change(usb_status_t status) {
    g_usb_status = status;
}

usb_status_t hcn_get_usb_status(void) {
    return g_usb_status;
}


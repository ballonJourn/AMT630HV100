/**
*
* @file hcn_msg_manage.h
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
#ifndef __HCN_MSG_MANAGE_H__
#define __HCN_MSG_MANAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    HCN_MSG_USB_STATUS,   ///< U盘状态变更
    HCN_MSG_SD_STATUS,   ///< SD卡状态变更
    HCN_MSG_OTA_STAUS,   ///< OTA状态变更
} msg_type_t;

/**
 * @brief USB状态枚举
 */
typedef enum {
    USB_STATUS_REMOVED = 0,   ///< U盘未插入/已拔出
    USB_STATUS_INSERTED = 1   ///< U盘已插入
} usb_status_t;

void hcn_usb_status_change(usb_status_t status);
usb_status_t hcn_get_usb_status(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MSG_MANAGE_H__
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

#include <FreeRTOS.h>
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    HCN_MSG_MCU_UPDATE_STATUS,   ///< MCU升级状态变更
    HCN_MSG_USB_UPDATE_STATUS,   ///< U盘升级状态变更
    HCN_MSG_SD_UPDATE_STATUS,   ///< SD卡升级状态变更
    HCN_MSG_OTA_STAUS,          ///< OTA状态变更
} msg_type_t;

typedef struct {
    msg_type_t type;
    union {
       void *p;
       uint32_t val;
       uint16_t val16[2];
       uint8_t val8[4];
    } data;
} mw_msg_data_t;

/**
 * @brief USB状态枚举
 */
typedef enum {
    USB_STATUS_REMOVED = 0,   ///< U盘未插入/已拔出
    USB_STATUS_INSERTED = 1   ///< U盘已插入
} usb_status_t;

/**
 * @brief  设置USB状态
 * @param  status USB状态
 * @return none
 */
void hcn_usb_status_change(usb_status_t status);

/**
 * @brief  获取USB状态
 * @param  none
 * @return USB状态
 */
usb_status_t hcn_get_usb_status(void);

/**
 * @brief  初始化消息管理模块
 * @param  none
 * @return 0:成功，-1:失败
 */
int mw_msg_manage_init(void);

/**
 * @brief  发送消息到消息管理模块(中断中调用)
 * @param  msg 消息指针     
 * @return 0:成功，-1:失败
 */
BaseType_t send_mw_msg_from_isr(mw_msg_data_t *msg);

/**
 * @brief  发送消息到消息管理模块
 * @param  msg 消息指针     
 * @return 0:成功，-1:失败
 */
int send_mw_msg(mw_msg_data_t *msg);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MSG_MANAGE_H__
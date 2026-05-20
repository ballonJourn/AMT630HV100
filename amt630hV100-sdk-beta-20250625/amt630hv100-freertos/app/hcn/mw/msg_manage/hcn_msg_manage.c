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
#include "ota_manage/hcn_ota.h"
#include "log/hcn_log.h"

#define MW_MSG_QUEUE_LENGTH (20)
#define MW_MSG_ITEM_SIZE    sizeof(mw_msg_data_t)

static usb_status_t g_usb_status = USB_STATUS_REMOVED;
static QueueHandle_t mw_msg_queue = NULL;

void hcn_usb_status_change(usb_status_t status) {
    g_usb_status = status;
}

usb_status_t hcn_get_usb_status(void) {
    return g_usb_status;
}

int send_mw_msg(mw_msg_data_t *msg) {
    if (mw_msg_queue == NULL || msg == NULL) {
        hcn_log_error("mw msg queue is NULL or msg is NULL.\n");
        return -1;
    }

    if (xQueueSend(mw_msg_queue, msg, pdMS_TO_TICKS(100)) != pdPASS) {
        hcn_log_error("xQueueSend mw msg error.\n");
        return -1;
    }
    return 0;
}

BaseType_t send_mw_msg_from_isr(mw_msg_data_t *msg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (mw_msg_queue == NULL || msg == NULL) {
        hcn_log_error("mw msg queue is NULL or msg is NULL.\n");
        return pdFALSE;
    }

    if (xQueueSendFromISR(mw_msg_queue, msg, &xHigherPriorityTaskWoken) != pdPASS) {
        hcn_log_error("xQueueSendFromISR mw msg error.\n");
        return pdFALSE;
    }

    return xHigherPriorityTaskWoken;
}

static void mw_msg_manage_thread(void *param) {
    mw_msg_data_t msg;

    for (;;) {
        if (xQueueReceive(mw_msg_queue, &msg, portMAX_DELAY) != pdPASS) {
            hcn_log_error("xQueueRecv mw msg error.\n");
            continue;
        }

        switch (msg.type) {
            case HCN_MSG_MCU_UPDATE_STATUS:
            case HCN_MSG_USB_UPDATE_STATUS:
            case HCN_MSG_SD_UPDATE_STATUS:
            case HCN_MSG_OTA_STAUS:
                parse_update_status(&msg);
                break;

            default:
                hcn_log_error("Unknown mw msg type: %d\n", msg.type);
                break;
        }
    }
}

int mw_msg_manage_init(void) {
    if (mw_msg_queue == NULL) {
        mw_msg_queue = xQueueCreate(MW_MSG_QUEUE_LENGTH, MW_MSG_ITEM_SIZE);
        if (mw_msg_queue == NULL) {
            hcn_log_error("Create mw msg queue fail.\n");
            return -1;
        }
    }

    if (xTaskCreate(mw_msg_manage_thread, "mw_msg_manage_thread",
                    configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 3,
                    NULL) != pdPASS) {
        hcn_log_error("Create mw msg manage task fail.\n");
        vQueueDelete(mw_msg_queue);
        mw_msg_queue = NULL;
        return -1;
    }

    hcn_log_info("mw msg manage init success.\n");

    return 0;
}

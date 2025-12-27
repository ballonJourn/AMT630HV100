/**
*
* @file hcn_ota.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/11/18 17:19
* @author och
*
*/

#include <string.h>
#include "ota_manage/hcn_ota.h"
#include "log/hcn_log.h"

static update_info_t g_update_info = {
    .type = UPDATE_NONE,
    .status = UPDATE_STATUS_IDLE,
    .error = UPDATE_ERROR_NONE,
};

static const char* get_update_type_str(update_type_e type) {
    switch (type) {
        case UPDATE_USB_SOC:
            return "USB-SOC";
        case UPDATE_USB_MCU:
            return "USB-MCU";
        case UPDATE_SD_CARD:
            return "SD_CARD";
        case UPDATE_OTA:
            return "OTA";
        default:
            return "NONE";
    }
}

/**
 * @brief  更新进度回调函数
 * @param  type 更新类型
 * @param  error 错误码
 * @param  progress 更新进度，单位：百分比(0-100)
 * @return none
 */
static void on_update_process(update_type_e type, uint8_t error, 
                                uint8_t progress) 
{
    static uint8_t last_percent = 0xFF;
    if (progress == last_percent && error == UPDATE_ERROR_NONE) {
        return;
    }

    g_update_info.type = type;
    g_update_info.status = UPDATE_STATUS_IN_PROGRESS;
    g_update_info.error = error;
    g_update_info.progress = progress;
    last_percent = progress;

    hcn_log_info("Update type:%s, status:%s, progress:%d%\r\n", \
        get_update_type_str(type), (error == 0 ? "ok" : "error"), progress);

    if (g_update_info.error != UPDATE_ERROR_NONE) {
        g_update_info.status = UPDATE_STATUS_FAILED;
    } else {
        if (progress >= 100) {
            g_update_info.status = UPDATE_STATUS_SUCCESS;

            extern void wdt_cpu_reboot(void);
			printf("Ota update bin success, cpu will reboot...\r\n");
			vTaskDelay(500);
			wdt_cpu_reboot();
        }
    }
}

void send_update_status(uint8_t msg_type, uint32_t total_size, 
                        uint32_t cur_off, uint8_t error) 
{
    uint8_t percent = 0;
    if (total_size > 0) {
        percent = ((cur_off * 100) / total_size);
        if (percent > 100) {
            percent = 100;
        }
    }

    update_msg_info_t update_data = {
        .msg_type = msg_type,
        .error = error,
        .percent = percent,
    };

    mw_msg_data_t msg = {0};
    memcpy(&msg.data.val, &update_data, sizeof(update_msg_info_t)); 

    send_mw_msg(&msg);
}

void sens_ota_update_state(uint8_t msg_type, uint8_t percent, uint8_t error) {
    if (percent > 100) {
        percent = 100;
    }

    update_msg_info_t update_data = {
        .msg_type = msg_type,
        .error = error,
        .percent = percent,
    };

    mw_msg_data_t msg = {0};
    memcpy(&msg.data.val, &update_data, sizeof(update_msg_info_t)); 

    send_mw_msg(&msg);
 }

void parse_update_status(mw_msg_data_t *msg) {
    if (msg == NULL) {
        hcn_log_error("msg is NULL.\n");
        return;
    }

    update_msg_info_t update_info = {0};
    memcpy(&update_info, &msg->data.val, sizeof(update_msg_info_t));

    update_type_e type = UPDATE_NONE;
    switch (update_info.msg_type) {
        case HCN_MSG_MCU_UPDATE_STATUS:
            type = UPDATE_USB_MCU;
            break;

        case HCN_MSG_USB_UPDATE_STATUS:
            type = UPDATE_USB_SOC;
            break;

        case HCN_MSG_SD_UPDATE_STATUS:
            type = UPDATE_SD_CARD;
            break;

        case HCN_MSG_OTA_STAUS:
            type = UPDATE_OTA;
            break;

        default:
            hcn_log_error("Unknown update msg type: %d\n", update_info.msg_type);
            return;
    }

    if (type == UPDATE_NONE) {
        hcn_log_error("Invalid update type: %d\n", type);
        return;
    }

    on_update_process(type, update_info.error, update_info.percent);
}

update_info_t *get_current_update_info(void) {
    return &g_update_info;
}

void set_update_state_reset(void) {
    g_update_info.type = UPDATE_NONE;
    g_update_info.status = UPDATE_STATUS_IDLE;
    g_update_info.error = UPDATE_ERROR_NONE;
    g_update_info.progress = 0;
}

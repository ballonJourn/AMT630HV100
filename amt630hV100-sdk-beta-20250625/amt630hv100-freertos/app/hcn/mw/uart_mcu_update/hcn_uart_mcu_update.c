/**
*
* @file hcn_uart_mcu_update.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/31 17:03
* @author och
*
*/

#include <FreeRTOS.h>
#include "task.h"
#include "uart_mcu_update/hcn_uart_mcu_update.h"
#include "uart_communicate/hcn_uart_common.h"
#include "uart_communicate/hcn_uart_tx.h"
#include "storage_param1/hcn_read_nor_flash.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"
#include "app/md5.h"
#include "wdt.h"

#ifdef HCN_UART_MCU_UPDATE_ENABLE

#define MCU_UPDATE_PACKAGE_LEN (128)
#define MCU_UPDATE_FAILED_RETRY (5)
#define MCU_MD5_CRC_LEN (16)

static mcu_update_t mcu_update;
static QueueHandle_t mcu_update_queue = NULL;
static TaskHandle_t mcu_update_task = NULL;
static md5_context g_md5_ctx;
static char hcn_mcu_full_ver[MCU_VER_MAX_LEN] = {"DC001-GD_24.11.13V0"};

h_bool stop_mcu_update(void);
h_bool start_mcu_update(void);

static h_bool mcu_update_msg(uint8_t *buffer) {
    if (!buffer) {
        return h_false;
    }

    if (!mcu_update_queue ||
        xQueueSend(mcu_update_queue, &buffer, pdMS_TO_TICKS(100)) != pdPASS) {
        vPortFree(buffer);
        hcn_log_error("xQueueSend mcu update queue error\r\n");
        return h_false;
    }

    return h_true;
}

static h_bool send_start_update_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg) {
        return h_false;
    }

    uint16_t file_crc = mcu_update.file_info.file_crc;

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = (MSG_CMD_START_UPDATE >> 8) & 0xFF;
    msg->buffer[5] = (MSG_CMD_START_UPDATE & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x02;
    msg->buffer[8] = (uint8_t)(file_crc >> 8);
    msg->buffer[9] = (uint8_t)(file_crc & 0xFF);
    msg->buffer[10] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[11] = UART_MCU_MSG_TAIL;

    if (send_mcu_msg(msg) == 0) {
        mcu_update.status.start_mcu_update = h_true;
        return h_true;
    }

    return h_false;
}

static h_bool send_end_update_2_mcu(void) {
    meter_info_t *meter_info = get_hcn_info();
    if (meter_info->mcu_update == 2) {
        meter_info->mcu_update = 0;
    } else {
        meter_info->mcu_update = 0;
    }

    if (save_hcn_info() != 0) {
        hcn_log_error("Save usr param failed!\n");
    }

    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg) {
        return h_false;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = ((MSG_CMD_STOP_UPDATE >> 8) & 0xFF);
    msg->buffer[5] = (MSG_CMD_STOP_UPDATE & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x00;
    msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[10] = UART_MCU_MSG_TAIL;

    mcu_update.status.start_mcu_update = h_false;

    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }

    return h_false;
}

static h_bool send_update_data_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg || !mcu_update.status.start_mcu_update) {
        return h_false;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = ((MSG_CMD_SEND_DATA_2_MCU >> 8) & 0xFF);
    msg->buffer[5] = (MSG_CMD_SEND_DATA_2_MCU & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = MCU_UPDATE_PACKAGE_LEN;
    memset(msg->buffer + 8, 0x00, MCU_UPDATE_PACKAGE_LEN);

    int send_len, offset_tmp;
    send_len = (mcu_update.file_info.remain_size < MCU_UPDATE_PACKAGE_LEN)
                   ? mcu_update.file_info.remain_size
                   : MCU_UPDATE_PACKAGE_LEN;
    offset_tmp = mcu_update.file_info.offset_size;
    memcpy(msg->buffer + 8, &mcu_update.file_info.update_buff[offset_tmp],
           send_len);
    mcu_update.file_info.offset_size += send_len;
    mcu_update.file_info.remain_size -= send_len;

    msg->buffer[8 + MCU_UPDATE_PACKAGE_LEN] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[9 + MCU_UPDATE_PACKAGE_LEN] = UART_MCU_MSG_TAIL;
    memcpy(mcu_update.file_info.save_data_buff, msg->buffer,
           MCU_UPDATE_PACKAGE_LEN + MCU_UPDATE_MSG_MIN_LEN);
    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }

    return h_false;
}

static h_bool resend_update_data_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg || !mcu_update.status.start_mcu_update) {
        return h_false;
    }

    uint16_t data_len = (mcu_update.file_info.save_data_buff[6] << 8) +
                        mcu_update.file_info.save_data_buff[7];
    memcpy(msg->buffer, mcu_update.file_info.save_data_buff,
           data_len + MCU_UPDATE_MSG_MIN_LEN);
    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }

    return h_false;
}

h_bool mcu_update_msg_continue(void) {
    if (mcu_update.file_info.file_len > 0 &&
        mcu_update.file_info.remain_size == 0) {
        send_end_update_2_mcu();
        stop_mcu_update();
        return h_true;
    }

    uint8_t *type = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
    if (!type) {
        return h_false;
    }

    *type = CONTINUE_SEND_MSG;
    return mcu_update_msg(type);
}

h_bool mcu_update_msg_resend(void) {
    uint8_t *type = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
    if (!type) {
        return h_false;
    }
    *type = RESEND_MSG;
    return mcu_update_msg(type);
}

h_bool mcu_update_msg_data_fail(void) {
    if (mcu_update.file_info.update_repeat >= MCU_UPDATE_FAILED_RETRY) {
        ///< mcu update failed more than 5 times, cpu reboot 
        meter_info_t *meter_info = get_hcn_info();
        if (meter_info->mcu_update) {
            meter_info->mcu_update = 0;
            if (save_hcn_info() != 0) {
                hcn_log_error("Save usr param failed!\n");
            }
        }

        hcn_log_info("Hb mcu update failed more than five times!!!\n");
        hcn_log_info("Cpu will reboot...\n");

        extern void wdt_cpu_reboot(void);
        wdt_cpu_reboot();
    } else {
        stop_mcu_update();
        start_mcu_update();
    }

    return h_true;
}

static h_bool mcu_update_msg_exit(void) {
    uint8_t *type = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
    if (!type) {
        return h_false;
    }
    *type = MSG_EXIT;
    return mcu_update_msg(type);
}

static void state_idle_process(uint8_t *buffer) {
    if (!buffer) {
        hcn_log_error("state_idle_process pointer is null!\n");
        return;
    }

    uint8_t msg_type = *buffer;
    switch (msg_type) {
        case CONTINUE_SEND_MSG:
            send_update_data_2_mcu();
            break;

        case RESEND_MSG:
            resend_update_data_2_mcu();
            break;

        case SEND_MSG_DATA_FAIL:
            send_end_update_2_mcu();
            break;

        case MSG_EXIT:
            mcu_update_msg_exit();
            break;

        default:
            break;
    }
}

static void mcu_update_thread(void *param) {
    mcu_update.file_info.update_repeat++;
    mcu_update.file_info.remain_size = mcu_update.file_info.file_len;
    mcu_update.file_info.offset_size = 0;
    mcu_update.file_info.file_crc = 0;

    for (int count = 0; count < mcu_update.file_info.file_len; count++) {
        mcu_update.file_info.file_crc +=
            mcu_update.file_info.update_buff[count];
    }

    send_start_update_2_mcu();

    uint8_t *buffer;
    for (;;) {
        if (xQueueReceive(mcu_update_queue, &buffer, portMAX_DELAY) != pdPASS) {
            hcn_log_error("mcu_update_thread xQueueReceive error.\n");
            continue;
        }

        state_idle_process(buffer);
        vPortFree(buffer);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static h_bool start_mcu_update(void) {
    mcu_update_queue = xQueueCreate(MCU_UPDATE_QUEUE_LEN, sizeof(uint32_t));
    if (!mcu_update_queue) {
        hcn_log_error("Create mcu_update_queue error!\n");
        return h_false;
    }

    if (xTaskCreate(mcu_update_thread, "mcu_update_thread",
                    configMINIMAL_STACK_SIZE * 10, NULL,
                    configMAX_PRIORITIES / 3, &mcu_update_task) != pdPASS) {
        vQueueDelete(mcu_update_queue);
        mcu_update_queue = NULL;
        hcn_log_error("Create mcu_update_thread task fail!\n");
        return h_false;
    }

    return h_true;
}

static h_bool stop_mcu_update(void) {
    if (mcu_update_queue) {
        vQueueDelete(mcu_update_queue);
        mcu_update_queue = NULL;
    }

    if (mcu_update_task) {
        vTaskDelete(mcu_update_task);
        mcu_update_task = NULL;
    }

    return h_true;
}

static h_bool mcu_update_md5_crc(void) {
    int file_len_tmp = mcu_update.file_info.file_len;
    if (file_len_tmp <= MCU_MD5_CRC_LEN) {
        return h_false;
    }

    uint8_t out_digest[MCU_MD5_CRC_LEN] = {0};

    memset(&g_md5_ctx, 0, sizeof(g_md5_ctx));
    md5_starts(&g_md5_ctx);
    md5_update(&g_md5_ctx, mcu_update.file_info.update_buff,
               file_len_tmp - MCU_MD5_CRC_LEN);
    md5_finish(&g_md5_ctx, out_digest);
    hcn_hex_config_data_print(
        __FUNCTION__, ",src:(0x) ",
        &mcu_update.file_info.update_buff[file_len_tmp - MCU_MD5_CRC_LEN],
        MCU_MD5_CRC_LEN);
    hcn_hex_config_data_print(__FUNCTION__, ",md5:(0x) ", out_digest,
                             MCU_MD5_CRC_LEN);

    for (uint8_t index = 0; index < MCU_MD5_CRC_LEN; index++) {
        if (mcu_update.file_info.update_buff[file_len_tmp - MCU_MD5_CRC_LEN +
                                             index] != out_digest[index]) {
            hcn_log_error("Hcn mcu update file md5 crc error\r\n");
            return h_false;
        }
    }

    hcn_log_info("Hcn mcu update file md5 success\r\n");

    return h_true;
}

static h_bool is_mcu_ver_same(char *mcu_ver) {
    if (!mcu_ver) {
        return h_true;
    }

    int remain_size = mcu_update.file_info.file_len - strlen(mcu_ver);
    if (remain_size > 0) {
        for (int i = 0; i < mcu_update.file_info.file_len; i++) {
            if (mcu_update.file_info.update_buff[i] == 'M' &&
                mcu_update.file_info.update_buff[i + 1] == 'C' &&
                mcu_update.file_info.update_buff[i + 2] == 'U' &&
                mcu_update.file_info.update_buff[i + 3] == '-') {
                char mcu_ver_str[MCU_VER_MAX_LEN] = {0};
                memcpy(mcu_ver_str,
                       (void *)&mcu_update.file_info.update_buff[i],
                       strlen(mcu_ver));
                if (strcmp(mcu_ver_str, mcu_ver) == 0) {
                    hcn_log_info("Hcn mcu is same=> mcu_ver_str:%s , mcu_ver:%s\r\n", mcu_ver_str, mcu_ver);
                    return h_true;
                }
                break;
            }
        }
    } else {
        return h_true;
    }

    return h_false;
}

void mcu_updatet_init(uint8_t method, FF_FILE *mcu_file) {
    if (!mcu_file) {
        hcn_log_error("Hcn mcu update file not open!\n");
        return;
    }

    if (mcu_update.file_info.update_type == 0) {
        mcu_update.file_info.update_type = method;
        if (mcu_update.file_info.update_type == USB_UPDATE_MCU) {
            mcu_update.file_info.file_len = ff_filelength(mcu_file);
            mcu_update.file_info.update_buff =
                (uint8_t *)pvPortMalloc(mcu_update.file_info.file_len);
            if (mcu_update.file_info.update_buff) {
                int read_total_len = 0;

                while (1) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                    int len = ff_fread((void *)&mcu_update.file_info
                                           .update_buff[read_total_len],
                                       1, 1024, mcu_file);
                    if (len > 0) {
                        read_total_len += len;
                    } else {
                        ff_fclose(mcu_file);
                        printf("Hb read mcu update file finish\r\n");
                        goto md5_crc;
                    }
                }
            }
        } else if (mcu_update.file_info.update_type == OTA_UPDATE_MCU) {
            ///< ota mcu update in here
        }
    md5_crc:
        if (mcu_update_md5_crc() && !is_mcu_ver_same(hcn_mcu_full_ver) &&
            ((mcu_update.file_info.update_type == USB_UPDATE_MCU) ||
             (mcu_update.file_info.update_type == OTA_UPDATE_MCU))) {
            ///< start mcu update 
            if (!mcu_update.file_info.save_data_buff) {
                mcu_update.file_info.save_data_buff =
                    (uint8_t *)pvPortMalloc(MCU_UPDATE_MSG_MAX_LEN);
                if (mcu_update.file_info.save_data_buff) {
                    start_mcu_update();
                }
            }
        } else {
            if (mcu_update.file_info.update_buff) {
                vPortFree(mcu_update.file_info.update_buff);
                mcu_update.file_info.update_buff = NULL;
            }

            if (mcu_update.file_info.save_data_buff) {
                vPortFree(mcu_update.file_info.save_data_buff);
                mcu_update.file_info.save_data_buff = NULL;
            }

            mcu_update.file_info.file_len = 0;
            mcu_update.file_info.update_type = 0;                          
        }                                          
    }
}

uint8_t get_mcu_update_type(void) {
    return mcu_update.file_info.update_type;
}

#endif
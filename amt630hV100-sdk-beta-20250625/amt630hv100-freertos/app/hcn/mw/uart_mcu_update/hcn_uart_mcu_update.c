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
#include <string.h>
#include "task.h"
#include "uart_mcu_update/hcn_uart_mcu_update.h"
#include "uart_communicate/hcn_uart_common.h"
#include "uart_communicate/hcn_uart_parse_cmd.h"
#include "uart_communicate/hcn_uart_tx.h"
#include "storage_param1/hcn_read_nor_flash.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"
#include "app/md5.h"
#include "wdt.h"
#include "config/hcn_config.h"
#include "ota_manage/hcn_ota.h"
#include "storage_param1/hcn_usr_param.h"
#include "dashboard_state/hcn_dev_state.h"
#include "sfud.h"

#ifdef MCU_UPDATE_SIMULATE_ENABLE
#include "uart_mcu_update/mcu_update_simulate.h"
#endif

#ifdef HCN_MCU_PROTOCOL_UPDATE_ENABLE

#define MCU_UPDATE_PACKAGE_LEN (0x40)
#define MCU_UPDATE_FAILED_RETRY (5)
#define MCU_MD5_CRC_LEN (16)
#define MCU_HEAD_INFO_LEN (0x30)

static mcu_update_t mcu_update;
static QueueHandle_t mcu_update_queue = NULL;
static TaskHandle_t mcu_update_task = NULL;
static md5_context g_md5_ctx;
static char hcn_mcu_full_ver[MCU_VER_MAX_LEN] = {"MCU-DC001-GD_24.11.13V0"};

static bool is_recv_mcu_req = false;

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

static h_bool send_start_iap_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg) {
        return h_false;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = (MSG_CMD_REQ_MCU_IAP >> 8) & 0xFF;
    msg->buffer[5] = (MSG_CMD_REQ_MCU_IAP & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x00;
    msg->buffer[8] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[9] = UART_MCU_MSG_TAIL;

#ifndef MCU_UPDATE_SIMULATE_ENABLE
    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }
#else
    if (send_mcu_simulate_msg(msg) == 0) {
        return h_true;
    }
#endif

    return h_false;
}

static h_bool send_soc_ready_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg || !mcu_update.status.start_mcu_iap) {
        return h_false;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = (MSG_CMD_SEND_SOC_READY >> 8) & 0xFF;
    msg->buffer[5] = (MSG_CMD_SEND_SOC_READY & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x00;
    msg->buffer[8] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[9] = UART_MCU_MSG_TAIL;

#ifndef MCU_UPDATE_SIMULATE_ENABLE
    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }
#else
    if (send_mcu_simulate_msg(msg) == 0) {
        return h_true;
    }
#endif

    return h_false;
}

static h_bool send_exit_update_2_mcu(void) {
    meter_info_t *meter_info = get_hcn_info();
    uint32_t mcu_len = 0;

    if (meter_info->mcu_update == 2) {
        meter_info->mcu_update = 0;
        set_hcn_usr_param(HCN_PARAM_MCU_UPDATE_LEN, (void *)&mcu_len);
    } else {
        meter_info->mcu_update = 0;
        if (get_hcn_usr_param(HCN_PARAM_MCU_UPDATE_LEN, &mcu_len)) {
            if (mcu_len > 0) {
                mcu_len = 0;
                set_hcn_usr_param(HCN_PARAM_MCU_UPDATE_LEN, (void *)&mcu_len);
            }
        }
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
    msg->buffer[4] = ((MSG_CMD_EXIT_MCU_UPDATE >> 8) & 0xFF);
    msg->buffer[5] = (MSG_CMD_EXIT_MCU_UPDATE & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x00;
    msg->buffer[8] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[9] = UART_MCU_MSG_TAIL;

    mcu_update.status.start_mcu_update = h_false;
    mcu_update.status.start_mcu_iap = false;
    mcu_update.status.mcu_earse_ready = false;
    mcu_update.status.mcu_req_start_mcu_update = false;
    mcu_update.status.enter_mcu_task = false;
    
#ifndef MCU_UPDATE_SIMULATE_ENABLE
    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }
#else
    if (send_mcu_simulate_msg(msg) == 0) {
        return h_true;
    }
#endif

    return h_false;
}

static h_bool send_file_checksum_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg || !mcu_update.status.mcu_earse_ready) {
        return h_false;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = ((MSG_CMD_SEND_CRC_2_MCU >> 8) & 0xFF);
    msg->buffer[5] = (MSG_CMD_SEND_CRC_2_MCU & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x06;

    msg->buffer[8] = (uint8_t)(mcu_update.file_info.file_len & 0xFF);
    msg->buffer[9] = (uint8_t)((mcu_update.file_info.file_len >> 8) & 0xFF);
    msg->buffer[10] = (uint8_t)((mcu_update.file_info.file_len >> 16) & 0xFF);
    msg->buffer[11] = (uint8_t)((mcu_update.file_info.file_len >> 24) & 0xFF);

#if 0
    if (mcu_update.file_info.update_repeat == 1) {
        msg->buffer[12] = 0;
        msg->buffer[13] = 0;
    } else if (mcu_update.file_info.update_repeat > 1) {
        msg->buffer[12] = (uint8_t)(mcu_update.file_info.file_crc & 0xFF);
        msg->buffer[13] = (uint8_t)((mcu_update.file_info.file_crc >> 8) & 0xFF);
    }
#endif

#if 1
    msg->buffer[12] = (uint8_t)(mcu_update.file_info.file_crc & 0xFF);
    msg->buffer[13] = (uint8_t)((mcu_update.file_info.file_crc >> 8) & 0xFF);
#endif

    msg->buffer[14] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[15] = UART_MCU_MSG_TAIL;

#ifndef MCU_UPDATE_SIMULATE_ENABLE
    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }
#else
    if (send_mcu_simulate_msg(msg) == 0) {
        return h_true;
    }
#endif
    return h_false;
}

static h_bool send_update_data_2_mcu(void) {
    if (mcu_update.file_info.file_len > 0 &&
        mcu_update.file_info.remain_size == 0) {
        if (!send_file_checksum_2_mcu()) {
            hcn_log_error("Send file checksum failed!\r\n");
            return false;
        }
        return h_true;
    }

    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg || !mcu_update.status.mcu_earse_ready) {
        return h_false;
    }

    uint32_t send_addr = (uint32_t)mcu_update.file_info.offset_size;
    uint16_t pack_len = 0;

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = ((MSG_CMD_SEND_DATA_2_MCU >> 8) & 0xFF);
    msg->buffer[5] = (MSG_CMD_SEND_DATA_2_MCU & 0xFF);

    ///< 先将数据长度清零
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x00;

    memset(msg->buffer + 8, 0x00, MCU_UPDATE_PACKAGE_LEN);
    msg->buffer[8] = (uint8_t)(send_addr & 0xFF);
    msg->buffer[9] = (uint8_t)((send_addr >> 8) & 0xFF);
    msg->buffer[10] = (uint8_t)((send_addr >> 16) & 0xFF);
    msg->buffer[11] = (uint8_t)((send_addr >> 24) & 0xFF);
    msg->buffer[12] = MCU_UPDATE_PACKAGE_LEN; ///< 数据格式是64字节

    int send_len, offset_tmp;
    send_len = (mcu_update.file_info.remain_size < MCU_UPDATE_PACKAGE_LEN)
                   ? mcu_update.file_info.remain_size
                   : MCU_UPDATE_PACKAGE_LEN;
    offset_tmp = mcu_update.file_info.offset_size;
    memcpy(msg->buffer + 13, &mcu_update.file_info.update_buff[offset_tmp],
           send_len);

    ///< 计算实际数据长度 数据内容 + 1数据长度格式 + 4字节地址 
    pack_len = send_len + 4 + 1;
    msg->buffer[6] = (uint8_t)((pack_len >> 8) & 0xFF);
    msg->buffer[7] = (uint8_t)(pack_len & 0xFF);

    msg->buffer[13 + send_len] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[14 + send_len] = UART_MCU_MSG_TAIL;
    memcpy(mcu_update.file_info.save_data_buff, msg->buffer,
            send_len + MCU_UPDATE_MSG_MIN_LEN);
#ifndef MCU_UPDATE_SIMULATE_ENABLE
    if (send_mcu_msg(msg) == 0) {
        mcu_update.file_info.last_write_len = send_len;
        return h_true;
    }
#else
    if (send_mcu_simulate_msg(msg) == 0) {
        mcu_update.file_info.last_write_len = send_len;
        return h_true;
    }
#endif
    mcu_update.file_info.last_write_len = 0;

    return h_false;
}

#if  0
static h_bool resend_update_data_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg || !mcu_update.status.mcu_earse_ready) {
        return h_false;
    }

    uint16_t data_len = (mcu_update.file_info.save_data_buff[6] << 8) +
                        mcu_update.file_info.save_data_buff[7] + 5;
    memcpy(msg->buffer, mcu_update.file_info.save_data_buff,
           data_len + MCU_UPDATE_MSG_MIN_LEN);
    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }

    return h_false;
}
#endif

static h_bool send_restart_iap_2_mcu(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg) {
        return h_false;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = (MSG_CMD_RESTART_MCU_IAP >> 8) & 0xFF;
    msg->buffer[5] = (MSG_CMD_RESTART_MCU_IAP & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x00;
    msg->buffer[8] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[9] = UART_MCU_MSG_TAIL;

    if (send_mcu_msg(msg) == 0) {
        return h_true;
    }

    return h_false;
}

static h_bool mcu_ack_ready_msg(void) {
    uint8_t *type = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
    if (!type) {
        return h_false;
    }
    *type = SOC_READY;
    return mcu_update_msg(type);
}

static h_bool start_send_mcu_data(void) {
    uint8_t *type = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
    if (!type) {
        return h_false;
    }

    *type = SEND_FILE;
    return mcu_update_msg(type);
}

static h_bool mcu_update_msg_resend(void) {
    uint8_t *type = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
    if (!type) {
        return h_false;
    }
    *type = RESEND_FILE;
    return mcu_update_msg(type);
}

static h_bool send_mcu_exit_update(void) {
    uint8_t *type = (uint8_t *)pvPortMalloc(sizeof(uint8_t));
    if (!type) {
        return h_false;
    }
    *type = EXIT_UPDATE;
    return mcu_update_msg(type);
}

static h_bool mcu_update_msg_data_fail(void) {
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
        mcu_update.status.mcu_earse_ready = false;
        mcu_update.status.start_mcu_iap = false;
        mcu_update.file_info.last_write_len = 0;
        mcu_update.status.mcu_req_start_mcu_update = false;
        mcu_update.status.enter_mcu_task = false;

        stop_mcu_update();
        start_mcu_update();
    }

    return h_true;
}

void parse_mcu_update_msg(uint8_t ack_code) {
    switch (ack_code) {
        case ACK_UPDATE_DATA_SUCCESS:
            start_send_mcu_data();
            break;

        case ACK_UPDATE_DATA_FAILED: 
            ///< 升级失败后，不进行重发数据包，而是重新开始
            send_update_status(HCN_MSG_MCU_UPDATE_STATUS, \
                            mcu_update.file_info.file_len, \
							mcu_update.file_info.offset_size, UPDATE_ERROR_FLASH);
            vTaskDelay(pdMS_TO_TICKS(200));
            mcu_update_msg_data_fail();
            break;

        case ACK_MCU_IAP_IS_READY:
             is_recv_mcu_req = true;       
            if (mcu_update.status.start_mcu_update 
                && !mcu_update.status.mcu_req_start_mcu_update) {
                 mcu_ack_ready_msg();
            } else if (mcu_update.status.start_mcu_update 
                && mcu_update.status.mcu_req_start_mcu_update) {
                if (mcu_update.status.enter_mcu_task) {
                    ///< 升级失败后，mcu发送ready准备，断电重新上电发送请求升级mcu文件
                    vTaskDelay(pdMS_TO_TICKS(100));
                    mcu_ack_ready_msg();
                    hcn_log_info("Mcu req update, start...\r\n");
                }
            }
            break;

        case ACK_MCU_IAP_CRC_OK:
            send_mcu_exit_update();
            break;

        case ACK_MCU_IAP_CRC_FAILED:
            send_update_status(HCN_MSG_MCU_UPDATE_STATUS, \
                            mcu_update.file_info.file_len, \
							mcu_update.file_info.offset_size, UPDATE_ERROR_CRC);
            vTaskDelay(pdMS_TO_TICKS(200));
            mcu_update_msg_data_fail();
            break;

        default:
            break;
    }
}

static void state_idle_process(uint8_t *buffer) {
    if (!buffer) {
        hcn_log_error("state_idle_process pointer is null!\n");
        return;
    }

    uint8_t msg_type = *buffer;
    switch (msg_type) {
        case SOC_READY: {
                if (!mcu_update.status.start_mcu_iap) {
                    mcu_update.status.start_mcu_iap = true;
                    if (!send_soc_ready_2_mcu()) {
                        hcn_log_error("Send soc ready to mcu failed!\r\\n");
                        send_update_status(HCN_MSG_MCU_UPDATE_STATUS, \
                            mcu_update.file_info.file_len, \
							mcu_update.file_info.offset_size, UPDATE_ERROR_FLASH);
                    }
                }
            }
            break;

        case SEND_FILE: {
                if (mcu_update.status.start_mcu_iap 
                    && !mcu_update.status.mcu_earse_ready) {
                      mcu_update.status.mcu_earse_ready = true;
                      ///< 接收到MCU擦除成功,发送的第一包
                    send_update_data_2_mcu();
                } else if (mcu_update.status.start_mcu_iap && 
                    mcu_update.status.mcu_earse_ready) {
                    ///< 继续发送数据
                    if (mcu_update.file_info.last_write_len > 0 
                        && mcu_update.file_info.last_write_len \
                        <= MCU_UPDATE_PACKAGE_LEN) {
                        mcu_update.file_info.offset_size += \
                        mcu_update.file_info.last_write_len;
                        mcu_update.file_info.remain_size -= \
                        mcu_update.file_info.last_write_len;
                        send_update_status(HCN_MSG_MCU_UPDATE_STATUS, \
                            mcu_update.file_info.file_len, \
							mcu_update.file_info.offset_size, UPDATE_ERROR_NONE);
                        send_update_data_2_mcu();
                    }
                }
            }
            break;

        case RESEND_FILE:
            mcu_update_msg_resend();
            break;

        case EXIT_UPDATE:
            send_exit_update_2_mcu();
            break;

        default:
            break;
    }
}

static void mcu_update_thread(void *param) {
    mcu_update.file_info.update_repeat++;
    mcu_update.file_info.remain_size = mcu_update.file_info.file_len;
    mcu_update.file_info.offset_size = 0;

    hcn_log_info("Mcu update enter thread!\r\n");
    mcu_update.status.start_mcu_update = true;

    if (!mcu_update.status.mcu_req_start_mcu_update) {
        if (mcu_update.file_info.update_repeat == 1) {
            send_start_iap_2_mcu();
        } else if (mcu_update.file_info.update_repeat > 1) {
            ///< 重试从resart iap开始
            hcn_log_info("Repeat mcu update start.....\r\n");
            send_restart_iap_2_mcu();
        }
    } else {
        if (mcu_update.file_info.update_repeat > 1) {
            ///< 重试从resart iap开始
            hcn_log_info("Mcu req, Repeat mcu update start.....\r\n");
            send_restart_iap_2_mcu();
        }
    }
   
    if (mcu_update.status.mcu_req_start_mcu_update) {
        if (!mcu_update.status.enter_mcu_task) {
            mcu_update.status.enter_mcu_task = true;
        }
    }

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

#ifdef MCU_UPDATE_SIMULATE_ENABLE
    mcu_simulate_init();
#endif

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
    printf("Hcn mcu update file len:%d\r\n", file_len_tmp);
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
                hcn_log_info("Hcn mcu file update ver:%s\r\n", mcu_ver_str);
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

static bool is_force_mcu_update() {
	FF_FILE *fp = ff_fopen("/usb/test_hv100_mcu.img", "rb");
	if (fp){
		FF_Close(fp);
		return true;
	} else {
		return false;
	}
}

static bool read_mcu_file_header(mcu_update_t *update, FF_FILE *file, 
                    uint8_t read_mode, sfud_flash *sflash) {
    if (!update || (!file && read_mode == MCU_FILE_DATA) 
        || !update->file_info.update_buff 
        || (read_mode == MCU_FLASH_DATA && !sflash)) {
        hcn_log_info("Read mcu update file param is null!\r\n");
        return false;
    }

#if 1

    if (read_mode == MCU_FILE_DATA) {
        int ret = ff_fread(update->file_info.update_buff, 1, MCU_HEAD_INFO_LEN, file);
        if (ret < 0) {
            hcn_log_error("Read mcu update first byte error!\r\n");
            ff_fclose(file);
            return false;
        }
    }  else if (read_mode == MCU_FLASH_DATA) {
        uint32_t size = FLASH_PRIV_TYPE_BYTE;
        if (sfud_read(sflash, MCU_OTA_FILE_OFFSET, size, 
            (void *)update->file_info.update_buff) != SFUD_SUCCESS) {
            hcn_log_error("Read mcu update first byte from flash error!\r\n");   
            return false; 
        }
    }
    
    update->file_info.file_crc = ((update->file_info.update_buff[0] << 8) + \
                                    update->file_info.update_buff[1]);
    ///< offset默认为0x30                
    printf("compare mcu update header..\r\n");
    ///< 判断字符串是否是mcu_update.bin
    if (strncmp((const char *)&update->file_info.update_buff[2], "mcu_update.bin", \
            strlen("mcu_update.bin")) != 0) {
        hcn_log_info("Mcu update file is invalid!\r\n");

        if (read_mode == MCU_FILE_DATA) {
            ff_fclose(file);
        }
        send_update_status(HCN_MSG_MCU_UPDATE_STATUS, \
                update->file_info.file_len, \
                update->file_info.offset_size, UPDATE_ERROR_FILE_TYPE);
        return false;
    }

#ifdef MCU_UPDATE_SIMULATE_ENABLE
        send_update_status(HCN_MSG_MCU_UPDATE_STATUS, \
                update->file_info.file_len, \
                update->file_info.offset_size, UPDATE_ERROR_NONE);
#endif

#endif

    return true;
}

void mcu_req_update_init(uint8_t method, FF_FILE *mcu_file) {
    if ((method == 1) && !mcu_file) {
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
                uint8_t head_info_offset = MCU_HEAD_INFO_LEN;

                if (!read_mcu_file_header(&mcu_update, mcu_file, 
                                        MCU_FILE_DATA, NULL)) {
                    if (mcu_update.file_info.update_buff) {
                        vPortFree(mcu_update.file_info.update_buff);
                        mcu_update.file_info.update_buff = NULL;
                    }
                    hcn_log_info("Mcu update file is invalid!\r\n");
                    return;
                }

                read_total_len += head_info_offset;

                while (1) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                    int len = ff_fread((void *)&mcu_update.file_info
                                           .update_buff[read_total_len],
                                       1, 1024, mcu_file);
                    if (len > 0) {
                        read_total_len += len;
                    } else {
                        ff_fclose(mcu_file);
                        if (read_total_len ==  mcu_update.file_info.file_len) {
                             hcn_log_info("Hcn read mcu update file finish\r\n");
                            goto md5_crc;
                        } else {
                            hcn_log_error("Read mcu update file length failed!\r\n");
                            return;
                        }
                    }
                }
            }
        } else if (mcu_update.file_info.update_type == OTA_UPDATE_MCU) {
            uint32_t ota_mcu_size = 0;
            uint32_t temp_data = 0;

            if (!get_hcn_usr_param(HCN_PARAM_MCU_UPDATE_LEN, 
                                &ota_mcu_size)) {
                hcn_log_error("Get ota mcu image size failed!\r\n");
                return;
            }

            if (ota_mcu_size == 0) {
                hcn_log_error("Flash mcu code error, can not update!\r\n");
                return;
            }

            ///< 头部0x30 + 尾部16字节md5
            temp_data = 0x30 + 16; 
            if (ota_mcu_size > temp_data &&  ota_mcu_size < MCU_OTA_FILE_SIZE) {
                mcu_update.file_info.file_len = ota_mcu_size;
                mcu_update.file_info.update_buff =
                    (uint8_t *)pvPortMalloc(mcu_update.file_info.file_len);
                if (mcu_update.file_info.update_buff) {
                    sfud_flash *sflash = sfud_get_device(0);
                    if (!sflash) {
                        hcn_log_error("Get sfud flash device failed!\r\n");
                        vPortFree(mcu_update.file_info.update_buff);
                        mcu_update.file_info.update_buff = NULL;
                        return;
                    }

                    uint32_t read_total_len = 0;
                    uint32_t head_info_offset = FLASH_PRIV_TYPE_BYTE;
                    if (!read_mcu_file_header(&mcu_update, NULL, 
                                        MCU_FLASH_DATA, sflash)) {
                        hcn_log_error("Ota read header error!\r\n");
                        if (mcu_update.file_info.update_buff) {
                            vPortFree(mcu_update.file_info.update_buff);
                            mcu_update.file_info.update_buff = NULL;
                        }
                        return;
                    }

                    read_total_len += head_info_offset;

                    uint32_t remain_size = 
                        mcu_update.file_info.file_len - head_info_offset;
                    uint32_t read_size = FLASH_PRIV_TYPE_BYTE;
                    uint32_t offset = head_info_offset;

                    while (remain_size > 0) {
                        vTaskDelay(1);
                        if (remain_size < FLASH_PRIV_TYPE_BYTE) {
                            read_size = remain_size;
                        }

                        if (sfud_read(sflash, MCU_OTA_FILE_OFFSET + offset, 
                            read_size, 
                            (void *)&mcu_update.file_info.update_buff[offset]) 
                            != SFUD_SUCCESS) {
                            hcn_log_error("Ota read mcu update file from flash error!\r\n");   
                            if (mcu_update.file_info.update_buff) {
                                vPortFree(mcu_update.file_info.update_buff);
                                mcu_update.file_info.update_buff = NULL;
                            }
                            return; 
                        }

                        offset += read_size;
                        read_total_len += read_size;
                        remain_size -= read_size;
                    }

                    if (read_total_len == mcu_update.file_info.file_len) {
                        hcn_log_info("Hcn ota read mcu update file finish\r\n");
                        goto md5_crc;
                    } else {
                        hcn_log_error("Ota read mcu update file length failed!\r\n");
                        if (mcu_update.file_info.update_buff) {
                            vPortFree(mcu_update.file_info.update_buff);
                            mcu_update.file_info.update_buff = NULL;
                        }
                        return;
                    }
                }
            }
        }
    md5_crc:
        if (mcu_update_md5_crc() && 
            ((mcu_update.file_info.update_type == USB_UPDATE_MCU) || 
            (mcu_update.file_info.update_type == OTA_UPDATE_MCU))) {
            ///< start mcu update 
            if (!mcu_update.file_info.save_data_buff) {
                mcu_update.file_info.save_data_buff =
                    (uint8_t *)pvPortMalloc(MCU_UPDATE_MSG_MAX_LEN);
                if (mcu_update.file_info.save_data_buff) { 
                    if (!mcu_update.status.start_mcu_update) {
                        ///< 移动buffer位置,去除头部信息0x30
                        size_t new_size = mcu_update.file_info.file_len - \
                                            MCU_HEAD_INFO_LEN;
                        memmove(mcu_update.file_info.update_buff, 
                            mcu_update.file_info.update_buff + MCU_HEAD_INFO_LEN, 
                            new_size);
                        mcu_update.file_info.file_len -= MCU_HEAD_INFO_LEN;

                        ///< 再省略尾部16字节MD5校验
                        mcu_update.file_info.file_len -= 16;
                        mcu_update.status.mcu_req_start_mcu_update = true;
                        start_mcu_update();
                        hcn_log_info("Mcu req update real start mcu update!\r\n");
                    } else {
                        hcn_log_info("Please reboot mcu...\r\n");
                    }
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

void mcu_update_init(uint8_t method, FF_FILE *mcu_file) {
    if ((method == 1) && !mcu_file) {
        hcn_log_error("Hcn mcu update file not open!\n");
        return;
    }

    if (get_mcu_version()) {
        snprintf(hcn_mcu_full_ver, MCU_VER_MAX_LEN, "%s", get_mcu_version());
    }

    if (mcu_update.file_info.update_type == 0) {
        mcu_update.file_info.update_type = method;
        if (mcu_update.file_info.update_type == USB_UPDATE_MCU) {
            mcu_update.file_info.file_len = ff_filelength(mcu_file);
            mcu_update.file_info.update_buff =
                (uint8_t *)pvPortMalloc(mcu_update.file_info.file_len);
            if (mcu_update.file_info.update_buff) {
                int read_total_len = 0;
                uint8_t head_info_offset = MCU_HEAD_INFO_LEN;
                if (!read_mcu_file_header(&mcu_update, mcu_file, 
                                        MCU_FILE_DATA, NULL)) {
                    hcn_log_error("Read header error!\r\n");
                    if (mcu_update.file_info.update_buff) {
                        vPortFree(mcu_update.file_info.update_buff);
                        mcu_update.file_info.update_buff = NULL;
                    }
                    return;
                }
                read_total_len += head_info_offset;
                
                while (1) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                    int len = ff_fread((void *)&mcu_update.file_info
                                           .update_buff[read_total_len],
                                       1, 1024, mcu_file);
                    if (len > 0) {
                        read_total_len += len;
                    } else {
                        ff_fclose(mcu_file);
                        if (read_total_len ==  mcu_update.file_info.file_len) {
                             hcn_log_info("Hcn read mcu update file finish\r\n");
                            goto md5_crc;
                        } else {
                            hcn_log_error("Read mcu update file length failed!\r\n");
                            return;
                        }
                    }
                }
            }
        } else if (mcu_update.file_info.update_type == OTA_UPDATE_MCU) {
            ///< ota mcu update in here
            uint32_t ota_mcu_size = 0;
            uint32_t temp_data = 0;

            if (!get_hcn_usr_param(HCN_PARAM_MCU_UPDATE_LEN, 
                                &ota_mcu_size)) {
                hcn_log_error("Get ota mcu image size failed!\r\n");
                return;
            }

            if (ota_mcu_size == 0) {
                hcn_log_error("Flash mcu code error, can not update!\r\n");
                return;
            }

            ///< 头部0x30 + 尾部16字节md5
            temp_data = 0x30 + 16; 
            if (ota_mcu_size > temp_data &&  ota_mcu_size < MCU_OTA_FILE_SIZE) {
                mcu_update.file_info.file_len = ota_mcu_size;
                mcu_update.file_info.update_buff =
                    (uint8_t *)pvPortMalloc(mcu_update.file_info.file_len);
                if (mcu_update.file_info.update_buff) {
                    sfud_flash *sflash = sfud_get_device(0);
                    if (!sflash) {
                        hcn_log_error("Get sfud flash device failed!\r\n");
                        vPortFree(mcu_update.file_info.update_buff);
                        mcu_update.file_info.update_buff = NULL;
                        return;
                    }

                    uint32_t read_total_len = 0;
                    uint32_t head_info_offset = FLASH_PRIV_TYPE_BYTE;
                    if (!read_mcu_file_header(&mcu_update, NULL, 
                                        MCU_FLASH_DATA, sflash)) {
                        hcn_log_error("Ota read header error!\r\n");
                        if (mcu_update.file_info.update_buff) {
                            vPortFree(mcu_update.file_info.update_buff);
                            mcu_update.file_info.update_buff = NULL;
                        }
                        return;
                    }

                    read_total_len += head_info_offset;

                    uint32_t remain_size = 
                        mcu_update.file_info.file_len - head_info_offset;
                    uint32_t read_size = FLASH_PRIV_TYPE_BYTE;
                    uint32_t offset = head_info_offset;

                    while (remain_size > 0) {
                        vTaskDelay(1);
                        if (remain_size < FLASH_PRIV_TYPE_BYTE) {
                            read_size = remain_size;
                        }

                        if (sfud_read(sflash, MCU_OTA_FILE_OFFSET + offset, 
                            read_size, 
                            (void *)&mcu_update.file_info.update_buff[offset]) 
                            != SFUD_SUCCESS) {
                            hcn_log_error("Ota read mcu update file from flash error!\r\n");   
                            if (mcu_update.file_info.update_buff) {
                                vPortFree(mcu_update.file_info.update_buff);
                                mcu_update.file_info.update_buff = NULL;
                            }
                            return; 
                        }

                        offset += read_size;
                        read_total_len += read_size;
                        remain_size -= read_size;
                    }

                    if (read_total_len == mcu_update.file_info.file_len) {
                        hcn_log_info("Hcn ota read mcu update file finish\r\n");
                        goto md5_crc;
                    } else {
                        hcn_log_error("Ota read mcu update file length failed!\r\n");
                        if (mcu_update.file_info.update_buff) {
                            vPortFree(mcu_update.file_info.update_buff);
                            mcu_update.file_info.update_buff = NULL;
                        }
                        return;
                    }
                }
            }
        }
    md5_crc:
        if (mcu_update_md5_crc() && ((!is_mcu_ver_same(hcn_mcu_full_ver) || 
            is_force_mcu_update()) &&
            (mcu_update.file_info.update_type == USB_UPDATE_MCU)) ||
             (mcu_update.file_info.update_type == OTA_UPDATE_MCU)) {
            ///< start mcu update 
            if (!mcu_update.file_info.save_data_buff) {
                mcu_update.file_info.save_data_buff =
                    (uint8_t *)pvPortMalloc(MCU_UPDATE_MSG_MAX_LEN);
                if (mcu_update.file_info.save_data_buff) { 
                    if (!mcu_update.status.start_mcu_update) {
                        ///< 移动buffer位置,去除头部信息0x30
                        size_t new_size = mcu_update.file_info.file_len - \
                                            MCU_HEAD_INFO_LEN;
                        memmove(mcu_update.file_info.update_buff, 
                            mcu_update.file_info.update_buff + MCU_HEAD_INFO_LEN, 
                            new_size);
                        mcu_update.file_info.file_len -= MCU_HEAD_INFO_LEN;

                        ///< 再省略尾部16字节MD5校验
                        mcu_update.file_info.file_len -= 16;
                        start_mcu_update();
                        hcn_log_info("Mcu real start mcu update!\r\n");
                    } else {
                        hcn_log_info("Please reboot mcu...\r\n");
                    }
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

static bool ota_mcu_first = true;
void ota_mcu_process(void) {
#if 0
     if (hcn_get_usb_status() != USB_STATUS_INSERTED) {
        hcn_log_info("please remove usb!\r\n");
        return;
     }
#endif

     if (get_check_self_state() == CHECK_SELF_STATE_SUCCESS) {
        meter_info_t *meter_info = get_hcn_info();
        if (meter_info->mcu_update == 2 && ota_mcu_first && !is_recv_mcu_req) {
            ota_mcu_first = false;
            mcu_update_init(OTA_UPDATE_MCU, NULL);
        } else if (meter_info->mcu_update == 2 && ota_mcu_first && is_recv_mcu_req) {
            ota_mcu_first = false;
            hcn_log_info("Flash update, recv mcu req....\r\n");
            mcu_req_update_init(2, NULL);
        }
     }
}

#endif

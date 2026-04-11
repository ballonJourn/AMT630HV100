/**
*
* @file mcu_update_simulate.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/30 15:33
* @author och
*
*/

#include <stdbool.h>
#include <string.h>
#include <FreeRTOS.h>
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "uart_mcu_update/mcu_update_simulate.h"
#include "uart_mcu_update/hcn_mcu_update_def.h"
#include "uart_mcu_update/hcn_uart_mcu_update.h"
#include "uart_communicate/hcn_uart_tx.h"
#include "msg_manage/hcn_msg_manage.h"
#include "log/hcn_log.h"
#include "ff_stdio.h"
#include "utils/hcn_utils.h"

#ifdef MCU_UPDATE_SIMULATE_ENABLE

#define MCU_SIMULATE_DEBUG_ENABLE

static QueueHandle_t recv_queue = NULL;
static QueueHandle_t send_queue = NULL;
static FF_FILE * mcu_update_file = NULL;
static int write_total_len = 0;
static int send_mcu_ack_simulate_msg(hcn_mcu_msg_t *msg);

static int send_mcu_simulate_state(uint8_t ack_code) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg) {
        return -1;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = (MSG_CMD_MCU_ACK_STATE >> 8) & 0xFF;
    msg->buffer[5] = (MSG_CMD_MCU_ACK_STATE & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x05;
    msg->buffer[8] = ack_code;
    msg->buffer[9] = 0x00;
    msg->buffer[10] = 0x00;
    msg->buffer[11] = 0x00;
    msg->buffer[12] = 0x00;
    msg->buffer[13] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[14] = UART_MCU_MSG_TAIL;

    if (send_mcu_ack_simulate_msg(msg) == 0) {
        return 0;
    }

    return -1;
}

static int send_mcu_ack_write_state(uint8_t ack_code, uint32_t write_addr) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (!msg) {
        return -1;
    }

    msg->buffer[2] = 0x00;
    msg->buffer[3] = 0x01;
    msg->buffer[4] = (MSG_CMD_MCU_ACK_STATE >> 8) & 0xFF;
    msg->buffer[5] = (MSG_CMD_MCU_ACK_STATE & 0xFF);
    msg->buffer[6] = 0x00;
    msg->buffer[7] = 0x05;
    msg->buffer[8] = ack_code;
    msg->buffer[9] = (uint8_t)(write_addr & 0xFF);
    msg->buffer[10] = (uint8_t)((write_addr >> 8) & 0xFF);
    msg->buffer[11] = (uint8_t)((write_addr >> 16) & 0xFF);
    msg->buffer[12] = (uint8_t)((write_addr >> 24) & 0xFF);
    msg->buffer[13] = uart_mcu_calc_crc(msg->buffer);
    msg->buffer[14] = UART_MCU_MSG_TAIL;

    if (send_mcu_ack_simulate_msg(msg) == 0) {
        return 0;
    }

    return -1;
}

static void write_mcu_data_in_file(uint8_t *msg) {
    if (msg) {
        if (hcn_get_usb_status() != USB_STATUS_INSERTED) {
            hcn_log_error("usb not inserted, cannot write mcu update file!\r\n");
            return;
        }

        if (mcu_update_file == NULL) {
    
            mcu_update_file = ff_fopen("/usb/mcu_test_update.bin", "w+");
            if (mcu_update_file == NULL) {
                hcn_log_error("open mcu_test_update.bin file failed!\n");
                return;
            }
        }

        if (mcu_update_file) {
            int write_len = (msg[6] << 8) + msg[7] - 5;
            uint32_t write_addr = msg[8] + (msg[9] << 8) + (msg[10] << 16) + \
                                    (msg[11] << 24);
            size_t ret = ff_fwrite(&msg[13], 1, write_len, 
                                    mcu_update_file);
            if (ret != write_len) {
                hcn_log_error("write mcu test file faile!\r\n");
                ff_fclose(mcu_update_file);
                send_mcu_ack_write_state(ACK_UPDATE_DATA_FAILED, write_addr);
                return;
            } 
            write_total_len += write_len;
            hcn_log_info("Recv mcu update addr:%d\r\n", write_addr);
            send_mcu_ack_write_state(ACK_UPDATE_DATA_SUCCESS, write_addr);
        }
    }
}

static void mcu_simalute_crc_process(uint8_t *msg) {
    if (msg) {
        uint32_t file_len = 0;
        uint16_t file_crc = 0;

        file_len = (msg[8]) + (msg[9] << 8) + (msg[10] << 16) + (msg[11] << 24);
        file_crc = (msg[12] + (msg[13] << 8));

        if (file_len == write_total_len) {
            if (mcu_update_file) {
                ff_fclose(mcu_update_file);
                hcn_log_info("Recv mcu simualte data success!\r\n");
            }    
        }
        hcn_log_info("file_len = %d\r\n, file_crc = %d\r\n", file_len, file_crc);
        send_mcu_simulate_state(ACK_MCU_IAP_CRC_OK);
    }
}

static void mcu_simulate_exit_process(uint8_t *msg) {
    hcn_log_info("Mcu simulate update is ok, dev will reboot...\r\n");
}

static void mcu_simulate_recv_parse(uint8_t *msg) {
    if (msg) {
        uint16_t pack_len = ((msg[6] << 8) + msg[7]) + UART_MCU_MSG_MIN_LEN;
        uint8_t crc = msg[pack_len - 2];
        uint8_t tail = msg[pack_len - 1];

        if ((uart_mcu_calc_crc(msg) != crc) || (tail != UART_MCU_MSG_TAIL)) {
            hcn_log_error("Recv error msg, crc or tail failed!\r\n");
            return;
        } 

#ifdef MCU_SIMULATE_DEBUG_ENABLE
        uint16_t data_len_tmp = ((msg[6] << 8) + msg[7]);
        hcn_hex_config_data_print(__FUNCTION__, ":Recv(0x)", msg,
                                data_len_tmp + UART_MCU_MSG_MIN_LEN);
#endif
        uint16_t cmd_code = ((msg[4] << 8) + msg[5]);
        switch (cmd_code) {
            case MSG_CMD_REQ_MCU_IAP:
                send_mcu_simulate_state(ACK_MCU_IAP_IS_READY);
                break;

            case MSG_CMD_SEND_SOC_READY:
                send_mcu_simulate_state(ACK_UPDATE_DATA_SUCCESS);
                break;

            case MSG_CMD_SEND_DATA_2_MCU:
                write_mcu_data_in_file(msg);
                break;

            case MSG_CMD_SEND_CRC_2_MCU:
                mcu_simalute_crc_process(msg);
                break;

            case MSG_CMD_RESTART_MCU_IAP:
                hcn_log_info("Resend mcu into iap!\r\n");
                break;

            case MSG_CMD_EXIT_MCU_UPDATE:
                mcu_simulate_exit_process(msg);
                break;

            default:
                break;
        }
    }
}

int send_mcu_simulate_msg(hcn_mcu_msg_t *msg) {
    if (recv_queue == NULL ||
        xQueueSend(recv_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        vPortFree(msg); 
        msg = NULL;
        hcn_log_error("Send mcu msg error!\n");
        return -1;
    }

    return 0;
}

static int send_mcu_ack_simulate_msg(hcn_mcu_msg_t *msg) {
    if (send_queue == NULL ||
        xQueueSend(send_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        vPortFree(msg); 
        msg = NULL;
        hcn_log_error("Send mcu ack msg error!\n");
        return -1;
    }

    return 0;
}

static void send_simulate_send_parse(uint8_t *msg) {
    if (msg) {
        uint16_t pack_len = ((msg[6] << 8) + msg[7]) + UART_MCU_MSG_MIN_LEN;
        uint8_t crc = msg[pack_len - 2];
        uint8_t tail = msg[pack_len - 1];

        if ((uart_mcu_calc_crc(msg) != crc) || (tail != UART_MCU_MSG_TAIL)) {
            hcn_log_error("Recv error msg, crc or tail failed!\r\n");
            return;
        } 
#ifdef MCU_SIMULATE_DEBUG_ENABLE
        uint16_t data_len_tmp = ((msg[6] << 8) + msg[7]);
        hcn_hex_config_data_print(__FUNCTION__, ":Send(0x)", msg,
                                data_len_tmp + UART_MCU_MSG_MIN_LEN);
#endif
        uint8_t ack_code = msg[8];
        parse_mcu_update_msg(ack_code);
    }
}

static void mcu_simulate_recv_thread(void *param) {
    uint8_t *data;
    for (;;) {
        if (xQueueReceive(recv_queue, &data, portMAX_DELAY) != pdPASS) {
            hcn_log_error("xQueueRecv mcu simulate recv error.\n");
            continue;
        }

        mcu_simulate_recv_parse(data);
        vPortFree(data);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void mcu_simulate_send_thread(void *param) {
    uint8_t *data;
    for (;;) {
        if (xQueueReceive(send_queue, &data, portMAX_DELAY) != pdPASS) {
            hcn_log_error("xQueueRecv mcu simulate send error.\n");
            continue;
        } 

        send_simulate_send_parse(data);
        vPortFree(data);
        vTaskDelay(pdMS_TO_TICKS(1));
    }   
}

int mcu_simulate_init(void) {
    if (recv_queue || send_queue) {
        hcn_log_info("Mcu Simulate already init!\r\n");
        return 0;
    }

    recv_queue = xQueueCreate(20, sizeof(uint32_t));
    if (recv_queue == NULL) {
        hcn_log_error("create recv_queue error.\n");
        return -1;
    }

    send_queue = xQueueCreate(20, sizeof(uint32_t));
    if (send_queue == NULL) {
        hcn_log_error("create send_queue error.\n");
        return -1;
    }
    
    if (xTaskCreate(mcu_simulate_recv_thread, "mcu_simulate_recv_thread",
                    configMINIMAL_STACK_SIZE * 3, NULL, 6,
                    NULL) != pdPASS) {
        hcn_log_error("mcu_simulate_recv_thread failed!\n");
        return -1;
    }

    if (xTaskCreate(mcu_simulate_send_thread, "mcu_simulate_send_thread",
                    configMINIMAL_STACK_SIZE * 3, NULL, 7,
                    NULL) != pdPASS) {
        hcn_log_error("mcu_simulate_send_thread failed!\n");
        return -1;
    }

    hcn_log_info("Hcn mcu simulate init success!\r\n");

    return 0;
}

#endif

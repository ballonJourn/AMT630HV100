/**
*
* @file hcn_uart_common.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/29 09:36
* @author och
*
*/

#include <FreeRTOS.h>
#include "uart_communicate/hcn_uart_common.h"
#include "uart_communicate/hcn_uart_parse_cmd.h"
#include "uart_communicate/hcn_uart_tx.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#include "log/hcn_log.h"
#include "utils/hcn_utils.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#ifdef HCN_UART_COMM_ENABLE

#undef MCU_DATE_TIME_DEBUG
#undef UART_MCU_DATA_DEBUG

static void soc_ack_mcu_process(uint8_t *data) {
    if (!data) {
        hcn_log_error("recv null pointer!\n");
        return;
    }
}

static void timing_info_process(uint8_t *data) {
    if (!data) {
        hcn_log_error("recv null pointer!\n");
        return;
    }

    uint16_t cmd_code = ((data[4] << 8) + data[5]);
    switch (cmd_code) {
        case UART_MCU_CMD_REPORT_TIME_INFO:
            break;

        case UART_MCU_CMD_ACK_TIME_SET:
            break;

        default:
            break;
    }
}

static void mcu_update_process(uint8_t *data) {
    if (!data) {
        hcn_log_error("recv null pointer!\n");
        return;
    }

    uint16_t cmd_code = ((data[4] << 8) + data[5]);
    //uint8_t type = hcn_get_mcu_update_type();
    //if (type) {
        switch (cmd_code) {
            case UART_CONTINUE_UPDATE_MCU_CMD:
                break;

            case UART_RESEND_MCU_MSG_CMD:
                break;

            case UART_UPDATE_MCU_AGAIN_CMD:
                break;

            default:
                break;
        }
    //}
}

static void mcu_msg_type_divide(uint8_t *data) {
    if (!data) {
        hcn_log_error("recv null pointer!\n");
        return;
    }

    uint16_t cmd_type = ((data[UART_MCU_CMD_L_ADDR] << 8) + 
                        data[UART_MCU_CMD_H_ADDR]);
    if (cmd_type <= UART_MCU_NORMAL_END_CMD) {
        uart_mcu_parse_add_task(data);
    } else if (cmd_type >= UART_MCU_TIMING_START_CMD &&
               cmd_type <= UART_MCU_TIMING_END_CMD) {
        timing_info_process(data);
    } else if (cmd_type >= UART_MCU_UPDATE_STAT_CMD &&
               cmd_type <= UART_MCU_UPDATE_END_CMD) {
        mcu_update_process(data);
    } else if (cmd_type >= UART_MCU_SOC_REQ_START_CMD &&
               cmd_type <= UART_MCU_SOC_REQ_END_CMD) {
        soc_ack_mcu_process(data);
    }
}

uint8_t uart_mcu_calc_crc(uint8_t *buffer) {
    int i = 0;
    uint8_t calc_crc = 0;
    uint16_t data_len = ((buffer[6] << 8) + buffer[7]);

    if (buffer) {
        for (i = 0; i <= data_len + UART_MCU_MSG_MIN_LEN - 3; i++) {
            calc_crc ^= buffer[i];
        }
    }

    return calc_crc;
}

static void uart_mcu_rx_thread(void *param) {
    UartPort_t *uap = xUartOpen(HCN_UART_MCU_PORT);
    if (!uap) {
        hcn_log_error("open uart %d fail.\n", HCN_UART_MCU_PORT);
        vTaskDelete(NULL);
        return;
    }

    vUartInit(uap, HCN_UART_MCU_BAUDRATE, 0);

    if (uart_mcu_tx_init(uap) != 0) {
        hcn_log_error("Create uart mcu tx thead failed!\n");
        vUartClose(uap);
        vTaskDelete(NULL);
        return;
    }

    uint8_t uart_rx[UART_MCU_MSG_MAX_LEN];
    int uart_rx_pos = 0;
    int need_min_len = 0;
    int read_len = 0;

    ///< Send mcu heart beat
    vTaskDelay(pdMS_TO_TICKS(100));
    send_mcu_heartbeat();

    extern void start_handshake_timer(void);
    start_handshake_timer();

    need_min_len = UART_MCU_MSG_MIN_LEN;

    for (;;) {
        read_len = iUartRead(uap, uart_rx + uart_rx_pos, need_min_len,
                             pdMS_TO_TICKS(200));
        uart_rx_pos += read_len;
        if (uart_rx_pos < UART_MCU_MSG_MIN_LEN) {
            need_min_len -= read_len;
            continue;
        }

        if ((uart_rx[0] != UART_MCU_MSG_HEAD_1) &&
            (uart_rx[1] != UART_MCU_MSG_HEAD_2)) {
            memmove(uart_rx, uart_rx + 1, UART_MCU_MSG_MIN_LEN - 1);
            uart_rx_pos -= 1;
            need_min_len = 1;
            hcn_log_error("Uart mcu recv head error!\n");
            continue;
        }

        uint16_t data_len = ((uart_rx[6] << 8) + uart_rx[7]);
        need_min_len = data_len;
        uint8_t count = 0;

        for (;;) {
            count++;
            if (count > 3) {
                break;
            }

            read_len = iUartRead(uap, uart_rx + uart_rx_pos, need_min_len,
                                 pdMS_TO_TICKS(200));
            uart_rx_pos += read_len;
            if (uart_rx_pos < UART_MCU_MSG_MIN_LEN + data_len) {
                need_min_len -= read_len;
                continue;
            }
            break;
        }

        if (count > 3) {
            uart_rx_pos = 0;
            need_min_len = UART_MCU_MSG_MIN_LEN;
            continue;
        }

        uint8_t crc = uart_rx[uart_rx_pos - 2];
        uint8_t tail = uart_rx[uart_rx_pos - 1];
        uint16_t cmd = ((uart_rx[4] << 8) + uart_rx[5]);

        if ((uart_mcu_calc_crc(uart_rx) == crc) && tail == UART_MCU_MSG_TAIL) {
            if (cmd != UART_MCU_TIMING_START_CMD) {
#ifdef UART_MCU_DATA_DEBUG
                uint16_t data_len_tmp = ((uart_rx[6] << 8) + uart_rx[7]);
                hcn_hex_config_data_print(__FUNCTION__, ":Recv(0x)", uart_rx,
                                     data_len_tmp + UART_MCU_MSG_MIN_LEN);
#endif
            }
            mcu_msg_type_divide(uart_rx);
        } else {
            hcn_log_error("Cmd:0x%04x, Verify crc error!\n", cmd);
        }

        uart_rx_pos = 0;
        need_min_len = UART_MCU_MSG_MIN_LEN;
    }
}

int uart_mcu_init(void) {

    ///< 串口命令解析任务初始化
    uart_mcu_parse_task_init();

    if (xTaskCreate(uart_mcu_rx_thread, "uart_rx_thead",
                    configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES / 3,
                    NULL) != pdPASS) {
        hcn_log_error("Create uart mcu rx thread failed!\n");
        return -1;
    }

    return 0;
}

#endif
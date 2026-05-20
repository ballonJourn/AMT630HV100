/**
*
* @file hcn_uart_tx.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/29 14:38
* @author och
*
*/

#include <FreeRTOS.h>
#include "queue.h"
#include "board.h"
#include "uart_communicate/hcn_uart_tx.h"
#include "log/hcn_log.h"
#include "utils/hcn_utils.h"

#ifdef HCN_UART_COMM_ENABLE

static QueueHandle_t queue_uart_mcu = NULL;

void uart_mcu_free_msg(hcn_mcu_msg_t *msg) { 
    vPortFree(msg); 
}

int send_mcu_msg(hcn_mcu_msg_t *msg) {
    if (queue_uart_mcu == NULL ||
        xQueueSend(queue_uart_mcu, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        uart_mcu_free_msg(msg);
        hcn_log_error("Send mcu msg error!\n");
        return -1;
    }

    return 0;
}

hcn_mcu_msg_t *uart_alloc_msg(void) {
    hcn_mcu_msg_t *msg = pvPortMalloc(sizeof(hcn_mcu_msg_t));
    if (!msg) {
        hcn_log_error("pvPortMalloc hcn mcu msg error.\n");
        return NULL;
    }

    msg->buffer[0] = UART_MCU_MSG_HEAD_1;
    msg->buffer[1] = UART_MCU_MSG_HEAD_2;

    return msg;
}

static void uart_mcu_tx_thread(void *param) {
    UartPort_t *uap = param;
    hcn_mcu_msg_t *msg;
    int rtn;

    if (!uap) {
        hcn_log_error("Uart %d is not open\n", HCN_UART_MCU_PORT);
        vQueueDelete(queue_uart_mcu);
        vTaskDelete(NULL);
    }

    for (;;) {
        if (xQueueReceive(queue_uart_mcu, &msg, portMAX_DELAY) != pdPASS) {
            hcn_log_error("xQueueReceive send msg task error.\n");
            continue;
        }

        uint16_t data_len = ((msg->buffer[6] << 8) + msg->buffer[7]);
        uint16_t cmd =  ((msg->buffer[4] << 8) + msg->buffer[5]);
        
        if ((cmd != UART_MCU_CMD_SET_ODO_DATA) 
            && (cmd != UART_MCU_CMD_SET_TRIP_A_DATA)
            && (cmd != UART_MCU_CMD_SET_TRIP_B_DATA)) {
            hcn_hex_config_data_print("Soc", ":send(0x)", msg->buffer,
                                data_len + UART_MCU_MSG_MIN_LEN);   
        }

        rtn = iUartWrite(uap, msg->buffer, data_len + UART_MCU_MSG_MIN_LEN,
                         pdMS_TO_TICKS(100));
        uart_mcu_free_msg(msg);  // must release, otherwise memory leak.
        if (rtn != data_len + UART_MCU_MSG_MIN_LEN) {
            hcn_log_error("Soc send mcu msg error.\n");
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

int uart_mcu_tx_init(UartPort_t *uap) {
    if (!uap) {
        hcn_log_error("Uart is not open!\n");
        return -1;
    }

    queue_uart_mcu = xQueueCreate(UART_MCU_QUEUE_LEN, sizeof(uint32_t));
    if (queue_uart_mcu == NULL) {
        hcn_log_error("Create queue_uart_mcu failed!\n");
        return -1;
    }

    if (xTaskCreate(uart_mcu_tx_thread, "mcu_tx_thread",
                    configMINIMAL_STACK_SIZE, uap, configMAX_PRIORITIES / 3,
                    NULL) != pdPASS) {
        vQueueDelete(queue_uart_mcu);
        queue_uart_mcu = NULL;
        return -1;
    }

    return 0;
}

#endif
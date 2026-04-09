/**
*
* @file hcn_uart_dvr_tx.c
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
#include "uart_dvr/hcn_uart_dvr_tx.h"
#include "log/hcn_log.h"
#include "utils/hcn_utils.h"

#ifdef HCN_UART_DVR_ENABLE

static QueueHandle_t queue_uart_dvr = NULL;

void uart_dvr_free_msg(hcn_dvr_msg_t *msg) { 
    vPortFree(msg); 
}

int send_dvr_msg(hcn_dvr_msg_t *msg) {
    if (queue_uart_dvr == NULL ||
        xQueueSend(queue_uart_dvr, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        uart_dvr_free_msg(msg);
        hcn_log_error("Send dvr msg error!\n");
        return -1;
    }

    return 0;
}

hcn_dvr_msg_t *uart_dvr_alloc_msg(void) {
    hcn_dvr_msg_t *msg = pvPortMalloc(sizeof(hcn_dvr_msg_t));
    if (!msg) {
        hcn_log_error("pvPortMalloc hcn dvr msg error.\n");
        return NULL;
    }

    msg->buffer[0] = UART_DVR_MSG_HEAD;

    return msg;
}

static void uart_dvr_tx_thread(void *param) {
    UartPort_t *uap = param;
    hcn_dvr_msg_t *msg;
    int rtn;

    if (!uap) {
        hcn_log_error("Uart %d is not open\n", HCN_UART_DVR_PORT);
        vQueueDelete(queue_uart_dvr);
        vTaskDelete(NULL);
    }

    for (;;) {
        if (xQueueReceive(queue_uart_dvr, &msg, portMAX_DELAY) != pdPASS) {
            hcn_log_error("xQueueReceive send msg task error.\n");
            continue;
        }
        
        hcn_hex_config_data_print("Soc", ":send(0x)", msg->buffer,
                            UART_DVR_MSG_MIN_LEN);   

        rtn = iUartWrite(uap, msg->buffer, UART_DVR_MSG_MIN_LEN,
                         pdMS_TO_TICKS(100));
        uart_dvr_free_msg(msg);  // must release, otherwise memory leak.
        if (rtn != UART_DVR_MSG_MIN_LEN) {
            hcn_log_error("Soc send dvr msg error.\n");
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

int uart_dvr_tx_init(UartPort_t *uap) {
    if (!uap) {
        hcn_log_error("Uart is not open!\n");
        return -1;
    }

    queue_uart_dvr = xQueueCreate(UART_DVR_QUEUE_LEN, sizeof(uint32_t));
    if (queue_uart_dvr == NULL) {
        hcn_log_error("Create queue_uart_dvr failed!\n");
        return -1;
    }

    if (xTaskCreate(uart_dvr_tx_thread, "dvr_tx_thread",
                    configMINIMAL_STACK_SIZE, uap, configMAX_PRIORITIES / 3,
                    NULL) != pdPASS) {
        vQueueDelete(queue_uart_dvr);
        queue_uart_dvr = NULL;
        return -1;
    }

    return 0;
}

#endif
/**
*
* @file hcn_uart_dvr.c
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
#include "uart_dvr/hcn_uart_dvr.h"
#include "uart_dvr/hcn_uart_dvr_parse_cmd.h"
#include "uart_dvr/hcn_uart_dvr_tx.h"
#include "uart_dvr/hcn_uart_dvr_send_cmd.h"
#include "dashboard_state/hcn_dev_state.h"
#include "log/hcn_log.h"
#include "utils/hcn_utils.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "rtc.h"

#ifdef HCN_UART_DVR_ENABLE

#define UPDATE_DVR_DEBUG

#undef DVR_DATE_TIME_DEBUG
#undef UART_DVR_DATA_DEBUG

static void uart_dvr_rx_thread(void *param) {
    UartPort_t *uap = xUartOpen(HCN_UART_DVR_PORT);
    if (!uap) {
        hcn_log_error("open uart %d fail.\n", HCN_UART_DVR_PORT);
        vTaskDelete(NULL);
        return;
    }

    vUartInit(uap, HCN_UART_DVR_BAUDRATE, 0);

    if (uart_dvr_tx_init(uap) != 0) {
        hcn_log_error("Create uart mcu tx thead failed!\n");
        vUartClose(uap);
        vTaskDelete(NULL);
        return;
    }

    uint8_t uart_rx[UART_DVR_MSG_MAX_LEN];
    int uart_rx_pos = 0;
    int need_min_len = 0;
    int read_len = 0;

    need_min_len = UART_DVR_MSG_MIN_LEN;

    for (;;) {
        read_len = iUartRead(uap, uart_rx + uart_rx_pos, need_min_len,
                             pdMS_TO_TICKS(200));
        uart_rx_pos += read_len;
        if (uart_rx_pos < UART_DVR_MSG_MIN_LEN) {
            need_min_len -= read_len;
            continue;
        }

        if ((uart_rx[0] != UART_DVR_MSG_HEAD)) {
            memmove(uart_rx, uart_rx + 1, UART_DVR_MSG_MIN_LEN - 1);
            uart_rx_pos -= 1;
            need_min_len = 1;
            hcn_log_error("Uart mcu recv head error!\n");
            continue;
        }
        
        uint8_t tail = uart_rx[uart_rx_pos - 1];
        uint16_t cmd = uart_rx[1];

        if (tail == UART_DVR_MSG_TAIL) {
#ifdef UART_DVR_DATA_DEBUG
                hcn_hex_config_data_print(__FUNCTION__, ":Recv(0x)", uart_rx,
                                    UART_DVR_MSG_MIN_LEN);
#endif
            uart_dvr_parse(uart_rx);
        } else {
            hcn_log_error("Cmd:0x%04x, Verify crc error!\n", cmd);
        }

        uart_rx_pos = 0;
        need_min_len = UART_DVR_MSG_MIN_LEN;
    }
}

int uart_dvr_init(void) {

    ///< 串口命令解析任务初始化
    //mcu数据量较大，所以将数据获取和分发分离成两个线程 
    // uart_mcu_parse_task_init(); //创建任务分发进程

    if (xTaskCreate(uart_dvr_rx_thread, "uart_dvr_rx_thead",
                    configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES / 3,
                    NULL) != pdPASS) {
        hcn_log_error("Create uart mcu rx thread failed!\n");
        return -1;
    }

    return 0;
}

#endif
/**
*
* @file hcn_uart_dvr_parse_cmd.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/29 12:23
* @author och
*
*/

#include <stdbool.h>
#include <string.h>
#include <FreeRTOS.h>
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "uart_dvr/hcn_uart_dvr_parse_cmd.h"
#include "uart_dvr/hcn_uart_dvr.h"
#include "uart_dvr/hcn_uart_dvr_send_cmd.h"
#include "vehicle_param/vehicle_param.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"

#ifdef HCN_UART_DVR_ENABLE

static void uart_dvr_parse_msg_process(uint8_t *data) {
    if (!data) {
        return;
    }

    uint16_t cmd = data[1];
    switch (cmd) {
        case UART_DVR_CMD_TFCARD_LOAD_STATE:
        case UART_DVR_CMD_RECORD_STATE:
        case UART_DVR_CMD_MODE_TYPE:
        case UART_DVR_CMD_RECORD_TIME: {
#if 1
            hcn_hex_config_data_print("Soc analysis", ":Recv(0x)", data,
                                    UART_DVR_MSG_MIN_LEN);
#endif
        } break;

        default:
            break;
    }

    switch (cmd) {
        case UART_DVR_CMD_TFCARD_LOAD_STATE:
            hcn_log_info("Recv DVR TFCARD load state %d.\n", data[2]);
            vehicle_set_data(VEH_DVR_LOAD, data[2]);
            break;

        case UART_DVR_CMD_RECORD_STATE:
            hcn_log_info("Recv DVR Recording state %d.\n", data[2]);
            vehicle_set_data(VEH_DVR_RECORDING, data[2]);
            break;

        case UART_DVR_CMD_MODE_TYPE:
            hcn_log_info("Recv DVR mode type%d.\n", data[2]);
            vehicle_set_data(VEH_DVR_MODE, data[2]);
            break;

        case UART_DVR_CMD_RECORD_TIME:
            hcn_log_info("Recv DVR Record Time %d.\n", data[2]);
            vehicle_set_data(VEH_DRV_TIME, data[2]);
            break;
        default:
            break;
    }
}

void uart_dvr_parse(uint8_t *data){
    uart_dvr_parse_msg_process(data);
}
#endif
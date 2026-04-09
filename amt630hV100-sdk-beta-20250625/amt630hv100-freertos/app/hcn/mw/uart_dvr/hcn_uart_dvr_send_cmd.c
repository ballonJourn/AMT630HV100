/**
*
* @file hcn_uart_dvr_send_cmd.c
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

#include "uart_dvr/hcn_uart_dvr.h"
#include "uart_dvr/hcn_uart_dvr_send_cmd.h"
#include "uart_dvr/hcn_uart_dvr_tx.h"
#include "log/hcn_log.h"
#include "utils/hcn_utils.h"

#ifdef HCN_UART_DVR_ENABLE

int DVR_Record(int state) {
    hcn_dvr_msg_t *msg = uart_dvr_alloc_msg();
    if (msg != NULL) {
        msg->buffer[1] = UART_DVR_CMD_CTRL_RECORD;
        msg->buffer[2] = state;
        msg->buffer[3] = UART_DVR_MSG_TAIL;
        return send_dvr_msg(msg);
    }

    return -1;
}

int DVR_change_Mode(int type) {
    hcn_dvr_msg_t *msg = uart_dvr_alloc_msg();
    if (msg != NULL) {
        msg->buffer[1] = UART_DVR_CMD_CTRL_MODE;
        msg->buffer[2] = type;
        msg->buffer[3] = UART_DVR_MSG_TAIL;
        return send_dvr_msg(msg);
    }

    return -1;
}

int DVR_change_Camera(int type) {
    hcn_dvr_msg_t *msg = uart_dvr_alloc_msg();
    if (msg != NULL) {
        msg->buffer[1] = UART_DVR_CMD_CTRL_CAMERA;
        msg->buffer[2] = type;
        msg->buffer[3] = UART_DVR_MSG_TAIL;
        return send_dvr_msg(msg);
    }

    return -1;
}

int DVR_change_Time(int type) {
    hcn_dvr_msg_t *msg = uart_dvr_alloc_msg();
    if (msg != NULL) {
        msg->buffer[1] = UART_DVR_CMD_CTRL_TIME;
        msg->buffer[2] = type;
        msg->buffer[3] = UART_DVR_MSG_TAIL;
        return send_dvr_msg(msg);
    }

    return -1;
}

int DVR_print_screen(void) {
    hcn_dvr_msg_t *msg = uart_dvr_alloc_msg();
    if (msg != NULL) {
        msg->buffer[1] = UART_DVR_CMD_CTRL_PRINTSCREEN;
        msg->buffer[2] = 0x01;
        msg->buffer[3] = UART_DVR_MSG_TAIL;
        return send_dvr_msg(msg);
    }

    return -1;
}

#endif
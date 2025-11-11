/**
*
* @file hcn_uart_send_cmd.c
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

#include "uart_communicate/hcn_uart_common.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#include "uart_communicate/hcn_uart_tx.h"
#include "log/hcn_log.h"
#include "utils/hcn_utils.h"

#ifdef HCN_UART_COMM_ENABLE

int start_record(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_START_RECORD >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_START_RECORD & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x00;
        msg->buffer[8] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[9] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int stop_record(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_STOP_RECORD >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_STOP_RECORD & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x00;
        msg->buffer[8] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[9] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_mcu_heartbeat(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_SHAKE_HANDS >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_SHAKE_HANDS & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x01;
        msg->buffer[8] = 0x00;
        msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[10] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_req_mcu_version(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_REQ_VER >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_REQ_VER & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x01;
        msg->buffer[8] = 0x00;
        msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[10] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_checkself_state(uint8_t state) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_CHECKSELF >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_CHECKSELF & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x01;
        msg->buffer[8] = state;
        msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[10] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_mcu_shut_down(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_SHUTDOWN >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_SHUTDOWN & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x01;
        msg->buffer[8] = 0x00;
        msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[10] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_mcu_ctrl_system_mode(uint8_t abs_mode, uint8_t tcs_mode,
                              uint8_t trip_type) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_CTRL_SYSTEM_MODE >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_CTRL_SYSTEM_MODE & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x03;
        msg->buffer[8] = abs_mode;
        msg->buffer[9] = tcs_mode;
        msg->buffer[10] = trip_type;
        msg->buffer[11] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[12] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_mcu_set_time(SystemTime_t time) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_SET_TIME >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_SET_TIME & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x09;
        msg->buffer[8] = decimal_2_bcd(time.tm_year/100);
        msg->buffer[9] = decimal_2_bcd(time.tm_year%100);
        msg->buffer[10] = decimal_2_bcd(time.tm_mon);
        msg->buffer[11] = decimal_2_bcd(time.tm_mday);
        msg->buffer[12] = decimal_2_bcd(time.tm_hour);
        msg->buffer[13] = decimal_2_bcd(time.tm_min);
        msg->buffer[14] = decimal_2_bcd(time.tm_sec);
        msg->buffer[15] = 0x01; ///< 24小时制度
        msg->buffer[16] = 0;
        msg->buffer[17] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[18] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_mcu_clear_subtotal_mileage(uint8_t type) {
    if (type > 2) {
        hcn_log_error("set invalid type!\n");
        return -1;
    }

    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_CLAER_SUB_MILEAGE >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_CLAER_SUB_MILEAGE & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x01;
        msg->buffer[8] = type;
        msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[10] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_mcu_set_odo(uint32_t odo) {
    if (odo > 0) {
        hcn_mcu_msg_t *msg = uart_alloc_msg();
        if (msg != NULL) {
            msg->buffer[2] = 0x00;
            msg->buffer[3] = 0x01;
            msg->buffer[4] = (UART_MCU_CMD_SET_ODO_DATA >> 8) & 0xff;
            msg->buffer[5] = (UART_MCU_CMD_SET_ODO_DATA & 0xff);
            msg->buffer[6] = 0x00;
            msg->buffer[7] = 0x04;
            msg->buffer[8] = (uint8_t)((odo >> 24) & 0xff);
            msg->buffer[9] = (uint8_t)((odo >> 16) & 0xff);
            msg->buffer[10] = (uint8_t)((odo >> 8) & 0xff);
            msg->buffer[11] = (uint8_t)(odo & 0xff);
            msg->buffer[12] = uart_mcu_calc_crc(msg->buffer);
            msg->buffer[13] = UART_MCU_MSG_TAIL;
            return send_mcu_msg(msg);
        }
    }

    return -1;
}

int send_mcu_request_odo(void) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_REQUEST_ODO_DATA >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_REQUEST_ODO_DATA & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x01;
        msg->buffer[8] = 0x00;
        msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[10] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }

    return -1;
}

int send_mcu_set_trip_a(uint32_t trip_a) {
    if (trip_a > 0) {
        hcn_mcu_msg_t *msg = uart_alloc_msg();
        if (msg != NULL) {
            msg->buffer[2] = 0x00;
            msg->buffer[3] = 0x01;
            msg->buffer[4] = (UART_MCU_CMD_SET_TRIP_A_DATA >> 8) & 0xff;
            msg->buffer[5] = (UART_MCU_CMD_SET_TRIP_A_DATA & 0xff);
            msg->buffer[6] = 0x00;
            msg->buffer[7] = 0x04;
            msg->buffer[8] = (uint8_t)((trip_a >> 24) & 0xff);
            msg->buffer[9] = (uint8_t)((trip_a >> 16) & 0xff);
            msg->buffer[10] = (uint8_t)((trip_a >> 8) & 0xff);
            msg->buffer[11] = (uint8_t)(trip_a & 0xff);
            msg->buffer[12] = uart_mcu_calc_crc(msg->buffer);
            msg->buffer[13] = UART_MCU_MSG_TAIL;
            return send_mcu_msg(msg);
        }
    }
    
    return -1;
}

int send_mcu_set_trip_b(uint32_t trip_b) {
    if (trip_b > 0) {
        hcn_mcu_msg_t *msg = uart_alloc_msg();
        if (msg != NULL) {
            msg->buffer[2] = 0x00;
            msg->buffer[3] = 0x01;
            msg->buffer[4] = (UART_MCU_CMD_SET_TRIP_B_DATA >> 8) & 0xff;
            msg->buffer[5] = (UART_MCU_CMD_SET_TRIP_B_DATA & 0xff);
            msg->buffer[6] = 0x00;
            msg->buffer[7] = 0x04;
            msg->buffer[8] = (uint8_t)((trip_b >> 24) & 0xff);
            msg->buffer[9] = (uint8_t)((trip_b >> 16) & 0xff);
            msg->buffer[10] = (uint8_t)((trip_b >> 8) & 0xff);
            msg->buffer[11] = (uint8_t)(trip_b & 0xff);
            msg->buffer[12] = uart_mcu_calc_crc(msg->buffer);
            msg->buffer[13] = UART_MCU_MSG_TAIL;
            return send_mcu_msg(msg);
        }
    }
    
    return -1;
}

int send_mcu_request_trip(uint8_t type) {
    hcn_mcu_msg_t *msg = uart_alloc_msg();
    if (msg != NULL) {
        msg->buffer[2] = 0x00;
        msg->buffer[3] = 0x01;
        msg->buffer[4] = (UART_MCU_CMD_REQUEST_TRIP_DATA >> 8) & 0xff;
        msg->buffer[5] = (UART_MCU_CMD_REQUEST_TRIP_DATA & 0xff);
        msg->buffer[6] = 0x00;
        msg->buffer[7] = 0x01;
        msg->buffer[8] = type;
        msg->buffer[9] = uart_mcu_calc_crc(msg->buffer);
        msg->buffer[10] = UART_MCU_MSG_TAIL;
        return send_mcu_msg(msg);
    }
    
    return -1;
}

#endif
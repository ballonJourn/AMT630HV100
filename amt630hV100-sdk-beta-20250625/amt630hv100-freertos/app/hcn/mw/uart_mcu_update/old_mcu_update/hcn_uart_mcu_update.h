/**
*
* @file hcn_uart_mcu_update.h
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
#ifndef __HCN_UART_MCU_UPDATE_H__
#define __HCN_UART_MCU_UPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart_mcu_update/hcn_mcu_update_def.h"
#include "config/hcn_config.h"
#include "ff_stdio.h"

#ifdef HCN_UART_MCU_UPDATE_ENABLE

#define MCU_UPDATE_QUEUE_LEN (10)

typedef enum {
    NO_MCU_UPDATE,
    USB_UPDATE_MCU,
    OTA_UPDATE_MCU
} mcu_update_type_e;

typedef enum {
    CONTINUE_SEND_MSG,
    RESEND_MSG,
    SEND_MSG_DATA_FAIL,
    MSG_EXIT
} mcu_update_msg_e;

typedef struct {
    h_bool start_mcu_update;
    h_bool start_send_task;
    h_bool start_send2_mcu;
    h_bool mcu_ver_same;
} mcu_update_status_t;

typedef struct {
    uint8_t *update_buff;
    uint8_t *save_data_buff;
    int file_len;
    int remain_size;
    int offset_size;
    uint16_t file_crc;
    uint8_t update_type;
    uint8_t update_repeat;
} mcu_update_file_t;

typedef struct {
    mcu_update_status_t status;
    mcu_update_file_t file_info;
} mcu_update_t;

h_bool mcu_update_msg_continue(void);
h_bool mcu_update_msg_resend(void);
h_bool mcu_update_msg_data_fail(void);
void mcu_updatet_init(uint8_t method, FF_FILE *mcu_file);
uint8_t get_mcu_update_type(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_MCU_UPDATE_H__
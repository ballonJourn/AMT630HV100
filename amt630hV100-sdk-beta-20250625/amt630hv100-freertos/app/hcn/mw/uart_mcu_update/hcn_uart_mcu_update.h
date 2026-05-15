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
#include "dashboard_state/hcn_dev_state.h"
#include "config/hcn_config.h"
#include "ff_stdio.h"

#ifdef HCN_MCU_PROTOCOL_UPDATE_ENABLE

#define MCU_UPDATE_QUEUE_LEN (10)

typedef enum {
    NO_MCU_UPDATE,
    USB_UPDATE_MCU,
    OTA_UPDATE_MCU
} mcu_update_type_e;
typedef enum {
    SOC_READY,
    SEND_FILE,
    RESEND_FILE,
    EXIT_UPDATE,
} mcu_update_step_e;

typedef enum {
    MCU_FILE_DATA = 1,  ///< U盘或者SD卡是的mcu_update.bin文件
    MCU_FLASH_DATA = 2, ///< OTA升级时，flash中的mcu文件
} mcu_update_data_tyep_e;

typedef struct {
    h_bool start_mcu_update;
    h_bool mcu_ver_same;
    h_bool start_mcu_iap;  ///< MCU进入IAP模式
    h_bool mcu_earse_ready; ///< MCU擦除完成
    h_bool mcu_req_start_mcu_update; ///< MCU端请求时，启动任务状态
    h_bool enter_mcu_task; ///< 真正进入了mcu_task
} mcu_update_status_t;

typedef struct {
    uint8_t *update_buff;
    uint8_t *save_data_buff;
    int file_len;
    int remain_size;
    int offset_size;
    int last_write_len;  ///< 上次文件写入地址，从0x00开始
    uint16_t file_crc;
    uint8_t update_type;
    uint8_t update_repeat;
} mcu_update_file_t;

typedef struct {
    mcu_update_status_t status;
    mcu_update_file_t file_info;
} mcu_update_t;

void parse_mcu_update_msg(uint8_t ack_code);
void mcu_update_init(uint8_t method, FF_FILE *mcu_file);
void mcu_req_update_init(uint8_t method, FF_FILE *mcu_file);
uint8_t get_mcu_update_type(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_MCU_UPDATE_H__
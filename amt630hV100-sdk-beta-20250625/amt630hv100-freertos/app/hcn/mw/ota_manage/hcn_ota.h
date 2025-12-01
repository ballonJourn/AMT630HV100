/**
*
* @file hcn_ota.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/11/18 17:19
* @author och
*
*/
#ifndef __HCN_OTA_H__
#define __HCN_OTA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "msg_manage/hcn_msg_manage.h"

/**
 * @brief 升级类型枚举
 */
typedef enum  {
    UPDATE_NONE = 0,
    UPDATE_USB_SOC,      ///< 通过USB更新SOC(630)
    UPDATE_USB_MCU,      ///< 通过USB更新MCU
    UPDATE_SD_CARD,     ///< 通过SD卡更新
    UPDATE_OTA          ///< 通过OTA更新
} update_type_e;

/**
 * @brief 升级错误枚举
 */
typedef enum {
    UPDATE_ERROR_NONE = 0,  ///< 无错误
    UPDATE_ERROR_CRC,       ///< CRC校验错误
    UPDATE_ERROR_FLASH,     ///< Flash写入错误
    UPDATE_ERROR_FILE_TYPE  ///< 文件类型错误
} update_error_e;

/**
 * @brief 升级状态枚举
 */
typedef enum {
    UPDATE_STATUS_IDLE = 0,        ///< 空闲状态
    UPDATE_STATUS_IN_PROGRESS,     ///< 更新中
    UPDATE_STATUS_SUCCESS,         ///< 更新成功
    UPDATE_STATUS_FAILED,          ///< 更新失败
    UPDATE_STATUS_TIMEOUT          ///< 更新超时
} update_state_e;

/**
 * @brief 升级信息结构体
 */
typedef struct {
    update_type_e type;          ///< 更新类型
    update_state_e status;       ///< 更新状态
    update_error_e error;        ///< 更新错误
    uint8_t progress;           ///< 更新进度，单位：百分比(0-100)
} update_info_t;

typedef struct {
    uint8_t msg_type;    ///< 更新类型，参考msg_type_t
    uint8_t error;      ///< 错误码，参考update_error_e
    uint8_t percent;    ///< 更新进度，单位：百分比(0-100)
    uint8_t reserved; 
} update_msg_info_t;

void send_update_status(uint8_t msg_type, uint32_t total_size, 
                        uint32_t cur_off, uint8_t error);
void parse_update_status(mw_msg_data_t *msg);

update_info_t *get_current_update_info(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_OTA_H__
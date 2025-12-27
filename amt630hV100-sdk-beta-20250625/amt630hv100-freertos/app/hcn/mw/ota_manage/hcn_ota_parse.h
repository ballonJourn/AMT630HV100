/**
*
* @file hcn_ota_parse.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/16 09:22
* @author och
*
*/
#ifndef __HCN_OTA_PARSE_H__
#define __HCN_OTA_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "board.h"

#define OTA_SPILDR_START_ADDR   LOADER_OTA_FILE_OFFSET
#define OTA_STEPLDR_START_ADDR  STEPLDR_OTA_FILE_OFFSET
#define OTA_UPDATE_START_ADDR   UPDATE_OTA_FILE_OFFSET
#define OTA_MCU_START_ADDR      MCU_OTA_FILE_OFFSET

///< TCP OTA数据包长度定义
#define TCP_OTA_DATA_MIN_LEN        (7)
#define TCP_OTA_DATA_MAX_LEN        (1032)
#define TCP_OTA_FILE_NAME_MAX_LEN   (64)

///< TCP OTA协议命令码定义
#define TCP_HEARTBEAT_CMD           (0)
#define TCP_DEVICE_INFO_CMD         (1)
#define TCP_FILE_INFO_CMD           (2)
#define TCP_FILE_STREAM_CMD         (3)
#define TCP_TRANSFER_COMPLETE_CMD   (4)
#define TCP_UPDATE_START_CMD        (5)
#define TCP_CONFIG_CMD              (6)
#define TCP_FLASH_PERCENTAGE        (7)

///< OTA文件类型定义
#define OTA_SPILDR_FILE         (1)
#define OTA_STEPLDR_FILE        (2)
#define OTA_UPDATE_FILE         (3)
#define OTA_MCU_FILE            (4)
#define OTA_CONFIG_FILE         (5)
#define OTA_ALL_FILE            (5)

///< TCP OTA应答码定义
#define TCP_SUCCESS_ACK         (200)
#define TCP_FAILED_ACK          (500)

/**
 * @brief  ota解析状态枚举
 */
typedef enum {
    TCP_SEND_DEVICE_INFO = 0x01,
    TCP_RECV_DEVICE_ACK,
    TCP_RECV_FILE_INFO,
    TCP_SEND_FILE_ACK,
    TCP_RECV_STREAM,
    TCP_SEND_PERCENT,
    TCP_SEND_TRANSFER_COMPLETE,
    TCP_SEND_TRANSFER_COMPLETE_ACK   
} tcp_ota_state;

/**
 * @brief  ota文件结构体定义
 */
typedef struct {
    char file_name[TCP_OTA_FILE_NAME_MAX_LEN];  ///< 文件名称
    uint8_t *ota_file_buff;                     ///< 文件缓存区
    uint32_t ota_file_size;                     ///< 文件大小    
    uint32_t ota_addr_offset;                   ///< 写入spi nor flash的首地址
    uint32_t ota_write_offset;                  ///< 写入flash的偏移地址
} ota_file_t;

typedef struct {
    uint8_t ota_percent;                ///< ota升级百分比
    bool ota_percent_change;            ///< ota百分比变化标志
    bool is_ready_update;              ///< 是否准备好升级标志  
    bool is_first_update_file;         ///< 是否第一个update文件标志
    uint8_t *ota_buff;                 ///< ota缓存
    uint32_t ota_total_size;           ///< ota文件的总大小
    uint32_t ota_write_size;           ///< ota写入flash的大小
    uint32_t ota_cur_rx_size;         ///< 当前接收到数据大小
} ota_param_t;

/**
 * @brief  ota解析初始化函数
 * @param  无
 * @return 0:成功 -1：失败
 */
int ota_parse_init(void);

/**
 * @brief  ota解析复位
 * @param  无
 * @return 无
 */
void ota_parse_reset(void);

/**
 * @brief  ota任务消息添加函数
 * @param  msg ota消息指针
 * @param  len ota消息长度
 * @return 0:成功 -1：失败
 */
int ota_task_add(uint8_t *msg, uint16_t len);

/**
 * @brief  ota升级进度是否改过
 * @param  无
 * @return true:进度已改变  false:进度未改变
 */
bool get_ota_percent_change(void);

/**
 * @brief  设置ota升级状态改变
 * @param  change true:改变  false:未改变
 * @return 无
 */
void set_ota_percent_change(bool change);

/**
 * @brief  获取ota升级进度
 * @param  无
 * @return 0-100 进度状态
 */
uint8_t get_ota_percent(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_OTA_PARSE_H__
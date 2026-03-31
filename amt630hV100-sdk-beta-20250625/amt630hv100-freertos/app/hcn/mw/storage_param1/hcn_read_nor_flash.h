/**
*
* @file hcn_read_nor_flash.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/25 11:47
* @author och
*
*/
#ifndef __HCN_READ_NOR_FLASH_H__
#define __HCN_READ_NOR_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "config/hcn_config.h"
#include "storage_param1/hcn_usr_param.h"

#ifdef HCN_NOR_FLASH_PARAM_ENABLE

#define HCN_USR_PARAM_ADDR              (0x35000)       ///< nor falsh扇区开始地址
#define NOR_FLASH_MAGIC_NUM_PREFIX      (0xdd)          ///< 魔数前缀
#define NOR_FLASH_SECTOR_SIZE           (4096)          ///< 扇区大小
#define NOR_FALSH_MAGIC_NUM             (0xdd112233)    ///< 魔数，不同项目不一样
#define NOR_FLASH_EMPTY_FLAG            (0xffffffff)
#define PARAM_RECORD_HEADER_MAGIC       (0xAA55AA56)    ///< 记录头魔数
#define PARAN_RECORD_HEADER_MAGIC_PREFIX (0xAA)

/**
 * @brief 记录头信息 20字节
 */
typedef struct {
    uint32_t magic;             ///< 记录魔数 PARAM_RECORD_HEADER_MAGIC
    uint32_t write_count;       ///< 总写入计数，用于wear leveling
    uint32_t data_size;         ///< 数据大小
    uint32_t timestamp;         ///< 时间戳（可选，用于调试）
    uint32_t checksum;          ///< 头校验和
} record_header_t;

/**
 * @brief 仪表参数结构体，包含检验和，魔数，升级方式，用户参数等
 */
typedef struct {
    uint16_t check_sum;
    uint16_t mcu_update;  ///< 0:none 1:usb  2:ota		
    uint32_t magic_num;   ///< magic num, Storage Flag
    usr_param_t usr;
} meter_info_t;

///< 计算单个记录的大小（包含头和数据）
#define PARAM_RECORD_SIZE (sizeof(record_header_t) + sizeof(meter_info_t))

///< 计算单个扇区最大记录数量
#define MAX_RECORDS_PER_SECTOR (NOR_FLASH_SECTOR_SIZE / PARAM_RECORD_SIZE)

///< 计算理论最大擦写次数（考虑安全系数）
#define THEORETICAL_MAX_WRITES (MAX_RECORDS_PER_SECTOR * 100000)

/**
 * @brief Flash磨损统计信息
 */
typedef struct {
    uint32_t total_writes;      ///< 总写入次数
    uint32_t sector_erases;     ///< 扇区擦除次数
    uint32_t last_erase_time;   ///< 最后擦除时间
    uint32_t remaining_writes;  ///< 剩余写入次数估算
} flash_wear_info_t;

/**
 * @brief  读取hcn仪表参数
 * @param  none
 * @return 0:成功 -1:失败 
 */
int read_hcn_info(void);

/**
 * @brief  保存hcn仪表参数
 * @param  none
 * @return 0:成功 -1:失败 
 */
int save_hcn_info(void);

/**
 * @brief  获取hcn仪表参数指针
 * @param  none
 * @return hcn仪表参数指针
 */
meter_info_t *get_hcn_info(void);

/**
 * @brief  获取Flash磨损统计信息
 * @param  wear_info 磨损信息结构体指针
 * @return 0:成功 -1:失败
 */
int get_flash_wear_info(flash_wear_info_t *wear_info);

/**
 * @brief  获取剩余可写入次数估算
 * @param  none
 * @return 剩余可写入次数估算
 */
uint32_t get_remaining_writes_estimate(void);

/**
 * @brief  获取Flash使用统计信息（调试用）
 * @param  none
 * @return 无
 */
void print_flash_usage_stats(void);

#endif //HCN_NOR_FLASH_PARAM_ENABLE

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_READ_NOR_FLASH_H__
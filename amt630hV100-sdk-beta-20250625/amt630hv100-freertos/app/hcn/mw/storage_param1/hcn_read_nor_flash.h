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

#include "storage_param1/hcn_usr_param.h"

#define HCN_USR_PARAM_ADDR          (0x35000)  ///< nor falsh扇区开始地址
#define NOR_FLASH_MAGIC_NUM_PREFIX  (0xdd)  ///< 魔数前缀
#define NOR_FALSH_MAGIC_NUM         (0xdd112233) ///< 魔数，不同项目不一样
#define NOR_FLASH_EMPTY_FLAG        (0xffffffff)

/**
 * @brief 仪表参数结构体，包含检验和，魔数，升级方式，用户参数等
 */
typedef struct {
    uint16_t check_sum;
    uint16_t mcu_update;  ///< 0:none 1:usb  2:ota		
    uint32_t magic_num;   ///< magic num, Storage Flag
    usr_param_t usr;
} meter_info_t;

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

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_READ_NOR_FLASH_H__
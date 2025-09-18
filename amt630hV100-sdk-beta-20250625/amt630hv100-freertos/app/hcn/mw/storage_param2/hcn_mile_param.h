/**
*
* @file hcn_mile_param.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/01 12:23
* @author och
*
*/
#ifndef __HCN_MILE_PARAM_H__
#define __HCN_MILE_PARAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define HCN_E2PROM_MAGIC_NUM    (0x44556677)  ///< magic num

/**
 * @brief 用户里程参数结构体
 */
typedef struct {
    uint32_t magic_num;    ///< 魔数，标准数据
    uint32_t odo;          ///< 总里程，单位为m
    uint32_t tripa;        ///< 小计里程A 单位为m
    uint32_t tripb;        ///< 小计里程B 单位为m
    uint32_t ride_time_a;  ///< 骑行时间a: 单位秒
    uint32_t ride_time_b;  ///< 骑行时间b: 单位秒
    uint16_t avg_fuel_con_a;  ///< 平均油耗a 精度0.1 单位L/100km
    uint16_t avg_fuel_con_b;  ///< 平均油耗b 精度0.1 单位L/100km
    uint16_t range_a;         ///< 续航里程a km
    uint16_t range_b;         ///< 续航里程b km
    uint32_t all_fuel_cons;   ///< 累计油耗ml 
    uint32_t avg_spd_time_a;  ///< 小计里程a对应时间s
    uint32_t avg_spd_time_b; ///< 小计里程b对应时间s
    uint32_t erase_counter;  ///< 里程相关信息擦除次数:次
    uint32_t reserve[3];    ///< 预留字节
    uint32_t checksum;      ///< 检验位
} mile_param_t;  ///< 64字节

///< 划分40个页给来存储，640个字节

/**
 * @brief 用户里程参数枚举
 */
typedef enum {
    HCN_MILE_PARAM_ODO = 0x10,
    HCN_MILE_PARAM_TRIP_A,
    HCN_MILE_PARAM_TRIP_B,
    HCN_MILE_PARAM_RIDE_TIME_A,
    HCN_MILE_PARAM_RIDE_TIME_B,
    HCN_MILE_PARAM_AVG_FUEL_CON_A,
    HCN_MILE_PARAM_AVG_FUEL_CON_B,
    HCN_MILE_PARAM_RANGE_A,
    HCN_MILE_PARAM_RANGE_B,
    HCN_MILE_PARAM_ALL_FUEL_CONS,
    HCN_MILE_PARAM_AVG_SPD_TIME_A,
    HCN_MILE_PARAM_AVG_SPD_TIME_B,
} mile_param_handle_e;

bool set_hcn_mile_param(mile_param_handle_e id, void *param);
bool get_hcn_mile_param(mile_param_handle_e id, void *param);
bool get_recovery_mile_param(void);

void mile_param_init(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MILE_PARAM_H__



/**
*
* @file hcn_uart_tpms.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/25 16:48
* @author och
*
*/
#ifndef __HCN_UART_TPMS_H__
#define __HCN_UART_TPMS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MAX_WHEEL_POS_NUM   (4)
#define FRONT_WHEEL_POS     (0)
#define REAR_WHEEL_POS      (1)

typedef enum {
    TPMS_LEFT_FRONT,    ///< 左前轮
    TPMS_RIGHT_FRONT,   ///< 右前轮
	TPMS_LEFT_REAR,     ///< 左后轮
    TPMS_RIGHT_REAR,    ///< 右后轮
    TPMS_MAX_NUM,      ///< 轮胎胎压个数
} tpms_pos_e;

typedef struct {
    uint8_t high_pressure :1;  ///< 高压 0：正常 1:有高压
    uint8_t low_pressure :1;   ///< 低压 0：正常 1:有低压
    uint8_t high_temp :1;     ///< 高温 0：正常 1:有高温
    uint8_t air_leak :1;      ///< 漏气 0:正常  1:漏气
    uint8_t low_voltage:1;   ///< 低电压 0:正常 1:低电压
    uint8_t lost_sensor :1;  ///< 传感器丢失 0:正常 1:丢失
    uint8_t not_pair :1;    ///< 未配对 0:未配对 1:已配对
    uint8_t invalid_data :1; ///< 无效数据 0:无效数据 1:有效数据
} tpms_state_t;

typedef struct {
    uint32_t tpms_id;       ///< 胎压ID
    uint16_t tpms_pressure; ///< 胎压压力
    char tpms_temp;         ///< 胎温 有负值 例如  -10度
    tpms_state_t tpms_state; ///< 胎压状态
} tpms_param_t;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_TPMS_H__
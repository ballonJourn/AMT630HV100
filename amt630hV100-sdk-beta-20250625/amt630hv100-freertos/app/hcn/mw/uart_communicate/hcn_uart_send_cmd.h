/**
*
* @file hcn_uart_send_cmd.h
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
#ifndef __HCN_UART_SEND_CMD_H__
#define __HCN_UART_SEND_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "hcn_uart_common.h"
#include "rtc.h"

#ifdef HCN_UART_COMM_ENABLE

/**
 * @brief  send mcu start record
 * @param  none
 * @return 0:success -1:failed
 */
int start_record(void);

/**
 * @brief  send mcu stop record
 * @param  none
 * @return 0:success -1:failed
 */
int stop_record(void);

/**
 * @brief  send mcu heartbeat
 * @param  none
 * @return 0:success -1:failed
 */
int send_mcu_heartbeat(void);

/**
 * @brief  send mcu req mcu version
 * @param  none
 * @return 0:success -1:failed
 */
int send_req_mcu_version(void);

/**
 * @brief  send checkself state
 * @param  state 0:default 1:start 2:end
 * @return 0:success -1:failed
 */
int send_checkself_state(uint8_t state);

/**
 * @brief  send mcu shutdown
 * @param  none
 * @return 0:success -1:failed
 */
int send_mcu_shut_down(void);

/**
 * @brief  send mcu ctrl system mode
 * @param  abs_mode 0x00:open abs 0x01：close rear-wheel  0x02:close front rear
 * wheel 0x0A:long close rear wheel 0x0B:long close front rear wheel
 * @param  tcs_mode 0x00:open tcs 0x01:close tcs 0x02: mode 1  0x03:mode 2
 * 0x04：other mode 0x05：long close
 * @param  trip_type  0:trip_a 1:trip_b
 * @return 0:success -1:failed
 */
int send_mcu_ctrl_system_mode(uint8_t abs_mode, uint8_t tcs_mode,
                              uint8_t trip_type);

/**
 * @brief  send mcu set time
 * @param  time： set time
 * @return 0:success -1:failed
 */
int send_mcu_set_time(SystemTime_t time);

/**
 * @brief  get mcu time
 * @param  none
 * @return date time
 */
SystemTime_t get_mcu_time(void);

/**
 * @brief  send mcu clear subtotal mileage
 * @param  type 0x00:none 1:Trip A 2:Trip B 3:Trip A and Trip B
 * @return 0:success -1:failed
 */
int send_mcu_clear_subtotal_mileage(uint8_t type);

/**
 * @brief  设置ODO数据
 * @param  odo total mileage data,uint(m)
 * @return 0:success -1:failed
 */
int send_mcu_set_odo(uint32_t odo);

/**
 * @brief  请求ODO数据
 * @param  无
 * @return 0:success -1:failed
 */
int send_mcu_request_odo(void);

/**
 * @brief  设置tripA
 * @param  trip_a sub A mileage data,uint(m)
 * @return 0:success -1:failed
 */
int send_mcu_set_trip_a(uint32_t trip_a);

/**
 * @brief  设置tripB
 * @param  trip_a sub B mileage data,uint(m)
 * @return 0:success -1:failed
 */
int send_mcu_set_trip_b(uint32_t trip_b);

/**
 * @brief  请求trip数据
 * @param  type 0x00:trip A  0x01:tripB
 * @return 0:success -1:failed
 */
int send_mcu_request_trip(uint8_t type);

/**
 * @brief  清除eeprom数据
 * @param  none
 * @return 0:success -1:failed
 */
int send_mcu_clear_eeprom(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_SEND_CMD_H__
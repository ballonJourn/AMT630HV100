/**
*
* @file hcn_uart_dvr_send_cmd.h
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
#include "hcn_uart_dvr.h"
#include "rtc.h"

#ifdef HCN_UART_DVR_ENABLE

/**
 * @brief  send dvr start record
 * @param  state 0:stop recording 1:start recording
 * @return 0:success -1:failed
 */
int DVR_Record(int state);

/**
 * @brief  控制DVR芯片模式
 * @param  type 0:关闭USB 1:UDV Mode 2:MSDC Mode
 * @return 0:success -1:failed
 */
int DVR_change_Mode(int type);

/**
 * @brief  更改前后摄
 * @param  type 0:后摄 1:前摄
 * @return 0:success -1:failed
 */
int DVR_change_Camera(int type);

/**
 * @brief  更改录制时间
 * @param  type 0:1 minute 1: 2 minutes 2: 3 minutes
 * @return 0:success -1:failed
 */
int DVR_change_Time(int type);

/**
 * @brief  截图
 * @param  none
 * @return 0:success -1:failed
 */
int DVR_print_screen(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_SEND_CMD_H__
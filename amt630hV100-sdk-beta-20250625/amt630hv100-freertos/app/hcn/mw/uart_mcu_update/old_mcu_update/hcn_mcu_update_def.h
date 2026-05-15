/**
*
* @file hcn_mcu_update_def.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/31 17:08
* @author och
*
*/

#ifndef __HCN_MCU_UPDATE_DEF_H__
#define __HCN_MCU_UPDATE_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@brief  mcu update msg format
 *  |Msg head 1|  msg head2 | serial num | Cmd type|msg len | msg data | BCC check|Identifier tail|
 *  |  0x24    |  0x24      |  2Byte     |   2Byte |2Byte   | nByte    |   1Byte  |      0x23     |
 */

/**
 *@brief  BCC check
 *  BCC value = Msg head 1 +  msg head2 + serial num +  Cmd type + msg len +  msg data
 */

/* mcu update msg len define */
#define MCU_UPDATE_MSG_MAX_LEN     (260)
#define MCU_UPDATE_MSG_MIN_LEN     (10)

/* mcu to arm req or ack */
#define MSG_CMD_START_UPDATE        (0x8C01)
#define MSG_CMD_STOP_UPDATE         (0x8C02)
#define MSG_CMD_SEND_DATA_2_MCU     (0x8C03)

#define MCU_VER_MAX_LEN              (40)

#define h_true      (1)
#define h_false     (0)
typedef unsigned char h_bool;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MCU_UPDATE_DEF_H__
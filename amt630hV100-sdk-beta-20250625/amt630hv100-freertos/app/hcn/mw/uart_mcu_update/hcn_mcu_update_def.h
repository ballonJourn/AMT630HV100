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
 *@brief  mcu update msg format 具体见《MCU升级说明》
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

///< soc请求或者发送对应状态到mcu
#define MSG_CMD_REQ_MCU_IAP         (0x8C01)   ///< 请求mcu进入IAP
#define MSG_CMD_SEND_SOC_READY      (0x8C03)  ///< 通知mcu, soc端已准备好
#define MSG_CMD_SEND_DATA_2_MCU     (0x8C04)  ///< 发送数据给MCU
#define MSG_CMD_SEND_CRC_2_MCU      (0x8C05) ///< 发送数据检验给MCU
#define MSG_CMD_RESTART_MCU_IAP     (0x8C06) ///< 重新进入IAP状态
#define MSG_CMD_EXIT_MCU_UPDATE     (0x8C0A) ///< 退出MCU升级模式

///< MCU回复指令
#define MSG_CMD_MCU_ACK_STATE       (0x1002) ///< mcu回复对应的状态
 
typedef enum {
    ACK_UPDATE_DATA_SUCCESS = 0x79,
    ACK_UPDATE_DATA_FAILED  = 0x1F,
    ACK_MCU_IAP_IS_READY  = 0x86,
    ACK_MCU_IAP_CRC_OK = 0xB1,
    ACK_MCU_IAP_CRC_FAILED = 0xC2
} ack_state_e;

#define MCU_VER_MAX_LEN              (40)

#define h_true      (1)
#define h_false     (0)
typedef unsigned char h_bool;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MCU_UPDATE_DEF_H__
/**
*
* @file hcn_key_common.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/26 17:27
* @author och
*
*/
#ifndef __HCN_KEY_COMMON_H__
#define __HCN_KEY_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 按键事件枚举
 */
typedef enum {
    NO_KEY_EVENT = 0x0,
    MODE_KEY_SHORT_PR = 0x01,
    MODE_KEY_LONG_PR = 0x02,
    SET_KEY_SHORT_PR = 0x03,
    SET_KEY_LONG_PR = 0x04,
    COM_KEY_SHORT_PR = 0x05,
    COM_KEY_LONG_PR = 0x06,
    UP_KEY_SHORT_PR = 0x07,
    UP_KEY_LONG_PR = 0x08,
    DOWN_KEY_SHORT_PR = 0x09,
    DOWN_KEY_LONG_PR = 0x0A,
    ENTER_KEY_SHORT_PR = 0x10,
    ENTER_KEY_LONG_PR = 0x11,
    BACK_KEY_SHORT_PR = 0x12,
    BACK_KEY_LONG_PR = 0x13,
    COM_KEY_SHORT_PR1 = 0x14,
    COM_KEY_LONG_PR1 = 0x15,
    SET_KEY_SUPER_LONG_PR = 0x16,
} key_event_e;

typedef uint8_t key_event_t_;
typedef void (*key_event_cb_t)(key_event_t_);

/**
 * @brief  设置按键事件回调
 * @param  event_cb 按键事件回调函数
 * @return 0:成功 -1:失败  1:已经初始过
 */
int set_key_event_cb(key_event_cb_t event_cb);

void send_key_event(uint8_t key_event);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_KEY_COMMON_H__
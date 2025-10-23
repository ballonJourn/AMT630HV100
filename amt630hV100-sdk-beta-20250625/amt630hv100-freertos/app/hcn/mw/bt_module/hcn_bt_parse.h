/**
*
* @file hcn_bt_parse.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/26 09:35
* @author och
*
*/
#ifndef __HCN_BT_PARSE_H__
#define __HCN_BT_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define UART_BT_MSG_MAX_LEN (256)

/**
 * @brief  add bt message to bt task queue
 * @param  bt_msg: bt message pointer
 * @param  len: bt message length
 * @return 0:success -1:fail
 */
int bt_msg_task_add(char *bt_msg, uint16_t len);

/**
 * @brief  bt module init
 * @param  none
 * @return 0:success -1:fail
 */
int bt_module_init(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_BT_PARSE_H__
/**
*
* @file hcn_cp_keyevent.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2026/05/15 17:19
* @author och
*
*/
#ifndef __HCN_CP_KEYEVENT_H__
#define __HCN_CP_KEYEVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

void send_keyevent_to_cp(uint8_t keyevent);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CP_KEYEVENT_H__
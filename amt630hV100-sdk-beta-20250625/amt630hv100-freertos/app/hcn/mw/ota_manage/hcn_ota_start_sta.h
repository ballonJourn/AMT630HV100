/**
*
* @file hcn_ota_start_sta.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/11 15:45
* @author och
*
*/
#ifndef __HCN_OTA_START_STA_H__
#define __HCN_OTA_START_STA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

int start_sta_init(void);
void wifi_mode_switching(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_OTA_START_STA_H__
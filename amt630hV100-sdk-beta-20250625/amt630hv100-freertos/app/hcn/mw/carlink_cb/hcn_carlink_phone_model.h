/**
*
* @file hcn_carlink_phone_model.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/20 16:41
* @author och
*
*/
#ifndef __HCN_CARLINK_PHONE_MODEL_H__
#define __HCN_CARLINK_PHONE_MODEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "carlink_cb/hcn_carlink_cb.h"

void parse_phone_model_info(const char *data);
const hcnPhoneInfo* get_phone_model_info(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CARLINK_PHONE_MODEL_H__
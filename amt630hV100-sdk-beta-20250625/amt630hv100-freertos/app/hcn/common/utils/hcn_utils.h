/**
*
* @file hcn_utils.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 17:57
* @author och
*
*/
#ifndef __HCN_UTILS_H__
#define __HCN_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void hcn_hex_config_data_print(char const *function, char *prefix, uint8_t *data,
                              uint8_t length);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UTILS_H__
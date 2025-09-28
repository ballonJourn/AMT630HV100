/**
*
* @file hcn_bt_parse.c
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

#include "bt_module/hcn_bt_parse.h"
#include "log/hcn_log.h"

//#define BT_STR_DEBUG 

void on_bt_str_parse(char *at_str) {
    if (at_str == NULL) {
        return;
    }

#ifdef BT_STR_DEBUG
    hcn_log_info("bt str:%s", at_str);
#endif
}
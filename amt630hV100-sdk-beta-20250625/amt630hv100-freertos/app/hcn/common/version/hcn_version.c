/**
*
* @file hcn_version.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/11/10 17:51
* @author och
*
*/

#include <string.h>
#include "utils/hcn_utils.h"
#include "version/hcn_version.h"
#include "log/hcn_log.h"

static char g_soc_version[64] = {0};

void soc_version_init(void) {
    set_build_date_time();

    snprintf(g_soc_version, sizeof(g_soc_version), "%s_%s_%s_%s%s", \
            HCN_CUSTOMER_NAME, APP_PROJECT_NUM, \
            APP_UI_VERSION, get_build_date_time(), APP_SUB_NUM);
    hcn_log_info("Cur soc version:%s\r\n", g_soc_version);
}

const char *get_soc_version(void) {
    return g_soc_version;
}

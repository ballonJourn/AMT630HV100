/**
*
* @file hcn_carlink_provide.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/16 10:59
* @author och
*
*/

#include <string.h>
#include "carlink_cb/hcn_carlink_provide.h"
#include "log/hcn_log.h"

static IhcnCallBack* mHcnCallback = NULL;
static HcnLibConfig mHcnLibCfg = {0};
static char g_carlink_uuid[64] = {0};

void hcn_initialize(HcnLibConfig* HcnCfg, IhcnCallBack* HcnCallback) {
    if (HcnCfg) {
        memcpy(&mHcnLibCfg, HcnCfg, sizeof(HcnLibConfig));
    }

    if (HcnCallback) {
        mHcnCallback = HcnCallback;
    }

    hcn_log_info("hcn_initialize done.\n");
}

void update_carlink_uuid(const char *bt_mac) {

}

const char *get_carlink_uuid() {
    return g_carlink_uuid;
}

IhcnCallBack *get_hcn_callback(void) {
    return mHcnCallback;
}

HcnLibConfig *get_hcn_lib_config(void) {
    return &mHcnLibCfg;
}


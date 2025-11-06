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
#include "ECTiny.h"
#include "log/hcn_log.h"
#include "carlink_cb/hcn_carlink_provide.h"

static IhcnCallBack* mHcnCallback = NULL;
static HcnLibConfig mHcnLibCfg = {0};
static char g_carlink_uuid[32] = {0};
static char g_carlink_url[256] = {0};

void hcn_initialize(HcnLibConfig* HcnCfg, IhcnCallBack* HcnCallback) {
    if (HcnCfg) {
        memcpy(&mHcnLibCfg, HcnCfg, sizeof(HcnLibConfig));
    }

    if (HcnCallback) {
        mHcnCallback = HcnCallback;
    }

    hcn_log_info("hcn_initialize done.\n");
}

IhcnCallBack *get_hcn_callback(void) {
    return mHcnCallback;
}

HcnLibConfig *get_hcn_lib_config(void) {
    return &mHcnLibCfg;
}

void hcn_update_carlink_uuid(const char *uuid) {
    if (uuid) {
        snprintf(g_carlink_uuid, sizeof(g_carlink_uuid), "%s", uuid);
    }
}    

void hcn_update_carlink_url(const char *url) {
    if (url) {
        snprintf(g_carlink_url, sizeof(g_carlink_url), "%s", url);
    }
}

const char* hcn_ec_get_uuid() {
    return g_carlink_uuid;
}

const char* hcn_ec_get_qr_code_url() {
    return g_carlink_url;
}

const char* hcn_ec_get_version() {
    return EC_getVersion();
}

int32_t hcn_ec_loadNightModeStatus(uint32_t isNightModeOn) {
    return EC_uploadNightModeStatus(isNightModeOn);
}

int32_t hcn_ec_startMirror() {
    return EC_startMirror();
}

void hcn_ec_stopMirror() {
    EC_stopMirror();
}


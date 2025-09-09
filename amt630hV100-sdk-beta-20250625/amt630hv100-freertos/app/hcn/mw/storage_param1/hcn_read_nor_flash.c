/**
*
* @file hcn_read_nor_flash.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/25 11:47
* @author och
*
*/

#include <string.h>
#include "FreeRTOS.h"
#include "board.h"
#include "sfud.h"
#include "source/crc32.h"
#include "storage_param1/hcn_read_nor_flash.h"
#include "storage_param1/hcn_usr_param.h"
#include "log/hcn_log.h"

static meter_info_t meter_info;

int read_hcn_info(void) {
    uint16_t checksum;
    sfud_flash *sflash = sfud_get_device(0);
    sfud_read(sflash, HCN_USR_PARAM_ADDR, sizeof(meter_info_t),
              (void *)&meter_info);

    checksum = xcrc32(((unsigned char *)&meter_info) + 2,
                      sizeof(meter_info) - 2, 0xffffffff);
    if (checksum == meter_info.check_sum) {
        return 0;
    }

    return -1;
}

int save_hcn_info(void) {
    sfud_err ret = SFUD_ERR_NOT_FOUND;
    sfud_flash *sflash = sfud_get_device(0);
    if (sflash) {
        meter_info.check_sum = xcrc32(((unsigned char *)&meter_info) + 2,
                                     sizeof(meter_info) - 2, 0xffffffff);
        ret = sfud_erase_write(sflash, HCN_USR_PARAM_ADDR, sizeof(meter_info),
                               (void *)&meter_info);
    }

    hcn_log_info("Save param ret:%d\n", ret);
    
    return ret;
}

meter_info_t *get_hcn_info(void) {
     return &meter_info; 
}

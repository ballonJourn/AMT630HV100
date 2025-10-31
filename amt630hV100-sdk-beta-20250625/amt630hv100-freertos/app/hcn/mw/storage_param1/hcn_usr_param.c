/**
*
* @file hcn_usr_param.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/25 11:59
* @author och
*
*/

#include <string.h>
#include "FreeRTOS.h"
#include "board.h"
#include "sfud.h"
#include "storage_param1/hcn_usr_param.h"
#include "storage_param1/hcn_read_nor_flash.h"
#include "log/hcn_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "vehicle_param/vehicle_param.h"

#ifdef HCN_NOR_FLASH_PARAM_ENABLE

#define USE_PARAM_PRINTF (1)

static usr_param_t usr_param;
static usr_param_t usr_param_pre = {
    .usr_set = 0xffffffff,
    .carlink_uuid = {0xff},
};

static bool is_recovery_usr_param = false;
static bool start_by_acc = false;

static void set_default_usr_param(usr_param_t * set_param) {
    if (set_param) {
        memset(set_param, 0, sizeof(usr_param_t));
        set_param->maintain_info.maintain_mile.cur_maintain_mileage = 1000;
        set_param->maintain_info.maintain_time.maintain_days = 365;

        set_param->usr_set.mile_format = 0,
        set_param->usr_set.mile_display = 0,
        set_param->usr_set.theme = 0;
        set_param->usr_set.bt_switch = 1;
        set_param->usr_set.brightness = 3;
        set_param->usr_set.language = 0;
        set_param->usr_set.sys_log = 1;
        set_param->usr_set.time_format = 1;
    }
}

#if USE_PARAM_PRINTF
static void printf_usr_param(usr_param_t * param) {
    if (param) {
        printf("\r\n ...............usr param start......................\r\n");
        printf("maintain_count:%d\r\n",param->maintain_info.maintain_mile.maintain_count);
        printf("maintain_mileage:%d\r\n",param->maintain_info.maintain_mile.last_maintain_total_mileage);
        printf("cur_maintain_mileage:%d\r\n",param->maintain_info.maintain_mile.cur_maintain_mileage);

        printf("maintain_days:%d\r\n",param->maintain_info.maintain_time.maintain_days);
        printf("is sync time:%d\r\n",param->maintain_info.maintain_time.is_sync_time);
        printf("maintain data:%04d/%02d/%02d\r\n",param->maintain_info.maintain_time.last_maintain_date.year, 
                param->maintain_info.maintain_time.last_maintain_date.mon, 
                param->maintain_info.maintain_time.last_maintain_date.day);

        printf("mile_format :%d\r\n",param->usr_set.mile_format);
        printf("mile display:%d\r\n",param->usr_set.mile_display);
        printf("theme :%d\r\n",param->usr_set.theme);
        printf("brightness level :%d\r\n",param->usr_set.brightness);
        printf("system language :%d\r\n",param->usr_set.language);
        printf("bt_switch:%d\r\n",param->usr_set.bt_switch);
        printf("uuid_active_status:%d\r\n",param->usr_set.uuid_active_staus);
        printf("meter start src:%d\r\n",param->usr_set.start_src);
        printf("tpms Press Unit:%d\r\n",param->usr_set.tpms_unit);
        printf("tcs_switch:%d\r\n",param->usr_set.tcs_switch);
        printf("time_format:%d\r\n",param->usr_set.time_format);
        printf("temp_unit:%d\r\n",param->usr_set.temp_unit);
        printf("phone_type:%d\r\n", param->usr_set.phone_type);
        printf("sys_log:%d\r\n", param->usr_set.sys_log);
        printf("drive_mode:%d\r\n", param->usr_set.drive_mode);

        printf("ride_time_a:%d\r\n", param->ride_info.ride_time_a);     
        printf("ride_time_b:%d\r\n", param->ride_info.ride_time_b);     
    
        for (int i = 0; i < MAX_WHEEL_POS_NUM; i++) {
            printf("tpms_info[%d].id:0x%x\n", i, param->tpms[i].tpms_id);
            printf("tpms_info[%d].pressure:%d\n", i, param->tpms[i].tpms_pressure);
            printf("tpms_info[%d].temp:%d\n", i, param->tpms[i].tpms_temp);
        }

        printf("\r\nuuid:");
        for(int i = 0; i < 20; i++)
        {
            printf("%d",param->carlink_uuid[i]);
        }
        printf("\r\n");
        printf("\r\n ...............usr param end......................\r\n");
    }
}
#endif

static void check_usr_param(void) {
    if (usr_param.usr_set.mile_format > 1) {
        usr_param.usr_set.mile_format = 0;
    }

    if (usr_param.usr_set.mile_display > 2) {
        usr_param.usr_set.mile_display = 0;
    }

    if (usr_param.usr_set.theme > 2) {
        usr_param.usr_set.theme = 0;
    }

    if (usr_param.usr_set.brightness > 5) {
        usr_param.usr_set.brightness = 3;
    }

    if (usr_param.usr_set.language > 1) {
        usr_param.usr_set.language = 1;
    }

    if (usr_param.usr_set.uuid_active_staus > 1) {
        usr_param.usr_set.uuid_active_staus = 0;
    }

    if (usr_param.usr_set.tpms_unit > 2) {
        usr_param.usr_set.tpms_unit = 0;
    }

    if (usr_param.usr_set.tcs_switch > 1) {
        usr_param.usr_set.tcs_switch = 0;
    }

    if (usr_param.usr_set.time_format > 1) {
        usr_param.usr_set.time_format = 1;
    }

    if (usr_param.usr_set.temp_unit > 1) {
        usr_param.usr_set.temp_unit = 0;
    }

    if (usr_param.usr_set.auto_headlight > 1) {
        usr_param.usr_set.auto_headlight = 0;
    }

    if (usr_param.usr_set.phone_type > 1) {
        usr_param.usr_set.phone_type = 0;
    }

    if (usr_param.usr_set.drive_mode > 1) {
        usr_param.usr_set.drive_mode = 0;
    }

    if (usr_param.maintain_info.maintain_mile.maintain_count == 0xffff) {
        usr_param.maintain_info.maintain_mile.maintain_count = 0;
    }

    if (usr_param.maintain_info.maintain_mile.cur_maintain_mileage == 0xffff) {
        usr_param.maintain_info.maintain_mile.cur_maintain_mileage = 0;
    }

    if (usr_param.maintain_info.maintain_mile.last_maintain_total_mileage == 0xffffffff) {
        usr_param.maintain_info.maintain_mile.last_maintain_total_mileage = 0;
    }

    if (usr_param.maintain_info.maintain_time.maintain_days == 0xffff) {
        usr_param.maintain_info.maintain_time.maintain_days = 0;
    }

    if (usr_param.maintain_info.maintain_time.is_sync_time > 1) {
        usr_param.maintain_info.maintain_time.is_sync_time = 0;
    }

#if USE_PARAM_PRINTF
    printf_usr_param(&usr_param);
#endif

    vehicle_set_data(VEH_LICENSE_AUTH_STATUS, (int)usr_param.usr_set.uuid_active_staus);
}

void check_start_source(uint8_t start_src) {
    if (start_src) {
        start_by_acc = true;
        hcn_log_info("\r\nStart by acc\n");
    } else {
        hcn_log_info("\r\nStart by bat\n");
        usr_param.usr_set.mile_format = 0;
        usr_param.usr_set.mile_display = 0,
        usr_param.usr_set.language = 1;
        usr_param.usr_set.theme = 0;
        usr_param.usr_set.bt_switch = 1;
        usr_param.usr_set.tpms_unit = 0;
        usr_param.usr_set.brightness = 3;
        usr_param.usr_set.auto_headlight = 0;
    }

    check_usr_param();
    is_recovery_usr_param = true;
}

int save_hcn_usr_param(void) {
    if (!is_recovery_usr_param) {
        hcn_log_error("Usr param is not get ready!\n");
        return -1;
    }

    meter_info_t * param = get_hcn_info();
    if (!param) {
        hcn_log_error("Get meter pointer is null!\n");
        return -1;
    }

    param->usr = usr_param;

    if (memcmp(&usr_param_pre, &usr_param, sizeof(usr_param_t)) == 0) {
        hcn_log_error("param is same, do not save!\n");
        return -1;
    }

    memcpy(&usr_param_pre, &usr_param, sizeof(usr_param_t));

    if (save_hcn_info() != 0) {
        hcn_log_error("Save usr param failed!\n");
        return -1;
    }

    return 0;
}

bool get_hcn_usr_param(usr_param_handle_e id, void *param) {
    if (!get_recovery_usr_param()) {
        hcn_log_error("Usr param is not get ready!\n");
        return false;
    }

    if (!param) {
        hcn_log_error("Get usr param id:%d, param pointer null!\n", id);
        return false;
    }

    bool status = true;

    switch (id) {
        case HCN_PARAM_MAINTAIN_COUNTS:
            *((uint16_t *)param) = usr_param.maintain_info.maintain_mile.maintain_count;
            break;
        
        case HCN_PARAM_CUR_MAINTAIN_MILEAGE:
            *((uint16_t *)param) = usr_param.maintain_info.maintain_mile.cur_maintain_mileage;
            break;

        case HCN_PARAM_LAST_MAINTAIN_MILEAGE:
            *((uint16_t *)param) = usr_param.maintain_info.maintain_mile.last_maintain_total_mileage;
            break;

        case HCN_PARAM_MAINTAIN_DAYS:
            *((uint16_t *)param) = usr_param.maintain_info.maintain_time.maintain_days;
            break;

        case HCN_PARAM_MAINTAIN_DATE:
            memcpy(&param, &usr_param.maintain_info.maintain_time.last_maintain_date, 
                        sizeof(maintain_date_t));
            break;

        case HCN_PARAM_MAINTAIN_IS_SYNC_TIME:
            *((uint8_t *)param) = usr_param.maintain_info.maintain_time.is_sync_time;
            break;

        case HCN_PARAM_MILE_FORMAT:
            *((uint8_t *)param) = usr_param.usr_set.mile_format;
            break;

        case HCN_PARAM_MILE_DISPLAY:
            *((uint8_t *)param) = usr_param.usr_set.mile_display;
            break;

        case HCN_PARAM_THEME:
            *((uint8_t *)param) = usr_param.usr_set.theme;
            break;

        case HCN_PARAM_BRIGHTNESS_LEVEL:
            *((uint8_t *)param) = usr_param.usr_set.brightness;
            break;

        case HCN_PARAM_LANGUAGE:
            *((uint8_t *)param) = usr_param.usr_set.language;
            break;

        case HCN_PARAM_BT_SWITCH:
            *((uint8_t *)param) = usr_param.usr_set.bt_switch;
            break;

        case HCN_PARAM_UUID_REGISTER:
            *((uint8_t *)param) = usr_param.usr_set.uuid_active_staus;
            break;

        case HCN_PARAM_METER_START_SRC:
            *((uint8_t *)param) = usr_param.usr_set.start_src;
            break;

        case HCN_PARAM_TCS_SWITCH:
            *((uint8_t *)param) = usr_param.usr_set.tcs_switch;
            break;

        case HCN_PARAM_TEMP_UNIT:
            *((uint8_t *)param) = usr_param.usr_set.temp_unit;
            break;

        case HCN_PARAM_TIME_FORMAT:
            *((uint8_t *)param) = usr_param.usr_set.time_format;
            break;

        case HCN_PARAM_TPMS_PRESSURE_UNIT:
            *((uint8_t *)param) = usr_param.usr_set.tpms_unit;
            break;
            
        case HCN_PARAM_AUTO_HEAD_LIGHT:
            *((uint8_t *)param) = usr_param.usr_set.auto_headlight;
            break;

        case HCN_PARAM_PHONE_TYPE:
            *((uint8_t *)param) = usr_param.usr_set.phone_type;
            break;

        case HCN_PARAM_SYS_LOG:
            *((uint8_t *)param) = usr_param.usr_set.sys_log;
            break;

        case HCN_PARAM_DRIVE_MODE:
            *((uint8_t *)param) = usr_param.usr_set.drive_mode;
            break;

        case HCN_PARAM_EC_UUID:
            snprintf((char *)param, sizeof(usr_param.carlink_uuid), 
                    "%s", usr_param.carlink_uuid);
            break;
        
        case HCN_PARAM_RIDE_TIME_A:
             *((uint32_t *)param) = usr_param.ride_info.ride_time_a;
            break;

        case HCN_PARAM_RIDE_TIME_B:
            *((uint32_t *)param) = usr_param.ride_info.ride_time_b;
            break;
            
        case HCN_PARAM_LEFT_FRONT_TIRE_INFO:
            memcpy(param, &usr_param.tpms[TPMS_LEFT_FRONT], sizeof(tpms_param_t));
            break;

        case HCN_PARAM_RIGHT_FRONT_TIRE_INFO:
            memcpy(param, &usr_param.tpms[TPMS_RIGHT_FRONT], sizeof(tpms_param_t));
            break;

        case HCN_PARAM_LEFT_REAR_TIRE_INFO:
            memcpy(param, &usr_param.tpms[TPMS_LEFT_REAR], sizeof(tpms_param_t));
            break;

        case HCN_PARAM_RIGHT_REAR_TIRE_INFO:
            memcpy(param, &usr_param.tpms[TPMS_RIGHT_REAR], sizeof(tpms_param_t));
            break;

        default:
            status = false;
            break;
    }

    return status;
}

bool set_hcn_usr_param(usr_param_handle_e id, void *param) {
     if (!get_recovery_usr_param()) {
        hcn_log_error("Usr param is not get ready!\n");
        return false;
    }

    if (!param) {
        hcn_log_error("Set usr param id:%d, param pointer null!\n", id);
        return false;
    }

    bool is_save = true;

    switch (id) {
        case HCN_PARAM_MAINTAIN_COUNTS:
            if (usr_param.maintain_info.maintain_mile.maintain_count != *((uint16_t *)param)) {
                usr_param.maintain_info.maintain_mile.maintain_count = *((uint16_t *)param);
            }
            break;
        
        case HCN_PARAM_CUR_MAINTAIN_MILEAGE:
            if (usr_param.maintain_info.maintain_mile.cur_maintain_mileage != *((uint16_t *)param)) {
                usr_param.maintain_info.maintain_mile.cur_maintain_mileage = *((uint16_t *)param);
            }
            break;

        case HCN_PARAM_LAST_MAINTAIN_MILEAGE:
            if (usr_param.maintain_info.maintain_mile.last_maintain_total_mileage != *((uint16_t *)param)) {
                usr_param.maintain_info.maintain_mile.last_maintain_total_mileage = *((uint16_t *)param);
            }
            break;

        case HCN_PARAM_MAINTAIN_DAYS:
            if (usr_param.maintain_info.maintain_time.maintain_days != *((uint16_t *)param)) {
                usr_param.maintain_info.maintain_time.maintain_days = *((uint16_t *)param);
            }
            break;

        case HCN_PARAM_MAINTAIN_DATE:
            if (memcmp(&usr_param.maintain_info.maintain_time.last_maintain_date, param,
                       sizeof(maintain_date_t)) != 0) {
                memcpy(&usr_param.maintain_info.maintain_time.last_maintain_date, 
                        param, sizeof(maintain_date_t));
            }
            break;

        case HCN_PARAM_MAINTAIN_IS_SYNC_TIME:
            if (usr_param.maintain_info.maintain_time.is_sync_time != *((uint8_t *)param)) {
                usr_param.maintain_info.maintain_time.is_sync_time = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_MILE_FORMAT:
            if (usr_param.usr_set.mile_format != *((uint8_t *)param)) {
                usr_param.usr_set.mile_format = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_MILE_DISPLAY:
            if (usr_param.usr_set.mile_display != *((uint8_t *)param)) {
                usr_param.usr_set.mile_display = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_THEME:
            if (usr_param.usr_set.theme != *((uint8_t *)param)) {
                usr_param.usr_set.theme = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_BRIGHTNESS_LEVEL:
            if (usr_param.usr_set.brightness != *((uint8_t *)param)) {
                usr_param.usr_set.brightness = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_LANGUAGE:
            if (usr_param.usr_set.language != *((uint8_t *)param)) {
                usr_param.usr_set.language = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_BT_SWITCH:
            if (usr_param.usr_set.bt_switch != *((uint8_t *)param)) {
                usr_param.usr_set.bt_switch = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_UUID_REGISTER:
            if (usr_param.usr_set.uuid_active_staus != *((uint8_t *)param)) {
                usr_param.usr_set.uuid_active_staus = *((uint8_t *)param); 
            }
            break;

        case HCN_PARAM_METER_START_SRC:
            if (usr_param.usr_set.start_src != *((uint8_t *)param)) {
                usr_param.usr_set.start_src = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_TCS_SWITCH:
            if (usr_param.usr_set.tcs_switch != *((uint8_t *)param)) {
                usr_param.usr_set.tcs_switch = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_TEMP_UNIT:
            if (usr_param.usr_set.temp_unit != *((uint8_t *)param)) {
                usr_param.usr_set.temp_unit = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_TIME_FORMAT:
            if (usr_param.usr_set.time_format != *((uint8_t *)param)) {
                usr_param.usr_set.time_format = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_TPMS_PRESSURE_UNIT:
            if (usr_param.usr_set.tpms_unit != *((uint8_t *)param)) {
                usr_param.usr_set.tpms_unit = *((uint8_t *)param);  
            }
            break;
            
        case HCN_PARAM_AUTO_HEAD_LIGHT:
            if (usr_param.usr_set.auto_headlight != *((uint8_t *)param)) {
                usr_param.usr_set.auto_headlight = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_PHONE_TYPE:
            if (usr_param.usr_set.phone_type != *((uint8_t *)param)) {
                usr_param.usr_set.phone_type = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_SYS_LOG:
            if (usr_param.usr_set.sys_log != *((uint8_t *)param)) {
                usr_param.usr_set.sys_log = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_DRIVE_MODE:
            if (usr_param.usr_set.drive_mode != *((uint8_t *)param)) {
                usr_param.usr_set.drive_mode = *((uint8_t *)param);  
            }
            break;

        case HCN_PARAM_EC_UUID:
            if (strncmp((const char *)usr_param.carlink_uuid, ((const char *)param),
                        strlen(((const char *)param))) != 0) {
                memcpy(usr_param.carlink_uuid, param, strlen(((const char *)param)));
            }
            break;

        case HCN_PARAM_RIDE_TIME_A:
            is_save = false;
            if (usr_param.ride_info.ride_time_a != *((uint32_t *)param)) {
                usr_param.ride_info.ride_time_a = *((uint32_t *)param);  
            }
            break;

        case HCN_PARAM_RIDE_TIME_B:
            is_save = false;
            if (usr_param.ride_info.ride_time_b != *((uint32_t *)param)) {
                usr_param.ride_info.ride_time_b = *((uint32_t *)param);  
            }
            break;    

        case HCN_PARAM_LEFT_FRONT_TIRE_INFO:
            is_save = false;
            if (memcmp(&usr_param.tpms[TPMS_LEFT_FRONT], param,
                       sizeof(tpms_param_t)) != 0) {
                memcpy(&usr_param.tpms[TPMS_LEFT_FRONT], param,
                       sizeof(tpms_param_t));
            }
            break;
        case HCN_PARAM_RIGHT_FRONT_TIRE_INFO:
            is_save = false;
            if (memcmp(&usr_param.tpms[TPMS_RIGHT_FRONT], param,
                       sizeof(tpms_param_t)) != 0) {
                memcpy(&usr_param.tpms[TPMS_RIGHT_FRONT], param,
                       sizeof(tpms_param_t));
            }
            break;

        case HCN_PARAM_LEFT_REAR_TIRE_INFO:
            is_save = false;
            if (memcmp(&usr_param.tpms[TPMS_LEFT_REAR], param,
                       sizeof(tpms_param_t)) != 0) {
                memcpy(&usr_param.tpms[TPMS_LEFT_REAR], param,
                       sizeof(tpms_param_t));
            }
            break;

        case HCN_PARAM_RIGHT_REAR_TIRE_INFO:
            is_save = false;
            if (memcmp(&usr_param.tpms[TPMS_RIGHT_REAR], param,
                       sizeof(tpms_param_t)) != 0) {
                memcpy(&usr_param.tpms[TPMS_RIGHT_REAR], param,
                       sizeof(tpms_param_t));
            }
            break;

        default:
            is_save = false;
            break;
    }
    
#ifdef PARAM_WEAR_LEVEL_ENABLE
    if (is_save) {
        is_save = false;
        if (save_hcn_usr_param() != 0) {
            return false;
        }
    }
#endif

    return true;
}

static void read_usr_param(void) {
#ifndef PARAM_WEAR_LEVEL_ENABLE
    read_hcn_info();

    meter_info_t * meter_info = get_hcn_info();
    if (!meter_info) {
        hcn_log_error("Get Meter info pointer failed!\n");
        return;
    }
    
    if (meter_info->magic_num != NOR_FALSH_MAGIC_NUM) {
        ///< 使用默认设置参数
        hcn_log_info("Use default praram\n");

        maintain_info_t maintence_temp;
        memset(&maintence_temp, 0, sizeof(maintain_info_t));

        if (meter_info->magic_num != NOR_FLASH_EMPTY_FLAG) {
            uint8_t temp = ((meter_info->magic_num >> 24) & 0xff);
            if (temp == NOR_FLASH_MAGIC_NUM_PREFIX) {
                hcn_log_info("Read last maintenance info!\n");

                ///< 读取前面存储的保养参数
                if (meter_info->usr.maintain_info.maintain_mile.maintain_count < 255) {
                    memcpy(&maintence_temp, &meter_info->usr.maintain_info, 
                            sizeof(maintain_info_t));
                }
            }
        }

        meter_info->magic_num = NOR_FALSH_MAGIC_NUM;

        set_default_usr_param(&meter_info->usr);

        meter_info->usr.maintain_info = maintence_temp;
        usr_param_pre = meter_info->usr;

        if (save_hcn_info() != 0) {
            hcn_log_info("Save default failed!\n");
            return;
        }
  }

  usr_param = meter_info->usr;

  hcn_log_info("usr param init ok!\n");
#else
    if (read_hcn_info() != 0) {        
        meter_info_t * meter_info = get_hcn_info();
        if (!meter_info) {
            hcn_log_error("Get Meter info pointer failed!\n");
            return;
        }
        
        int scan_index = 0;
        uint32_t read_addr = 0;
        uint32_t offset_temp = 0;
        uint8_t temp = PARAN_RECORD_HEADER_MAGIC_PREFIX;

        ///< 使用默认设置参数
        hcn_log_info("Use default praram\n");

        maintain_info_t maintence_temp;
        memset(&maintence_temp, 0, sizeof(maintain_info_t));

        sfud_flash *sflash = sfud_get_device(0);
        if (!sflash) {
            hcn_log_error("Open spi nor flash failed!\r\n");
            return;
        }

        ///< 尝试从任意记录位置读取保养信息
        for (uint32_t offset = 0; offset <= NOR_FLASH_SECTOR_SIZE - PARAM_RECORD_SIZE; offset += PARAM_RECORD_SIZE) {
            uint32_t record_addr = HCN_USR_PARAM_ADDR + offset;
            record_header_t header;
            
            ///< 读取记录头
            if (sfud_read(sflash, record_addr, sizeof(record_header_t), (void *)&header) != SFUD_SUCCESS) {
                hcn_log_error("Read head info error!\r\n");
                continue;
            }
            
            ///< 遍历整个扇区，查找槽头对应的前缀
            if (header.magic != PARAM_RECORD_HEADER_MAGIC) {
                uint8_t data_tmp = ((header.magic >> 24) & 0xff);
                if (data_tmp == temp) {
                    scan_index++;
                    hcn_log_info("Read same head prefix, offset = %08x!\r\n", offset);
                    read_addr = record_addr;
                }
            }

            offset_temp = offset;
        }

        ///< 说明存在相同的槽头前缀，可以读取对应的保养信息
        if (scan_index >= 1) {
            meter_info_t temp_info = {0};
            if (sfud_read(sflash, read_addr + sizeof(record_header_t), 
                                sizeof(meter_info_t), (void *)&temp_info) == SFUD_SUCCESS) {
                if (temp_info.usr.maintain_info.maintain_mile.maintain_count > 0 
                    && temp_info.usr.maintain_info.maintain_mile.maintain_count < 255) {
                    memcpy(&maintence_temp, &temp_info.usr.maintain_info, 
                        sizeof(maintain_info_t));
                    hcn_log_info("Recover maintenance info from offset 0x%lx\n", offset_temp);
                }
            }
        }
       
        meter_info->magic_num = NOR_FALSH_MAGIC_NUM;
        set_default_usr_param(&meter_info->usr);
        meter_info->usr.maintain_info = maintence_temp;
        usr_param_pre = meter_info->usr;

        ///< 保存默认参数
        if (save_hcn_info() != 0) {
            hcn_log_error("Save default parameters failed!\n");
            return;
        }
    }

    meter_info_t * meter_info = get_hcn_info();
    if (!meter_info) {
        hcn_log_error("Get Meter info pointer failed!\n");
        return;
    }
    
    usr_param = meter_info->usr;
    
    ///< 打印Flash使用统计
    print_flash_usage_stats();
    
    hcn_log_info("User param init success!\n");

#endif
}

int usr_param_init(void) {
    read_usr_param();
    return 0;
}

bool get_recovery_usr_param(void) {
    return is_recovery_usr_param;
}

bool is_acc_start(void) {
  return start_by_acc;
}

#endif //HCN_NOR_FLASH_PARAM_ENABLE
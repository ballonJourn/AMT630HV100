/**
*
* @file hcn_mile_param.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/01 12:23
* @author och
*
*/

#include <string.h>
#include "storage_param2/hcn_mile_param.h"
#include "storage_param2/hcn_read_eeprom.h"
#include "log/hcn_log.h"
#include "source/crc32.h"

#define DEBUG_MILE_PARAM_ENABLE

#define MILEAGE_PARAM_START_ADDR (0x00)
#define OTHER_PARAM_START_ADDR  (0x280)
#define MILEAGE_PARAM_MAX_SIZE  (640)   ///< 里程参数最大区域为640个字节
#define MILEAGE_PARAM_PAGE_NUM  (40)    ///< 40页

static mile_param_t mile_param;
static mile_param_t mile_param_pre = {
    .erase_counter = 0xffffffff,
    .checksum = 0xffffffff,
};

static bool is_recovery_mile_param = false;
static uint16_t g_mile_write_addr = MILEAGE_PARAM_START_ADDR;

bool get_recovery_mile_param(void) {
    return is_recovery_mile_param;
}

bool set_hcn_mile_param(mile_param_handle_e id, void *param) {
#if 0
    if (!get_recovery_mile_param()) {
        return false;
    }
#endif

    if (!param) {
        hcn_log_error("Set mile param id:%d, param pointer null!\n", id);
        return false;
    }

    bool status = true;
    switch (id) {
        case HCN_MILE_PARAM_ODO:
            if (mile_param.odo != *((uint32_t *)param)) {
                mile_param.odo = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_TRIP_A:
            if (mile_param.tripa != *((uint32_t *)param)) {
                mile_param.tripa = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_TRIP_B:
            if (mile_param.tripb != *((uint32_t *)param)) {
                mile_param.tripb = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_RIDE_TIME_A:
            if (mile_param.ride_time_a != *((uint32_t *)param)) {
                mile_param.ride_time_a = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_RIDE_TIME_B:
            if (mile_param.ride_time_b != *((uint32_t *)param)) {
                mile_param.ride_time_b = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_AVG_FUEL_CON_A:
            if (mile_param.avg_fuel_con_a != *((uint16_t *)param)) {
                mile_param.avg_fuel_con_a = *((uint16_t *)param);
            }
            break;

        case HCN_MILE_PARAM_AVG_FUEL_CON_B:
            if (mile_param.avg_fuel_con_b != *((uint16_t *)param)) {
                mile_param.avg_fuel_con_b = *((uint16_t *)param);
            }
            break;

        case HCN_MILE_PARAM_RANGE_A:
            if (mile_param.range_a != *((uint32_t *)param)) {
                mile_param.range_a = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_RANGE_B:
            if (mile_param.range_b != *((uint32_t *)param)) {
                mile_param.range_b = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_ALL_FUEL_CONS:
            if (mile_param.all_fuel_cons != *((uint32_t *)param)) {
                mile_param.all_fuel_cons = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_AVG_SPD_TIME_A:
            if (mile_param.avg_spd_time_a != *((uint32_t *)param)) {
                mile_param.avg_spd_time_a = *((uint32_t *)param);
            }
            break;

        case HCN_MILE_PARAM_AVG_SPD_TIME_B:
            if (mile_param.avg_spd_time_b != *((uint32_t *)param)) {
                mile_param.avg_spd_time_b = *((uint32_t *)param);
            }
            break;

        default:
            status = false;
            break;
    }

    return status;
}

bool get_hcn_mile_param(mile_param_handle_e id, void *param) {
#if 0
    if (!get_recovery_mile_param()) {
        return false;
    }
#endif
    if (!param) {
        hcn_log_error("Get mile param id:%d, param pointer null!\n", id);
        return false;
    }

    bool status = true;

     switch (id) {
        case HCN_MILE_PARAM_ODO:
            *((uint32_t *)param) = mile_param.odo;
            break;
            
        case HCN_MILE_PARAM_TRIP_A:
            *((uint32_t *)param) = mile_param.tripa;
            break;

        case HCN_MILE_PARAM_TRIP_B:
            *((uint32_t *)param) = mile_param.tripb;
            break;

        case HCN_MILE_PARAM_RIDE_TIME_A:
            *((uint32_t *)param) = mile_param.ride_time_a;
            break;

        case HCN_MILE_PARAM_RIDE_TIME_B:
            *((uint32_t *)param) = mile_param.ride_time_b;
            break;

        case HCN_MILE_PARAM_AVG_FUEL_CON_A:
            *((uint16_t *)param) = mile_param.avg_fuel_con_a;
            break;

        case HCN_MILE_PARAM_AVG_FUEL_CON_B:
            *((uint16_t *)param) = mile_param.avg_fuel_con_b;
            break;

        case HCN_MILE_PARAM_RANGE_A:
            *((uint16_t *)param) = mile_param.range_a;
            break;

        case HCN_MILE_PARAM_RANGE_B:
            *((uint16_t *)param) = mile_param.range_b;
            break;

        case HCN_MILE_PARAM_ALL_FUEL_CONS:
            *((uint32_t *)param) = mile_param.all_fuel_cons;
            break;

        case HCN_MILE_PARAM_AVG_SPD_TIME_A:
            *((uint32_t *)param) = mile_param.avg_spd_time_a;
            break;

        case HCN_MILE_PARAM_AVG_SPD_TIME_B:
            *((uint32_t *)param) = mile_param.avg_spd_time_b;
            break;

        default:
            status = false;
            break;
    }

    return status;
}

static uint8_t get_page_num(void) {
    uint8_t page_num = sizeof(mile_param_t) % E2PROM_PAGE_SIZE;
    if (page_num != 0) {
        page_num = (sizeof(mile_param_t) / E2PROM_PAGE_SIZE) + 1;
    } else {
        page_num = sizeof(mile_param_t) / E2PROM_PAGE_SIZE;
    }

    return page_num;
}

static void printf_mile_param_info(mile_param_t *param) {
    if (param) {
        hcn_log_info("\n--------mile param start--------\n");
        hcn_log_info("ODO:%lu\n", param->odo);
        hcn_log_info("tripa:%lu\n", param->tripa);
        hcn_log_info("tripb:%lu\n", param->tripb);
        hcn_log_info("ride_time_a:%lu\n", param->ride_time_a);
        hcn_log_info("ride_time_b:%lu\n", param->ride_time_b);
        hcn_log_info("avg_fuel_con_a:%d\n", param->avg_fuel_con_a);
        hcn_log_info("avg_fuel_con_b:%d\n", param->avg_fuel_con_b);
        hcn_log_info("rang_a:%d\n", param->range_a);
        hcn_log_info("rang_b:%d\n", param->range_b);
        hcn_log_info("\n--------mile param end--------\n");
    }
}

int save_mile_param(void) {
    int ret = -1;
    
    if (memcmp(&mile_param_pre, &mile_param, (sizeof(mile_param_t) - 4) == 0)) {
        return 0;
    }

    mile_param.erase_counter++;
    memcpy(&mile_param_pre, &mile_param, sizeof(mile_param_t));

    uint8_t page_num = get_page_num();
    g_mile_write_addr = g_mile_write_addr + page_num * E2PROM_PAGE_SIZE;
    if (g_mile_write_addr >= OTHER_PARAM_START_ADDR) {
        g_mile_write_addr = MILEAGE_PARAM_START_ADDR;
    }
    mile_param.checksum =  xcrc32((uint8_t*)&mile_param, sizeof(mile_param_t) - 4, 0xffffffff);

    ret = e2prom_write_data(g_mile_write_addr, (uint8_t*)&mile_param, sizeof(mile_param_t));
    if (ret == sizeof(mile_param_t)) {
        return ret;
    }

    hcn_log_error("Save mile param failed!\n");

    return -1;
}   

static int read_mile_param(void) {
    uint32_t check_sum = 0;
    int ret = -1;

    ret = e2prom_read_data(g_mile_write_addr, (uint8_t *)&mile_param, 
                            sizeof(mile_param_t));
    if (ret > 0) {
        check_sum = xcrc32((uint8_t*)&mile_param, sizeof(mile_param_t) - 4, 0xffffffff);
        if (check_sum == mile_param.checksum) {
#ifdef DEBUG_MILE_PARAM_ENABLE
            printf_mile_param_info(&mile_param);
#endif
            return 0;
        } else {
            hcn_log_error("Read mile param check sum failed!\n");
        }
    } else {
        hcn_log_error("Read mile param failed!\n");
    }
    
    return ret;
}

static char check_mile_param(void) {
    uint8_t magic_buff[4] = {0};
    uint16_t write_addr = 0;
    uint32_t temp = 0;
    uint32_t max_value = 0;
    int ret = -1;

    if (e2prom_byte_read(MILEAGE_PARAM_START_ADDR, (uint8_t*)magic_buff, 4) > 0) {
        temp = ((magic_buff[0] << 24) + (magic_buff[1] << 16) + (magic_buff[2] << 8) 
                + magic_buff[3]);

        uint8_t page_num = get_page_num();

        ///< 里程区域未写入过数据
        if (temp != HCN_E2PROM_MAGIC_NUM) {
            mile_param.magic_num = HCN_E2PROM_MAGIC_NUM;
            memset(&mile_param, 0, sizeof(mile_param_t));
            mile_param.checksum = xcrc32((uint8_t*)&mile_param, sizeof(mile_param_t) - 4, 0xffffffff);
        
            for (int i = 0; i < (MILEAGE_PARAM_PAGE_NUM/page_num); i++) {
                write_addr = MILEAGE_PARAM_START_ADDR * i * (page_num * E2PROM_PAGE_SIZE);
                ret = e2prom_write_data(write_addr, (uint8_t*)&mile_param, sizeof(mile_param_t));
                if(ret < 0){
                    hcn_log_error("\ne2Prom check init failed, failed addr: 0x%x\n",write_addr);
                    return -1;
                }
            }

            g_mile_write_addr = MILEAGE_PARAM_START_ADDR;

            return 0;
        }
        
        ///< 初始化过的区域，查找到最大的擦写次数位置
        uint16_t earse_count_addr = 0;

        ///< erase_counter成员在结构体的位置
        uint16_t pos_earse = (11 * 4); 

        for (int i = 0; i < (MILEAGE_PARAM_PAGE_NUM/page_num); i++) {
            earse_count_addr = (MILEAGE_PARAM_START_ADDR + pos_earse) 
                                + (page_num * E2PROM_PAGE_SIZE * i);
            e2prom_read_data(earse_count_addr, (uint8_t*)temp, sizeof(temp));
            hcn_log_info("earse count value = %d\n", earse_count_addr);
            if (max_value >= temp) {
                max_value = temp;
                g_mile_write_addr = earse_count_addr - pos_earse;
            }
        }

        hcn_log_info("earse count max = %d\n", earse_count_addr);
        hcn_log_info("mile write addr = 0x%X\n", g_mile_write_addr);

        ///< 从e2prom读取出数据
        if (read_mile_param() == 0) {
            memcpy((uint8_t*)&mile_param_pre, (uint8_t*)&mile_param, sizeof(mile_param_t));
            is_recovery_mile_param = true;
            hcn_log_info("Read e2prom param success!\n");
            return 1;
        } else {
            hcn_log_info("Read e2prom param failed!\n");
            return -1;
        }

    } else {
        hcn_log_error("Read magic num error!\n");
        return -1;
    }
}

void mile_param_init(void) {
    if (check_mile_param() != 1) {
        memset(&mile_param, 0, sizeof(mile_param_t));
    }
}

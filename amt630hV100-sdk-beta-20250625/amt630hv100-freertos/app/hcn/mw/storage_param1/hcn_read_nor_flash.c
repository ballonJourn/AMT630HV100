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
#include "task.h"
#include "sfud.h"
#include "source/crc32.h"
#include "storage_param1/hcn_read_nor_flash.h"
#include "storage_param1/hcn_usr_param.h"
#include "log/hcn_log.h"

#ifdef HCN_NOR_FLASH_PARAM_ENABLE

static meter_info_t meter_info;
static bool is_need_erase = false;
static uint32_t total_write_count = 0;
static uint32_t sector_erase_count = 0;
static uint32_t current_record_offset = 0;

/**
 * @brief 计算数据校验和
 */
static uint32_t calculate_checksum(const uint8_t *data, uint32_t size) {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < size; i++) {
        checksum += data[i];
    }

    return checksum;
}

/**
 * @brief 验证记录头是否有效
 */
static bool is_record_header_valid(record_header_t *header) {
    if (header->magic != PARAM_RECORD_HEADER_MAGIC) {
        //hcn_log_error("head magic num is not same!, data:%08X\r\n", header->magic);
        return false;
    }
    
    if (header->data_size != sizeof(meter_info_t)) {
        hcn_log_error("head info len is not same!\r\n");
        return false;
    }
    
    static uint8_t times = 0;
    uint32_t calc_checksum = calculate_checksum((uint8_t *)header, 
                                               sizeof(record_header_t) - sizeof(uint32_t));
    if (calc_checksum != header->checksum) {
        hcn_log_error("\r\ncal sum error, calc_checksum = 0x%08x, header->checksum = 0x%08x\r\n", calc_checksum, header->checksum);
    } else {
        times++;
        hcn_log_info("Head crc success, time = %d\r\n", times);
        if (times >= 255) {
            times = 0;
        }
    }

    return (calc_checksum == header->checksum);
}

/**
 * @brief 查找最新的有效记录
 */
static int find_latest_record(meter_info_t *data) {
    uint32_t max_write_count = 0;
    bool found_valid = false;
    uint32_t valid_records = 0;
    int i = 0, j = 0;
    hcn_log_info("Scanning sector for valid records...\n");
    
    ///< 遍历扇区中的所有可能记录位置
    for (uint32_t offset = 0; offset <= NOR_FLASH_SECTOR_SIZE - PARAM_RECORD_SIZE; offset += PARAM_RECORD_SIZE) {
        uint32_t record_addr = HCN_USR_PARAM_ADDR + offset;
        record_header_t header = {0};
        meter_info_t record_data = {0};
        sfud_flash *sflash = sfud_get_device(0);
        
        ///< 读取记录头
        if (sfud_read(sflash, record_addr, sizeof(record_header_t), (void *)&header) != SFUD_SUCCESS) {
            //hcn_log_error("Scan head info error, error times = %d\r\n", i);
            i++;
            continue;
        }
        
        ///< 验证记录头
        if (!is_record_header_valid(&header)) {
            //hcn_log_error("Head info is invalid!\r\n");
            continue;
        }
        
        ///< 读取记录数据
        if (sfud_read(sflash, record_addr + sizeof(record_header_t), 
                      sizeof(meter_info_t), (void *)&record_data) != SFUD_SUCCESS) {
            //hcn_log_error("Scan meter info error, error times = %d\r\n", j);
            j++;
            continue;
        }
        
        ///< 验证数据校验和
        uint16_t checksum = xcrc32(((unsigned char *)&record_data) + 2, 
                                  sizeof(meter_info_t) - 2, 0xffffffff);
        if (checksum != record_data.check_sum) {
            hcn_log_error("Record at offset 0x%lx has invalid checksum\n", offset);
            hcn_log_error("Meter info check sum = %08x, record_data.check_sum = %08x\r\n", checksum, record_data.check_sum);
            continue;
        }
        
        valid_records++;
        
        ///< 找到写入计数最大的记录（最新的）
        if (header.write_count >= max_write_count) {
            max_write_count = header.write_count;
            memcpy(data, &record_data, sizeof(meter_info_t));
            current_record_offset = offset;
            total_write_count = max_write_count;
            found_valid = true;
            
            hcn_log_info("Found newer record: count=%lu, offset=0x%lx\n", 
                         header.write_count, offset);
        }
    }
    
    hcn_log_info("Scan complete: found %lu valid records, latest count=%lu\n", 
                 valid_records, max_write_count);
    
    return found_valid ? 0 : -1;
}

/**
 * @brief 写入新记录到下一个可用位置
 */
static int write_new_record(meter_info_t *data) {
    sfud_flash *sflash = sfud_get_device(0);
    if (!sflash) {
        hcn_log_error("Open spi nor flash error!\r\n");
        return -1;
    }
    
    ///< 计算下一个记录位置
    uint32_t next_offset = current_record_offset + PARAM_RECORD_SIZE;
    
    ///< 如果超出扇区范围，需要擦除扇区重新开始
    if (next_offset > (NOR_FLASH_SECTOR_SIZE - PARAM_RECORD_SIZE)) {
        hcn_log_info("Sector full, erasing and starting over... "
                     "Total writes before erase: %lu\n", total_write_count);
        
        if (sfud_erase(sflash, HCN_USR_PARAM_ADDR, NOR_FLASH_SECTOR_SIZE) != SFUD_SUCCESS) {
            hcn_log_error("Sector erase failed!\n");
            return -1;
        }
        
        sector_erase_count++;
        next_offset = 0;

        hcn_log_info("Sector erased successfully, erase count: %lu\n", sector_erase_count);
    }
    
    if (is_need_erase) {
        next_offset = 0;
        is_need_erase = false;
        hcn_log_info("Do not offset pos!\r\n");
    }

    uint32_t record_addr = HCN_USR_PARAM_ADDR + next_offset;
    
    ///< 准备记录头
    record_header_t header;
    header.magic = PARAM_RECORD_HEADER_MAGIC;
    header.write_count = ++total_write_count;
    header.data_size = sizeof(meter_info_t);
    header.timestamp = xTaskGetTickCount(); ///< 使用FreeRTOS时间戳
    header.checksum = calculate_checksum((uint8_t *)&header, 
                                        sizeof(record_header_t) - sizeof(uint32_t));
    
    ///< 计算数据校验和
    data->check_sum = xcrc32(((unsigned char *)data) + 2, 
                            sizeof(meter_info_t) - 2, 0xffffffff);
    
    ///< 准备写入缓冲区
    uint8_t write_buffer[PARAM_RECORD_SIZE];
    memcpy(write_buffer, &header, sizeof(record_header_t));
    memcpy(write_buffer + sizeof(record_header_t), data, sizeof(meter_info_t));
    
    hcn_log_info("record addr:0x%08x, next_offset:0x%08x\r\n", record_addr, next_offset);

    ///< 写入记录
    if (sfud_write(sflash, record_addr, PARAM_RECORD_SIZE, write_buffer) != SFUD_SUCCESS) {
        hcn_log_error("Write record failed at offset 0x%lx!\n", next_offset);
        return -1;
    }
    
    current_record_offset = next_offset;
    
    uint32_t remaining_writes = (NOR_FLASH_SECTOR_SIZE - next_offset - PARAM_RECORD_SIZE) / PARAM_RECORD_SIZE;
    
    hcn_log_info("Write success: count=%lu, offset=0x%lx, remaining=%lu\n", 
                 total_write_count, next_offset, remaining_writes);
    
    return 0;
}

/**
 * @brief 检查扇区是否为空（首次使用）
 */
static bool is_sector_empty(void) {
    sfud_flash *sflash = sfud_get_device(0);
    if (!sflash) {
        hcn_log_error("Open spi flash error!\r\n");
        return true;
    }
    
    ///< 检查扇区开头几个字节
    uint32_t test_data;
    if (sfud_read(sflash, HCN_USR_PARAM_ADDR, sizeof(uint32_t), (void *)&test_data) != SFUD_SUCCESS) {
        hcn_log_error("Read spi nor flash failed!\r\n");
        return true;
    }
    
    hcn_log_info("Head data:%08x\r\n", test_data);
    ///< Flash擦除后为0xFF
    if (test_data == 0xFFFFFFFF) {
        hcn_log_info("Spi empty rom is 0xffffffff!\r\n");
        return true;
    }

    hcn_log_info("First head is read success!\r\n");

    return false;
}

/**
 * @brief 初始化统计信息（首次使用）
 */
static void init_wear_statistics(void) {
    total_write_count = 0;
    sector_erase_count = 0;
    current_record_offset = 0;
}

int read_hcn_info(void) {
#ifndef PARAM_WEAR_LEVEL_ENABLE
    uint16_t checksum;
    sfud_flash *sflash = sfud_get_device(0);
    hcn_log_info("\r\nusr_param_sizeof = %d byte\r\n", sizeof(meter_info_t));
    sfud_read(sflash, HCN_USR_PARAM_ADDR, sizeof(meter_info_t),
              (void *)&meter_info);

    checksum = xcrc32(((unsigned char *)&meter_info) + 2,
                      sizeof(meter_info) - 2, 0xffffffff);
    if (checksum == meter_info.check_sum) {
        return 0;
    }

    return -1;
#else
    if (PARAM_RECORD_SIZE > NOR_FLASH_SECTOR_SIZE) {
        hcn_log_error("Record size exceeds sector size!"); 
        return -1;
    }
    
    hcn_log_info("usr param size = %d\r\n", PARAM_RECORD_SIZE);

    ///< 如果是首次使用，初始化统计信息
    if (is_sector_empty()) {
        hcn_log_info("First time use, initializing flash sector...\n");
        init_wear_statistics();
        memset(&meter_info, 0, sizeof(meter_info_t));
        is_need_erase = true;
        return -1;
    }
    
    ///< 查找最新的有效记录
    if (find_latest_record(&meter_info) == 0) {
        hcn_log_info("Found valid data: write_count=%lu, offset=0x%lx\n", 
                     total_write_count, current_record_offset);
        return 0;
    }
    
    ///< 没有找到有效数据，使用默认值(槽头魔数有改变，需要处理完以后再擦错整个扇区)
    hcn_log_info("No valid data found, use default parameters\n");
    memset(&meter_info, 0, sizeof(meter_info_t));
    init_wear_statistics();
    is_need_erase = true;

    return -1;

#endif
}

int save_hcn_info(void) {
#ifndef PARAM_WEAR_LEVEL_ENABLE
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
#else
    if (is_need_erase) {
        hcn_log_info("need earse ....\r\n");
        sfud_flash *sflash = sfud_get_device(0);
        if (sfud_erase(sflash, HCN_USR_PARAM_ADDR, NOR_FLASH_SECTOR_SIZE) != SFUD_SUCCESS) {
            hcn_log_error("First save sector erase failed!\n");
            return -1;
        } else {
            hcn_log_info("earse success! ....\r\n");
        }
    }

    ///< 写入新记录
    if (write_new_record(&meter_info) == 0) {
        hcn_log_info("Save usr param success! \r\n");
        return 0;
    }
    
    hcn_log_error("Save param failed!\n");

    return -1;

#endif
}

meter_info_t *get_hcn_info(void) {
     return &meter_info; 
}

int get_flash_wear_info(flash_wear_info_t *wear_info) {
    if (!wear_info) {
        return -1;
    }
    
    wear_info->total_writes = total_write_count;
    wear_info->sector_erases = sector_erase_count;
    wear_info->last_erase_time = xTaskGetTickCount();
    wear_info->remaining_writes = get_remaining_writes_estimate();
    
    return 0;
}

uint32_t get_remaining_writes_estimate(void) {
    if (sector_erase_count == 0) {
        ///< 首次使用，计算理论最大值
        return THEORETICAL_MAX_WRITES;
    }
    
    ///< 根据当前使用情况估算剩余寿命
    uint32_t records_per_sector = MAX_RECORDS_PER_SECTOR;
    uint32_t remaining_records = (NOR_FLASH_SECTOR_SIZE - current_record_offset) / PARAM_RECORD_SIZE;
    uint32_t estimated_remaining = (records_per_sector * 10000) - 
                                   (total_write_count % (records_per_sector * 10000)) + 
                                   remaining_records * 10000;
    
    return estimated_remaining;
}

void print_flash_usage_stats(void) {
    uint32_t record_size = PARAM_RECORD_SIZE;
    uint32_t max_records = MAX_RECORDS_PER_SECTOR;
    uint32_t remaining_writes = get_remaining_writes_estimate();
    uint32_t used_space = current_record_offset + PARAM_RECORD_SIZE;
    uint32_t free_space = NOR_FLASH_SECTOR_SIZE - used_space;
    
    hcn_log_info("=== Flash Usage Statistics ===\n");
    hcn_log_info("Record size: %lu bytes\n", record_size);
    hcn_log_info("Max records per sector: %lu\n", max_records);
    hcn_log_info("Total writes: %lu\n", total_write_count);
    hcn_log_info("Sector erases: %lu\n", sector_erase_count);
    hcn_log_info("Current offset: 0x%lx\n", current_record_offset);
    hcn_log_info("Used space: %lu bytes\n", used_space);
    hcn_log_info("Free space: %lu bytes\n", free_space);
    hcn_log_info("Remaining writes estimate: %lu\n", remaining_writes);
    hcn_log_info("Theoretical max writes: %lu\n", THEORETICAL_MAX_WRITES);
    hcn_log_info("==============================\n");
}

#endif //HCN_NOR_FLASH_PARAM_ENABLE
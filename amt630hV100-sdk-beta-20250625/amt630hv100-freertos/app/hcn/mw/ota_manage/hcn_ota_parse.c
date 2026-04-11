/**
*
* @file hcn_ota_parse.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/16 09:22
* @author och
*
*/

#include <stdio.h>
#include "chip.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "ff_stdio.h"
#include "sfud.h"
#include "updatefile.h"
#include "sysinfo.h"
#include "ota_manage/hcn_ota_parse.h"
#include "ota_manage/hcn_tcp_client.h"
#include "ota_manage/hcn_ota.h"
#include "storage_param1/hcn_read_nor_flash.h"
#include "log/hcn_log.h"
#include "source/crc32.h"
#include "utils/hcn_utils.h"

#define OTA_PARSE_DEBUG_ENABLE

#define OTA_PARSE_QUEUE_LEN   (30)
#define OTA_CRC_ALL_FLASH_ENABLE   ///< 检验所有写入nor flash的参数
#define OTA_UPDATE_PROGRESS_ENABLE  ///< 升级进度条显示

static QueueHandle_t ota_parse_queue = NULL;
static ota_file_t ota_files[OTA_ALL_FILE];
static ota_param_t ota_param = {
    .ota_percent = 0,
    .ota_percent_change = false,
    .is_ready_update = false,
    .is_first_update_file = true,
    .ota_buff = NULL,
    .ota_total_size = 0,
    .ota_write_size = 0,
    .ota_cur_rx_size = 0,
};

static UpFileHeader header;
static sfud_flash *sflash_ota = NULL;
static SemaphoreHandle_t ota_sem = NULL;
static uint32_t write_flash_error = 0;
static bool is_first_parse = true;
static uint32_t g_calc_checksum = 0;

static bool is_update_file(UpFileHeader *pbuff){
    if (pbuff == NULL) {
        return false;
    }

    if (pbuff->magic != MKTAG('U', 'P', 'D', 'F')) {
        hcn_log_error("update file isn't found, can't support module update.\n");
        return false;
    }
    return true;
}

static bool check_update_file_header(uint32_t offset) {
    uint8_t *readbuf = (uint8_t *)pvPortMalloc(128);
    if (readbuf == NULL) {
        return false;
    }

    memset(readbuf, 0, 128);
    sfud_flash *sflash = sfud_get_device(0);
    sfud_read(sflash, offset, 128, (void*)readbuf);  
    UpFileHeader * header = (UpFileHeader *)readbuf;

    hcn_log_info("\r\nheader->files[0].magic = 0x%08X*******\r\n",header->files[0].magic);
    hcn_log_info("\r\nheader->files[0].offset = 0x%08X*******\r\n",header->files[0].offset);
    hcn_log_info("\r\nheader->files[0].size = 0x%08X*******\r\n",header->files[0].size);

    if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
        hcn_log_error("Wrong ota update file, don't update");
        vPortFree(readbuf);
        return false;
    }

    vPortFree(readbuf);
    return true;
}

static uint32_t get_update_file_checksum(uint32_t start_addr, uint32_t file_size) {
    uint32_t calc_checksum = 0xffffffff;
    uint32_t checksum = 0;
    uint32_t offset = 0;
    uint32_t remain_size = 0;

    if (file_size < 0x10000) {
        hcn_log_error("Update file filesize is too small!\r\n");
        return 0;
    }

    uint8_t *read_buff = (uint8_t *)pvPortMalloc(0x10000);
    if (read_buff == NULL) {
        return 0;
    }

    sfud_flash *sflash = sfud_get_device(0);

    while (offset < file_size) {
        if (file_size - offset  >  0x10000) {
            remain_size = 0x10000;
        } else {
            remain_size = file_size - offset;
        }

        if (sfud_read(sflash, start_addr + offset, 
                remain_size, (void *)read_buff) == SFUD_SUCCESS) {
            if (offset == 0) {
                UpFileHeader * header = (UpFileHeader *)read_buff;
                checksum = header->checksum;
                header->checksum = 0;
            }

            calc_checksum = xcrc32(read_buff, remain_size, calc_checksum);
            offset += remain_size;
        } else {
            vPortFree(read_buff);
            read_buff = NULL;
            hcn_log_error("Ota err read flash!\r\n");

            return 0;
        }
    }

    vPortFree(read_buff);
    read_buff = NULL;

    hcn_log_info("read flash crc checksum = %08x, calc_checksum = %08x\r\n", \
                checksum, calc_checksum);

    return calc_checksum;
}

static uint32_t get_ota_file_offset(uint8_t file_type, int toburn) {
    uint32_t offset = 0xffffffff;
    switch (file_type) {
        case OTA_SPILDR_FILE:
            offset = LOADER_OTA_FILE_OFFSET;
            break;
        case OTA_STEPLDR_FILE:
            offset = STEPLDR_OTA_FILE_OFFSET;
            break;
        case OTA_UPDATE_FILE: {
                SysInfo *sysinfo = GetSysInfo();
                hcn_log_info("\r\nsysinfo->image_offset=0x%08x\n", sysinfo->image_offset);
                if (file_type == OTA_UPDATE_FILE) {
                    if (!toburn) {
                        offset = sysinfo->image_offset;
                        ota_files[OTA_UPDATE_FILE - 1].ota_addr_offset = offset;
                    } else {
                        if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET) {
                            offset = UPDATEFILE_MEDIA_B_OFFSET;
                        } else{
                            offset = UPDATEFILE_MEDIA_OFFSET;
                        }
                    }
                }
            }
            
            break;
        case OTA_MCU_FILE:
            offset = MCU_OTA_FILE_OFFSET;
            break;

        default:
            hcn_log_error("unknown ota file type %d!\n", file_type);
            break;
    }

    return offset;
}

static void start_ota(void) {
    if (!ota_param.is_ready_update) {
        hcn_log_error("Start ota is not ready!\r\n");
        return;
    }

    SysInfo *sysinfo = GetSysInfo();
    static bool is_error = true;
    meter_info_t *meter_info = get_hcn_info();

    if (ota_files[OTA_MCU_FILE - 1].ota_write_offset == 
        ota_files[OTA_MCU_FILE - 1].ota_file_size &&
        ota_files[OTA_MCU_FILE - 1].ota_write_offset > 0) {
        hcn_log_info("Ota Mcu file is success!\r\n");
        meter_info->mcu_update = 2;
        meter_info->usr.mcu_update_len = ota_files[OTA_MCU_FILE - 1].ota_file_size;
    } else {
        is_error = false;
    }

    if (ota_files[OTA_SPILDR_FILE - 1].ota_write_offset == 
        ota_files[OTA_SPILDR_FILE - 1].ota_file_size &&
        ota_files[OTA_SPILDR_FILE - 1].ota_write_offset > 0) {
        hcn_log_info("Ota spi ldr file is success!\r\n");
    } else {
        is_error = false;
    }

    if (ota_files[OTA_STEPLDR_FILE - 1].ota_write_offset == 
        ota_files[OTA_STEPLDR_FILE - 1].ota_file_size &&
        ota_files[OTA_STEPLDR_FILE - 1].ota_write_offset > 0) {
        hcn_log_info("Ota step ldr file is success!\r\n");
    } else {
        is_error = false;
    }

    if (ota_files[OTA_UPDATE_FILE - 1].ota_write_offset == 
        ota_files[OTA_UPDATE_FILE - 1].ota_file_size &&
        ota_files[OTA_UPDATE_FILE - 1].ota_write_offset > 0) {
        hcn_log_info("Ota update file is success!\r\n");
    } else {
        is_error = false;
    }
 
    if (is_error) {
        if ((g_calc_checksum != 0) && (g_calc_checksum != 0xffffffff)) {
            sysinfo->app_checksum = g_calc_checksum;
        }

        sysinfo->app_size = ota_files[OTA_UPDATE_FILE - 1].ota_file_size;
        if ((ota_files[OTA_UPDATE_FILE - 1].ota_addr_offset == UPDATEFILE_MEDIA_OFFSET)
            || (ota_files[OTA_UPDATE_FILE - 1].ota_addr_offset == UPDATEFILE_MEDIA_B_OFFSET)) {
            hcn_log_info("current image offset = 0x%08x\r\n", ota_files[OTA_UPDATE_FILE - 1].ota_addr_offset);
            sysinfo->image_offset = ota_files[OTA_UPDATE_FILE - 1].ota_addr_offset;
        }

        sysinfo->update_media_type = UPDATE_MEDIA_WIFI;
        sysinfo->update_status = UPDATE_STATUS_END;
        SaveSysInfo();
        if (save_hcn_info() != 0) {
            hcn_log_info("Save hcn usr error!\r\n");
            return;
        }
        hcn_log_info("OTA update success, os will reboot!\r\n");
        wdt_cpu_reboot();
    } else {
        send_update_status(HCN_MSG_OTA_STAUS, \
                ota_param.ota_total_size, \
                ota_param.ota_write_size, UPDATE_ERROR_FLASH);
        hcn_log_error("OTA file exist error!\r\n");
    }
}

static void write_update_data(uint8_t file_type, int len) {
    if (get_ota_state() != TCP_SEND_DEVICE_INFO) {
        set_ota_state(TCP_SEND_PERCENT);
        
        uint32_t write_addr = ota_files[file_type - 1].ota_addr_offset + 
                              ota_files[file_type - 1].ota_write_offset;
        if (sflash_ota) {
            if (sfud_erase_write(sflash_ota, write_addr, len, 
                                ota_param.ota_buff) != SFUD_SUCCESS) {
                hcn_log_error("Ota burn %s flash error!\r\n", 
                        ota_files[file_type - 1].file_name);
                write_flash_error++;
                goto exit;
            }

            ota_param.ota_write_size += len;
            ota_files[file_type - 1].ota_write_offset += len;
            send_update_status(HCN_MSG_OTA_STAUS, \
                ota_param.ota_total_size, \
                ota_param.ota_write_size, UPDATE_ERROR_NONE);
            uint8_t temp_percent = 100 - (((ota_param.ota_total_size - \
                    ota_param.ota_write_size) * 100) / ota_param.ota_total_size);
            if (ota_param.ota_percent != temp_percent) {
                ota_param.ota_percent = temp_percent;
                ota_param.ota_percent_change = true;
            }
        }
    }

    return;

exit:
    hcn_log_info("Ota write error!\r\n");
}

static bool ota_check_sum(uint8_t *msg) {
    ///< 包头长度 + 消息长度 + 数据内容长度 + CRC32长度
    uint32_t msg_len = 2 + ((msg[2] << 8) | msg[3]) + 2 + 4;  
    uint32_t cacl_checksum = ota_calc_crc32(msg, msg_len - 4);
    if ((msg[msg_len - 4] != ((cacl_checksum >> 24) & 0xFF))
        ||  (msg[msg_len - 3] != ((cacl_checksum >> 16) & 0xFF))
        ||  (msg[msg_len - 2] != ((cacl_checksum >> 8) & 0xFF))
        ||  (msg[msg_len - 1] != ((cacl_checksum >> 0) & 0xFF)) ) {
        hcn_log_error("ota_parse_process crc32 check failed!\n");
        return false;
    }

    return true;
}

static void parse_file_info(uint8_t *msg) {
    if (!is_first_parse) {
        hcn_log_error("parse_file_info is not first parse!\n");
        return;
    }
    ///< 每次接收到文件信息时，都需要重置相关参数
    ota_parse_reset();

    uint16_t msg_size = 2 + ((msg[2] << 8) + msg[3]) + 2 + 4;
    uint16_t file_type_offset = 5;
    uint16_t file_size_offset = 6;
    uint8_t file_type = 0;
    uint32_t file_size = 0;
    uint16_t crc32_offset = msg_size - 4;

next_file_info:
    file_type = msg[file_type_offset];
    file_size = ((msg[file_size_offset] << 24) + 
                        (msg[file_size_offset + 1] << 16) + 
                        (msg[file_size_offset + 2] << 8) + 
                        (msg[file_size_offset + 3]));
    if (file_type >= OTA_SPILDR_FILE && file_type <= OTA_CONFIG_FILE) {
        if (file_size > 0) {
            ota_files[file_type - 1].ota_file_size = file_size;
        }

        ota_param.ota_total_size += file_size;
        uint16_t file_name_size = (msg[file_size_offset + 4] << 8) + 
                                    msg[file_size_offset + 5];
        char name_buff[64] = {0};  
        memcpy(name_buff, &msg[file_size_offset + 6], file_name_size);
        strncpy(ota_files[file_type - 1].file_name, name_buff, \
                    sizeof(ota_files[file_type - 1].file_name) - 1);
        
        hcn_log_info("recv file info: type=%d, size=%d bytes, name=%s\n", 
                        file_type, file_size, ota_files[file_type - 1].file_name);
        
        ///< 文件标识1 + 文件大小4 + 文件名长度2 + 文件名N
        uint16_t offset = 1 + 4 + 2 + file_name_size;
        file_type_offset += offset;
        file_size_offset += offset;

        if ((msg_size > file_type_offset) && (file_type_offset < crc32_offset)) {
            goto next_file_info;
        }

        set_ota_state(TCP_RECV_FILE_INFO);

        hcn_log_info("ota total size: %ld bytes\n", ota_param.ota_total_size);

        is_first_parse = false;
    } else {
        hcn_log_error("unknown ota file type %d!\n", file_type);
    }
}

static void parse_file_steam(uint8_t *msg) {
    if (msg == NULL) {
        hcn_log_error("parse_file_steam msg is NULL!\n");
        return;
    }
    
    xSemaphoreTake(ota_sem, portMAX_DELAY);

    uint8_t file_type = 0;
    int data_len = 0;
    
    if (ota_param.ota_total_size == 0) {
        hcn_log_error("Parse file stream ota totalsize = 0!\n");
        goto exit;
    }

    if (ota_param.ota_buff == NULL) {
        ota_param.ota_buff = pvPortMalloc(FLASH_PRIV_TYPE_BYTE * 2);
        if (ota_param.ota_buff == NULL) {
            hcn_log_error("pvPortMalloc ota buff failed!\n");
            goto exit;
        }

        memset(ota_param.ota_buff, 0, FLASH_PRIV_TYPE_BYTE * 2);
    }

    file_type = msg[5];

    ///< 文件流消息内容：消息类型1 + 文件类型1 + 文件状态1
    data_len = ((msg[2] << 8) + msg[3]) -3; 
    if (file_type >= OTA_SPILDR_FILE && file_type <= OTA_CONFIG_FILE) {
        if (file_type == OTA_UPDATE_FILE && ota_param.is_first_update_file) {
            memcpy((void *)&header, &msg[7], sizeof(header));
            ota_param.is_first_update_file = false;

             ///< 检查OTA文件头是否合法
            if (!is_update_file(&header)) {
                vTaskDelay(pdMS_TO_TICKS(20));
                hcn_log_error("\r\nOTA update file header is invalid!\n");
                goto exit;
            }

            ///< 获取update write flash start
            uint32_t start_addr = get_ota_file_offset(file_type, 1);
            if (start_addr != 0xffffffff) {
                ota_files[file_type - 1].ota_addr_offset = start_addr;
            } else {
                vTaskDelay(pdMS_TO_TICKS(20));
                hcn_log_error("\r\nOTA update wrire file offset invalid!\r\n");
                goto exit;
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            hcn_log_info("OTA update write offset:0x%08x\r\n", \
                            ota_files[file_type - 1].ota_addr_offset);
        }

        memcpy(&ota_param.ota_buff[ota_param.ota_cur_rx_size], &msg[7], data_len);
        ota_param.ota_cur_rx_size += data_len;
        if (ota_param.ota_cur_rx_size >= FLASH_PRIV_TYPE_BYTE) {
            ///< 接收的数据超过4K时，写入flash
            write_update_data(file_type, FLASH_PRIV_TYPE_BYTE);

            ///< 除了4k外还有剩余数据，将剩余数据移动到前面
            if (ota_param.ota_cur_rx_size >= FLASH_PRIV_TYPE_BYTE) {
                memmove(ota_param.ota_buff, ota_param.ota_buff + FLASH_PRIV_TYPE_BYTE,
                        ota_param.ota_cur_rx_size - FLASH_PRIV_TYPE_BYTE);
                ota_param.ota_cur_rx_size -= FLASH_PRIV_TYPE_BYTE;
            }
        } 

        ///< 文件接收完成时，写入flash
        if (msg[6] == 1) {
            if (ota_param.ota_cur_rx_size > 0) {
                write_update_data(file_type, ota_param.ota_cur_rx_size);
                ota_param.ota_cur_rx_size = 0;
                hcn_log_info("ota write file success:%d\r\n", file_type);
            } else {
                hcn_log_info("ota write file success:%d\r\n", file_type);
            }

            if (ota_param.ota_total_size == ota_param.ota_write_size) {
                ///< 接收数据完成，开始检验写入flash中的update头
                if (!check_update_file_header(ota_files[OTA_UPDATE_FILE - 1].ota_addr_offset)) {
                    hcn_log_error("Write wrong update file to flash!\r\n");
                    goto exit;
                }

#ifdef OTA_CRC_ALL_FLASH_ENABLE
                ///< 再检验update全文
                uint32_t checksum = get_update_file_checksum(
                    ota_files[OTA_UPDATE_FILE - 1].ota_addr_offset, 
                    ota_files[OTA_UPDATE_FILE - 1].ota_file_size);
                hcn_log_info("update file calc checksum:0x%08x, header checksum:0x%08x\r\n", \
                        checksum, header.checksum);
                
                if (checksum != header.checksum) {
                    hcn_log_error("Ota update file crc32 check failed!\r\n");
                    send_update_status(HCN_MSG_OTA_STAUS, \
                        ota_param.ota_total_size, \
                        ota_param.ota_write_size, UPDATE_ERROR_CRC);
                    goto exit;
                }
#endif
                if (write_flash_error != 0) {
                    hcn_log_error("Ota write flash has %d errors!\r\n", write_flash_error);
                    goto exit;
                }

                vTaskDelay(pdMS_TO_TICKS(1000));
                ota_param.is_ready_update = true;
                g_calc_checksum = checksum;
                set_ota_state(TCP_SEND_TRANSFER_COMPLETE);
                vTaskDelay(pdMS_TO_TICKS(100));
                start_ota();
            }
            
            hcn_log_info("otaTotalSize:%d, otaWriteSize:%d\n", \
                    ota_param.ota_total_size, ota_param.ota_write_size);
            hcn_log_info("otaFileSize:%d, otaWriteOffset:%d\n", \
                    ota_files[file_type - 1].ota_file_size, 
                    ota_files[file_type - 1].ota_write_offset);
        } else {
            ///< 正在发送
        }
    } else {
        hcn_log_error("unknown ota file type %d!\n", file_type);
    }
exit:
    xSemaphoreGive(ota_sem);
}

static void ota_parse_process(uint8_t *msg) {
    if (msg == NULL) {
        hcn_log_error("ota_parse_process msg is NULL!\n");
        return;
    }

    switch (msg[4]) {
        case TCP_DEVICE_INFO_CMD: {
                if (!ota_check_sum(msg)) {
                    hcn_log_error("recv device info crc32 check failed!\r\n");
                    return;
                }
                uint16_t ack = (msg[5] << 8) | msg[6];
                if (ack == TCP_SUCCESS_ACK) {
                    hcn_log_info("recv device info ack success!\n");
                    set_ota_state(TCP_RECV_DEVICE_ACK);
                } else if (ack == TCP_FAILED_ACK) {
                    hcn_log_error("recv device info ack failed!\n");
                }
            }
            break;

        case TCP_FILE_INFO_CMD:
            if (!ota_check_sum(msg)) {
                hcn_log_error("recv file info crc32 check failed!\n");
                return;
            }
            parse_file_info(msg);
            break;

        case TCP_FILE_STREAM_CMD:
            if (!ota_check_sum(msg)) {
                hcn_log_error("recv file stream crc32 check failed!\n");
                return;
            }
            parse_file_steam(msg);
            break;

        case TCP_TRANSFER_COMPLETE_CMD: {
                if (!ota_check_sum(msg)) {
                    hcn_log_error("recv transfer complete crc32 check failed!\r\n");
                    return;
                }

                uint16_t ack = (msg[5] << 8) + msg[6];
                if (ack == TCP_SUCCESS_ACK) {
                    set_ota_state(TCP_SEND_TRANSFER_COMPLETE_ACK);
                    hcn_log_info("Transfer completer success ack!\r\n");
                } else if (ack == TCP_FAILED_ACK) {
                    hcn_log_error("Transfer complete failed ack!\r\n");
                }
            }
            break;

        case TCP_UPDATE_START_CMD:
            start_ota();
            break;

        case TCP_CONFIG_CMD:
            break;

        default:
            break;
    }
}

static void ota_parse_thread(void *param) {
    uint8_t *msg;
    for (;;) {
        if (xQueueReceive(ota_parse_queue, &msg, portMAX_DELAY) != pdPASS) {
            vTaskDelay(pdMS_TO_TICKS(100));
            hcn_log_error("ota_parse_thread xQueueReceive failed!\n");
            continue;
        }

        ota_parse_process(msg);
        if (msg) {
            vPortFree(msg);
            msg = NULL;
        }
    }   
}

bool get_ota_percent_change(void) {
    return ota_param.ota_percent_change;
}

void set_ota_percent_change(bool change) {
    ota_param.ota_percent_change = change;
}

uint8_t get_ota_percent(void) {
    return ota_param.ota_percent;
}

int ota_task_add(uint8_t *msg, uint16_t len) {
    if (ota_parse_queue == NULL || msg == NULL) {
        hcn_log_error("ota parse queue or msg is NULL!\n");
        return -1;
    }

    uint8_t *data = pvPortMalloc(TCP_OTA_DATA_MAX_LEN);
    if (data == NULL) {
        hcn_log_error("pvPortMalloc ota parse msg failed!\n");
        return -1;
    }

    memset(data, 0, TCP_OTA_DATA_MAX_LEN);
    int data_len = len > TCP_OTA_DATA_MAX_LEN ? TCP_OTA_DATA_MAX_LEN : len;
    memcpy(data, msg, data_len);

    //hcn_log_info("ota_task_add data_len=%d\r\n", data_len);
#ifdef OTA_PARSE_DEBUG_ENABLE
    if (data_len < 128) {
        //hcn_hex_config_data_print("ota", ":recv(0x)", data, data_len);   
    }  
#endif

    if (xQueueSend(ota_parse_queue, &data, pdMS_TO_TICKS(1000)) != pdPASS) {
        hcn_log_error("ota_task_add xQueueSend failed!\n");
        vPortFree(data);
        return -1;
    }

    return 0;
}

int ota_parse_init(void) {
    if (ota_parse_queue) {
        hcn_log_info("ota parse already init!\n");
        if (!is_first_parse) {
            is_first_parse = true;
        }
        return 0; 
    }
    
    if (ota_parse_queue == NULL) {
        ota_parse_queue = xQueueCreate(OTA_PARSE_QUEUE_LEN, sizeof(uint32_t));
        if (ota_parse_queue == NULL) {
            hcn_log_error("create ota parse queue failed!\n");
            return -1;
        }
    }

    if (xTaskCreate(ota_parse_thread, "ota_parse_thread", 
                    configMINIMAL_STACK_SIZE * 2,
                    NULL, configMAX_PRIORITIES / 3 + 2, 
                    NULL) != pdPASS) {
        hcn_log_error("create ota parse thread failed!\n");
        return -1;
    }

    if (ota_sem == NULL) {
        ota_sem = xSemaphoreCreateMutex();
        if (ota_sem == NULL) {
            hcn_log_error("create ota sem failed!\n");
            return -1;
        }
    }

    sflash_ota = sfud_get_device(0);
    if (sflash_ota == NULL) {   
        hcn_log_error("get spi flash device failed!\n");
        return -1;
    }

    ///< 初始赋值
    ota_files[0].ota_addr_offset = OTA_SPILDR_START_ADDR;
    ota_files[1].ota_addr_offset = OTA_STEPLDR_START_ADDR;
    ota_files[2].ota_addr_offset = OTA_UPDATE_START_ADDR;
    ota_files[3].ota_addr_offset = OTA_MCU_START_ADDR;
    is_first_parse = true;
    write_flash_error = 0;

    for (uint8_t i  = 0; i < OTA_ALL_FILE; i++) {
        ota_files[i].ota_file_buff = NULL;
        ota_files[i].ota_file_size = 0;
        ota_files[i].ota_write_offset = 0;
        memset(ota_files[i].file_name, 0, TCP_OTA_FILE_NAME_MAX_LEN);
    }

    hcn_log_info("Ota parse init ok!\r\n");

    return 0;
}

void ota_parse_reset(void) {
    xSemaphoreTake(ota_sem, portMAX_DELAY);

    ota_param.ota_percent = 0;
    ota_param.ota_percent_change = false;
    ota_param.is_ready_update = false;
    ota_param.is_first_update_file = true;
    is_first_parse = true;
    ota_param.ota_total_size = 0;
    ota_param.ota_write_size = 0;
    ota_param.ota_cur_rx_size = 0;
    write_flash_error = 0;

    //set_ota_state(TCP_SEND_DEVICE_INFO);
    set_update_state_reset();
    
    for (uint8_t i  = 0; i < OTA_ALL_FILE; i++) {
        if (ota_files[i].ota_file_buff) {
            vPortFree(ota_files[i].ota_file_buff);
            ota_files[i].ota_file_buff = NULL;
        }
        ota_files[i].ota_file_size = 0;
        ota_files[i].ota_write_offset = 0;
        memset(ota_files[i].file_name, 0, TCP_OTA_FILE_NAME_MAX_LEN);
    }

    xSemaphoreGive(ota_sem);
}

/**
*
* @file hcn_easy_navi.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 10:11
* @author och
*
*/

#include <FreeRTOS.h>
#include "task.h"
#include "queue.h"
#include "timers.h"
#include <string.h>
#include "ff_stdio.h"
#include "log/hcn_log.h"
#include "carlink_cb/hcn_easy_navi.h"
#include "vehicle_param/vehicle_param.h"
#include  "msg_manage/hcn_msg_manage.h"

#define LANE_GUIDANCE_PIC_LENGTH        (50*1024)

//#define HCN_SAVE_NAVI_LANE_GUIDE_PIC_ENABLE

static hcnNavigationHudInfo easy_navi_info;

#ifdef HCN_CARLINK_ROAD_PIC_ENABLE
static char lane_guide_buff[LANE_GUIDANCE_PIC_LENGTH];
static int lane_guide_pic_len = 0;
static uint8_t lan_guide_pic_state = 0;
#endif

#ifdef HCN_SAVE_NAVI_LANE_GUIDE_PIC_ENABLE
static QueueHandle_t navi_lane_guide_queue = NULL;
static FF_FILE * lane_guide_pic_file = NULL;
static uint32_t lane_guide_pic_index = 0;
#endif

#ifdef HCN_SAVE_NAVI_LANE_GUIDE_PIC_ENABLE
static char* lane_guide_pic_malloc_msg(void);
static int lange_guide_pic_task_add(char *pic_data);
#endif

const hcnNavigationHudInfo *get_easy_navi_info(void) {
    return &easy_navi_info;
} 

#ifdef HCN_CARLINK_ROAD_PIC_ENABLE

road_junction_pic_t *get_lane_guidance_pic_info(void) {
    static road_junction_pic_t *lane_guide_info = NULL;
    if (lane_guide_info == NULL) {
        lane_guide_info = pvPortMalloc(sizeof(road_junction_pic_t));
        if (lane_guide_info == NULL) {
            hcn_log_error("get_lane_guidance_pic_info pvPortMalloc failed!\n");
            return NULL;
        }
    }

    lane_guide_info->status = lan_guide_pic_state;
    lane_guide_info->pictureLength = lane_guide_pic_len;
    lane_guide_info->pictureData = lane_guide_buff;

    return lane_guide_info;
}

#endif

void parse_easy_navi_info(const hcnNavigationHudInfo *info) {
    if (info == NULL) {
        return;
    }

    int navi_status = info->status ? 0 : 1;
    memset(&easy_navi_info, 0, sizeof(easy_navi_info));

    easy_navi_info.status = info->status;
    easy_navi_info.naviIcon = info->naviIcon;
    easy_navi_info.destinationRemainingDistance = info->destinationRemainingDistance;
    easy_navi_info.roadRemainingDistance = info->roadRemainingDistance;
    easy_navi_info.signalIntensity = info->signalIntensity;
    snprintf(easy_navi_info.currentRoad,sizeof(easy_navi_info.currentRoad), 
            "%s", info->currentRoad);
    snprintf(easy_navi_info.nextRoad, sizeof(easy_navi_info.nextRoad), 
            "%s", info->nextRoad);

    hcn_log_info("parse_easy_navi_info status:%d, naviIcon:%d, destRemainDist:%d, roadRemainDist:%d, signalIntensity:%d, currentRoad:%s, nextRoad:%s\n",
        easy_navi_info.status,
        easy_navi_info.naviIcon,
        easy_navi_info.destinationRemainingDistance,
        easy_navi_info.roadRemainingDistance,
        easy_navi_info.signalIntensity,
        easy_navi_info.currentRoad,
        easy_navi_info.nextRoad);        
    vehicle_set_data(VEH_EASY_NAV_STATUS, navi_status);		
}

#ifdef HCN_CARLINK_ROAD_PIC_ENABLE

void parse_lane_guidance_pic_info(const road_junction_pic_t *info) {
    if (info == NULL) {
        return;
    }

    hcn_log_info("\r\nparse_lane_guidance_pic_info status:%d, picDataLen:%d\r\n",
        info->status,
        info->pictureLength); 

    lan_guide_pic_state = (uint8_t)info->status;
    if (lan_guide_pic_state) {
        hcn_log_info("\r\nshow lane guidance pic data\r\n");
        memset(lane_guide_buff, 0, LANE_GUIDANCE_PIC_LENGTH);
        if (info->pictureLength < LANE_GUIDANCE_PIC_LENGTH) {
            memcpy(lane_guide_buff, info->pictureData, info->pictureLength);
            lane_guide_pic_len = info->pictureLength;
#ifdef HCN_SAVE_NAVI_LANE_GUIDE_PIC_ENABLE
            char *msg = lane_guide_pic_malloc_msg();
            if (msg == NULL) {
                return;
            }

            *msg = 1;  ///< 表示有新图片数据
            lange_guide_pic_task_add(msg);
#endif
        } else {
            hcn_log_error("lane guidance pic length too large! len:%d\n", info->pictureLength);
        }
    } else {
        lane_guide_pic_len = info->pictureLength;
        hcn_log_info("\r\nhide lane guidance pic data\r\n");
    }
}

void parse_road_junction_pic_info(const road_junction_pic_t *info) {
    if (info == NULL) {
        return;
    }

    ///< 路口放大图片太大了，最大需要2MB内存，亿连如果不做优化，后期不建议在简易导航中使用
    hcn_log_info("\r\nparse_road_junction_pic_info status:%d, format:%d, picDataLen:%d\r\n",
        info->status,
        info->format,
        info->pictureLength); 
    if (info->status) {
        hcn_log_info("\r\nshow road junction pic data\r\n");
    } else {
        hcn_log_info("\r\nhide road junction pic data\r\n");
    }
}

#endif

#ifdef HCN_SAVE_NAVI_LANE_GUIDE_PIC_ENABLE
static char* lane_guide_pic_malloc_msg(void) {
    char *pic_data = pvPortMalloc(sizeof(char*));
    if (pic_data == NULL) {
        hcn_log_error("lane_guide_pic_malloc_msg pvPortMalloc failed!\n");
        return NULL;
    }

    return pic_data;
}

static void lane_guide_pic_free_msg(char *pic_data) {
    if (pic_data) {
        vPortFree(pic_data);
        pic_data = NULL;
    }
}

static int lange_guide_pic_task_add(char *pic_data) {
    if (navi_lane_guide_queue == NULL) {
        hcn_log_error("navi lane guide queue is NULL!\n");
        return -1;
    }

    if (pic_data == NULL) {
        hcn_log_error("lane guide pic data is NULL!\n");
        return -1;
    }

    if (xQueueSend(navi_lane_guide_queue, &pic_data, pdMS_TO_TICKS(100)) != pdPASS) {
        hcn_log_error("lange_guide_pic_task_add xQueueSend failed!\n");
        lane_guide_pic_free_msg(pic_data);
        return -1;
    }

    return 0;
}

static void save_lane_guide_pic_src(char *pic_data) {
    if (hcn_get_usb_status() != USB_STATUS_INSERTED) {
        hcn_log_error("usb not inserted, cannot save lane guide pic!\r\n");
        return;
    }

    if (*pic_data != 1) {
        hcn_log_error("lane guide pic status error!\n");
        return;
    }
  
    if (lane_guide_pic_file == NULL) {
        char file_name[64] = {0};

        if (lane_guide_pic_index > 100) {
            lane_guide_pic_index = 0;
        }

        snprintf(file_name, sizeof(file_name), "/usb/lan_%03d.bin", lane_guide_pic_index);
        lane_guide_pic_index++;  
        lane_guide_pic_file = ff_fopen(file_name, "w+");
        if (lane_guide_pic_file == NULL) {
            hcn_log_error("open lane guide pic file failed!\n");
            return;
        }

        size_t ret = ff_fwrite(lane_guide_buff, 1, lane_guide_pic_len, 
                                lane_guide_pic_file);
        if (ret != lane_guide_pic_len) {
            hcn_log_error("write lane guide pic file failed! ret:%d, len:%d\n", ret, lane_guide_pic_len);
            ff_fclose(lane_guide_pic_file);
            lane_guide_pic_file = NULL;
            lane_guide_pic_len = 0;
            return;
        } 

        ff_fclose(lane_guide_pic_file);
        lane_guide_pic_file = NULL;
        lane_guide_pic_len = 0;

        hcn_log_info("\r\nsave lane guide pic file success! len:%d\r\n", ret);
    }
}

static void lane_guide_pic_parse_task(void *param) {
    char *pic_data;

    for (;;) {
        if (xQueueReceive(navi_lane_guide_queue, &pic_data, portMAX_DELAY) != pdPASS) {
            hcn_log_error("\rlane_guide_pic_parse_task xQueueReceive failed!\r\n");
            continue;
        }

        ///< 处理车道引导图片数据
        save_lane_guide_pic_src(pic_data);
        lane_guide_pic_free_msg(pic_data);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static int lan_guide_pic_task_init(void) {
    navi_lane_guide_queue = xQueueCreate(10, sizeof(char *));
    if (navi_lane_guide_queue == NULL) {
        hcn_log_error("navi lane guide queue create failed!\n");
        return -1;
    }

    if (xTaskCreate(lane_guide_pic_parse_task, "lane_guide_pic_parse_task",
                    configMINIMAL_STACK_SIZE * 10, NULL,
                    configMAX_PRIORITIES / 3, NULL) != pdPASS) {
        hcn_log_error("Create lane guide pic parse task failed!\n");
        vQueueDelete(navi_lane_guide_queue);
        navi_lane_guide_queue = NULL;
        return -1;
    }

    return 0;
}

#endif

int carlink_easy_navi_init(void) {
#ifdef HCN_SAVE_NAVI_LANE_GUIDE_PIC_ENABLE
    if (lan_guide_pic_task_init() != 0) {
        hcn_log_error("carlink_easy_navi_init lane guide pic task init failed!\n");
        return -1;
    }
#endif
      
    return 0;
}

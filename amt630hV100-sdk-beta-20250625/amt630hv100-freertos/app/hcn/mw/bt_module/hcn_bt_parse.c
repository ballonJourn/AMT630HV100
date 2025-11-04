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

#include <string.h>
#include <FreeRTOS.h>
#include "queue.h"
#include "task.h"
#include "timers.h"
#include "console.h"
#include "bt_module/hcn_bt_parse.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"
#include "config/hcn_config.h"
#include "carlink_cb/hcn_carlink_cb.h"
#include "carlink_cb/hcn_carlink_provide.h"
#include "vehicle_param/vehicle_param.h"
#include "board.h"

//#define BT_STR_DEBUG 
#define DOWNLOAD_TIMER_PERIOD_MS   (3000)
#define BT_TASK_QUEUE_LENGTH       (10)

static QueueHandle_t bt_task_queue = NULL;
static bt_phone_book_t *bt_phonebook = NULL;

static bt_data_t g_bt_data = {0};
static bt_call_t g_bt_call = {0};
static bt_music_info_t music_info = {0};

static char g_bt_version[32] = {0};
static int g_pb_count = 0;
static uint32_t last_switch_tick = 0;

static int bt_send_cmd(const char *buff) {
    char buffer[UART_BT_SEND_BUF_LEN] = {0};

    uint16_t len = (uint16_t)strlen(buff);
    if (len > (UART_BT_SEND_BUF_LEN - 2)) {
        hcn_log_error("The command send is too long!\r\n");
        return -1;
    }

    memcpy(buffer, buff, len);
    buffer[len] = 0x0D;
    buffer[len + 1] = 0x0A;

    hcn_log_info("bt send cmd buff:%s\r\n", buff);

    return console_send_atcmd(buffer, len + 2);
}

static void hcn_bt_switch(void) {
    if (xTaskGetTickCount() - last_switch_tick > pdMS_TO_TICKS(500)) {
        last_switch_tick = xTaskGetTickCount();
        if (g_bt_data.btSwitchState == 1) {
            ///< 当前是打开状态，就关闭
            bt_send_cmd("AT+PAGE=0");
        } else if (g_bt_data.btSwitchState == 0) {
            ///< 当前是关闭状态，就打开
             bt_send_cmd("AT+PAGE=1");
        }
    } 
}

void hcn_bt_switch_state(bool state) {
    if (g_bt_data.btSwitchState != state) {
        hcn_bt_switch();
    }
}

void hcn_bt_download_book() {
    static uint32_t timer = 0;
    if ((g_bt_data.btConnected == 1) && (xTaskGetTickCount() - timer > 5000)) {
        char buff[64] = {0};
        hcn_log_info("\r\n start download phone book!\r\n");
        snprintf(buff, sizeof(buff), "AT+PBDOWN=1,%d", BT_PHONE_BOOK_MAX_NUM);
        bt_send_cmd(buff);
        timer = xTaskGetTickCount();
    }
}

void hcn_bt_pick_up() {
    if (g_bt_call.btHfpState == INCOMING_CALL) {
        bt_send_cmd("AT+HFPANSW");
    }
}

void hcn_bt_hung_up() {
    if (g_bt_call.btHfpState >= OUTGOING_CALL) {
        bt_send_cmd("AT+HFPCHUP");
    }   
}

bool hcn_bt_is_Call() {
    return (g_bt_call.btHfpState >= OUTGOING_CALL ? true : false);
}

const bt_call_t* hcn_bt_get_call() {
    return &g_bt_call;
}

const bt_data_t* hcn_bt_get_data() {
    return &g_bt_data;
}

const bt_music_info_t* hcn_bt_get_music_data() {
    return &music_info;
}

void hcn_send_music_cmd(bt_music_cmd_e cmd) {
    if (g_bt_data.btConnected == 0) {
        return;
    }

    switch (cmd) {
        case BT_MUSIC_CMD_PLAYPAUSE:
            bt_send_cmd("AT+PLAYPAUSE");
            break;
        case BT_MUSIC_CMD_PLAY:
            bt_send_cmd("AT+PLAY");
            break;

        case BT_MUSIC_CMD_PAUSE:
            bt_send_cmd("AT+PAUSE");
            break;

        case BT_MUSIC_CMD_STOP:
            bt_send_cmd("AT+STOP");
            break;

        case BT_MUSIC_CMD_FORWARD:
            bt_send_cmd("AT+FORWARD");
            break;

        case BT_MUSIC_CMD_BACKWARD:
            bt_send_cmd("AT+BACKWARD");
            break;

        case BT_MUSIC_CMD_REPEAT:
            bt_send_cmd("AT+REPEAT");
            break;

        default:
            break;
    }
  
}

const char* hcn_bt_get_name() {
    static char name[32] = {0};

    extern bool carlink_ble_mac_addr_is_ready();
    if (carlink_ble_mac_addr_is_ready()) {
        return g_bt_data.btDevName;
    } else {
        snprintf(name, sizeof(name), "%s-NONE", HCN_CUSTOMER_NAME);
        return name;
    }
}

const char *hcn_get_bt_version(void) {
    return g_bt_version;
}

static void clean_bt_phone_book(void) {
    g_pb_count = 0;
    hcn_log_info("\r\nClean phone book!\r\n");
    if (g_bt_data.btBookCount > 0) {
        g_bt_data.btBookCount = 0;
        if (bt_phonebook) {
            vPortFree(bt_phonebook);
            bt_phonebook = NULL;
        }

        if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
            get_hcn_callback()->onHcnBtChange(VEH_BT_PHONEBOOK_COUNT, 
                                    (uint32_t)g_bt_data.btBookCount);
        }
    }
}

static void find_name_contact(char *call1, char *call2) {
    if (call1 && (strcmp(g_bt_call.btCallNumber1, call1) != 0)) {
        ///< 如果号码有变化，遍历电话本，查看是否有匹配的号码，查找到联系人
        if (bt_phonebook && (g_bt_data.btPbState != PB_STATE_DOWNDING)) {
            for (int i = 0; i < g_bt_data.btBookCount; i++) {
                if (strcmp(bt_phonebook[i].number, call1) == 0) {
                    strcpy(g_bt_call.btCallPerson1, bt_phonebook[i].name);
                    break;
                }
            }
        }

        strncpy(g_bt_call.btCallNumber1, call1, 
                strlen(call1) < TEXT_PARAM_LEN ? \
                strlen(call1) : TEXT_PARAM_LEN - 1);
    }

    if (call2 && (strcmp(g_bt_call.btCallNumber1, call2) != 0)) {
        ///< 如果号码有变化，遍历电话本，查看是否有匹配的号码，查找到联系人
        if (bt_phonebook && (g_bt_data.btPbState != PB_STATE_DOWNDING)) {
            for (int i = 0; i < g_bt_data.btBookCount; i++) {
                if (strcmp(bt_phonebook[i].number, call2) == 0) {
                    strcpy(g_bt_call.btCallPerson2, bt_phonebook[i].name);
                    break;
                }
            }
        }

         strncpy(g_bt_call.btCallNumber2, call2, 
                strlen(call2) < TEXT_PARAM_LEN ? \
                strlen(call2) : TEXT_PARAM_LEN - 1);
    }
}

static void on_bt_dev_state_change(const char *state_str) {
    if (state_str == NULL) {
        hcn_log_error("\r\nbt state_str is NULL!\r\n");
        return;
    }

    int dev_state = atoi(state_str);
    g_bt_data.btDevState = dev_state;

    if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
            get_hcn_callback()->onHcnBtChange(VEH_BT_DEV_STATE, 
                                    g_bt_data.btDevState);
    }

    hcn_log_info("bt device state change:%d\r\n", dev_state);
}

static void on_phone_book_num(int count) {
    ///< +PBCNT 解析电话本条目数，count是条目数，一个条目下面可能有多个子条目，加上50
    int pb_count = count + 50;
    if (pb_count > BT_PHONE_BOOK_MAX_NUM) {
        pb_count = BT_PHONE_BOOK_MAX_NUM;
    }

    g_bt_data.btBookCount = 0;

    hcn_log_info("\r\npbcnt:%d, pb_count:%d\r\n", count, pb_count);
    if (pb_count != g_pb_count) {
        if (bt_phonebook) {
            vPortFree(bt_phonebook); 
            bt_phonebook = NULL;
        }

        g_pb_count = pb_count;
        bt_phonebook = pvPortMalloc(sizeof(bt_phone_book_t)*g_pb_count);
        if (bt_phonebook == NULL) {
            hcn_log_error("\r\npvPortMalloc phone book num failed!\r\n");
            return;
        }
    }

    if (bt_phonebook) {
        memset(bt_phonebook, 0, sizeof(bt_phone_book_t) * g_pb_count);
    }
}

static void on_phone_book_proc(char (*data)[TEXT_PARAM_LEN], int param_count) {
    if (param_count > 1 && g_bt_data.btConnected) {
        if (strcmp(data[0], "E") == 0) {
            if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
                get_hcn_callback()->onHcnBtChange(VEH_BT_PHONEBOOK_COUNT, 
                                        (uint32_t)g_bt_data.btBookCount);
            }
            hcn_log_info("\r\n bt phone book num:%d\r\n", g_bt_data.btBookCount);
            return;
        }

        ///< 保存电话本
        if (g_bt_data.btBookCount < g_pb_count) {
            if (bt_phonebook) {
                strncpy(bt_phonebook[g_bt_data.btBookCount].name, data[1], 
                        strlen(data[1]) < TEXT_PARAM_LEN ? \
                        strlen(data[1]) : TEXT_PARAM_LEN - 1);
                strncpy(bt_phonebook[g_bt_data.btBookCount].number, data[2], 
                        strlen(data[2]) < TEXT_PARAM_LEN ? \
                        strlen(data[2]) : TEXT_PARAM_LEN - 1);
            }
            g_bt_data.btBookCount++;
        }
    }
}

static void on_bt_hfp_state_proc(char (*state)[TEXT_PARAM_LEN], 
                                uint16_t param_count) {
    ///< +HFPSTAT= 设备地址,4,10086
    hfp_state_e hfp_state =  param_count > 1 ? atoi(state[1]) : atoi(state[0]);

    hcn_log_info("\r\nhfp state is = %d\r\n", hfp_state);

    uint8_t bt_connect = hfp_state >= CONNECTED ? 1 : 0;
    if (bt_connect != g_bt_data.btConnected) {
        g_bt_data.btConnected = bt_connect;

        if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
            get_hcn_callback()->onHcnBtChange(VEH_BT_CONNECTED_STATUS, 
                                    (uint32_t)g_bt_data.btConnected);
        }

        if (bt_connect) {
            ///< 蓝牙连接成功以后，开始下载电话本
            hcn_bt_download_book();
        } else {
            if (g_bt_data.btBookCount > 0) {
                clean_bt_phone_book();
            }
        }
    }
    
    if (g_bt_call.btHfpState != hfp_state) {
        if ((hfp_state <= CONNECTED) && (g_bt_call.btHfpState > CONNECTED)) {
            g_bt_call.btHfpState = hfp_state;
            memset(g_bt_call.btCallNumber1, 0, sizeof(g_bt_call.btCallNumber1));
            memset(g_bt_call.btCallPerson1, 0, sizeof(g_bt_call.btCallPerson1));
        }

        ///< 过滤掉微信电话状态，防止电话信息显示在主界面
        if (!strstr(state[2], "000000")) {
            g_bt_call.btHfpState = hfp_state;
            if (param_count > 3) {
                ///< 三方通话，解析电话号码
                find_name_contact(state[1], state[2]);
            } else if (param_count > 2) {
                ///< 通话，解析号码
                find_name_contact(state[2], NULL);
            }
        }

        if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
            get_hcn_callback()->onHcnBtChange(VEH_BT_CALL_STATE, 
                                    g_bt_call.btHfpState);
        }
    }
} 

static void on_cur_music_play_state(char *param_str) {
    if (param_str) {
        char* token;
        int numbers[3] = {0};
        int count = 0;
        
        ///< 使用strtok分割字符串
        token = strtok(param_str, ",");
        while (token != NULL && count < 3) {
            numbers[count++] = atoi(token);
            token = strtok(NULL, ",");
        }

        music_info.music.cur_track_state = numbers[0];
        music_info.music.cur_time_music_play = (uint16_t)numbers[1];
        music_info.music.music_total_time = (uint16_t)numbers[2];
        hcn_log_info("cur music info, state:%d, cur times = %d, total time = %d\r\n", music_info.music.cur_track_state,
        music_info.music.cur_time_music_play, music_info.music.music_total_time);
    }
}

static void on_music_play_mode(char *param_str) {
    if (param_str) {
        char* token;
        int numbers[2] = {0};
        int count = 0;
        
        ///< 使用strtok分割字符串
        token = strtok(param_str, ",");
        while (token != NULL && count < 2) {
            numbers[count++] = atoi(token);
            token = strtok(NULL, ",");
        }

        hcn_log_info("repeat_mode:%d; single_repeat_mode:%d\r\n", numbers[0], numbers[1]);
    }
}

static void on_music_tracks_info(char (*state)[TEXT_PARAM_LEN], 
                                uint16_t param_count) {
    
     if (state && param_count > 2) {
        memset(music_info.title, 0, sizeof(music_info.title));
        memset(music_info.artist, 0, sizeof(music_info.artist));
        memset(music_info.album, 0, sizeof(music_info.album));

        snprintf(music_info.title, 
                sizeof(music_info.title), "%s", state[0]);
        snprintf(music_info.artist, 
                sizeof(music_info.artist), "%s", state[1]);    
        snprintf(music_info.album, 
                sizeof(music_info.album), "%s", state[2]);     
     }                               
}

static void on_bt_str_parse(char *at_str) {
    if (at_str == NULL) {
        hcn_log_error("\r\nbt at_str is NULL!\r\n");
        return;
    }

    ///< 无对应回复格式的AT指令直接丢弃
    if (strchr(at_str, '=') == NULL) {
        hcn_log_error("\r\nbt at_str format error!\r\n");
        return;
    }

#ifdef BT_STR_DEBUG
    hcn_log_info("bt str:%s", at_str);
#endif

    char prama_data[UART_BT_MSG_MAX_LEN] = {0};
    char cmd_str[20] = {0};

    strcpy(prama_data, strchr(at_str, '=') + 1);
    int pos = strchr(at_str, '=') - (strchr(at_str, '+'));

    char *temp_data = strchr(at_str, '+');
    if (substring(cmd_str, temp_data, 0, pos) == NULL) {
        hcn_log_error("\r\nbt substring cmd_str failed!\r\n");
        return;
    }

#if 0
    printf("\r\npos:%d, cmd_str:%s, prama_data:%s\r\n", pos, cmd_str, prama_data);
#endif

    if (strstr(cmd_str, "+PAGE")) {
        g_bt_data.btSwitchState = (uint8_t)atoi(prama_data);
        hcn_log_info("\r\nbt switch state:%d\r\n", g_bt_data.btSwitchState);

        if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
            get_hcn_callback()->onHcnBtChange(VEH_BT_SWITCH_STATUS, 
                                    (uint32_t) g_bt_data.btSwitchState);
        }
    } else if (strstr(cmd_str, "+DEVSTAT")) {
        on_bt_dev_state_change(prama_data);
    } else if (strstr(cmd_str, "+PBCNT")) {
        on_phone_book_num(atoi(prama_data));
    } else if (strstr(cmd_str, "+PWRSTAT")) {
        g_bt_data.btPowerState = (uint8_t)atoi(prama_data);
    } else if (strstr(cmd_str, "+PIN")) {
        memcpy(g_bt_data.btDevPin, prama_data, strlen(prama_data)\
         < TEXT_PARAM_LEN ? strlen(prama_data) : TEXT_PARAM_LEN);
    } else if (strstr(cmd_str, "+HFPSIG")) {
        int signal =  atoi(prama_data);
        if (signal >= 0 && signal <= 5 && g_bt_data.btSignal != signal) {
            g_bt_data.btSignal = signal;
            if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
                get_hcn_callback()->onHcnBtChange(VEH_BT_PHONE_SIGNAL, 
                                        (uint8_t)g_bt_data.btSignal);
            }       
        }
    } else if (strstr(cmd_str, "+HFPBATT")) {
        int battery = atoi(prama_data);
        if (battery >= 0 && battery <= 5 && 
            g_bt_data.btBatteryLevel != battery) {
            g_bt_data.btBatteryLevel = battery; 

            if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
                get_hcn_callback()->onHcnBtChange(VEH_BT_PHONE_BATTERY, 
                                        (uint8_t)g_bt_data.btBatteryLevel);
            }          
        }
    } else if (strstr(cmd_str, "+HFPIBR")) {
        g_bt_data.btHfpIBR = (uint8_t)atoi(prama_data);
    } else if (strstr(cmd_str, "+HFPAUDIO")) {
        uint8_t audio_state = atoi(prama_data);
    } else if (strstr(cmd_str, "+A2DPSTAT")) {
        g_bt_data.btA2dpState = (uint8_t)atoi(prama_data);
    } else if (strstr(cmd_str, "+PBSTAT")) {
        g_bt_data.btPbState = (uint8_t)atoi(prama_data);
        if (get_hcn_callback() && get_hcn_callback()->onHcnBtChange) {
            get_hcn_callback()->onHcnBtChange(VEH_BT_PHONEBOOK_STATE, 
                                    (uint8_t) g_bt_data.btPbState);
        }  
    } else if (strstr(cmd_str, "+PLAYSTAT")) {
        music_info.play_state = atoi(prama_data);
        hcn_log_info("\r\naudio paly state:%d\r\n", music_info.play_state);
        if (music_info.play_state == BT_MUSIC_PLAY_STATE_PLAYING) {
            hcn_log_info("Music start play....\r\n");
            vTaskDelay(pdMS_TO_TICKS(5));
            aw_pa_start();
        } else if (music_info.play_state == \
                    BT_MUSIC_PLAY_STATE_PAUSED) {
            vTaskDelay(pdMS_TO_TICKS(5));
            hcn_log_info("Music stop play....\r\n");
            aw_pa_stop();
        }
    }  else if (strstr(cmd_str, "+TRACKSTAT")) {
        on_cur_music_play_state(prama_data);
    } else if (strstr(cmd_str, "+PLAYMODE")) {
        on_music_play_mode(prama_data);
    } else {
        char param_array[UART_BT_PARAM_NUMBER][TEXT_PARAM_LEN] = {0};
        char delimit[2] = {0xFF, '\0'};

        uint16_t param_count = string_split(prama_data, delimit, param_array,
                                sizeof(param_array) / sizeof(param_array[0]));
        if (strstr(cmd_str, "+HFPSTAT")) {
            on_bt_hfp_state_proc(param_array, param_count);
        } else if (strstr(cmd_str, "+VER")) {
            if (param_count == 1) {
                memset(g_bt_version, 0, sizeof(g_bt_version));
                snprintf(g_bt_version, sizeof(g_bt_version), "%s", param_array[0]);
            }
            bt_send_cmd("AT+ADDR");
        } else if (strstr(cmd_str, "+NAME")) {
            static bool is_firsend = true;
            hcn_log_info("\r\n bt name;%s\r\n ", param_array[0]);
            if (strstr(param_array[0], HCN_CUSTOMER_NAME)) {
                memcpy(g_bt_data.btDevName, param_array[0], \
                strlen(param_array[0]) < TEXT_PARAM_LEN ? \
                strlen(param_array[0]) : TEXT_PARAM_LEN);
            }

            if (is_firsend) {
                is_firsend = false;
                bt_send_cmd("AT+LEADDR");
            }
        } else if (strstr(cmd_str, "+LENAME")) {

        } else if (strstr(cmd_str, "+PBDATA")) {
            on_phone_book_proc(param_array, param_count);
        } else if (strstr(cmd_str, "+HFPDEV")) {
            memcpy(g_bt_data.btHfpAddr, param_array[1], \
                strlen(param_array[0]) < TEXT_PARAM_LEN ? \
                strlen(param_array[0]) : TEXT_PARAM_LEN);
            memcpy(g_bt_data.btConnectDevName, param_array[1], \
                strlen(param_array[1]) < BT_CONNECT_DEV_NAME_LEN ? \
                strlen(param_array[1]) : BT_CONNECT_DEV_NAME_LEN);
        } else if (strstr(cmd_str, "+TRACKINFO")) {
            on_music_tracks_info(param_array, param_count);
        } 
     }
}

static void bt_msg_parse_task(void *param) {
    char *bt_msg;

    for (;;) {
        if (xQueueReceive(bt_task_queue, &bt_msg, portMAX_DELAY) != pdPASS) {
            hcn_log_error("\r\nbt_msg_parse_task xQueueReceive failed!\r\n");
            continue;
        }

        on_bt_str_parse(bt_msg);
        vPortFree(bt_msg);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

int bt_msg_task_add(char *bt_msg, uint16_t len) {
    if (bt_task_queue == NULL) {
        hcn_log_error("bt task queue is NULL!\n");
        return -1;
    }

    char *msg = pvPortMalloc(len + 1);
    if (msg == NULL) {
        hcn_log_error("pvPortMalloc bt msg failed!\n");
        return -1;
    }

    memset(msg, 0, len + 1);
    memcpy(msg, bt_msg, len);
    msg[len] = '\0';

    if (xQueueSend(bt_task_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        hcn_log_error("bt_msg_task_add xQueueSend failed!\n");
        vPortFree(msg);
        return -1;
    }

    return 0;
}

int bt_module_init(void) {
    if (bt_task_queue) {
        hcn_log_info("bt module has initialized");
        return 0;
    }

    if (bt_task_queue == NULL) {
        bt_task_queue = xQueueCreate(BT_TASK_QUEUE_LENGTH, sizeof(char*));
        if (bt_task_queue == NULL) {
            hcn_log_error("Failed to create BT task queue");
            return -1;
        }
    }

    if (xTaskCreate(bt_msg_parse_task, "bt_msg_parse_task", 
                    configMINIMAL_STACK_SIZE * 4, NULL, 
                    configMAX_PRIORITIES / 2, NULL) != pdPASS) {
        vQueueDelete(bt_task_queue);
        bt_task_queue = NULL;

        hcn_log_error("Failed to create BT message task\n");
        return -1;
    }
    
    hcn_log_info("\r\nbt module init ok!\r\n");

    return 0;
}

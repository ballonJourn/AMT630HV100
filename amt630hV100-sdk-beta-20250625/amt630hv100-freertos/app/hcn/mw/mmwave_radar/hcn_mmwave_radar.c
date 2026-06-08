/**
*
* @file hcn_mmwave_radar.c
*
* @brief 毫米波雷达模块实现
*        协议参考：毫米波雷达串口通信协议V08_8_4
*        - 预警帧: 0x80 0x05 (12B) + 0xA0 0x04 (12B) 连续上传，间隔100ms
*        - 目标帧: 0xAA 0x55 (22B) 间隔100ms
*        - 通用命令帧: 0xDF 0x07 (12B) 发送 / 0x60 0x07 (12B) 回复
*
* @ingroup mmwave_radar
*
* @date	2026/05/27
* @author yunlong
*
*/

#include <FreeRTOS.h>
#include <string.h>
#include "mmwave_radar/hcn_mmwave_radar.h"
#include "storage_param1/hcn_usr_param.h"
#include "vehicle_param/vehicle_param.h"
#include "log/hcn_log.h"
#include "utils/hcn_utils.h"
#include "chip.h"
#include "board.h"
#include "uart.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#ifdef HCN_MMWAVE_RADAR_ENABLE

/*=============================================================================
 * 宏定义
 *===========================================================================*/

#define RADAR_TAG                   "Radar"
#define RADAR_ONLINE_TIMEOUT_MS     (3000)   ///< 3秒无数据视为离线
#define RADAR_VEH_SPEED_INTERVAL_MS (250)    ///< 车速输入间隔

#define RADAR_DEBUG_ENABLE                 ///< 调试打印使能(调试中)

/*=============================================================================
 * 静态变量
 *===========================================================================*/

static radar_warn_data_t   g_warn_data   = {0};
static radar_target_data_t g_target_data = {0};
static volatile bool       g_radar_online = false;
static volatile uint32_t   g_last_rx_tick = 0;

static QueueHandle_t       g_tx_queue = NULL;
// static UartPort_t      *g_radar_uap = NULL;   ///< 保留供后续发送扩展
static TimerHandle_t       g_online_timer = NULL;
static TimerHandle_t       g_veh_speed_timer = NULL;

/*=============================================================================
 * 调试用: 车速/BSD速度覆盖
 * g_dbg_speed_override >= 0 时，用此值替代真实车速发送给雷达 (单位 km/h)
 * g_dbg_bsd_speed >= 0 时，设置BSD启动速度 (单位 km/h)
 *===========================================================================*/
static volatile int32_t g_dbg_speed_override = -1; ///< <0表示不覆盖，>=0为覆盖车速
static volatile int32_t g_dbg_bsd_speed      = -1; ///< <0表示不设置

/**
 * @brief  调试接口: 设置虚拟车速 (km/h), 传 -1 取消覆盖恢复真实车速
 */
void mmwave_radar_dbg_set_speed(int32_t speed_kmh) {
    g_dbg_speed_override = speed_kmh;
    printf("Radar DBG: speed override = %d km/h (%s)\n",
           (int)speed_kmh, speed_kmh < 0 ? "OFF" : "ON");
}

/**
 * @brief  调试接口: 设置BSD启动速度 (km/h), 传 -1 不操作
 */
void mmwave_radar_dbg_set_bsd_speed(int32_t speed_kmh) {
    if (speed_kmh >= 0) {
        g_dbg_bsd_speed = speed_kmh;
        mmwave_radar_set_bsd_speed((uint8_t)speed_kmh);
        printf("Radar DBG: BSD start speed set to %d km/h\n", (int)speed_kmh);
    }
}

/*=============================================================================
 * 校验和计算
 *===========================================================================*/

/**
 * @brief 计算数据域校验和 (LSB, 16位)
 *        对数据域字节求和，结果低字节在前
 */
static uint16_t radar_calc_checksum(const uint8_t *data, int offset, int len) {
    uint16_t sum = 0;
    for (int i = offset; i < offset + len; i++) {
        sum += data[i];
    }
    return sum;
}

/*=============================================================================
 * 预警帧解析
 *===========================================================================*/

/**
 * @brief 解析预警信息 Part1 (帧头 0x80 0x05)
 *        data[0]=0x80, data[1]=0x05
 *        data[2]=0x07(len), data[3]=0x11(SID), data[4]=0x10(ADDR)
 *        data[5]=预警标志位, data[6]=预留, data[7]=BSD启动车速
 *        data[8]=当前车速, data[9]=预留
 *        data[10]=chkL, data[11]=chkH
 */
static bool radar_parse_warn_part1(const uint8_t *data) {
    uint16_t chk_calc = radar_calc_checksum(data, 2, 8);
    uint16_t chk_recv = (uint16_t)(data[10]) | ((uint16_t)(data[11]) << 8);

    if (chk_calc != chk_recv) {
        hcn_log_error("%s: warn P1 checksum err, calc=0x%04X recv=0x%04X\n",
                      RADAR_TAG, chk_calc, chk_recv);
        return false;
    }

    g_warn_data.warn_flags      = data[5];
    g_warn_data.bsd_start_speed = data[7];
    g_warn_data.current_speed   = data[8];

    return true;
}

/**
 * @brief 解析预警信息 Part2 (帧头 0xA0 0x04)
 *        data[0]=0xA0, data[1]=0x04
 *        data[2]=左侧距离, data[3]=右侧距离, data[4]=后侧距离
 *        data[5]=左侧速度, data[6]=右侧速度, data[7]=后侧速度
 *        data[8], data[9]=预留
 *        data[10]=chkL, data[11]=chkH
 */
static bool radar_parse_warn_part2(const uint8_t *data) {
    uint16_t chk_calc = radar_calc_checksum(data, 2, 8);
    uint16_t chk_recv = (uint16_t)(data[10]) | ((uint16_t)(data[11]) << 8);

    if (chk_calc != chk_recv) {
        hcn_log_error("%s: warn P2 checksum err, calc=0x%04X recv=0x%04X\n",
                      RADAR_TAG, chk_calc, chk_recv);
        return false;
    }

    g_warn_data.left_distance  = data[2];
    g_warn_data.right_distance = data[3];
    g_warn_data.rear_distance  = data[4];
    g_warn_data.left_speed     = data[5];
    g_warn_data.right_speed    = data[6];
    g_warn_data.rear_speed     = data[7];

    return true;
}

/**
 * @brief 处理完整的预警帧 (24字节)，更新 vehicle_param
 */
static void radar_update_warn_to_vehicle(void) {
    ///< BSD预警状态
    vehicle_set_data(VEH_RADAR_BSD_LEFT,
                     (g_warn_data.warn_flags & RADAR_WARN_BSD_LEFT) ? 1 : 0);
    vehicle_set_data(VEH_RADAR_BSD_RIGHT,
                     (g_warn_data.warn_flags & RADAR_WARN_BSD_RIGHT) ? 1 : 0);

    ///< CVW预警状态
    vehicle_set_data(VEH_RADAR_CVW_LEFT,
                     (g_warn_data.warn_flags & RADAR_WARN_CVW_LEFT) ? 1 : 0);
    vehicle_set_data(VEH_RADAR_CVW_RIGHT,
                     (g_warn_data.warn_flags & RADAR_WARN_CVW_RIGHT) ? 1 : 0);

    ///< RCW预警状态
    vehicle_set_data(VEH_RADAR_RCW,
                     (g_warn_data.warn_flags & RADAR_WARN_RCW) ? 1 : 0);

    ///< 预警目标距离/速度
    vehicle_set_data(VEH_RADAR_LEFT_DISTANCE,  (int)g_warn_data.left_distance);
    vehicle_set_data(VEH_RADAR_RIGHT_DISTANCE, (int)g_warn_data.right_distance);
    vehicle_set_data(VEH_RADAR_REAR_DISTANCE,  (int)g_warn_data.rear_distance);
    vehicle_set_data(VEH_RADAR_LEFT_SPEED,     (int)g_warn_data.left_speed);
    vehicle_set_data(VEH_RADAR_RIGHT_SPEED,    (int)g_warn_data.right_speed);
    vehicle_set_data(VEH_RADAR_REAR_SPEED,     (int)g_warn_data.rear_speed);

    ///< 车速/BSD启动速度
    vehicle_set_data(VEH_RADAR_CURRENT_SPEED,    (int)g_warn_data.current_speed);
    vehicle_set_data(VEH_RADAR_BSD_START_SPEED,  (int)g_warn_data.bsd_start_speed);

    ///< 在线状态
    vehicle_set_data(VEH_RADAR_STATUS, 1);

#ifdef RADAR_DEBUG_ENABLE
    hcn_log_info("%s: warn=0x%02X spd=%d L=%dm R=%dm B=%dm\n",
                 RADAR_TAG, g_warn_data.warn_flags,
                 g_warn_data.current_speed,
                 g_warn_data.left_distance,
                 g_warn_data.right_distance,
                 g_warn_data.rear_distance);
#endif
}

/*=============================================================================
 * 目标帧解析
 *===========================================================================*/

/**
 * @brief 解析目标信息帧 (帧头 0xAA 0x55, 22字节)
 *        data[0]=0xAA, data[1]=0x55
 *        data[2]=目标数量(最大3)
 *        data[3..8]=目标1 (X_H,X_L, Y_H,Y_L, V_H,V_L) MSB
 *        data[9..14]=目标2
 *        data[15..20]=目标3
 *        data[21..22] 不对，总长22字节: 2(头)+1(num)+3*6(目标)+2(校验)=23
 *        实际: data[20]=chkL, data[21]=chkH
 */
static bool radar_parse_target_frame(const uint8_t *data, int len) {
    ///< 校验: 从data[2]到data[len-3]求和
    int payload_len = len - 2 - 2; ///< 去掉帧头2字节和校验2字节
    uint16_t chk_calc = radar_calc_checksum(data, 2, payload_len);
    uint16_t chk_recv = (uint16_t)(data[len - 2]) |
                        ((uint16_t)(data[len - 1]) << 8);

    if (chk_calc != chk_recv) {
        hcn_log_error("%s: target checksum err, calc=0x%04X recv=0x%04X\n",
                      RADAR_TAG, chk_calc, chk_recv);
        return false;
    }

    uint8_t num = data[2];
    if (num > RADAR_TARGET_MAX_NUM) {
        num = RADAR_TARGET_MAX_NUM;
    }

    g_target_data.target_num = num;

    for (int i = 0; i < RADAR_TARGET_MAX_NUM; i++) {
        int offset = 3 + i * 6;
        g_target_data.targets[i].x     = (int16_t)((data[offset] << 8) |
                                                     data[offset + 1]);
        g_target_data.targets[i].y     = (int16_t)((data[offset + 2] << 8) |
                                                     data[offset + 3]);
        g_target_data.targets[i].speed = (int16_t)((data[offset + 4] << 8) |
                                                     data[offset + 5]);
    }

#ifdef RADAR_DEBUG_ENABLE
    hcn_log_info("%s: targets=%d", RADAR_TAG, num);
    for (int i = 0; i < num; i++) {
        hcn_log_info(" T%d(x=%d y=%d v=%d)", i,
                     g_target_data.targets[i].x,
                     g_target_data.targets[i].y,
                     g_target_data.targets[i].speed);
    }
    hcn_log_info("\n");
#endif

    return true;
}

/*=============================================================================
 * 通用命令回复帧解析
 *===========================================================================*/

/**
 * @brief 解析通用回复帧 (帧头 0x60 0x07, 12字节)
 */
static void radar_parse_cmd_response(const uint8_t *data) {
    uint8_t sid = data[3];
    uint8_t did = data[5];

    uint16_t chk_calc = radar_calc_checksum(data, 2, 8);
    uint16_t chk_recv = (uint16_t)(data[10]) | ((uint16_t)(data[11]) << 8);

    if (chk_calc != chk_recv) {
        hcn_log_error("%s: cmd resp checksum err\n", RADAR_TAG);
        return;
    }

    switch (did) {
        case RADAR_DID_VERSION:
            if (sid == RADAR_SID_READ_RESP) {
                ///< 版本号在 data[6..9], ASCII
                char ver[5] = {0};
                memcpy(ver, &data[6], 4);
                hcn_log_info("%s: FW version=%s\n", RADAR_TAG, ver);
            }
            break;

        case RADAR_DID_BSD_START_SPEED:
            if (sid == RADAR_SID_READ_RESP) {
                hcn_log_info("%s: BSD start speed=%d km/h\n",
                             RADAR_TAG, data[6]);
            } else if (sid == RADAR_SID_WRITE_RESP) {
                hcn_log_info("%s: BSD start speed set OK\n", RADAR_TAG);
            }
            break;

        case RADAR_DID_DATA_SWITCH:
            if (sid == RADAR_SID_WRITE_RESP) {
                hcn_log_info("%s: data switch set OK\n", RADAR_TAG);
            }
            break;

        case RADAR_DID_DATA_TYPE:
            if (sid == RADAR_SID_WRITE_RESP) {
                hcn_log_info("%s: data type switch OK\n", RADAR_TAG);
            }
            break;

        default:
            hcn_log_info("%s: cmd resp SID=0x%02X DID=0x%02X\n",
                         RADAR_TAG, sid, did);
            break;
    }
}

/*=============================================================================
 * TX 发送
 *===========================================================================*/

static void radar_tx_thread(void *param) {
    UartPort_t *uap = (UartPort_t *)param;
    radar_tx_msg_t *msg;
    int rtn;

    if (!uap) {
        hcn_log_error("%s: tx thread uap is NULL\n", RADAR_TAG);
        vQueueDelete(g_tx_queue);
        g_tx_queue = NULL;
        vTaskDelete(NULL);
        return;
    }

    for (;;) {
        if (xQueueReceive(g_tx_queue, &msg, portMAX_DELAY) != pdPASS) {
            hcn_log_error("%s: tx queue recv error\n", RADAR_TAG);
            continue;
        }

#ifdef RADAR_DEBUG_ENABLE
        hcn_hex_config_data_print(RADAR_TAG, ":TX(0x)", msg->buffer,
                                  RADAR_CMD_FRAME_LEN);
#endif

        rtn = iUartWrite(uap, msg->buffer, RADAR_CMD_FRAME_LEN,
                         pdMS_TO_TICKS(100));
        vPortFree(msg);

        if (rtn != RADAR_CMD_FRAME_LEN) {
            hcn_log_error("%s: tx write error\n", RADAR_TAG);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static int radar_tx_init(UartPort_t *uap) {
    if (!uap) {
        return -1;
    }

    g_tx_queue = xQueueCreate(RADAR_TX_QUEUE_LEN, sizeof(uint32_t));
    if (g_tx_queue == NULL) {
        hcn_log_error("%s: create tx queue failed\n", RADAR_TAG);
        return -1;
    }

    if (xTaskCreate(radar_tx_thread, "radar_tx",
                    configMINIMAL_STACK_SIZE, uap,
                    configMAX_PRIORITIES / 3, NULL) != pdPASS) {
        vQueueDelete(g_tx_queue);
        g_tx_queue = NULL;
        hcn_log_error("%s: create tx thread failed\n", RADAR_TAG);
        return -1;
    }

    return 0;
}

/*=============================================================================
 * 公共发送接口
 *===========================================================================*/

/**
 * @brief 构造并发送一条通用命令帧
 *        帧格式: DF 07 len SID 10 DID D1 D2 D3 D4 chkL chkH
 */
int mmwave_radar_send_cmd(uint8_t sid, uint8_t did,
                          uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4) {
    if (g_tx_queue == NULL) {
        return -1;
    }

    radar_tx_msg_t *msg = pvPortMalloc(sizeof(radar_tx_msg_t));
    if (!msg) {
        hcn_log_error("%s: alloc tx msg failed\n", RADAR_TAG);
        return -1;
    }

    uint8_t len;
    if (sid == RADAR_SID_READ_REQ) {
        len = 0x03;  ///< 读取请求 len=3
    } else {
        len = 0x04;  ///< 写入请求 len=4
    }

    msg->buffer[0]  = RADAR_CMD_TX_HEAD0;   ///< 0xDF
    msg->buffer[1]  = RADAR_CMD_TX_HEAD1;   ///< 0x07
    msg->buffer[2]  = len;
    msg->buffer[3]  = sid;
    msg->buffer[4]  = RADAR_PRODUCT_ID;     ///< 0x10
    msg->buffer[5]  = did;
    msg->buffer[6]  = d1;
    msg->buffer[7]  = d2;
    msg->buffer[8]  = d3;
    msg->buffer[9]  = d4;

    ///< 校验和: data[2]~data[9] 求和, LSB
    uint16_t chk = radar_calc_checksum(msg->buffer, 2, 8);
    msg->buffer[10] = (uint8_t)(chk & 0xFF);
    msg->buffer[11] = (uint8_t)((chk >> 8) & 0xFF);

    if (xQueueSend(g_tx_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        vPortFree(msg);
        hcn_log_error("%s: send cmd queue full\n", RADAR_TAG);
        return -1;
    }

    return 0;
}

int mmwave_radar_read_version(void) {
    return mmwave_radar_send_cmd(RADAR_SID_READ_REQ,
                                 RADAR_DID_VERSION, 0, 0, 0, 0);
}

int mmwave_radar_set_bsd_speed(uint8_t speed_kmh) {
    return mmwave_radar_send_cmd(RADAR_SID_WRITE_REQ,
                                 RADAR_DID_BSD_START_SPEED,
                                 speed_kmh, 0, 0, 0);
}

int mmwave_radar_data_switch(uint8_t enable) {
    return mmwave_radar_send_cmd(RADAR_SID_WRITE_REQ,
                                 RADAR_DID_DATA_SWITCH,
                                 enable ? 0x01 : 0x00, 0, 0, 0);
}

int mmwave_radar_set_data_type(uint8_t type) {
    return mmwave_radar_send_cmd(RADAR_SID_WRITE_REQ,
                                 RADAR_DID_DATA_TYPE,
                                 type, 0, 0, 0);
}

int mmwave_radar_input_vehicle_speed(uint8_t enable, int16_t speed) {
    return mmwave_radar_send_cmd(RADAR_SID_WRITE_REQ,
                                 RADAR_DID_VEHICLE_SPEED,
                                 enable,
                                 (uint8_t)(speed & 0xFF),
                                 (uint8_t)((speed >> 8) & 0xFF),
                                 0);
}

/*=============================================================================
 * 在线检测定时器
 *===========================================================================*/

static void radar_online_check_callback(TimerHandle_t xTimer) {
    uint32_t now = xTaskGetTickCount();
    if ((now - g_last_rx_tick) > pdMS_TO_TICKS(RADAR_ONLINE_TIMEOUT_MS)) {
        if (g_radar_online) {
            g_radar_online = false;
            vehicle_set_data(VEH_RADAR_STATUS, 0);
            hcn_log_info("%s: offline detected\n", RADAR_TAG);
        }
    }
}

/*=============================================================================
 * 车速输入定时器 (定期将车速发给雷达)
 *===========================================================================*/

static void radar_veh_speed_callback(TimerHandle_t xTimer) {
    if (!g_radar_online) {
        return;
    }

    int32_t speed;
    if (g_dbg_speed_override >= 0) {
        speed = g_dbg_speed_override;  ///< 使用调试覆盖车速
    } else {
        speed = vehicle_get_data(VEH_SPEED_CURRENT);
    }
    ///< vehicle speed 精度0.1km/h, 当前speed单位为km/h
    int16_t speed_01 = (int16_t)(speed * 10);
    mmwave_radar_input_vehicle_speed(1, speed_01);
}

/*=============================================================================
 * RX 接收线程 - 核心状态机
 *===========================================================================*/

typedef enum {
    RX_STATE_FIND_HEAD,     ///< 寻找帧头
    RX_STATE_WARN_P1,       ///< 接收预警Part1剩余字节
    RX_STATE_WARN_P2_HEAD,  ///< 等待预警Part2帧头
    RX_STATE_WARN_P2,       ///< 接收预警Part2剩余字节
    RX_STATE_CMD_RESP,      ///< 接收通用回复帧剩余字节
    RX_STATE_TARGET,        ///< 接收目标帧剩余字节
} radar_rx_state_e;

extern bool get_recovery_usr_param(void);
static void radar_rx_thread(void *param) {
    UartPort_t *uap = xUartOpen(HCN_UART_RADAR_PORT);
    if (!uap) {
        hcn_log_error("%s: open uart %d fail\n", RADAR_TAG, HCN_UART_RADAR_PORT);
        vTaskDelete(NULL);
        return;
    }

    vUartInit(uap, HCN_UART_RADAR_BAUDRATE, 0);
    // g_radar_uap = uap;  ///< 保留供后续发送扩展

    printf("\r\n========Radar: uart %d opened, baud=%d=========\r\n",
                 HCN_UART_RADAR_PORT, HCN_UART_RADAR_BAUDRATE);

    if (radar_tx_init(uap) != 0) {
        hcn_log_error("%s: tx init failed\n", RADAR_TAG);
        vUartClose(uap);
        vTaskDelete(NULL);
        return;
    }

    ///< 延时后读取版本号并确认数据开启
    vTaskDelay(pdMS_TO_TICKS(500));
    printf("\r\n--------adar: sending version query...\r\n");
    mmwave_radar_read_version();
    vTaskDelay(pdMS_TO_TICKS(100));

    while (!get_recovery_usr_param()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ///< 读取用户存储的雷达开关状态，决定是否开启数据上传
    uint8_t radar_sw = 1;
#ifdef HCN_NOR_FLASH_PARAM_ENABLE
    get_hcn_usr_param(HCN_PARAM_RADAR_SWITCH, &radar_sw);
#endif
    printf("\r\n====Radar: radar_sw=%d, enabling data upload...\r\n",
                 radar_sw);
    mmwave_radar_data_switch(radar_sw);

    uint8_t rx_buf[RADAR_RX_BUF_SIZE];
    uint8_t frame_buf[RADAR_WARN_FRAME_TOTAL_LEN]; ///< 最大帧缓冲
    int frame_pos = 0;
    int frame_need = 0;
    radar_rx_state_e state = RX_STATE_FIND_HEAD;
    int read_len;
    uint32_t rx_timeout_cnt = 0;    ///< RX超时计数(调试用)
    uint32_t rx_byte_cnt = 0;       ///< RX收到字节计数(调试用)
    uint32_t rx_discard_cnt = 0;    ///< RX丢弃字节计数(调试用)

    printf("\r\n*******Radar: rx loop started, waiting for data...\n");

    for (;;) {
        switch (state) {
        case RX_STATE_FIND_HEAD: {
            ///< 逐字节寻找帧头
            read_len = iUartRead(uap, rx_buf, 1, pdMS_TO_TICKS(500));
            // printf("\r\n++++++++++mmrecv len = %d\r\n", read_len);
            if (read_len <= 0) {
                rx_timeout_cnt++;
                ///< 每10秒(20次*500ms)输出一条诊断, 不淹没日志
                if (rx_timeout_cnt % 20 == 0) {
                    printf("Radar: [DIAG] no RX data for %lus "
                                 "(total_bytes=%lu discard=%lu)\n",
                                 (unsigned long)(rx_timeout_cnt / 2),
                                 (unsigned long)rx_byte_cnt,
                                 (unsigned long)rx_discard_cnt);
                }
                continue;
            }

            rx_byte_cnt++;
            uint8_t b = rx_buf[0];

            if (b == RADAR_WARN_P1_HEAD0) {
                ///< 可能是预警Part1: 0x80
                frame_buf[0] = b;
                frame_pos = 1;
                frame_need = 12 - 1; ///< 还需11字节
                state = RX_STATE_WARN_P1;
                rx_timeout_cnt = 0;
            } else if (b == RADAR_CMD_RX_HEAD0) {
                ///< 可能是通用回复帧: 0x60
                frame_buf[0] = b;
                frame_pos = 1;
                frame_need = 12 - 1;
                state = RX_STATE_CMD_RESP;
                rx_timeout_cnt = 0;
            } else if (b == RADAR_TARGET_HEAD0) {
                ///< 可能是目标帧: 0xAA
                frame_buf[0] = b;
                frame_pos = 1;
                frame_need = 1; ///< 先读第二个字节确认
                state = RX_STATE_TARGET;
                rx_timeout_cnt = 0;
            } else {
                ///< 非帧头字节,丢弃并记录
                rx_discard_cnt++;
                ///< 前64字节全部打印,帮助判断是否有数据/乱码/波特率错
                if (rx_byte_cnt <= 64) {
                    printf("Radar: [RX] byte[%lu]=0x%02X (discard)\n",
                                 (unsigned long)rx_byte_cnt, b);
                }
            }
        } break;

        case RX_STATE_WARN_P1: {
            read_len = iUartRead(uap, frame_buf + frame_pos, frame_need,
                                 pdMS_TO_TICKS(200));
            if (read_len <= 0) {
                state = RX_STATE_FIND_HEAD;
                continue;
            }
            frame_pos += read_len;
            frame_need -= read_len;

            if (frame_need > 0) {
                continue;
            }

            ///< 收满12字节，检查第二个字节
            if (frame_buf[1] != RADAR_WARN_P1_HEAD1) {
                state = RX_STATE_FIND_HEAD;
                continue;
            }

            if (radar_parse_warn_part1(frame_buf)) {
                g_last_rx_tick = xTaskGetTickCount();
                if (!g_radar_online) {
                    g_radar_online = true;
                    hcn_log_info("%s: online\n", RADAR_TAG);
                }
                ///< 预警Part1解析成功，等待Part2帧头
                state = RX_STATE_WARN_P2_HEAD;
            } else {
                state = RX_STATE_FIND_HEAD;
            }
        } break;

        case RX_STATE_WARN_P2_HEAD: {
            ///< 读取Part2的第一个字节
            read_len = iUartRead(uap, rx_buf, 1, pdMS_TO_TICKS(200));
            if (read_len <= 0) {
                ///< 超时，Part2没来，也更新Part1已有的数据
                radar_update_warn_to_vehicle();
                state = RX_STATE_FIND_HEAD;
                continue;
            }

            if (rx_buf[0] == RADAR_WARN_P2_HEAD0) {
                frame_buf[0] = rx_buf[0];
                frame_pos = 1;
                frame_need = 12 - 1;
                state = RX_STATE_WARN_P2;
            } else {
                ///< 不是预期的Part2帧头，先更新Part1
                radar_update_warn_to_vehicle();
                state = RX_STATE_FIND_HEAD;
            }
        } break;

        case RX_STATE_WARN_P2: {
            read_len = iUartRead(uap, frame_buf + frame_pos, frame_need,
                                 pdMS_TO_TICKS(200));
            if (read_len <= 0) {
                state = RX_STATE_FIND_HEAD;
                continue;
            }
            frame_pos += read_len;
            frame_need -= read_len;

            if (frame_need > 0) {
                continue;
            }

            ///< 收满12字节
            if (frame_buf[1] != RADAR_WARN_P2_HEAD1) {
                state = RX_STATE_FIND_HEAD;
                continue;
            }

            radar_parse_warn_part2(frame_buf);
            radar_update_warn_to_vehicle();
            state = RX_STATE_FIND_HEAD;
        } break;

        case RX_STATE_CMD_RESP: {
            read_len = iUartRead(uap, frame_buf + frame_pos, frame_need,
                                 pdMS_TO_TICKS(200));
            if (read_len <= 0) {
                state = RX_STATE_FIND_HEAD;
                continue;
            }
            frame_pos += read_len;
            frame_need -= read_len;

            if (frame_need > 0) {
                continue;
            }

            if (frame_buf[1] != RADAR_CMD_RX_HEAD1) {
                state = RX_STATE_FIND_HEAD;
                continue;
            }

            g_last_rx_tick = xTaskGetTickCount();
            radar_parse_cmd_response(frame_buf);
            state = RX_STATE_FIND_HEAD;
        } break;

        case RX_STATE_TARGET: {
            if (frame_pos == 1) {
                ///< 读第二个字节确认
                read_len = iUartRead(uap, frame_buf + 1, 1,
                                     pdMS_TO_TICKS(200));
                if (read_len <= 0 || frame_buf[1] != RADAR_TARGET_HEAD1) {
                    state = RX_STATE_FIND_HEAD;
                    continue;
                }
                frame_pos = 2;
                ///< 目标帧: 2(头)+1(num)+18(3*6)+2(chk)=23字节，还需21字节
                frame_need = 23 - 2;
            }

            read_len = iUartRead(uap, frame_buf + frame_pos, frame_need,
                                 pdMS_TO_TICKS(200));
            if (read_len <= 0) {
                state = RX_STATE_FIND_HEAD;
                continue;
            }
            frame_pos += read_len;
            frame_need -= read_len;

            if (frame_need > 0) {
                continue;
            }

            g_last_rx_tick = xTaskGetTickCount();
            if (!g_radar_online) {
                g_radar_online = true;
                hcn_log_info("%s: online\n", RADAR_TAG);
            }

            radar_parse_target_frame(frame_buf, 23);
            state = RX_STATE_FIND_HEAD;
        } break;

        default:
            state = RX_STATE_FIND_HEAD;
            break;
        }
    }
}

/*=============================================================================
 * 公共查询接口
 *===========================================================================*/

const radar_warn_data_t *mmwave_radar_get_warn_data(void) {
    return &g_warn_data;
}

const radar_target_data_t *mmwave_radar_get_target_data(void) {
    return &g_target_data;
}

bool mmwave_radar_is_online(void) {
    return g_radar_online;
}

/*=============================================================================
 * 模块初始化
 *===========================================================================*/
int mmwave_radar_init(void) {
    ///< 创建在线检测定时器 (1秒周期)
    g_online_timer = xTimerCreate("radar_online",
                                   pdMS_TO_TICKS(1000), pdTRUE,
                                   NULL, radar_online_check_callback);
    if (g_online_timer) {
        xTimerStart(g_online_timer, 0);
    }

    ///< 创建车速输入定时器 (250ms周期，协议要求200~300ms)
    g_veh_speed_timer = xTimerCreate("radar_vspd",
                                      pdMS_TO_TICKS(RADAR_VEH_SPEED_INTERVAL_MS),
                                      pdTRUE, NULL,
                                      radar_veh_speed_callback);
    if (g_veh_speed_timer) {
        xTimerStart(g_veh_speed_timer, 0);
    }

    ///< 创建RX接收线程
    if (xTaskCreate(radar_rx_thread, "radar_rx",
                    configMINIMAL_STACK_SIZE * 3, NULL,
                    configMAX_PRIORITIES / 3, NULL) != pdPASS) {
        hcn_log_error("%s: create rx thread failed\n", RADAR_TAG);
        return -1;
    }

    hcn_log_info("%s: mmWave radar module init OK\n", RADAR_TAG);
    return 0;
}

#endif /* HCN_MMWAVE_RADAR_ENABLE */
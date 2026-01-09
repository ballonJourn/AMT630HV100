/**
*
* @file hcn_tcp_client.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/16 09:13
* @author och
*
*/

#include <FreeRTOS.h>
#include <task.h>
#include "iot_wifi.h"
#include "sfud.h"
#include "board.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_DHCP_Server.h"
#include "sockets.h"
#include "hcn_tcp_client.h"
#include "log/hcn_log.h"
#include "ota_manage/hcn_ota_parse.h"
#include "carlink_cb/hcn_carlink_cb.h"
#include "version/hcn_version.h"
#include "source/crc32.h"

#define OTA_SERVER_PORT    (8008)           ///< 服务器端口号
#define TCP_OTA_SEND_BUFF_LEN   (256)       ///< tcp发送数据最大长度

typedef struct {
    bool is_socket_connnected;      ///< socket客户端连接状态
    uint32_t socket_rx_pos;         ///< socket接收数据的总长度
    uint32_t recv_timeout_ms;      ///< 接收数据超时时间
    uint32_t send_timeout_ms;     ///< 发送数据超时时间
} client_param_t;

static client_param_t g_client_param = {
    .is_socket_connnected = false,
    .socket_rx_pos = 0,                 
    .recv_timeout_ms = pdMS_TO_TICKS(2000),
    .send_timeout_ms = pdMS_TO_TICKS(2000),
};

static TaskHandle_t client_send_task = NULL;
static TaskHandle_t client_recv_task = NULL;

static uint8_t socket_rx[TCP_OTA_DATA_MAX_LEN] = {0};
static uint8_t socket_tx[TCP_OTA_SEND_BUFF_LEN] = {0};
static int g_socket_fd = -1;
static struct sockaddr_in g_server_addr;

static tcp_ota_state ota_state = TCP_SEND_DEVICE_INFO;

uint32_t ota_calc_crc32(uint8_t *data, uint32_t len) {
    if (data) {
        uint32_t calc_checksum = 0xffffffff;
        return xcrc32(data, len, calc_checksum);
    }

    return 0xffffffff;
}

static uint32_t last_device_tick = 0;
static int send_device_info(void) {
    if (xTaskGetTickCount() - last_device_tick < pdMS_TO_TICKS(1500)) {
        return 0;
    }

    last_device_tick = xTaskGetTickCount();
    memset(socket_tx, 0, sizeof(socket_tx));

    uint16_t offset = 4;
    socket_tx[offset++] = TCP_DEVICE_INFO_CMD;

    ///< 设备号长度
    uint16_t dev_id_len = strlen(hcn_bt_get_mac_addr());
    socket_tx[offset++] = (dev_id_len >> 8) & 0xFF;
    socket_tx[offset++] = (dev_id_len >> 0) & 0xFF;

    strncpy((char *)&socket_tx[offset], hcn_bt_get_mac_addr(), dev_id_len);
    offset += dev_id_len;

    ///< 版本号长度
    uint16_t ver_len = strlen(get_soc_version());
    socket_tx[offset++] = (ver_len >> 8) & 0xFF;
    socket_tx[offset++] = (ver_len >> 0) & 0xFF;
    strncpy((char *)&socket_tx[offset], get_soc_version(), ver_len);
    offset += ver_len;
    
    socket_tx[0] = TCP_OTA_FRAME_HEAD_1;
    socket_tx[1] = TCP_OTA_FRAME_HEAD_2;
	socket_tx[2] = ((offset - 4) >> 8) & 0xFF;
	socket_tx[3] = (offset - 4) & 0xFF;

    ///<CRC32校验
    uint32_t calc_checksum = ota_calc_crc32(socket_tx, offset);
    socket_tx[offset++] = (calc_checksum >> 24) & 0xFF;
    socket_tx[offset++] = (calc_checksum >> 16) & 0xFF;
    socket_tx[offset++] = (calc_checksum >> 8) & 0xFF;
    socket_tx[offset++] = (calc_checksum >> 0) & 0xFF;

    hcn_log_info("device info:%s!\r\n", hcn_bt_get_mac_addr());
    return send(g_socket_fd, socket_tx, offset, 0);
}

static int send_percent(void) {
    uint32_t calc_checksum = 0;

    memset(socket_tx, 0, sizeof(socket_tx));
    socket_tx[0] = TCP_OTA_FRAME_HEAD_1;
    socket_tx[1] = TCP_OTA_FRAME_HEAD_2;
    socket_tx[2] = 0x00;
    socket_tx[3] = 0x02;
    socket_tx[4] = TCP_FLASH_PERCENTAGE;
    socket_tx[5] = get_ota_percent();

    calc_checksum = ota_calc_crc32(socket_tx, 4);
    socket_tx[6] = (calc_checksum >> 24) & 0xFF;
    socket_tx[7] = (calc_checksum >> 16) & 0xFF;
    socket_tx[8] = (calc_checksum >> 8) & 0xFF;
    socket_tx[9] = (calc_checksum >> 0) & 0xFF;

    hcn_log_info("send ota percent:%d\r\n", get_ota_percent());
    return send(g_socket_fd, socket_tx, 10, 0);
}

static int send_transfer_complete(void) {
    uint32_t calc_checksum = 0;

    memset(socket_tx, 0, sizeof(socket_tx));
    socket_tx[0] = TCP_OTA_FRAME_HEAD_1;
    socket_tx[1] = TCP_OTA_FRAME_HEAD_2;
    socket_tx[2] = 0x00;
    socket_tx[3] = 0x01;
    socket_tx[4] = TCP_TRANSFER_COMPLETE_CMD;

    calc_checksum = ota_calc_crc32(socket_tx, 3);
    socket_tx[5] = (calc_checksum >> 24) & 0xFF;
    socket_tx[6] = (calc_checksum >> 16) & 0xFF;
    socket_tx[7] = (calc_checksum >> 8) & 0xFF;
    socket_tx[8] = (calc_checksum >> 0) & 0xFF;

    hcn_log_info("send transfer complete \r\n");

    return send(g_socket_fd, socket_tx, 9, 0);
}

static int send_ack(uint8_t msg_type, bool success) {
    uint32_t calc_checksum = 0;
    uint16_t ack_code = success ? 200 : 500;

    memset(socket_tx, 0, sizeof(socket_tx));
    socket_tx[0] = TCP_OTA_FRAME_HEAD_1;
    socket_tx[1] = TCP_OTA_FRAME_HEAD_2;
    socket_tx[2] = 0x00;
    socket_tx[3] = 0x03;
    socket_tx[4] = msg_type;
    socket_tx[5] = (ack_code >> 8) & 0xFF;
    socket_tx[6] = (ack_code >> 0) & 0xFF;

    calc_checksum = ota_calc_crc32(socket_tx, 5);
    socket_tx[7] = (calc_checksum >> 24) & 0xFF;
    socket_tx[8] = (calc_checksum >> 16) & 0xFF;
    socket_tx[9] = (calc_checksum >> 8) & 0xFF;
    socket_tx[10] = (calc_checksum >> 0) & 0xFF;
    return send(g_socket_fd, socket_tx, 11, 0);
}

static int send_heartbeat(void) {
    uint32_t calc_checksum = 0;

    memset(socket_tx, 0, sizeof(socket_tx));
    socket_tx[0] = TCP_OTA_FRAME_HEAD_1;
    socket_tx[1] = TCP_OTA_FRAME_HEAD_2;
    socket_tx[2] = 0x00;
    socket_tx[3] = 0x01;
    socket_tx[4] = TCP_HEARTBEAT_CMD;

    calc_checksum = ota_calc_crc32(socket_tx, 3);
    socket_tx[5] = (calc_checksum >> 24) & 0xFF;
    socket_tx[6] = (calc_checksum >> 16) & 0xFF;
    socket_tx[7] = (calc_checksum >> 8) & 0xFF;
    socket_tx[8] = (calc_checksum >> 0) & 0xFF;

    hcn_log_info("send heartbeat \r\n");
    return send(g_socket_fd, socket_tx, 9, 0);
}

static int socket_deinit(void) {
    g_client_param.is_socket_connnected = false;
    g_client_param.socket_rx_pos = 0;
    ota_state = TCP_SEND_DEVICE_INFO;

    if (g_socket_fd >= 0) {
        closesocket(g_socket_fd);
        g_socket_fd = -1;
    }

    g_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_socket_fd < 0) {
        hcn_log_info("lwip create socket failed!\r\n");
        return -1;
    }

    hcn_log_info("reset socket fd:%d\r\n", g_socket_fd);

    memset(&g_server_addr, 0, sizeof(g_server_addr));
    g_server_addr.sin_family = AF_INET;
    g_server_addr.sin_port = htons(OTA_SERVER_PORT);

    struct timeval timeout;
    int optval = 1;
    uint32_t gw_addr = 0;

    ///< 设置发送和接收超时时间为2s
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_SNDTIMEO, 
              (char*)&timeout, sizeof(timeout)) < 0) {
        hcn_log_error("Set socket send timeout faile!\r\n");
        goto exit;
    }

    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_RCVTIMEO, 
              (char*)&timeout, sizeof(timeout)) < 0) {
        hcn_log_error("Set socket recv timeout faile!\r\n");
        goto exit;
    }

    ///< 允许地址重复
    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_REUSEADDR, 
              (char*)&optval, sizeof(optval)) < 0) {
        hcn_log_error("Set socket so reuseaddr faile!\r\n");
        goto exit;       
    }      

    extern uint32_t get_ota_sta_gw_addr(void);
    while (get_ota_sta_gw_addr() == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    gw_addr = get_ota_sta_gw_addr();
    g_server_addr.sin_addr.s_addr = gw_addr; 
    return 0;

exit:
    if (g_socket_fd >= 0) {
        closesocket(g_socket_fd);
    }

    return -1;
}

static int socket_init(void) {
    hcn_log_info("tcp socket init!\r\n");

    g_client_param.is_socket_connnected = false;
    g_client_param.socket_rx_pos = 0;
    ota_state = TCP_SEND_DEVICE_INFO;

    if (g_socket_fd >= 0) {
        closesocket(g_socket_fd);
        g_socket_fd = -1;
    }

    g_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_socket_fd < 0) {
        hcn_log_info("lwip create socket failed!\r\n");
        return -1;
    }

    hcn_log_info("socket fd:%d\r\n", g_socket_fd);

    memset(&g_server_addr, 0, sizeof(g_server_addr));
    g_server_addr.sin_family = AF_INET;
    g_server_addr.sin_port = htons(OTA_SERVER_PORT);

    struct timeval timeout;
    int optval = 1;
    uint32_t gw_addr = 0;

    ///< 设置发送和接收超时时间为2s
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_SNDTIMEO, 
              (char*)&timeout, sizeof(timeout)) < 0) {
        hcn_log_error("Set socket send timeout faile!\r\n");
        goto exit;
    }

    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_RCVTIMEO, 
              (char*)&timeout, sizeof(timeout)) < 0) {
        hcn_log_error("Set socket recv timeout faile!\r\n");
        goto exit;
    }

    ///< 允许地址重复
    if (setsockopt(g_socket_fd, SOL_SOCKET, SO_REUSEADDR, 
              (char*)&optval, sizeof(optval)) < 0) {
        hcn_log_error("Set socket so reuseaddr faile!\r\n");
        goto exit;       
    }      

    hcn_log_info("tcp client socket int ok!\r\n");
    extern uint32_t get_ota_sta_gw_addr(void);
    while (get_ota_sta_gw_addr() == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    gw_addr = get_ota_sta_gw_addr();
    g_server_addr.sin_addr.s_addr = gw_addr; 
    hcn_log_info("servei ip addr:%d.%d.%d.%d\r\n",  (gw_addr >> 0) & 0xFF,
        (gw_addr >> 8) & 0xFF,
        (gw_addr >> 16) & 0xFF,
        (gw_addr >> 24) & 0xFF);
    return 0;

exit:
    if (g_socket_fd >= 0) {
        closesocket(g_socket_fd);
    }

    return -1;
}

static void socket_reset(void) {
    if (g_client_param.is_socket_connnected) {
        g_client_param.is_socket_connnected = false;
        socket_init();
        ota_parse_reset();
        hcn_log_info("tcp client socket reset!\r\n");
    }
}

tcp_ota_state get_ota_state(void) {
    return ota_state;
}

void set_ota_state(tcp_ota_state state) {
    ota_state = state;
}

static void tcp_client_send_thread(void *pvParameters) {
    hcn_log_info("tcp client send thread start!\r\n");

    int len = 0;
    int ret = -1;
    for (;;) {
        wait_connect:
        ret = connect(g_socket_fd, (struct sockaddr*)&g_server_addr, 
                    sizeof(g_server_addr));
        if (ret == 0)
        {
            g_client_param.is_socket_connnected = true;
            hcn_log_info("tcp client socket connected!\r\n");

            uint8_t tick_time = 0;
            while (1) {
                if (!g_client_param.is_socket_connnected) {
                    hcn_log_error("tcp client socket disconnected, reconnect!\r\n");
                    goto wait_connect;
                }

                len = 0; 
                switch (ota_state) {
                    case TCP_SEND_DEVICE_INFO:
                        len = send_device_info();
                        break;

                    case TCP_SEND_TRANSFER_COMPLETE:
                         if (get_ota_percent_change()) {
                            set_ota_percent_change(false);
                            len = send_percent();
                        }
                        len = send_transfer_complete();
                        break;

                    case TCP_RECV_FILE_INFO:
                        len = send_ack(TCP_FILE_INFO_CMD, true);
                        if (len > 0 && ota_state == TCP_RECV_FILE_INFO) {
                            ota_state = TCP_SEND_FILE_ACK;
                        }
                        break;

                    case TCP_SEND_PERCENT:
                        if (get_ota_percent_change()) {
                            set_ota_percent_change(false);
                            len = send_percent();
                        }
                        break;    

                    default:
                       break;
                }

                if (len == 0) {
                    tick_time++;
                } else if (len > 0) {
                    tick_time = 0;
                }

                if (tick_time >= 10) {
                    len = send_heartbeat();
                    tick_time = 0;
                }

                ///< 发送错误
                if (len < 0) {
                    hcn_log_error("send task freeRTOS_send failed:%d!\r\n", len);
#if !USE_LWIP
                    switch (len) {
                        case -pdFREERTOS_ERRNO_ENOTCONN:
                             ///< 套接字已关闭或者已关闭无法发送数据
                             break;

                        case -pdFREERTOS_ERRNO_ENOMEM:
                            ///< 内存不足,无法发送数据
                            break;

                        case -pdFREERTOS_ERRNO_EINVAL:
                            ///< xSocket不是有效的TCP套接字而无法发送数据
                            break;
                            
                        case -pdFREERTOS_ERRNO_ENOSPC:
                            ///< 在任何数据被发送之前发生超时
                            break;
                        default:
                            break;
                    }
#endif
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        } else {
            hcn_log_info("try connect server,socket_fd = %d, ret = %d\r\n", g_socket_fd, ret);
            vTaskDelay(pdMS_TO_TICKS(2000));
            socket_deinit();
        }
    }
    
    if (g_socket_fd >= 0) {
        closesocket(g_socket_fd);
        g_socket_fd = -1;
    }

	hcn_log_info("tcp_client_send_thread exit\n");

	vTaskDelete(NULL);
    client_send_task = NULL;
}

static void tcp_client_recv_thread(void *pvParameters) {
    hcn_log_info("tcp client recv thread start!\r\n");

    int recv_len = 0;
    static int sync_search = 0;

    while (1) {
wait_recv_connnect:
        if (g_client_param.is_socket_connnected) {
            hcn_log_info("Connect client....\r\n");
           recv_len = recv(g_socket_fd, 
                        (void *)(socket_rx + g_client_param.socket_rx_pos), 
                        TCP_OTA_DATA_MAX_LEN - g_client_param.socket_rx_pos, 0);      
            if (recv_len > 0) {
                if (!g_client_param.is_socket_connnected) {
                    hcn_log_error("tcp client socket disconnected, reconnect!\r\n");
                    goto wait_recv_connnect;
                }

                g_client_param.socket_rx_pos += recv_len;
                                
    check_data:
                if (g_client_param.socket_rx_pos >= TCP_OTA_DATA_MIN_LEN) { 
                    
                    ///< 如果没有找到同步头，先搜索同步头
                    if (!sync_search) {
                        int found_sync = 0;
                        ///< 在缓冲区中搜索同步头
                        for (int i = 0; i <= g_client_param.socket_rx_pos - 2; i++) {
                            if (socket_rx[i] == TCP_OTA_FRAME_HEAD_1 
                                && socket_rx[i+1] == TCP_OTA_FRAME_HEAD_2) {
                                ///< 找到同步头，将有效数据移动到缓冲区起始位置
                                if (i > 0) {
                                    memmove(socket_rx, socket_rx + i, 
                                            g_client_param.socket_rx_pos - i);
                                    g_client_param.socket_rx_pos -= i;
                                }
                                found_sync = 1;
                                sync_search = 1;
                                break;
                            }
                        }
                        
                        ///< 如果没有找到同步头，清除所有数据重新开始
                        if (!found_sync) {
                            ///< 保留最后1个字节（可能下一个字节就是0xA5）
                            if (g_client_param.socket_rx_pos > 1) {
                                memmove(socket_rx, socket_rx + g_client_param.socket_rx_pos - 1, 1);
                                g_client_param.socket_rx_pos = 1;
                            }
                            vTaskDelay(pdMS_TO_TICKS(5));
                            continue;
                        }
                    }
                    
                    int total_packet_len = 2 + ((socket_rx[2] << 8) + socket_rx[3]) + 2 + 4; 
                    if (g_client_param.socket_rx_pos >= total_packet_len) {
                        ///< 验证同步头是否正确
                        if (socket_rx[0] == TCP_OTA_FRAME_HEAD_1 
                                && socket_rx[1] == TCP_OTA_FRAME_HEAD_2) {
                            ota_task_add(socket_rx, total_packet_len);
                            
                            memmove(socket_rx, socket_rx + total_packet_len, 
                                    g_client_param.socket_rx_pos - total_packet_len);
                            g_client_param.socket_rx_pos -= total_packet_len;
                            memset(socket_rx + g_client_param.socket_rx_pos, 0, 
                                    sizeof(socket_rx) - g_client_param.socket_rx_pos);
                            
                            sync_search = 0;
                            goto check_data;
                        } else {
                            sync_search = 0;
                            ///< 丢弃第一个字节，重新搜索
                            if (g_client_param.socket_rx_pos > 1) {
                                memmove(socket_rx, socket_rx + 1, 
                                        g_client_param.socket_rx_pos - 1);
                                g_client_param.socket_rx_pos--;
                            }
                            goto check_data;
                        }
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(5));
            } else if (recv_len < 0) {
                hcn_log_error("recv task FreeRTOS_recv failed:%d!\r\n", recv_len);
                switch (recv_len) {
#if !USE_LWIP
                    case -pdFREERTOS_ERRNO_ENOTCONN:
#else
                    case -ERR_CONN:
#endif
                         ///< 套接字已关闭或者已关闭无法接收数据
                        socket_reset();
                        // 重置同步头搜索标志
                        sync_search = 0;
                        goto wait_recv_connnect;

                    case -pdFREERTOS_ERRNO_ENOMEM:
                        ///< 内存不足,无法接收数据
                        break;

                    case -pdFREERTOS_ERRNO_EINTR:
                        ///< 套接字收到信号，导致读取操作中止
                        break;
                        
                    case -pdFREERTOS_ERRNO_EINVAL:
                        ///< 套接字无效，不是TCP套接字，或者未绑定
                        break;
                    default:
                        break;
                }
                break;  
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    hcn_log_info("tcp_client_recv_thread exit\n");

    vTaskDelete(NULL);
    client_recv_task = NULL;
}

void stop_tcp_client(void) {
    if (g_socket_fd >= 0) {
        closesocket(g_socket_fd);
        g_socket_fd = -1;
    }

    if (client_send_task) {
        vTaskDelete(client_send_task);
        client_send_task = NULL;
    }

    if (client_recv_task) {
        vTaskDelete(client_recv_task);
        client_recv_task = NULL;
    }

    g_client_param.is_socket_connnected = false;
    g_client_param.socket_rx_pos = 0;
    hcn_log_info("tcp client socket stop ok!\r\n");
}

void start_tcp_client(void) {
    ota_parse_init();
    socket_init();

    if (client_send_task == NULL) {
        if (xTaskCreate(tcp_client_send_thread, "tcp_client_send_thread",
                        configMINIMAL_STACK_SIZE*4 ,
                        NULL, configMAX_PRIORITIES / 3 + 1,
                        &client_send_task) != pdPASS) {
            hcn_log_error("create tcp client send task fail.\n");
            return;
        }
    }

    if (client_recv_task == NULL) {
        if (xTaskCreate(tcp_client_recv_thread, "tcp_client_recv_thread",
                        configMINIMAL_STACK_SIZE*4,
                        NULL, configMAX_PRIORITIES / 3 + 2,
                        &client_recv_task) != pdPASS) {
            hcn_log_error("create tcp client recv task fail.\n");
            return;
        }
    }
}
/**
*
* @file test_tcp.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/25 12:02
* @author och
*
*/
#if 0

#include <FreeRTOS.h>
#include <task.h>
#include "iot_wifi.h"
#include "sfud.h"
#include "board.h"
#include "animation.h"
#include "sockets.h"

static const char *server_ip_str = "10.45.33.58";
static int socket_fd = -1;
static struct sockaddr_in server_addr;

static void test_connect_server(void *param) {
    int ret = -1;
    for (;;) {
        ret = connect(socket_fd, (struct sockaddr*)&server_addr, 
                sizeof(server_addr));
        if (ret == 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));\
            printf("Connect server success!\r\n");
        } else {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    if (socket_fd >= 0) {
        closesocket(socket_fd);
        socket_fd = -1;
    }
}

int test_tcp_client_init(void) {
     if (socket_fd >= 0) {
        closesocket(socket_fd);
        socket_fd = -1;
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        printf("lwip create socket failed!\r\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8008);

    if (inet_aton(server_ip_str, &server_addr.sin_addr) == 0) {
        printf("Invalid server ip addr!\r\n");
        closesocket(socket_fd);
        socket_fd = -1;
        return -1;
    }
    
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

     if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, 
              (char*)&timeout, sizeof(timeout)) < 0) {
        printf("Set socket send timeout faile!\r\n");
        closesocket(socket_fd);
        socket_fd = -1;
        return -1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, 
              (char*)&timeout, sizeof(timeout)) < 0) {
        printf("Set socket recv timeout faile!\r\n");
        closesocket(socket_fd);
        socket_fd = -1;
        return -1;
    }

    if (xTaskCreate(test_connect_server, "tcp_client_connect_thread",
                        configMINIMAL_STACK_SIZE*10 ,
                        NULL, configMAX_PRIORITIES / 3 + 1,
                        NULL) != pdPASS) {
        printf("create tcp client connect task fail.\n");
        return -1;
    }

    return 0;
}

extern int start_sta_ext(const char* ssid, const char* passwd, char need_passwd);
static void test_wifi_sta(void *param) {
    if (get_animation_status()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelay(pdMS_TO_TICKS(10000));
    start_sta_ext("ota630hcn", "88888888", 1);
    test_tcp_client_init();
}   

void start_wifi_sta(void) {
    if (xTaskCreate(test_wifi_sta, "test_wifi_sta", 2048, 
                    NULL, 4, NULL) != pdPASS) {
        printf("test_wifi_sta!\n");
        return;
    }
}

#endif
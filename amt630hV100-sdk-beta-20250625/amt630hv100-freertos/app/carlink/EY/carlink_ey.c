#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include "board.h"
#if !USE_LWIP
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#else
#include "ethernet.h"
#include "tcpip.h"
#include "lwip/apps/lwiperf.h"
#include "dhcp.h"
#include "lwip/sockets.h"
#endif

#include "mmcsd_core.h"
#include "mfcapi.h"
#include "lcd.h"
#include "timer.h"
#if !USE_LWIP
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_DHCP_Server.h"
#endif
#include "carlink_video.h"
#include "carlink_ey_audio.h"
#include "carlink_ey.h"
#include "console.h"
#include "iot_wifi.h"
#include "carlink_utils.h"

#if CARLINK_EY

#define EY_BT_EVENT_CONNECT               0
#define EY_BT_EVENT_DISCONNECT            1
#define EY_BT_EVENT_NOTIFY_PHONE_TYPE     2
#define EY_BT_EVENT_NOTIFY_SSID           3
#define EY_BT_EVENT_NOTIFY_PASSWD         4
#define EY_BT_EVENT_NOTIFY_CONNECT_STATUS 5
#define EY_BT_EVENT_NOTIFY_PHONE_AP_READY 6
#define EY_BT_EVENT_NOTIFY_USE_TCP        7
#define EY_BT_EVENT_BLE_CONNECT           8
#define EY_CARLINK_EVENT_CONNECT           9
#define EY_CARLINK_EVENT_DISCONNECT           10
struct bt_event
{
    int type;
    uint8_t para[32];
};

static uint8_t                gbt_ready = 0;
static TaskHandle_t       ey_bt_task = NULL;
static TaskHandle_t       ey_network_task = NULL;
static QueueHandle_t     ey_bt_event_queue = NULL;
static const uint8_t        ucIPAddress[4] = {192, 168, 13, 1};
static const uint8_t        ucNetMask[4] = {255, 255, 255, 0};
static const uint8_t        ucGatewayAddress[4] = {0, 0, 0, 0};
static const uint8_t        ucDNSServerAddress[4] = {8, 8, 8, 8};
static const uint8_t        ucMACAddress[6] = {0x30, 0x4a, 0x26, 0x78, 0xfd, 0x12};
static uint8_t                ap_ssid[64]     = {"ap63011"};
static uint8_t                ap_passwd[16]  = {"88888888"};
static uint8_t                gphone_type = 0;
//static uint8_t                guse_tcp = 1;
static int                      gWifiOn = 0;
//static int                      gWIfiConnecting = 0;
#if !USE_LWIP
static Socket_t              gClientSocket = FREERTOS_INVALID_SOCKET, gXClientSocket = FREERTOS_INVALID_SOCKET;
static SemaphoreHandle_t    gSocketMutex = NULL;
#else
#define lwip_ipv4_addr(addr) ((addr[0]) | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24))

static struct netif  gnetif[4];
static int  lwip_tcpip_init_done_flag = 0;
static int  lwip_dhcp_server_start_flag = 0;

extern err_t dhcp_server_start(struct netif *netif, ip4_addr_t *start, ip4_addr_t *end);
extern err_t wlan_ethernetif_init(struct netif *netif);

static void notify_phone_ip_addr(uint32_t ipAddr);


static void ey_dump_ip_addr(const char *msg, const ip_addr_t *server_addr)
{
	printf("%s %d.%d.%d.%d \r\n", msg,
              ip4_addr1_16(ip_2_ip4(server_addr)), ip4_addr2_16(ip_2_ip4(server_addr)), ip4_addr3_16(ip_2_ip4(server_addr)), ip4_addr4_16(ip_2_ip4(server_addr)));
}

static void ey_dhcp_client_status_callback(struct netif *netif, int status, const ip_addr_t *server_addr)
{
	if (&gnetif[0] == netif) {
		ey_dump_ip_addr("dhcp server ip     :", (const ip_addr_t *)server_addr);
		ey_dump_ip_addr("dhcp client ip     :", (const ip_addr_t *)&netif->ip_addr);
		ey_dump_ip_addr("dhcp client netmask:", (const ip_addr_t *)&netif->netmask);
		ey_dump_ip_addr("dhcp client gw     :", (const ip_addr_t *)&netif->gw);

		printf("ey dhcp addr : %08x\r\n", netif->ip_addr.addr);

		notify_phone_ip_addr((uint32_t)netif->ip_addr.addr);
	}
}

static void ey_lwiperf_report_cb_impl(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
	(void)arg;

	printf("lwiperf_report_cb_impl bytes:%d %d ms \r\n", bytes_transferred, ms_duration);
}


static void ey_lwip_tcpip_init_done(void *arg)
{
	(void)arg;
}

static void ey_lwip_server_init()
{
	ip4_addr_t ip_addr;
	ip4_addr_t netmask;
	ip4_addr_t gw;
	ip4_addr_t dhcp_addr_start;
	ip4_addr_t dhcp_addr_end;

	ip_addr.addr = lwip_ipv4_addr(ucIPAddress);
	netmask.addr = lwip_ipv4_addr(ucNetMask);
	gw.addr      = lwip_ipv4_addr(ucGatewayAddress);
	if (!lwip_tcpip_init_done_flag) {
		tcpip_init(ey_lwip_tcpip_init_done, NULL);
		lwip_tcpip_init_done_flag = 1;
	} else {
		netif_set_down(&gnetif[0]);
		netif_remove(&gnetif[0]);
	}
	netif_add(&gnetif[0],
#if LWIP_IPV4
		&ip_addr, &netmask, &gw,
#endif
		NULL, wlan_ethernetif_init, tcpip_input);
	
	netif_set_default(&gnetif[0]);

	if (!lwip_dhcp_server_start_flag) {
		uint8_t 	 addr_start[4] = {192, 168, 13, 20};
		uint8_t 	 addr_end[4] = {192, 168, 13, 30};

		/*memcpy(addr_start, ucIPAddress, 4);
		memcpy(addr_end,   ucIPAddress, 4);
		addr_end[3] = 30;*/
		dhcp_addr_start.addr 	= lwip_ipv4_addr(addr_start);
		dhcp_addr_end.addr 		= lwip_ipv4_addr(addr_end);
		dhcp_server_start(&gnetif[0], &dhcp_addr_start, &dhcp_addr_end);

		lwiperf_start_tcp_server_default(ey_lwiperf_report_cb_impl, NULL);//iperf test
		lwip_dhcp_server_start_flag = 1;
	}
}

static void ey_lwip_client_init()
{
	ip4_addr_t ip_addr;
	ip4_addr_t netmask;
	ip4_addr_t gw;
	ip4_addr_t dhcp_addr_start;
	ip4_addr_t dhcp_addr_end;

	ip_addr.addr = lwip_ipv4_addr(ucIPAddress);
	netmask.addr = lwip_ipv4_addr(ucNetMask);
	gw.addr      = lwip_ipv4_addr(ucGatewayAddress);
	if (!lwip_tcpip_init_done_flag) {
		tcpip_init(ey_lwip_tcpip_init_done, NULL);
		lwip_tcpip_init_done_flag = 1;
	} else {
		netif_set_down(&gnetif[0]);
		netif_remove(&gnetif[0]);
	}

	netif_add(&gnetif[0],
#if LWIP_IPV4
		&ip_addr, &netmask, &gw,
#endif
		NULL, wlan_ethernetif_init, tcpip_input);
	netif_set_default(&gnetif[0]);
	dhcp_regisger_status_callback(ey_dhcp_client_status_callback);
	netif_set_up(&gnetif[0]);
	dhcp_start(&gnetif[0]);
}

#endif

void cmd_test(const char* temp_uart_buf);

static void ey_reset_wifi_ap_info(const char *btmac)
{
    sprintf((char *)ap_ssid,   "AP630_%s", btmac);
}

static void notify_carlink_state(int state)
{
    struct bt_event ev = {0};
    ev.type = -1;

    if (state == 0) {
        ev.type = EY_CARLINK_EVENT_DISCONNECT;
    } else {
        ev.type = EY_CARLINK_EVENT_CONNECT;
    }

    if (0 != ev.type && NULL != ey_bt_event_queue) {
        xQueueSend(ey_bt_event_queue, &ev, 0);
    }
}

static void notify_phone_ip_addr(uint32_t ipAddr);
static void ey_wifi_start_ap(const uint8_t* ssid, const uint8_t* passwd, int channel)
{
    char cmd[64] = {0};
    uint32_t IPAddress = (32 << 24) | (13 << 16) | (168 << 8) | (192 << 0);

    if (gWifiOn == 0) {
        printf("Turning on wifi ap...\r\n");
        WIFI_SetMode(eWiFiModeAP);
        gWifiOn = 1;
        //WIFI_On();
        sprintf(cmd, "wifi_ap %s %d %s", ssid, channel, passwd);
        cmd_test(cmd);

        IPAddress = (20 << 24) | (13 << 16) | (168 << 8) | (192 << 0);
#if !USE_LWIP
        dhcpserver_start(ucIPAddress, IPAddress, 10);
#endif
    }
}

#define ucNumNetworks  12

static int ey_wifi_connect_remote_ap(const char* ssid, const char* passwd)
{
    WIFINetworkParams_t xNetworkParams = {0};
    WIFIReturnCode_t xWifiStatus;
    int retry  = 0;

    WIFI_Context_init();

    if (gWifiOn == 0) {
        WIFI_SetMode(eWiFiModeStation);

        printf("Turning on wifi...\r\n");
        xWifiStatus = WIFI_On();

        printf("Checking status...\r\n");
        if( xWifiStatus == eWiFiSuccess ) {
        printf("WiFi module initialized.\r\n");
        } else {
        printf("WiFi module failed to initialize.\r\n" );
        return -1;
        }
        gWifiOn = 1;
    }

	memset(&xNetworkParams, 0, sizeof(xNetworkParams));
	xNetworkParams.ucSSIDLength = strlen( ssid );
	memcpy(xNetworkParams.ucSSID, ssid, xNetworkParams.ucSSIDLength);
	xNetworkParams.xPassword.xWPA.ucLength = strlen( passwd );
	memcpy(xNetworkParams.xPassword.xWPA.cPassphrase, passwd, xNetworkParams.xPassword.xWPA.ucLength);
	xNetworkParams.xSecurity = eWiFiSecurityWPA2;
wifi_conn_retry:

	xWifiStatus = WIFI_ConnectAP( &( xNetworkParams ) );

	if( xWifiStatus == eWiFiSuccess ) {
		printf( "WiFi Connected to AP.\r\n" );
	} else {
		printf( "WiFi failed to connect to AP.\r\n" );
		if (retry++ < 15) {vTaskDelay(pdMS_TO_TICKS(300));
			goto wifi_conn_retry;}
		return -1;
	}
	return 0;
}

#if ( ipconfigUSE_DHCP_HOOK != 0 && !USE_LWIP )
eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase,
                                            uint32_t ulIPAddress )
{
    eDHCPCallbackAnswer_t eReturn;
    //uint32_t ulStaticIPAddress, ulStaticNetMask;
    char ip_str[20] = {0};
    //const uint8_t        ucIPAddressDef[4] = {192, 168, 13, 37};
    
    sprintf(ip_str, "%d.%d.%d.%d\r\n", (ulIPAddress >> 0) & 0xFF, 
    	(ulIPAddress >> 8) & 0xFF, (ulIPAddress >> 16) & 0xFF, (ulIPAddress >> 24) & 0xFF);
    printf("\r\n  eDHCPPhase:%d  ulIPAddress:%s state:%d \r\n", eDHCPPhase, ip_str, getDhcpClientState());
    if (getDhcpClientState() == 0)
        return eDHCPStopNoChanges;

    switch( eDHCPPhase )
    {
        case eDHCPPhaseFinished:
            notify_phone_ip_addr(ulIPAddress);//FreeRTOS_GetIPAddress()
            eReturn = eDHCPContinue;
        break;
        case eDHCPPhasePreDiscover  :
            eReturn = eDHCPContinue;
        break;

        case eDHCPPhasePreRequest  :
            eReturn = eDHCPContinue;
        break;
        default :
            eReturn = eDHCPContinue;
        break;
    }

    return eReturn;
}
#endif
static void notify_phone_ip_addr(uint32_t ipAddr)
{
    char at_cmd[64] = {0};

    if (!gbt_ready)
        return;
    memset(at_cmd, 0, sizeof(at_cmd));
    //uint32_t IPAddress = (32 << 24) | (13 << 16) | (168 << 8) | (192 << 0);
    sprintf(at_cmd, "AT+IPADDR=%d.%d.%d.%d\r\n", (ipAddr >> 0) & 0xFF, 
    	(ipAddr >> 8) & 0xFF, (ipAddr >> 16) & 0xFF, (ipAddr >> 24) & 0xFF);
    printf("at_cmd:%s\r\n", at_cmd);
    console_send_atcmd(at_cmd, strlen(at_cmd));
}

static void notify_iphone_ssid_passwd()
{
    char at_cmd[128]            = {0};
    char bt_ssid_hex_str[64]    = {0};
    char bt_passwd_hex_str[32]  = {0};

    if (!gbt_ready)
        return;
    
    memset(at_cmd, 0, sizeof(at_cmd));

    str2asiistr((const char *)ap_ssid, strlen((const char *)ap_ssid), bt_ssid_hex_str, 2 * strlen((const char *)ap_ssid));
    printf("bt_ssid_hex_str:%s\r\n", bt_ssid_hex_str);
    sprintf(at_cmd, "AT+GATTSET=AAA2%s\r\n", bt_ssid_hex_str);
    printf("ssid   at:%s     |||  \r\n", at_cmd);
    console_send_atcmd(at_cmd, strlen(at_cmd));

    memset(at_cmd, 0, sizeof(at_cmd));
    str2asiistr((const char *)ap_passwd, strlen((const char *)ap_passwd), bt_passwd_hex_str, 2 * strlen((const char *)ap_passwd));
    sprintf(at_cmd, "AT+GATTSET=AAA3%s\r\n", bt_passwd_hex_str);
    printf("passwd at:%s\r\n", at_cmd);
    console_send_atcmd(at_cmd, strlen(at_cmd));
}

static void notify_iphone_ap_connect_status(int status)
{
    char at_cmd[128]             = {0};
    char json_str[64]              = {0};
    char json_hex_str[128]     = {0};

    if (!gbt_ready)
        return;

    sprintf(json_str, "{\"fun\": \"hotspot\",\"type\": \"%d\"}", status);
    printf("\r\n%s    \r\n", json_str);
    str2asiistr(json_str, strlen(json_str), json_hex_str, 2 * strlen(json_str));
    sprintf(at_cmd, "AT+GATTSET=AAA7%s\r\n", json_hex_str);
    printf("ssid   at:%s\r\n", at_cmd);
    console_send_atcmd(at_cmd, strlen(at_cmd));
}

#if !USE_LWIP
static int readn(Socket_t xSocket, uint8_t * pvBuffer, size_t uxBufferLength)
{
    int totalLen = 0, lBytes, retry_count = 0;

    while (uxBufferLength > 0) {
        lBytes = FreeRTOS_recv( xSocket, (void *)pvBuffer, uxBufferLength, 0);
        if (lBytes < 0) {
            printf("FreeRTOS_recv err:%d\r\n", lBytes);
            totalLen = -1;
            break;
        }
        if (lBytes == 0) {
            printf("FreeRTOS_recv lBytes:%d\r\n", lBytes);
            if (retry_count++ > 1)
                break;
        }
        pvBuffer       += lBytes;
        uxBufferLength -= lBytes;
        totalLen       += lBytes;
    }
    return totalLen;
}


static void set_socket_win(Socket_t xSockets)
{
    WinProperties_t xWinProperties;

    memset(&xWinProperties, '\0', sizeof xWinProperties);

    xWinProperties.lTxBufSize   = ipconfigIPERF_TX_BUFSIZE;	/* Units of bytes. */
    xWinProperties.lTxWinSize   = ipconfigIPERF_TX_WINSIZE;	/* Size in units of MSS */
    xWinProperties.lRxBufSize   = ipconfigIPERF_RX_BUFSIZE;	/* Units of bytes. */
    xWinProperties.lRxWinSize   = ipconfigIPERF_RX_WINSIZE; /* Size in units of MSS */

    FreeRTOS_setsockopt( xSockets, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProperties, sizeof( xWinProperties ) );
}

static void ey_send_hu_info(Socket_t xSockets)
{
    uint8_t reply[13] = {0};

    reply[0] = 'O'; reply[1] = 'K';
    WRITE_LE16(reply + 2, get_carlink_video_width());
    WRITE_LE16(reply + 4, get_carlink_video_height());
    WRITE_LE16(reply + 6, get_carlink_video_width());
    WRITE_LE16(reply + 8, get_carlink_video_height());
    reply[10] = get_carlink_video_fps();
    WRITE_LE16(reply + 11, 2000);

    FreeRTOS_send(xSockets, reply, sizeof(reply), 0);
}
uint32_t gey_ts = 0;
#define EY_DEBUG  0

static void ey_network_task_proc(void* arg)
{
    SocketSet_t xFD_Set;
    BaseType_t xResult;
    Socket_t xSockets = FREERTOS_INVALID_SOCKET;
    Socket_t xClientSocket = FREERTOS_INVALID_SOCKET;
    Socket_t xXSSockets = FREERTOS_INVALID_SOCKET;
    Socket_t xXSClientSocket = FREERTOS_INVALID_SOCKET;
    static const TickType_t xNoTimeOut = pdMS_TO_TICKS( 2000 );
    struct freertos_sockaddr xAddress, xRemoteAddr;
    uint32_t xClientLength = sizeof( struct freertos_sockaddr );
    int32_t lBytes;
    uint8_t *header_ptr;
    msg_header   header;
#if EY_DEBUG
    void *h264SrcBuf = NULL;
#endif
    uint8_t *h264SrcBufPtr = NULL;
    int32_t h264SrcSize = 0;
    int i;
    video_frame_s* frame = NULL;
    TickType_t xBlockingTime = pdMS_TO_TICKS( 2000 );uint32_t ts/*, r_ts*/;
    int cur_data_size = 0, pre_data_size = 0, retry_count;
    int err_flag = 0;

#if EY_DEBUG
    h264SrcBuf = pvPortMalloc(H264DEC_INBUF_SIZE);
    if(!h264SrcBuf) {
        printf("%s, h264SrcBuf pvPortMalloc failed.\n", __func__);
        return ;
    }
#endif
#if DISABLE_CARLINK_H264_FRAME_BUF
	void* video_handle = NULL;
#endif
    xFD_Set = FreeRTOS_CreateSocketSet();
    xSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt(xSockets, 0, FREERTOS_SO_RCVTIMEO, &xNoTimeOut, sizeof(xNoTimeOut));
    //FreeRTOS_setsockopt(xXSSockets, 0, FREERTOS_SO_REUSE_LISTEN_SOCKET, (void *)&option, sizeof(option));
    set_socket_win(xSockets);
    xAddress.sin_port = ( uint16_t ) 11111;
    xAddress.sin_port = FreeRTOS_htons( xAddress.sin_port );
    FreeRTOS_bind( xSockets, &xAddress, sizeof( xAddress ) );
    FreeRTOS_listen( xSockets, 1 );

    xXSSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xXSSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt(xXSSockets, 0, FREERTOS_SO_RCVTIMEO, &xNoTimeOut, sizeof(xNoTimeOut));
    //FreeRTOS_setsockopt(xXSSockets, 0, FREERTOS_SO_REUSE_LISTEN_SOCKET, (void *)&option, sizeof(option));
    set_socket_win(xXSSockets);
    xAddress.sin_port = ( uint16_t ) 11113;
    xAddress.sin_port = FreeRTOS_htons( xAddress.sin_port );
    FreeRTOS_bind( xXSSockets, &xAddress, sizeof( xAddress ) );
    FreeRTOS_listen( xXSSockets, 1 );

    while(1) {
        if (err_flag != 0) {
            xSemaphoreTake(gSocketMutex, portMAX_DELAY);
            if (FREERTOS_INVALID_SOCKET != xClientSocket) {
                FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
                FreeRTOS_closesocket(xClientSocket);
                xClientSocket = FREERTOS_INVALID_SOCKET;
            }
            if (FREERTOS_INVALID_SOCKET != xXSClientSocket) {
                FreeRTOS_FD_CLR(xXSClientSocket, xFD_Set, eSELECT_READ);
                FreeRTOS_closesocket(xXSClientSocket);
                xXSClientSocket = FREERTOS_INVALID_SOCKET;
            }
            
            gClientSocket   = FREERTOS_INVALID_SOCKET;
            gXClientSocket = FREERTOS_INVALID_SOCKET;
            xSemaphoreGive(gSocketMutex);
            notify_carlink_state(0);
            err_flag = 0;
        }
        FreeRTOS_FD_CLR(xSockets, xFD_Set, eSELECT_READ);
        FreeRTOS_FD_SET(xSockets, xFD_Set, eSELECT_READ);
        FreeRTOS_FD_CLR(xXSSockets, xFD_Set, eSELECT_READ);
        FreeRTOS_FD_SET(xXSSockets, xFD_Set, eSELECT_READ);
        if (xClientSocket && xClientSocket != FREERTOS_INVALID_SOCKET) {
            FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
            FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
        }
        if (xXSClientSocket && xXSClientSocket != FREERTOS_INVALID_SOCKET) {
            FreeRTOS_FD_CLR(xXSClientSocket, xFD_Set, eSELECT_READ);
            FreeRTOS_FD_SET(xXSClientSocket, xFD_Set, eSELECT_READ);
        }
        xResult = FreeRTOS_select( xFD_Set, xBlockingTime );
        if (xResult == 0) {//printf("select timeout\r\n");
            if (FREERTOS_INVALID_SOCKET != xClientSocket) {//maybe socket don't recv close info;
                if (cur_data_size == pre_data_size) {// no data recv
                    printf("no data, retry count:%d\r\n", retry_count);
                    if (retry_count++ >= 1) {// no data for 4s
                        err_flag = 1;
                        notify_carlink_state(0);
                    }
                }
                pre_data_size = cur_data_size;
            }
            xBlockingTime = pdMS_TO_TICKS( 2000 );
            continue;
        }
        if (xResult < 0) {
            break;
        }
        pre_data_size = cur_data_size;

        if (0){
        } else if(FreeRTOS_FD_ISSET ( xSockets, xFD_Set )) {
            xClientSocket = FreeRTOS_accept( xSockets, &xRemoteAddr, &xClientLength);
            if((xClientSocket != NULL) && (xClientSocket != FREERTOS_INVALID_SOCKET)) {
                char pucBuffer[32] = {0};
                FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
                FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
                FreeRTOS_GetRemoteAddress( xClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
                FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
                set_socket_win(xClientSocket);
                printf("\r\nServer 11111: Received a connection from %s:%u\r\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
                cur_data_size = 0;
                pre_data_size = 0;
                retry_count = 0;
                h264_dec_ctx_init();
                notify_carlink_state(1);
                xSemaphoreTake(gSocketMutex, portMAX_DELAY);
                gClientSocket = xClientSocket;
                xSemaphoreGive(gSocketMutex);
            }
            continue;
        } else if (FreeRTOS_FD_ISSET ( xXSSockets, xFD_Set )) {
            xXSClientSocket = FreeRTOS_accept( xXSSockets, &xRemoteAddr, &xClientLength);
            if ((xXSClientSocket != NULL) && (xXSClientSocket != FREERTOS_INVALID_SOCKET)) {
                char pucBuffer[32] = {0};
                FreeRTOS_FD_CLR(xXSClientSocket, xFD_Set, eSELECT_READ);
                FreeRTOS_FD_SET(xXSClientSocket, xFD_Set, eSELECT_READ);
                FreeRTOS_GetRemoteAddress( xXSClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
                FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
                printf("\r\nServer 11113: Received a connection from %s:%u\r\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
                set_socket_win(xXSClientSocket);
                xSemaphoreTake(gSocketMutex, portMAX_DELAY);
                gXClientSocket = xXSClientSocket;
                xSemaphoreGive(gSocketMutex);
            }
            continue;
        } else if (FreeRTOS_FD_ISSET ( xXSClientSocket, xFD_Set )) {
            lBytes = FreeRTOS_recv( xXSClientSocket, (void*)&header, sizeof(header), 0);
            if (lBytes < 0) {
                printf("FreeRTOS_recv port 11113 err:%d\r\n", lBytes);
                err_flag = 1;
            } else {
                cur_data_size += lBytes;
            }
            
            continue;
        }

        lBytes = FreeRTOS_recv( xClientSocket, (void*)&header, sizeof(header), 0);

        if (lBytes < 0) {
            printf("FreeRTOS_recv port 11111 err:%d\r\n", lBytes);
            err_flag = 1;
            continue;
        }
        cur_data_size += lBytes;

        if (lBytes == 0)
            continue;

        if (!(header.sync_word[0] == 0xAA && header.sync_word[1] == 0xBB && header.sync_word[2] == 0xCC))
            continue;

        if (header.type == 4) {
            uint16_t height, width;
            header_ptr = (uint8_t *)&header;
            for (i = 0; i < lBytes; i++) {
                printf("%02x ", header_ptr[i]);
            }printf("\r\n");

            height = (header_ptr[12] | (header_ptr[13] << 8));
            width  = (header_ptr[14] | (header_ptr[15] << 8));
            printf("#receive phone info width:%d height:%d\r\n", width, height);gey_ts = get_timer(0);//r_ts = header.ts;

            ey_send_hu_info(xClientSocket);
            continue;
        } else if (header.type == 6) {
            printf("heart beat\r\n");
            continue;
        } else if (header.type != 0) {
            continue;
        }
        
        h264SrcSize    = header.payload_size - 4;//skip timestamp
        if (h264SrcSize <= 0 || h264SrcSize > H264_FRAME_BUF_SIZE) {
            printf("h264 data len:%d err\r\n", h264SrcSize);
            continue;
        }
#if EY_DEBUG
        h264SrcBufPtr  = (uint8_t *)h264SrcBuf;
#else
#if !DISABLE_CARLINK_H264_FRAME_BUF
get_retry:
        frame = get_h264_frame_buf();
        if (NULL == frame) {
            printf("h264 frame is empty\r\n");
            vTaskDelay(pdMS_TO_TICKS(20));
            goto get_retry;
            //continue;
        }

        h264SrcBufPtr  = (uint8_t *)frame->cur;
        frame->len     = h264SrcSize;
#else
		if (NULL == video_handle) {
			video_handle = h264_video_player_init();
		}
		if (NULL == h264SrcBufPtr)
			h264SrcBufPtr = pvPortMalloc(H264DEC_INBUF_SIZE);
		if (NULL == h264SrcBufPtr) {
			printf("h264SrcBufPtr malloc failed\n");
			return;
		}
#endif
#endif
        //ts = get_timer(0);
        //printf("Size:%05d lts:%03d rts:%06d\r\n", h264SrcSize, (ts - gey_ts) / 1000, header.ts - r_ts);
        gey_ts = ts;//r_ts = header.ts;
        lBytes = readn(xClientSocket, h264SrcBufPtr, h264SrcSize);
#if !DISABLE_CARLINK_H264_FRAME_BUF
        if (h264SrcSize != lBytes) {
            set_h264_frame_free(frame);
            if (lBytes == -1) {
                printf("###FreeRTOS_recv port 11111 err:%d\r\n", lBytes);
                err_flag = 1;
            } else{
                cur_data_size += lBytes;
            }
        } else {
            cur_data_size += lBytes;
            notify_h264_frame_ready(&frame);
        }
#else
		 if (h264SrcSize == lBytes) {
		 	h264_video_player_proc(video_handle, h264SrcBufPtr, h264SrcSize);
		 }
#endif
    }

    printf("%s exit...\r\n", __func__);
    xSemaphoreTake(gSocketMutex, portMAX_DELAY);
    if (FREERTOS_INVALID_SOCKET != xSockets)
        FreeRTOS_closesocket(xSockets);
    if (FREERTOS_INVALID_SOCKET != xXSSockets)
        FreeRTOS_closesocket(xXSSockets);
    if (FREERTOS_INVALID_SOCKET != xClientSocket)
        FreeRTOS_closesocket(xClientSocket);
    if (FREERTOS_INVALID_SOCKET != xXSClientSocket)
        FreeRTOS_closesocket(xXSClientSocket);
    gClientSocket   = FREERTOS_INVALID_SOCKET;
    gXClientSocket = FREERTOS_INVALID_SOCKET;
    xSemaphoreGive(gSocketMutex);
#if DISABLE_CARLINK_H264_FRAME_BUF
	h264_video_player_uninit(video_handle);
	if (NULL == h264SrcBufPtr) {
		free(h264SrcBufPtr);
	}
#endif
    vTaskDelete(NULL);
}
#else
static int readn(int xSocket, uint8_t * pvBuffer, size_t uxBufferLength)
{
    int totalLen = 0, lBytes, retry_count = 0;

    while (uxBufferLength > 0) {
		lBytes = recv( xSocket, (void *)pvBuffer, uxBufferLength, 0);
        if (lBytes < 0) {
            printf("FreeRTOS_recv err:%d\r\n", lBytes);
            totalLen = -1;
            break;
        }
        if (lBytes == 0) {
            printf("FreeRTOS_recv lBytes:%d\r\n", lBytes);
            if (retry_count++ > 1)
                break;
        }
        pvBuffer       += lBytes;
        uxBufferLength -= lBytes;
        totalLen       += lBytes;
    }
    return totalLen;
}


static void set_socket_win(int xSockets)
{
	(void)xSockets;
}

static void ey_send_hu_info(int xSockets)
{
    uint8_t reply[13] = {0};

    reply[0] = 'O'; reply[1] = 'K';
    WRITE_LE16(reply + 2, get_carlink_video_width());
    WRITE_LE16(reply + 4, get_carlink_video_height());
    WRITE_LE16(reply + 6, get_carlink_video_width());
    WRITE_LE16(reply + 8, get_carlink_video_height());
    reply[10] = get_carlink_video_fps();
    WRITE_LE16(reply + 11, 2000);

	send(xSockets, reply, sizeof(reply), 0);
}

uint32_t gey_ts = 0;
#define EY_DEBUG  0

static void ey_network_task_proc(void* arg)
{
    fd_set xFD_Set;
    int xResult;
    int xSockets = -1;
    int xClientSocket = -1;
    int xXSSockets = -1;
    int xXSClientSocket = -1;
    static const struct timeval xNoTimeOut = { 2, 0 };
    struct sockaddr_in xAddress, xRemoteAddr;
    socklen_t xClientLength = sizeof( struct sockaddr_in );
    int32_t lBytes;
    uint8_t *header_ptr;
    msg_header   header;
#if EY_DEBUG
    void *h264SrcBuf = NULL;
#endif
    uint8_t *h264SrcBufPtr = NULL;
    int32_t h264SrcSize = 0;
    int i;
    video_frame_s* frame = NULL;
    struct timeval xBlockingTime = { 2, 0 };uint32_t ts/*, r_ts*/;
    int cur_data_size = 0, pre_data_size = 0, retry_count;
    int err_flag = 0;

#if EY_DEBUG
    h264SrcBuf = pvPortMalloc(H264DEC_INBUF_SIZE);
    if(!h264SrcBuf) {
        printf("%s, h264SrcBuf pvPortMalloc failed.\n", __func__);
        return ;
    }
#endif
#if DISABLE_CARLINK_H264_FRAME_BUF
	void* video_handle = NULL;
#endif

    xSockets = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    configASSERT( xSockets != -1 );
    setsockopt(xSockets, SOL_SOCKET, SO_RCVTIMEO, &xNoTimeOut, sizeof(xNoTimeOut));
    set_socket_win(xSockets);
    xAddress.sin_port 			= htons(11111);
	xAddress.sin_family      	= AF_INET;
    xAddress.sin_addr.s_addr 	= htonl(INADDR_ANY);
    bind( xSockets, (struct sockaddr *)&xAddress, sizeof( xAddress ) );
    listen( xSockets, 1 );

    xXSSockets = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    configASSERT( xXSSockets != -1 );
    setsockopt(xXSSockets, SOL_SOCKET, SO_RCVTIMEO, &xNoTimeOut, sizeof(xNoTimeOut));
    set_socket_win(xXSSockets);
    xAddress.sin_port 			= htons(11113);
	xAddress.sin_family      	= AF_INET;
    xAddress.sin_addr.s_addr 	= htonl(INADDR_ANY);
    bind( xXSSockets, (struct sockaddr *)&xAddress, sizeof( xAddress ) );
    listen( xXSSockets, 1 );

    while(1) {
        if (err_flag != 0) {
            if (xClientSocket != -1) {
                FD_CLR(xClientSocket, &xFD_Set);
                close(xClientSocket);
                xClientSocket = -1;
            }
            if (xXSClientSocket != -1) {
                FD_CLR(xXSClientSocket, &xFD_Set);
                close(xXSClientSocket);
                xXSClientSocket = -1;
            }
            
            notify_carlink_state(0);
            err_flag = 0;
        }
        FD_ZERO(&xFD_Set);
        FD_SET(xSockets, &xFD_Set);
        FD_SET(xXSSockets, &xFD_Set);
        if (xClientSocket != -1) {
            FD_SET(xClientSocket, &xFD_Set);
        }
        if (xXSClientSocket != -1) {
            FD_SET(xXSClientSocket, &xFD_Set);
        }
        xResult = select( FD_SETSIZE, &xFD_Set, NULL, NULL, &xBlockingTime );
        if (xResult == 0) {//printf("select timeout\r\n");
            if (xClientSocket != -1) {//maybe socket don't recv close info;
                if (cur_data_size == pre_data_size) {// no data recv
                    printf("no data, retry count:%d\r\n", retry_count);
                    if (retry_count++ >= 1) {// no data for 4s
                        err_flag = 1;
                        notify_carlink_state(0);
                    }
                }
                pre_data_size = cur_data_size;
            }
            xBlockingTime.tv_sec = 2;
            xBlockingTime.tv_usec = 0;
            continue;
        }
        if (xResult < 0) {
            break;
        }
        pre_data_size = cur_data_size;

        if (0){
        } else if(FD_ISSET ( xSockets, &xFD_Set )) {
            xClientSocket = accept( xSockets, (struct sockaddr *)&xRemoteAddr, &xClientLength);
            if(xClientSocket != -1) {
                char pucBuffer[32] = {0};
                FD_CLR(xClientSocket, &xFD_Set);
                FD_SET(xClientSocket, &xFD_Set);
                inet_ntop(AF_INET, &xRemoteAddr.sin_addr, pucBuffer, sizeof(pucBuffer));
                set_socket_win(xClientSocket);
                printf("\r\nServer 11111: Received a connection from %s:%u\r\n", pucBuffer, ntohs(xRemoteAddr.sin_port));
                cur_data_size = 0;
                pre_data_size = 0;
                retry_count = 0;
                h264_dec_ctx_init();
                notify_carlink_state(1);
            }
            continue;
        } else if (FD_ISSET ( xXSSockets, &xFD_Set )) {
            xXSClientSocket = accept( xXSSockets, (struct sockaddr *)&xRemoteAddr, &xClientLength);
            if (xXSClientSocket != -1) {
                char pucBuffer[32] = {0};
                FD_CLR(xXSClientSocket, &xFD_Set);
                FD_SET(xXSClientSocket, &xFD_Set);
                inet_ntop(AF_INET, &xRemoteAddr.sin_addr, pucBuffer, sizeof(pucBuffer));
                printf("\r\nServer 11113: Received a connection from %s:%u\r\n", pucBuffer, ntohs(xRemoteAddr.sin_port));
                set_socket_win(xXSClientSocket);
            }
            continue;
        } else if (FD_ISSET ( xXSClientSocket, &xFD_Set )) {
            lBytes = recv( xXSClientSocket, (void*)&header, sizeof(header), 0);
            if (lBytes < 0) {
                printf("recv port 11113 err:%d\r\n", lBytes);
                err_flag = 1;
            } else {
                cur_data_size += lBytes;
            }
            
            continue;
        }

        lBytes = recv( xClientSocket, (void*)&header, sizeof(header), 0);

        if (lBytes < 0) {
            printf("recv port 11111 err:%d\r\n", lBytes);
            err_flag = 1;
            continue;
        }
        cur_data_size += lBytes;

        if (lBytes == 0)
            continue;

        if (!(header.sync_word[0] == 0xAA && header.sync_word[1] == 0xBB && header.sync_word[2] == 0xCC))
            continue;

        if (header.type == 4) {
            uint16_t height, width;
            header_ptr = (uint8_t *)&header;
            for (i = 0; i < lBytes; i++) {
                printf("%02x ", header_ptr[i]);
            }printf("\r\n");

            height = (header_ptr[12] | (header_ptr[13] << 8));
            width  = (header_ptr[14] | (header_ptr[15] << 8));
            printf("#receive phone info width:%d height:%d\r\n", width, height);gey_ts = get_timer(0);//r_ts = header.ts;

            ey_send_hu_info(xClientSocket);
            continue;
        } else if (header.type == 6) {
            printf("heart beat\r\n");
            continue;
        } else if (header.type != 0) {
            continue;
        }
        
        h264SrcSize    = header.payload_size - 4;//skip timestamp
        if (h264SrcSize <= 0 || h264SrcSize > H264_FRAME_BUF_SIZE) {
            printf("h264 data len:%d err\r\n", h264SrcSize);
            continue;
        }
#if EY_DEBUG
        h264SrcBufPtr  = (uint8_t *)h264SrcBuf;
#else
#if !DISABLE_CARLINK_H264_FRAME_BUF
get_retry:
        frame = get_h264_frame_buf();
        if (NULL == frame) {
            printf("h264 frame is empty\r\n");
            vTaskDelay(pdMS_TO_TICKS(20));
            goto get_retry;
            //continue;
        }

        h264SrcBufPtr  = (uint8_t *)frame->cur;
        frame->len     = h264SrcSize;
#else
		if (NULL == video_handle) {
			video_handle = h264_video_player_init();
		}
		if (NULL == h264SrcBufPtr)
			h264SrcBufPtr = pvPortMalloc(H264DEC_INBUF_SIZE);
		if (NULL == h264SrcBufPtr) {
			printf("h264SrcBufPtr malloc failed\n");
			break;
		}
#endif
#endif
        //ts = get_timer(0);
        //printf("Size:%05d lts:%03d rts:%06d\r\n", h264SrcSize, (ts - gey_ts) / 1000, header.ts - r_ts);
        gey_ts = ts;//r_ts = header.ts;
        lBytes = readn(xClientSocket, h264SrcBufPtr, h264SrcSize);
#if !DISABLE_CARLINK_H264_FRAME_BUF
        if (h264SrcSize != lBytes) {
            set_h264_frame_free(frame);
            if (lBytes == -1) {
                printf("###recv port 11111 err:%d\r\n", lBytes);
                err_flag = 1;
            } else{
                cur_data_size += lBytes;
            }
        } else {
            cur_data_size += lBytes;
            notify_h264_frame_ready(&frame);
        }
#else
		if (h264SrcSize == lBytes) {
			h264_video_player_proc(video_handle, h264SrcBufPtr, h264SrcSize);
		}
#endif
    }

    printf("%s exit...\r\n", __func__);
    if (xSockets != -1)
        close(xSockets);
    if (xXSSockets != -1)
        close(xXSSockets);
    if (xClientSocket != -1)
        close(xClientSocket);
    if (xXSClientSocket != -1)
        close(xXSClientSocket);
#if DISABLE_CARLINK_H264_FRAME_BUF
		h264_video_player_uninit(video_handle);
		if (NULL == h264SrcBufPtr) {
			free(h264SrcBufPtr);
		}
#endif
    vTaskDelete(NULL);
}

#endif

static int ey_start_network()
{
	BaseType_t ret;
	ret = xTaskCreate(ey_network_task_proc, "ey_network", 2048 * 8, NULL, configMAX_PRIORITIES - 3, &ey_network_task);
	return ret;
}

void ey_wifi_event_handler( WIFIEvent_t * xEvent )
{
    WIFIEventType_t xEventType = xEvent->xEventType;

    if (0) {
    } else if (eWiFiEventConnected                    == xEventType) {// meter is sta
        notify_iphone_ap_connect_status(0);
        printf("\r\n The meter is connected to ap \r\n");
    } else if (eWiFiEventDisconnected                == xEventType) {// meter is sta
        printf("\r\n The meter is disconnected from ap \r\n");
#if !USE_LWIP
        xSemaphoreTake(gSocketMutex, portMAX_DELAY);
        if (gClientSocket != FREERTOS_INVALID_SOCKET)
            FreeRTOS_SignalSocket(gClientSocket);// wake up socket recv
        if (gXClientSocket != FREERTOS_INVALID_SOCKET)
            FreeRTOS_SignalSocket(gClientSocket);
        xSemaphoreGive(gSocketMutex);
#endif
        notify_phone_ip_addr(0);
        notify_carlink_state(0);
    } else if (eWiFiEventAPStationConnected       == xEventType) {// meter is ap
        printf("\r\n The meter in AP is connected by sta %s \r\n", xEvent->xInfo.xAPStationConnected.ucMac);
    } else if (eWiFiEventAPStationDisconnected   == xEventType) {// meter is ap
        printf("\r\n The sta %s is disconnected from the meter \r\n", xEvent->xInfo.xAPStationDisconnected.ucMac);
        //notify_phone_ip_addr(0);
    }
}


int mmcsd_wait_sdio_ready(int32_t timeout);
static int carlink_ey_network_init()
{
    BaseType_t ret;
    unsigned int status;
#if !USE_LWIP
    gSocketMutex = xSemaphoreCreateMutex();
#endif
    WIFI_Context_init();
    WIFI_RegisterEvent(eWiFiEventMax, ey_wifi_event_handler);
    for (;;) {
        status = mmcsd_wait_sdio_ready((int32_t)portMAX_DELAY);
        if (status == MMCSD_HOST_PLUGED) {
            printf("detect sdio device\r\n");
            break;
        }
    }
#if !USE_LWIP
    ret = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,ucDNSServerAddress, ucMACAddress);
#else
	ey_lwip_server_init();
#endif
    ey_start_network();
    return (int)ret;
}

static void carlink_ey_network_uninit()
{
}


static void ey_bt_callback(char * cAtStr)
{
    char* cmd = NULL, *cmd_para = NULL;
    struct bt_event ev = {0};
    ev.type = -1;
    printf("\r\nfsc_bt_callback %s\r\n", cAtStr);
    if (0) {
    } else if (0 == strncmp(cAtStr, "+WLCONN",      7)) {
        ev.type = EY_BT_EVENT_NOTIFY_PHONE_AP_READY;

    } else if (0 == strncmp(cAtStr, "+ADDR=",       6)) {
        char cmd_str[64] = {0};
        sprintf(cmd_str, "AT+NAME=EY_%s\r\n", (cAtStr + 6));
        ey_reset_wifi_ap_info(cAtStr + 6);
        printf("ADDR:%s\r\n", cAtStr + 6);
        console_send_atcmd(cmd_str, strlen(cmd_str));//get mac addr
        memset(cmd_str, 0, sizeof(cmd_str));
        sprintf(cmd_str, "AT+LENAME=EY_%s\r\n", (cAtStr + 6));
        console_send_atcmd(cmd_str, strlen(cmd_str));//get mac addr
        gbt_ready = 1;
    } else if (0 == strncmp(cAtStr, "+WLSSID=",     8)) {
        ev.type = EY_BT_EVENT_NOTIFY_SSID;
        cmd_para = cAtStr + 8;
        memcpy((void*)ev.para, (void*)cmd_para, strlen(cmd_para));
    } else if (0 == strncmp(cAtStr, "+WLPASSWD=",   10)) {
        ev.type = EY_BT_EVENT_NOTIFY_PASSWD;
        cmd_para = cAtStr + 10;
        memcpy((void*)ev.para, (void*)cmd_para, strlen(cmd_para));
    } else if (0 == strncmp(cAtStr, "+HFPSTAT=1",   10)) {
        ev.type = EY_BT_EVENT_DISCONNECT;
    } else if (0 == strncmp(cAtStr, "+HFPDEV=",     8)) {
        ev.type = EY_BT_EVENT_CONNECT;
        cmd_para = cAtStr + 8;
        memcpy((void*)ev.para, (void*)cmd_para, 12);
    } else if (0 == strncmp(cAtStr, "+AAA5=",       6)) {
        ev.type = EY_BT_EVENT_NOTIFY_PHONE_TYPE;
        cmd_para = cAtStr + 6;
        memcpy((void*)ev.para, (void*)cmd_para, strlen(cmd_para));
    } else if (0 == strncmp(cAtStr, "+AAA7=",       6)) {
        ev.type = EY_BT_EVENT_NOTIFY_CONNECT_STATUS;
        cmd_para = cAtStr + 7;
        memcpy((void*)ev.para, (void*)cmd_para, strlen(cmd_para));
    } else if (0 == strncmp(cAtStr, "+GATTSTAT=3",  11)) {
        ev.type = EY_BT_EVENT_BLE_CONNECT;
    } else if (0 == strncmp(cAtStr, "+AAA9",        5)) {
        ev.type = EY_BT_EVENT_NOTIFY_USE_TCP;
    } else if (0 == strncmp(cAtStr, "+VER",         4)) {
        cmd = "AT+ADDR\r\n";
        console_send_atcmd(cmd, strlen(cmd));//get mac addr
    }

    if (-1 != ev.type && NULL != ey_bt_event_queue) {
        xQueueSend(ey_bt_event_queue, &ev, 0);
    }
}

static void ey_bt_task_proc(void* arg)
{
    struct bt_event ev;
    char* cmd_para      = NULL;
    char at_cmd[128]     = {0};
    char phone_type     = -1;
    char ssid[32]       = {0};
    char passwd[32]     = {0};
    char bt_mac[12 + 1] = {0};
    int ret             = -1;
    int carlink_ready_flag = 0;

    for (;;) {
        memset((void*)&ev, 0, sizeof(ev));
        if (xQueueReceive(ey_bt_event_queue, &ev,  portMAX_DELAY) != pdPASS) {
            printf("%s xQueueReceive err!\r\n", __func__);
            continue;
        }
        cmd_para = (char*)ev.para;
        if (0) {
        } else if (EY_BT_EVENT_NOTIFY_CONNECT_STATUS == ev.type) {
        } else if (EY_BT_EVENT_CONNECT               == ev.type) {
            memcpy(bt_mac, cmd_para, 12);
            bt_mac[12] = '0';

        } else if (EY_BT_EVENT_DISCONNECT            == ev.type) {
            memset(ssid,   0, sizeof(ssid));
            memset(passwd, 0, sizeof(passwd));
            memset(bt_mac, 0, sizeof(bt_mac));
            phone_type = -1;
            notify_iphone_ssid_passwd();
            notify_phone_ip_addr(0);
        } else if (EY_BT_EVENT_NOTIFY_PHONE_TYPE     == ev.type) {
            string2hex(cmd_para, strlen(cmd_para), &phone_type, 2);
            printf("\r\nphone type:%d\n", phone_type);
            if (phone_type != gphone_type) {
                gWifiOn = 0;// reboot wifi
                WIFI_Off();
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            gphone_type = phone_type;
            if (0 == phone_type) {//android
                char type_str[5] = {0};
                char *tmp_buf = "-1";
                str2asiistr(tmp_buf, strlen(tmp_buf), type_str, 2 * strlen(tmp_buf));
                sprintf(at_cmd, "AT+GATTSET=AAA2%s\r\n", type_str);
                printf("\r\nat:%s\r\n", at_cmd);
                console_send_atcmd(at_cmd, strlen(at_cmd));
            } else if (1 == phone_type) {//iphone  
#if !USE_LWIP
                setDhcpClientState(0);
                vDHCPProcess(1, eInitialWait);
                xSendDHCPEvent();
#else
				netif_set_down(&gnetif[0]);
#endif
                notify_iphone_ssid_passwd();
                ey_wifi_start_ap(ap_ssid, ap_passwd, 36);
#if USE_LWIP
				
				ey_lwip_server_init();
				netif_set_up(&gnetif[0]);

#endif
                uint32_t ap_addr = (ucIPAddress[0]) | (ucIPAddress[1] << 8) | (ucIPAddress[2] << 16) | (ucIPAddress[3] << 24);
                notify_phone_ip_addr(ap_addr);
            }
        } else if (EY_BT_EVENT_NOTIFY_SSID           == ev.type) {
            memcpy(ssid,    cmd_para, strlen(cmd_para) + 1);
            printf("\r\nssid:%s\r\n", ssid);
        } else if (EY_BT_EVENT_NOTIFY_PASSWD         == ev.type) {
            memcpy(passwd,  cmd_para, strlen(cmd_para) + 1);
            printf("\r\npasswd:%s\r\n", passwd);
        } else if (EY_BT_EVENT_NOTIFY_PHONE_AP_READY == ev.type) {
            printf("\r\nssid:%s passwd:%s\r\n", ssid, passwd);
#if !USE_LWIP
            setDhcpClientState(1);
            vDHCPProcess(1, eInitialWait);
            xSendDHCPEvent();
#else
			netif_set_down(&gnetif[0]);
#endif
            notify_iphone_ap_connect_status(4);
            ret = ey_wifi_connect_remote_ap(ssid, passwd);
#if !USE_LWIP
            if (ret == 0) {
                vDHCPProcess(1, eWaitingSendFirstDiscover);
                xSendDHCPEvent();
            }
#else
			ey_lwip_client_init();
#endif
        } else if (EY_BT_EVENT_NOTIFY_USE_TCP        == ev.type) {
        } else if (EY_BT_EVENT_BLE_CONNECT           == ev.type) {
            char bt_mac_hex_str[25] = {0};
            str2asiistr(bt_mac, 12, bt_mac_hex_str, 24);
            sprintf(at_cmd, "AT+GATTSEND=AAA5%s\r\n", bt_mac_hex_str);
            printf("\r\nat:%s\r\n", at_cmd);
            console_send_atcmd(at_cmd, strlen(at_cmd));
        } else if (EY_CARLINK_EVENT_CONNECT         == ev.type) {
            carlink_ready_flag = 1;
            printf("EY_CARLINK_EVENT_CONNECT\r\n");
            set_carlink_display_state(1);
 #if EY_CARLINK_NOTIFY_HOOK
            carlink_ey_notify_state_hook(CARLINK_EY_READY);
 #endif
        } else if (EY_CARLINK_EVENT_DISCONNECT    == ev.type) {
            if (carlink_ready_flag == 1) {
                printf("EY_CARLINK_EVENT_DISCONNECT\r\n");
                carlink_ready_flag = 0;
                set_carlink_display_state(0);
#if EY_CARLINK_NOTIFY_HOOK
                carlink_ey_notify_state_hook(CARLINK_EY_EXIT);
#endif
            }
        }
    }

	vTaskDelete(NULL);
}

static int carlink_ey_bt_init()
{
    int ret = -1;

    ret = fsc_bt_main();
    if (ret != 0)
        return ret;
    console_register_cb(NULL, ey_bt_callback);

    ey_bt_event_queue = xQueueCreate(1, sizeof(struct bt_event));
    ret = xTaskCreate(ey_bt_task_proc, "ey_bt", 2048 * 8, NULL, configMAX_PRIORITIES - 1, &ey_bt_task);
    return ret;
}

static void carlink_ey_bt_uninit()
{
}

static void carlink_ey_init_thread(void *param)
{
    carlink_ey_audio_init();
    carlink_ey_video_init();
    carlink_ey_network_init();
    carlink_ey_bt_init();
    //ey_wifi_start_ap(ap_ssid, ap_passwd, 36);
    vTaskDelete(NULL);
}

int carlink_ey_init()
{
#if 0
    int ret = -1;

    ret = carlink_ey_audio_init();
    ret = carlink_ey_video_init();
    ret = carlink_ey_network_init();
    ret = carlink_ey_bt_init();
    //ey_wifi_start_ap(ap_ssid, ap_passwd, 36);

	return ret;
#else
	//避免wifi模块异常后导致初始化接口阻塞，影响机器正常启动。
	xTaskCreate(carlink_ey_init_thread, "carlink_ey_init", 1024 * 16, NULL, 8, NULL);
    return 0;
#endif
}

void carlink_ey_uninit()
{
    carlink_ey_bt_uninit();
    carlink_ey_network_uninit();
}
#else
int carlink_ey_init()
{
    return 0;
}
void carlink_ey_uninit()
{
}
#endif
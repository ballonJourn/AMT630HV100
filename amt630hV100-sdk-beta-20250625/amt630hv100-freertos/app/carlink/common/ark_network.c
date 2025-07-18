#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include "mmcsd_core.h"
#include "board.h"
#include "timer.h"

#include "iot_wifi.h"

const uint8_t        ucIPAddress[4] = {192, 168, 13, 1};
const uint8_t        ucNetMask[4] = {255, 255, 255, 0};
const uint8_t        ucGatewayAddress[4] = {0, 0, 0, 0};
const uint8_t        ucDNSServerAddress[4] = {8, 8, 8, 8};
const uint8_t        ucMACAddress[6] = {0x30, 0x4a, 0x26, 0x78, 0xfd, 0x12};


#if !USE_LWIP
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_DHCP_Server.h"
#include "iperf_task.h"



//static uint8_t                ap_ssid[64]     = {"ap63011"};
//static uint8_t                ap_passwd[16]  = {"88888888"};

int vTestTCPClientSocket( void )
{
	SocketSet_t xFD_Set = NULL;
	Socket_t xSocket = FREERTOS_INVALID_SOCKET;
	struct freertos_sockaddr xServerAddress;
	WinProperties_t xWinProps;
	BaseType_t ret;
	uint8_t wifi_data_buffer[2048] = {0};
	int xReturn = -1;

	start_sta("AndroidAP", "12345678", 1);

	xFD_Set = FreeRTOS_CreateSocketSet();
	if (NULL == xFD_Set) {
		return xReturn;
	}
	/* Create a TCP socket. */
	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
	if( xSocket != FREERTOS_INVALID_SOCKET ) {
		printf( "Open socket failed\r\n");
		goto exit;
	}

	if (0) {
		/* Set a time out so a missing reply does not cause the task to block indefinitely. */
		const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 4000 );
		const TickType_t xSendTimeOut = pdMS_TO_TICKS( 2000 );
		ret = FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
		ret = FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xSendTimeOut ) );
	}

	/* Set the window and buffer sizes. */
	xWinProps.lTxBufSize   = ipconfigIPERF_TX_BUFSIZE;	/* Units of bytes. */
	xWinProps.lTxWinSize   = ipconfigIPERF_TX_WINSIZE;	/* Size in units of MSS */
	xWinProps.lRxBufSize   = ipconfigIPERF_RX_BUFSIZE;	/* Units of bytes. */
	xWinProps.lRxWinSize   = ipconfigIPERF_RX_WINSIZE; /* Size in units of MSS */
	FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );

	/* Connect to the echo server. */
	printf( "connecting to echo server....\r\n" );
	xServerAddress.sin_port = FreeRTOS_htons( 10010 );
	xServerAddress.sin_addr = FreeRTOS_inet_addr_quick( 192, 168, 43, 1 );
	ret = FreeRTOS_connect( xSocket, &xServerAddress, sizeof( xServerAddress ) );
	if( ret != 0 )		{
		printf( "Could not connect to server %ld\r\n", ret );
		goto exit;
	}


	ret = FreeRTOS_send( xSocket, (void*)wifi_data_buffer, sizeof wifi_data_buffer, 0 );
	if( ret < 0 ) {
		printf( "Could not send data to server %ld\r\n", ret );
		goto exit;
	}

	while (1) {//waiting for data from server
		FreeRTOS_FD_CLR(xSocket, xFD_Set, eSELECT_READ);
		FreeRTOS_FD_SET(xSocket, xFD_Set, eSELECT_READ);
		ret = FreeRTOS_select( xFD_Set, portMAX_DELAY );
		if (ret < 0) {
			printf("Select failed\r\n");
			break;
		}

		if( FreeRTOS_FD_ISSET ( xSocket, xFD_Set ) ) {
			ret = FreeRTOS_recv(xSocket, wifi_data_buffer, sizeof wifi_data_buffer, 0);
			if (ret > 0) {
				printf("recv buf size:%d\r\n", ret);
			} else {
				printf("FreeRTOS_recv err:%d\r\n", ret);
				break;
			}
		}
	}
	xReturn = 0;

exit:
	/* Close this socket before looping back to create another. */
	if ( xSocket != FREERTOS_INVALID_SOCKET)
		FreeRTOS_closesocket( xSocket );

	if (NULL != xFD_Set)
		FreeRTOS_DeleteSocketSet(xFD_Set);
	return xReturn;
}

static int vTestTCPServerSocket( void )
{
	SocketSet_t xFD_Set;
	struct freertos_sockaddr xAddress, xRemoteAddr;
	Socket_t xSockets = FREERTOS_INVALID_SOCKET, xClientSocket = FREERTOS_INVALID_SOCKET;
	socklen_t xClientLength = sizeof( xAddress );
	static const TickType_t xNoTimeOut = portMAX_DELAY;
	BaseType_t ret = -1;
	BaseType_t xResult;
	uint8_t wifi_data_buffer[2048] = {0};

	xFD_Set = FreeRTOS_CreateSocketSet();
	xSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
	configASSERT( xSockets != FREERTOS_INVALID_SOCKET );
	FreeRTOS_setsockopt( xSockets,
	                     0,
	                     FREERTOS_SO_RCVTIMEO,
	                     &xNoTimeOut,
	                     sizeof( xNoTimeOut ) );
	xAddress.sin_port = ( uint16_t ) 11111;
	xAddress.sin_port = FreeRTOS_htons( xAddress.sin_port );
	FreeRTOS_bind( xSockets, &xAddress, sizeof( xAddress ) );
	FreeRTOS_listen( xSockets, 1 );

	while (1) {
		FreeRTOS_FD_CLR(xSockets, xFD_Set, eSELECT_READ);
		FreeRTOS_FD_SET(xSockets, xFD_Set, eSELECT_READ);
		if (xClientSocket && xClientSocket != FREERTOS_INVALID_SOCKET) {
			FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
			FreeRTOS_FD_SET( xClientSocket, xFD_Set, eSELECT_READ );
		}

		xResult = FreeRTOS_select( xFD_Set, portMAX_DELAY );
		if (xResult < 0) {
			break;
		}

		if( FreeRTOS_FD_ISSET ( xSockets, xFD_Set ) ) {
			xClientSocket = FreeRTOS_accept( xSockets, &xRemoteAddr, &xClientLength);
			if( ( xClientSocket != NULL ) && ( xClientSocket != FREERTOS_INVALID_SOCKET ) ) {
				char pucBuffer[32] = {0};
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_GetRemoteAddress( xClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
				FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
				printf("Carlink: Received a connection from %s:%u\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
			}
			continue;
        } else if( FreeRTOS_FD_ISSET ( xClientSocket, xFD_Set ) ) {
			
			ret = FreeRTOS_recv(xClientSocket, wifi_data_buffer, sizeof wifi_data_buffer, 0);
			if (ret > 0) {
				printf("recv buf size:%d\r\n", ret);
			} else {
				printf("FreeRTOS_recv err:%d\r\n", ret);
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				continue;
			}
		}
	}
	
	if (FREERTOS_INVALID_SOCKET != xClientSocket)
		FreeRTOS_closesocket(xClientSocket);
	if (FREERTOS_INVALID_SOCKET != xSockets)
		FreeRTOS_closesocket(xSockets);
	return 0;
}

#if !CARLINK_EY && !CARLINK_EC
eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase, uint32_t ulIPAddress )
{
    eDHCPCallbackAnswer_t eReturn;
    char ip_str[20] = {0};

    sprintf(ip_str, "%d.%d.%d.%d\r\n", (ulIPAddress >> 0) & 0xFF, 
    	(ulIPAddress >> 8) & 0xFF, (ulIPAddress >> 16) & 0xFF, (ulIPAddress >> 24) & 0xFF);
    printf("\r\n  eDHCPPhase:%d  ulIPAddress:%s state:%d \r\n", eDHCPPhase, ip_str, getDhcpClientState());
    if (getDhcpClientState() == 0)
        return eDHCPStopNoChanges;

    switch( eDHCPPhase )
    {
        case eDHCPPhaseFinished:
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
#else
#include "ethernet.h"
#include "tcpip.h"
static struct netif  glwip_netif[4];

err_t dhcp_server_start(struct netif *netif, ip4_addr_t *start, ip4_addr_t *end);

err_t wlan_ethernetif_init(struct netif *netif);
#define lwip_ipv4_addr(addr) ((addr[0]) | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24))
extern err_t ncm_ethernetif_init(struct netif *netif);
#endif

#ifdef WIFI_SUPPORT
void wifi_event_handler( WIFIEvent_t * xEvent )
{
    //WIFIEventType_t xEventType = xEvent->xEventType;
}

static BaseType_t wifi_init()
{

	WIFI_Context_init();
	WIFI_RegisterEvent(eWiFiEventMax, wifi_event_handler);
	for (;;) {
		int status = mmcsd_wait_sdio_ready((int32_t)portMAX_DELAY);
		if (status == MMCSD_HOST_PLUGED) {
			printf("detect sdio device\r\n");
			break;
		}
	}
	return 0;
}
#endif

static int g_flag_init_finished = 0;
static void ark_network_init_thread(void *param)
{
#ifdef WIFI_SUPPORT
	BaseType_t ret = 0;
    ret = wifi_init();
#if !USE_LWIP
    ret = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,ucDNSServerAddress, ucMACAddress);
    
    start_ap(36, "arkn141m_ap_3ca8", "88888888", 1);
	vIPerfInstall();
	vTestTCPServerSocket();
#else
	ip4_addr_t ip_addr;
	ip4_addr_t netmask;
	ip4_addr_t gw;
	ip4_addr_t dhcp_addr_start;
	ip4_addr_t dhcp_addr_end;

	//start_ap(36, "arkn141m_lwip_ap_3ca8", "88888888", 1);
	start_p2p("ark630hv100_p2p", "ark630hv100", "12345678");

	ip_addr.addr = lwip_ipv4_addr(ucIPAddress);
	netmask.addr = lwip_ipv4_addr(ucNetMask);
	gw.addr      = lwip_ipv4_addr(ucGatewayAddress);
	tcpip_init(NULL, NULL);
	netif_add(&glwip_netif[0],
#if LWIP_IPV4
		&ip_addr, &netmask, &gw,
#endif
		NULL, wlan_ethernetif_init, tcpip_input);
	netif_set_default(&glwip_netif[0]);

	uint8_t 	 addr_start[4] = {192, 168, 13, 20};
	uint8_t 	 addr_end[4] = {192, 168, 13, 30};
	dhcp_addr_start.addr 	= lwip_ipv4_addr(addr_start);
	dhcp_addr_end.addr 		= lwip_ipv4_addr(addr_end);
	dhcp_server_start(&glwip_netif[0], &dhcp_addr_start, &dhcp_addr_end);

	netif_set_up(&glwip_netif[0]);

#endif
#elif defined(CONFIG_USB_DEVICE_CDC_NCM)
#if !USE_LWIP
	BaseType_t ret = 0;
	ret = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,ucDNSServerAddress, ucMACAddress);
#else
	ip4_addr_t ip_addr;
	ip4_addr_t netmask;
	ip4_addr_t gw;
	//ip4_addr_t dhcp_addr_start;
	//ip4_addr_t dhcp_addr_end;

	ip_addr.addr = lwip_ipv4_addr(ucIPAddress);
	netmask.addr = lwip_ipv4_addr(ucNetMask);
	gw.addr      = lwip_ipv4_addr(ucGatewayAddress);
	tcpip_init(NULL, NULL);
	netif_add(&glwip_netif[0],
#if LWIP_IPV4
		&ip_addr, &netmask, &gw,
#endif
		NULL, ncm_ethernetif_init, tcpip_input);
	netif_set_default(&glwip_netif[0]);

	/*uint8_t 	 addr_start[4] = {192, 168, 13, 20};
	uint8_t 	 addr_end[4] = {192, 168, 13, 30};
	dhcp_addr_start.addr 	= lwip_ipv4_addr(addr_start);
	dhcp_addr_end.addr 		= lwip_ipv4_addr(addr_end);
	dhcp_server_start(&glwip_netif[0], &dhcp_addr_start, &dhcp_addr_end);*/

	netif_set_up(&glwip_netif[0]);
#endif
#endif
	g_flag_init_finished = 1;
#ifdef WIFI_SUPPORT
        ret = ret;
#endif
    vTaskDelete(NULL);
}

int ark_network_init()
{
    BaseType_t ret = 0;
    if (g_flag_init_finished)
      return 0;
	g_flag_init_finished = 0;
	ret = xTaskCreate(ark_network_init_thread, "ark_net_init", 1024, NULL, 8, NULL);

	while(!g_flag_init_finished) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}

    return (int)ret;
}

/*
 * FreeRTOS Wi-Fi V1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file iot_wifi.c
 * @brief Wi-Fi Interface.
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* Socket and Wi-Fi interface includes. */
#include "FreeRTOS.h"
#include "iot_wifi.h"

/* Wi-Fi configuration includes. */
#include "aws_wifi_config.h"

#include "wifi_constants.h"
#include "net_stack_intf.h"
#include "semphr.h"
#include "wifi_structures.h"
#include "wifi_conf.h"
#include "board.h"
#include "wifi_p2p_config.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_DHCP_Server.h"
#include "vehicle_param/vehicle_param.h"
#if USE_LWIP
#include "dhcp.h"
#endif

static rtw_mode_t g_wifi_mode = RTW_MODE_STA;
static SemaphoreHandle_t xWiFiSem;
static bool wifi_started;

/*-----------------------------------------------------------*/
bool get_wifi_start_state(void) {
	return wifi_started;
}

WIFIReturnCode_t WIFI_On( void )
{
	WIFIReturnCode_t ret = eWiFiFailure;
	char *countrycode={"CN"}; 

	xSemaphoreTake(xWiFiSem, portMAX_DELAY );
	if (wifi_started) {
		ret = eWiFiSuccess;
		goto exit;
	}
	printf("wext_set_adaptivity_th_l2h_ini\r\n");
	//wext_set_adaptivity_th_l2h_ini(-17);
	//wext_set_expire_time(2);
	wext_set_countrycode(WLAN0_NAME, (u8*)countrycode);
		
    if (wifi_on(g_wifi_mode) < 0) {
		printf("\r\nopen wifi failed \r\n");
		goto exit;
	}
	
	wifi_started = true;
	ret = eWiFiSuccess;
exit:
	xSemaphoreGive(xWiFiSem);
    return ret;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_P2P_Stop(void) {
	xSemaphoreTake(xWiFiSem, portMAX_DELAY );
	cmd_wifi_p2p_stop(0, NULL);
	wifi_started = false;
	xSemaphoreGive(xWiFiSem);
    return eWiFiSuccess;
}

WIFIReturnCode_t WIFI_Off( void )
{
	xSemaphoreTake(xWiFiSem, portMAX_DELAY );
    wifi_off();
	wifi_started = false;
	xSemaphoreGive(xWiFiSem);
    return eWiFiSuccess;
}
/*-----------------------------------------------------------*/

static rtw_security_t convertWIFISecurity2rtw_security(WIFISecurity_t type)
{
	rtw_security_t	security_type = RTW_MODE_NONE;

	switch (type) {
	case eWiFiSecurityOpen:
		security_type = RTW_SECURITY_OPEN;
		break;
	case eWiFiSecurityWPA2:
		security_type = RTW_SECURITY_WPA2_AES_PSK;
		break;
	default:
		security_type = RTW_SECURITY_UNKNOWN;
		break;
	}
	return security_type;
}

static WIFISecurity_t convertrtw_security2WIFISecurity(rtw_security_t type)
{
	WIFISecurity_t	security_type = eWiFiSecurityNotSupported;

	switch (type) {
	case RTW_SECURITY_OPEN:
		security_type = eWiFiSecurityOpen;
		break;
	case RTW_SECURITY_WPA2_AES_PSK:
		security_type = eWiFiSecurityWPA2;
		break;
	default:
		break;
	}
	return security_type;
}

#if 0
static rtw_mode_t convertWIFIDevMode2rtwMode(WIFIDeviceMode_t mode)
{
	rtw_mode_t	rtw_mode = RTW_MODE_NONE;

	switch (mode) {
	case eWiFiModeStation:
		rtw_mode = RTW_MODE_STA;
		break;
	case eWiFiModeAP:
		rtw_mode = RTW_MODE_AP;
		break;
	case eWiFiModeP2P:
		rtw_mode = RTW_MODE_P2P;
		break;
	case eWiFiModeAPStation:
		rtw_mode = RTW_MODE_STA_AP;
		break;
	case eWiFiModeNotSupported:
		rtw_mode = RTW_MODE_NONE;
		break;
	default:
		break;
	}
	return rtw_mode;
}

static WIFIDeviceMode_t convertrtwMode2WIFIDevMode(rtw_mode_t rtw_mode)
{
	WIFIDeviceMode_t	dev_mode = eWiFiModeNotSupported;

	switch (rtw_mode) {
	case RTW_MODE_STA:
		dev_mode = eWiFiModeStation;
		break;
	case RTW_MODE_AP:
		dev_mode = eWiFiModeAP;
		break;
	case RTW_MODE_P2P:
		dev_mode = eWiFiModeP2P;
		break;
	case RTW_MODE_STA_AP:
		dev_mode = eWiFiModeAPStation;
		break;
	case RTW_MODE_NONE:
		dev_mode = eWiFiModeNotSupported;
		break;
	default:
		break;
	}
	return dev_mode;
}
#endif

WIFIReturnCode_t WIFI_ConnectAP( const WIFINetworkParams_t * const pxNetworkParams )
{
	rtw_security_t	security_type;
	rtw_result_t ret = (rtw_result_t)RTW_UNSUPPORTED;
    if (pxNetworkParams == NULL)
		return eWiFiFailure;
	xSemaphoreTake(xWiFiSem, portMAX_DELAY );
	security_type = convertWIFISecurity2rtw_security(pxNetworkParams->xSecurity);
	if (security_type == RTW_SECURITY_WPA2_AES_PSK) {
		ret = wifi_connect((char *)pxNetworkParams->ucSSID,
			security_type,
			(char *)pxNetworkParams->xPassword.xWPA.cPassphrase,
			pxNetworkParams->ucSSIDLength,
			(int)pxNetworkParams->xPassword.xWPA.ucLength, 0, NULL);
	} else if (security_type == RTW_SECURITY_OPEN) {
		ret = wifi_connect((char *)pxNetworkParams->ucSSID,
			security_type,
			(char *)pxNetworkParams->xPassword.xWPA.cPassphrase,
			pxNetworkParams->ucSSIDLength,
			RTW_MIN_PSK_LEN, 0, NULL);
	}
	xSemaphoreGive(xWiFiSem);
	if (ret == RTW_TIMEOUT)
		return eWiFiTimeout;
	else if (ret != RTW_SUCCESS)
		return eWiFiFailure;
    return eWiFiSuccess;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_Disconnect( void )
{
	xSemaphoreTake(xWiFiSem, portMAX_DELAY );
    wifi_disconnect();
	xSemaphoreGive(xWiFiSem);
    return eWiFiSuccess;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_Reset( void )
{
    return eWiFiSuccess;
}

/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_SetMode( WIFIDeviceMode_t xDeviceMode )
{
	WIFIReturnCode_t ret = eWiFiNotSupported; 
	switch (xDeviceMode) {
	case eWiFiModeStation:
		ret = eWiFiSuccess;
		g_wifi_mode = RTW_MODE_STA;
		break;
	case eWiFiModeAP:
		ret = eWiFiSuccess;
		g_wifi_mode = RTW_MODE_AP;
		break;
	default:
		break;
			
	}
    return ret;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetMode( WIFIDeviceMode_t * pxDeviceMode )
{
    WIFIReturnCode_t ret = eWiFiNotSupported;
	if (NULL == pxDeviceMode)
		return ret;
	switch (g_wifi_mode) {
	case RTW_MODE_STA:
		ret = eWiFiSuccess;
		*pxDeviceMode = eWiFiModeStation;
		break;
	case RTW_MODE_AP:
		ret = eWiFiSuccess;
		*pxDeviceMode = eWiFiModeAP;
		break;
	default:
		break;
			
	}
    return ret;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_NetworkAdd( const WIFINetworkProfile_t * const pxNetworkProfile,
                                  uint16_t * pusIndex )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_NetworkGet( WIFINetworkProfile_t * pxNetworkProfile,
                                  uint16_t usIndex )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_NetworkDelete( uint16_t usIndex )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_Ping( uint8_t * pucIPAddr,
                            uint16_t usCount,
                            uint32_t ulIntervalMS )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetMAC( uint8_t * pucMac )
{
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetHostIP( char * pcHost,
                                 uint8_t * pucIPAddr )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/
struct scan_data_ctx
{
	WIFIScanResult_t *result;
	int index;
	int max_num;
	bool scan_stop;
	QueueHandle_t sem;
};
static rtw_result_t WIFI_scan_result_handler( rtw_scan_handler_result_t* malloced_scan_result )
{
	char bssid[32] = {0};
	WIFIScanResult_t *result;
	struct scan_data_ctx* pscan_data = (struct scan_data_ctx*)malloced_scan_result->user_data;
	int len;
	unsigned char *ptr = malloced_scan_result->ap_details.BSSID.octet;
	sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
	printf("\r\nSSID:%s Bssid:%s Signal strength:%d DB\r\n", malloced_scan_result->ap_details.SSID.val, bssid,
		malloced_scan_result->ap_details.signal_strength);
	if (pscan_data->scan_stop)
		return (rtw_result_t)RTW_SUCCESS;
	if (pscan_data->index < pscan_data->max_num) {
		result 					= pscan_data->result + pscan_data->index;
		result->ucChannel		= malloced_scan_result->ap_details.channel;
		result->cRSSI 			= malloced_scan_result->ap_details.signal_strength;
		result->xSecurity 		= convertrtw_security2WIFISecurity(malloced_scan_result->ap_details.security);
		len = wificonfigMAX_SSID_LEN;
		if (len < malloced_scan_result->ap_details.SSID.len)
			len = malloced_scan_result->ap_details.SSID.len;
		memcpy(result->ucSSID, malloced_scan_result->ap_details.SSID.val, len);
		len = wificonfigMAX_BSSID_LEN;
		if (len != sizeof(malloced_scan_result->ap_details.BSSID.octet))
			printf("wrong bssid len\r\n");
		memcpy(result->ucBSSID, malloced_scan_result->ap_details.BSSID.octet, len);
	}
	
	if (malloced_scan_result->scan_complete != 0) {
		printf("scan complete!\r\n");
		xQueueSend(pscan_data->sem, NULL, 0);
	}	else
		pscan_data->index++;
	if (pscan_data->index >= 1)
		return (rtw_result_t)RTW_SUCCESS;
	return (rtw_result_t)RTW_NOTSTA;
}

WIFIReturnCode_t WIFI_Scan( WIFIScanResult_t * pxBuffer,
                            uint8_t ucNumNetworks )
{
	WIFIReturnCode_t ret = eWiFiNotSupported;
	struct scan_data_ctx scan_data = {0}; 
	if (!wifi_started)
		ret = WIFI_On();
	scan_data.index 		= 0;
	scan_data.scan_stop 	= false;
	scan_data.max_num 		= ucNumNetworks;
	scan_data.result 		= pxBuffer;
	scan_data.sem 			= xQueueCreate(1, 0);
	xSemaphoreTake(xWiFiSem, portMAX_DELAY );
    wifi_scan_networks(WIFI_scan_result_handler, (void *)&scan_data);

	ret = xQueueReceive(scan_data.sem, NULL, portMAX_DELAY);
	if( ret != pdPASS ) {
		ret = eWiFiFailure;
		goto exit;
	}
	scan_data.scan_stop 	= true;
	vSemaphoreDelete(scan_data.sem);
	if (scan_data.index >= 1)
		ret = eWiFiSuccess;
	else
		ret = eWiFiFailure;
exit:
	xSemaphoreGive(xWiFiSem);
    return ret;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_StartAP( void )
{
	WIFIReturnCode_t xWifiStatus;    
	WIFI_SetMode(eWiFiModeAP);
	
	xWifiStatus = WIFI_On();
    return xWifiStatus;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_StopAP( void )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_ConfigureAP( const WIFINetworkParams_t * const pxNetworkParams )
{
    WIFIReturnCode_t xWifiStatus;
	int ret = -1;

	if (NULL == pxNetworkParams)
		return eWiFiFailure;
	
	if (eWiFiSecurityOpen == pxNetworkParams->xSecurity) {printf("WIFI_ConfigureAP channel:%d\r\n", pxNetworkParams->ucChannel);
		ret = wifi_start_ap((char *)pxNetworkParams->ucSSID, RTW_SECURITY_OPEN, NULL, 
			pxNetworkParams->ucSSIDLength, 0, pxNetworkParams->ucChannel);
	} else if (eWiFiSecurityWPA2 == pxNetworkParams->xSecurity) {
		ret = wifi_start_ap((char *)pxNetworkParams->ucSSID, RTW_SECURITY_WPA2_AES_PSK, 
			(char *)pxNetworkParams->xPassword.xWPA.cPassphrase, 
			pxNetworkParams->ucSSIDLength, pxNetworkParams->xPassword.xWPA.ucLength, 
			pxNetworkParams->ucChannel);
	}
	if (ret < 0)
		xWifiStatus = eWiFiFailure;
	else
		xWifiStatus = eWiFiSuccess;
    return xWifiStatus;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_SetPMMode( WIFIPMMode_t xPMModeType,
                                 const void * pvOptionValue )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetPMMode( WIFIPMMode_t * pxPMModeType,
                                 void * pvOptionValue )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_RegisterEvent( WIFIEventType_t xEventType,
                                     WIFIEventHandler_t xHandler )
{
    return (WIFIReturnCode_t)wifi_register_user_event(xEventType, xHandler);
}
/*-----------------------------------------------------------*/

BaseType_t WIFI_IsConnected( const WIFINetworkParams_t * pxNetworkParams )
{
	/* FIX ME. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_StartScan( WIFIScanConfig_t * pxScanConfig )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetScanResults( const WIFIScanResult_t ** pxBuffer,
                                      uint16_t * ucNumNetworks )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_StartConnectAP( const WIFINetworkParams_t * pxNetworkParams )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_StartDisconnect( void )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetConnectionInfo( WIFIConnectionInfo_t * pxConnectionInfo )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetIPInfo( WIFIIPConfiguration_t * pxIPConfig )
{
    /* FIX ME. */
    return eWiFiNotSupported;
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetRSSI( int8_t * pcRSSI )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetStationList( WIFIStationInfo_t * pxStationList,
                                      uint8_t * pcStationListSize )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_StartDisconnectStation( uint8_t * pucMac )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_SetMAC( uint8_t * pucMac )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_SetCountryCode( const char * pcCountryCode )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetCountryCode( char * pcCountryCode )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetStatistic( WIFIStatisticInfo_t * pxStats )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

WIFIReturnCode_t WIFI_GetCapability( WIFICapabilityInfo_t * pxCaps )
{
    /* FIX ME. */
    return eWiFiNotSupported;    
}
/*-----------------------------------------------------------*/

int WIFI_Context_init()
{
	if (xWiFiSem == NULL)
		xWiFiSem = xSemaphoreCreateMutex();

	return 0;
}

//#define clientcredentialWIFI_SSID    "ap630"
//#define clientcredentialWIFI_PASSWORD   "12345678"
//#define clientcredentialWIFI_SSID    "AndroidShare_7957"
//#define clientcredentialWIFI_PASSWORD   "y5g7swiyabmu7p3"
//#define clientcredentialWIFI_SSID    "dongle_sim-783c"
//#define clientcredentialWIFI_PASSWORD   "88888888"
#define clientcredentialWIFI_SSID    "ark-9528"
#define clientcredentialWIFI_PASSWORD   "02345678"

//#define clientcredentialWIFI_SSID    "liu"
//#define clientcredentialWIFI_PASSWORD   "12345678"

#define ucNumNetworks  12

#define serverWIFI_SSID    "ap630"
extern void cmd_test(const char* temp_uart_buf);
extern struct sdio_func *wifi_sdio_func;
#if 0
eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase,
                                                    uint32_t ulIPAddress )
{
	printf("%d  %x\r\n", eDHCPPhase, ulIPAddress);
}
#endif
static TaskHandle_t       ping_task_handle = NULL;
static int flag_start_ping = 0;
static QueueHandle_t     ping_event_queue = NULL;
static BaseType_t xPingTotalSuccess = 0, xPingSendCount = 0;
struct ping_para
{
    uint32_t addr; 
    int         start;
};

 void vApplicationPingReplyHook( ePingReplyStatus_t eStatus,
								 uint16_t usIdentifier )
 {
	 if( eStatus == eSuccess )
	 {
		 //FreeRTOS_printf( ( "\r\nPing response received. ID: %d\r\n", usIdentifier ) );
		 printf("\r\nPing response received. ID: %d eStatus:%d\r\n", usIdentifier, eStatus);
 
		 /* Increment successful ping replies. */
		 xPingTotalSuccess++;
	 }
 }

static void ping_task_proc(void* arg)
{
    struct ping_para para = {0};

    for (;;) {
        if (flag_start_ping == 0) {printf("\r\n%s:%d\r\n", __func__, __LINE__);
            memset((void*)&para, 0, sizeof(para));
            if (xQueueReceive(ping_event_queue, &para,  portMAX_DELAY) != pdPASS) {
                printf("%s xQueueReceive err!\r\n", __func__);
                continue;
            }
            flag_start_ping = para.start;
        } else {
            FreeRTOS_SendPingRequest(para.addr, 64, 1000);
            xPingSendCount++;
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    vTaskDelete(NULL);
}

void start_ping(const char *ipaddr)
{
    uint8_t ipAddress[4] = {0};
    uint32_t addr;
    struct ping_para para = {0};
    
    sscanf(ipaddr, "%d.%d.%d.%d", (int*)&ipAddress[0], (int*)&ipAddress[1], (int*)&ipAddress[2], (int*)&ipAddress[3]);
    addr = (ipAddress[3] << 24) | (ipAddress[2] << 16) | (ipAddress[1] << 8) | (ipAddress[0] << 0);
    printf("ipAddress[1] %d.%d.%d.%d\r\n", ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3]);
    para.addr = addr;
    

    if (ping_event_queue == NULL) {
        ping_event_queue = xQueueCreate(1, sizeof(struct ping_para));
    }

    if (ping_task_handle == NULL) {
        xTaskCreate(ping_task_proc, "ping task",  512, NULL, configMAX_PRIORITIES / 3, &ping_task_handle);
    }

    if (flag_start_ping == 0) {
        printf("\r\nping start!\r\n");
        para.start = 1;
        xPingTotalSuccess = 0;
        xPingSendCount = 0;
        if (ping_event_queue) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            xQueueSend(ping_event_queue, &para, 0);
        }
    } else {
        printf("\r\nping stop!\r\n");
        printf("\r\n%d packets transmitted, %d received, %d%% packet loss\r\n", xPingSendCount, xPingTotalSuccess, (xPingSendCount - xPingTotalSuccess) * 100 / xPingSendCount );
        flag_start_ping = 0;
    }
}

#if !USE_LWIP
static const uint8_t ucIPAddressAp[4] = {192, 168, 13, 1};
#endif
WIFIDeviceMode_t g_current_mode = eWiFiModeNotSupported;

int wifi_initialize(WIFIDeviceMode_t mode)
{
	WIFIReturnCode_t xWifiStatus;

	WIFI_Context_init();

	if (!(mode == eWiFiModeStation || mode ==eWiFiModeAP)) {
		printf("wifi mode is not supported\r\n");
		return -1;
	}
	
	if (g_current_mode != mode && g_current_mode != eWiFiModeNotSupported) {
		WIFI_Off();
	}

	WIFI_SetMode(mode);
	g_current_mode = mode;

	xWifiStatus = WIFI_On();
	if( xWifiStatus == eWiFiSuccess ) {
		printf("WiFi module initialized.\r\n");
		return 0;
	} else {
		printf("WiFi module failed to initialize.\r\n" );
		return -1;
	}
}

int start_ap(int channel, const char* ssid, const char* passwd, char need_passwd)
{
	char cmd[128] = {0};
#if !USE_LWIP
	uint32_t IPAddress;
	setDhcpClientState(0);
	vDHCPProcess(1, eInitialWait);
	xSendDHCPEvent();
#endif
	WIFI_Context_init();
	if (g_current_mode != eWiFiModeAP || !wifi_started) {
		printf("current mode is not ap\r\n");
		g_current_mode = eWiFiModeAP;
		WIFI_SetMode(eWiFiModeAP);
		
		if (wifi_started) {
			printf("wifi is start, so reboot wifi\r\n");
			WIFI_Off();
		}
		if (need_passwd)
		    sprintf(cmd, "wifi_ap %s %d %s", ssid, channel, passwd);
		else
		    sprintf(cmd, "wifi_ap %s %d", ssid, channel);
		wifi_started = true;
		/*char channel_set[] = {36, 38, 40, 44, 46, 48, 149, 151, 153, 157, 159, 161, 165};
		int ch = wext_get_auto_chl("wlan0", channel_set, sizeof(channel_set));
		printf("ap is work at channel : %d\r\n", ch);*/
		cmd_test(cmd);
	}
#if !USE_LWIP
	IPAddress = (20 << 24) | (13 << 16) | (168 << 8) | (192 << 0);
	dhcpserver_start(ucIPAddressAp, IPAddress, 10);
#else
	//dhcpd_start("wi");
#endif
	return 0;
}

#if USE_LWIP
struct netif *get_lwip_net_interface();
#endif
int start_sta(const char* ssid, const char* passwd, char need_passwd)
{
    WIFINetworkParams_t xNetworkParams = {0};
    WIFIReturnCode_t xWifiStatus;
    int retry_cnt = 0;
#if !USE_LWIP
    setDhcpClientState(1);
    vDHCPProcess(1, eInitialWait);
    xSendDHCPEvent();
#endif
    WIFI_Context_init();
    if (g_current_mode != eWiFiModeStation) {
	g_current_mode = eWiFiModeStation;
	 WIFI_SetMode(eWiFiModeStation);
	 printf("Current mode is not sta, so reboot wifi\r\n");
        WIFI_Off();
    	  vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    xWifiStatus = WIFI_On();

    if( xWifiStatus == eWiFiSuccess ) {
        printf("WiFi module initialized.\r\n");
    } else {
        printf("WiFi module failed to initialize.\r\n" );
        // Handle module init failure
        return -1;
    }

    /* Some boards might require additional initialization steps to use the Wi-Fi library. */

    while (0) {
        printf("Starting scan\r\n");
        WIFIScanResult_t xScanResults[ ucNumNetworks ] = {0};
        xWifiStatus = WIFI_Scan( xScanResults, ucNumNetworks ); // Initiate scan

        printf("Scan started\r\n");

        // For each scan result, print out the SSID and RSSI
        if ( xWifiStatus == eWiFiSuccess ) {
            printf("Scan success\r\n");
            for ( uint8_t i=0; i<ucNumNetworks; i++ ) {
                printf("%s : %d \r\n", xScanResults[i].ucSSID, xScanResults[i].cRSSI);
            }
            break;
        } else {
            printf("Scan failed, status code: %d\n", (int)xWifiStatus);
            goto exit;
            //return -1;
        }

        vTaskDelay(200);
    }

    /* Setup parameters. */
    memset(&xNetworkParams, 0, sizeof(xNetworkParams));
    xNetworkParams.ucSSIDLength = strlen( ssid );
    memcpy(xNetworkParams.ucSSID, ssid, xNetworkParams.ucSSIDLength);
    xNetworkParams.xPassword.xWPA.ucLength = strlen( passwd );
    memcpy(xNetworkParams.xPassword.xWPA.cPassphrase, passwd, xNetworkParams.xPassword.xWPA.ucLength);
    if (need_passwd)
        xNetworkParams.xSecurity = eWiFiSecurityWPA2;
    else
        xNetworkParams.xSecurity = eWiFiSecurityOpen;

retry:
    // Connect!
    xWifiStatus = WIFI_ConnectAP( &( xNetworkParams ) );

    if( xWifiStatus == eWiFiSuccess ){
        printf( "WiFi Connected to AP.\r\n" );
    } else {
        printf( "WiFi failed to connect to AP.\r\n" );
        // Handle connection failure

        if (retry_cnt++ < 5)
            goto retry;
        return -1;
    }
#if !USE_LWIP
    vDHCPProcess(1, eWaitingSendFirstDiscover);
    xSendDHCPEvent();
#else
	//vTaskDelay(pdMS_TO_TICKS(1000));
	//dhcpd_stop("wi");
	//dhcp_start(get_lwip_net_interface());
#endif
exit:
    return 0;
}

int start_p2p(const char *dev_name, const char *ssid, const char *passwd)
{
	//char cmd[128] = {0};
#if !USE_LWIP
	uint32_t IPAddress;
	setDhcpClientState(0);
#endif

	WIFI_Context_init();
	if (g_current_mode != eWiFiModeP2P || !wifi_started) {
		//char mac[6] = {0};
		printf("current mode is not ap\r\n");
		g_current_mode = eWiFiModeP2P;
		WIFI_SetMode(eWiFiModeP2P);

		if (wifi_started) {
			printf("wifi is start, so reboot wifi\r\n");
			WIFI_Off();
		}

		wifi_started = true;
		(void)dev_name;
		//sscanf((const char*)dev_addr_str, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

		cmd_wifi_p2p_start_ex(NULL);

		//wifi_cmd_p2p_find();
		char *argv[3] = {(char *)dev_name, (char *)ssid, (char *)passwd};
		cmd_wifi_p2p_auto_go_start(3, argv);

		//wifi_start_p2p_go("amt630v100_p2p", "12345678", 1);
	}
#if !USE_LWIP
	IPAddress = (20 << 24) | (13 << 16) | (168 << 8) | (192 << 0);
	dhcpserver_start(ucIPAddressAp, IPAddress, 10);
#else
	//dhcpd_start("wi");
#endif

	return 0;
}


void iwpriv_dbg_btco()
{
	cmd_test("iwpriv dbg btco");
}

/*static void iwpriv_task_proc(void* arg)
{
	vTaskDelay(pdMS_TO_TICKS(2000));
	while(1) {
		cmd_test("iwpriv dbg btco");
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
	vTaskDelete(NULL);
}*/

void enable_btco_log()
{
	cmd_test("wifi_debug log btco on");
	//xTaskCreate(iwpriv_task_proc, "iwpriv_task",  4096, NULL, configMAX_PRIORITIES / 3, NULL);
}


int wifi_sta_test_proc()
{
#if 0
	wifi_on(RTW_MODE_STA);
	cmd_test("wifi_scan 1 2");
#else
        static int wifi_is_on = 0;
	WIFINetworkParams_t xNetworkParams = {0};
	WIFIReturnCode_t xWifiStatus;
        int retry_cnt = 0;

	WIFI_Context_init();

	WIFI_SetMode(eWiFiModeStation);

	//if (NULL != wifi_sdio_func)
	//	wifi_fake_driver_probe_rtlwifi(wifi_sdio_func);

	printf("Turning on wifi...\r\n");
    if (wifi_is_on == 0) {
	xWifiStatus = WIFI_On();
    wifi_is_on = 1;
    } else {xWifiStatus = eWiFiSuccess;
       }

	printf("Checking status...\r\n");
	if( xWifiStatus == eWiFiSuccess ) {
		printf("WiFi module initialized.\r\n");
	} else {
		printf("WiFi module failed to initialize.\r\n" );
		// Handle module init failure
		return -1;
	}

	/* Some boards might require additional initialization steps to use the Wi-Fi library. */

	while (1) {
		printf("Starting scan\r\n");
		WIFIScanResult_t xScanResults[ ucNumNetworks ] = {0};
		xWifiStatus = WIFI_Scan( xScanResults, ucNumNetworks ); // Initiate scan

		printf("Scan started\r\n");

		// For each scan result, print out the SSID and RSSI
		if ( xWifiStatus == eWiFiSuccess ) {
		    printf("Scan success\r\n");
		    for ( uint8_t i=0; i<ucNumNetworks; i++ ) {
		        printf("%s : %d \r\n", xScanResults[i].ucSSID, xScanResults[i].cRSSI);
		    }
		    break;
		} else {
		    printf("Scan failed, status code: %d\n", (int)xWifiStatus);
			goto exit;
			//return -1;
		}
		
		vTaskDelay(200);
	}

	/* Setup parameters. */
	memset(&xNetworkParams, 0, sizeof(xNetworkParams));
	xNetworkParams.ucSSIDLength = strlen( clientcredentialWIFI_SSID );
	memcpy(xNetworkParams.ucSSID, clientcredentialWIFI_SSID, xNetworkParams.ucSSIDLength);
	xNetworkParams.xPassword.xWPA.ucLength = strlen( clientcredentialWIFI_PASSWORD );
	memcpy(xNetworkParams.xPassword.xWPA.cPassphrase, clientcredentialWIFI_PASSWORD, xNetworkParams.xPassword.xWPA.ucLength);
	xNetworkParams.xSecurity = eWiFiSecurityWPA2;

retry:
	// Connect!
	xWifiStatus = WIFI_ConnectAP( &( xNetworkParams ) );

	if( xWifiStatus == eWiFiSuccess ){
		printf( "WiFi Connected to AP.\r\n" );
	} else {
		printf( "WiFi failed to connect to AP.\r\n" );
		// Handle connection failure

		if (retry_cnt++ < 3)
			goto retry;
		return -1;
	}
	/*vTaskDelay(pdMS_TO_TICKS(1000));
	printf("###resend dhcp \r\n");
	vDHCPProcess(1, eInitialWait);
	xSendDHCPEvent();*/
exit:
	/*cmd_test("iwpriv dbg mac");
	vTaskDelay(2000);
	cmd_test("iwpriv dbg bb");
	vTaskDelay(2000);
	cmd_test("iwpriv dbg rf");
	vTaskDelay(2000);*/
	
#endif
	return 0;
}

int wifi_ap_test_proc()
{
	
	//WIFINetworkParams_t xNetworkParams = {0};
	//WIFIReturnCode_t xWifiStatus;

	WIFI_Context_init();
#if 0

	WIFI_SetMode(eWiFiModeAP);

	printf("Turning on wifi...\r\n");
	xWifiStatus = WIFI_On();
	vTaskDelay(pdMS_TO_TICKS(1000));

	printf("Checking status...\r\n");
	if( xWifiStatus == eWiFiSuccess ) {
		printf("WiFi module initialized.\r\n");
	} else {
		printf("WiFi module failed to initialize.\r\n" );
		// Handle module init failure
		return -1;
	}

	//xNetworkParams.ucChannel = 1;
	//xNetworkParams.ucSSIDLength = (uint8_t)strlen(serverWIFI_SSID);
	//memcpy(xNetworkParams.ucSSID, serverWIFI_SSID, strlen(serverWIFI_SSID) + 1);
	//xNetworkParams.xSecurity = eWiFiSecurityOpen;

	//xWifiStatus = WIFI_ConfigureAP(&xNetworkParams);
	//WIFINetworkParams_t *pxNetworkParams = &xNetworkParams;
	//printf("WIFI_ConfigureAP channel:%d\r\n", pxNetworkParams->ucChannel);
	//wifi_start_ap(pxNetworkParams->ucSSID, RTW_SECURITY_OPEN, NULL, 
	//		pxNetworkParams->ucSSIDLength, 0, pxNetworkParams->ucChannel);
	wifi_start_ap("ap630", RTW_SECURITY_OPEN, NULL, 5, 0, 1);
	xWifiStatus = eWiFiSuccess;
	if( xWifiStatus == eWiFiSuccess ){
		printf( "WiFi Configure AP.\r\n" );
	} else {
		printf( "WiFi failed to Configure AP.\r\n" );
		// Handle connection failure
		return -1;
	}
#else
#if 1
#if ( ipconfigUSE_DHCP != 0 )
	//setDhcpState(0);
#endif
	cmd_test("wifi_ap ap63011 36 88888888");
#else
	char *ssid = "ap630";
	int channel = 1;
	int timeout = 20;
	wifi_off();
	vTaskDelay(pdMS_TO_TICKS(20));
	if (wifi_on(RTW_MODE_AP) < 0) {
		printf("\r\nopen wifi failed \r\n");
		return -1;
	}
	printf("\r\n wifi init finished!\r\n");
	//wifi_start_ap(ssid, RTW_SECURITY_OPEN, NULL, strlen(ssid), 0, 1);

	wifi_start_ap(ssid,
							 RTW_SECURITY_OPEN,
							 NULL,
							 strlen((const char *)ssid),
							 0,
							 channel
							 );

	while(1) {
		char essid[33];

		if(wext_get_ssid("wlan0", (unsigned char *) essid) > 0) {
			if(strcmp((const char *) essid, (const char *)ssid) == 0) {
				printf("%s started\r\n", ssid);
				break;
			}
		}

		if(timeout == 0) {
			printf("ERROR: Start AP timeout!\r\n");
			break;
		}

		vTaskDelay(pdMS_TO_TICKS(1000));

		timeout --;
	}
#endif
#endif
	return 0;
}

#if (!CARLINK_EY && !CARLINK_EC) && (USE_LWIP == 1)
eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase, uint32_t ulIPAddress )
{
	(void)eDHCPPhase;
	(void)ulIPAddress;
	return eDHCPStopNoChanges;
}
#endif

int start_sta_proc(const char* ssid, const char* passwd, char need_passwd)
{
	if (ssid == NULL || strlen(ssid) == 0)
	{
		printf("ssid is invalid \n");
		return -1;
	}

	vehicle_set_data(VEH_OTA_START_STATUS, 1);

    WIFINetworkParams_t xNetworkParams = {0};
    WIFIReturnCode_t xWifiStatus;
#if !USE_LWIP
    setDhcpClientState(1);
    vDHCPProcess(1, eInitialWait);
    xSendDHCPEvent();
#endif
    WIFI_Context_init();
    if (g_current_mode != eWiFiModeStation) {
	g_current_mode = eWiFiModeStation;
	 WIFI_SetMode(eWiFiModeStation);
	 printf("Current mode is not sta, so reboot wifi\r\n");
        WIFI_Off();
	 printf("wifi off start....\r\n");
    	  vTaskDelay(pdMS_TO_TICKS(2000));
		  	 printf("wifi off end....\r\n");

    }
    printf("wifi on start....\r\n");
    xWifiStatus = WIFI_On();
	printf("wifi on end....\r\n");
    if( xWifiStatus == eWiFiSuccess ) {
        printf("WiFi module initialized.\r\n");
    } else {
        printf("WiFi module failed to initialize.\r\n" );
        // Handle module init failure
        return -1;
    }

    /* Some boards might require additional initialization steps to use the Wi-Fi library. */

    while (0) {
        printf("Starting scan\r\n");
        WIFIScanResult_t xScanResults[ ucNumNetworks ] = {0};
        xWifiStatus = WIFI_Scan( xScanResults, ucNumNetworks ); // Initiate scan

        printf("Scan started\r\n");

        // For each scan result, print out the SSID and RSSI
        if ( xWifiStatus == eWiFiSuccess ) {
            printf("Scan success\r\n");
            for ( uint8_t i=0; i<ucNumNetworks; i++ ) {
                printf("%s : %d \r\n", xScanResults[i].ucSSID, xScanResults[i].cRSSI);
            }
            break;
        } else {
            printf("Scan failed, status code: %d\n", (int)xWifiStatus);
            goto exit;
            //return -1;
        }

        vTaskDelay(200);
    }

    /* Setup parameters. */
    memset(&xNetworkParams, 0, sizeof(xNetworkParams));
    xNetworkParams.ucSSIDLength = strlen( ssid );
    memcpy(xNetworkParams.ucSSID, ssid, xNetworkParams.ucSSIDLength);
    xNetworkParams.xPassword.xWPA.ucLength = strlen( passwd );
    memcpy(xNetworkParams.xPassword.xWPA.cPassphrase, passwd, xNetworkParams.xPassword.xWPA.ucLength);
    if (need_passwd)
        xNetworkParams.xSecurity = eWiFiSecurityWPA2;
    else
        xNetworkParams.xSecurity = eWiFiSecurityOpen;

	vehicle_set_data(VEH_OTA_START_STATUS, 2);
retry:
    // Connect!
	if (g_current_mode != eWiFiModeStation) {
		printf("wifi mode is not station");
		return -1;
	}
	
    xWifiStatus = WIFI_ConnectAP( &( xNetworkParams ) );

    if( xWifiStatus == eWiFiSuccess ){
        printf( "WiFi Connected to AP:%s.\r\n", xNetworkParams.ucSSID );
    } else {
        printf( "WiFi failed to connect to AP:%s.\r\n", xNetworkParams.ucSSID);
        // Handle connection failure
		vTaskDelay(3000);
		goto retry;
    }
#if !USE_LWIP
    vDHCPProcess(1, eWaitingSendFirstDiscover);
    xSendDHCPEvent();
#else
	//vTaskDelay(pdMS_TO_TICKS(1000));
	//dhcpd_stop("wi");
	//dhcp_start(get_lwip_net_interface());
#endif
exit:
    return 0;
}
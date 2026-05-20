/**
*
* @file hcn_ota_start_sta.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/11 15:45
* @author och
*
*/

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include <FreeRTOS.h>
#include <task.h>
#include "iot_wifi.h"
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_DHCP_Server.h"
#include "wifi_structures.h"
#include "ota_manage/hcn_ota_start_sta.h"
#include "log/hcn_log.h"
#include "dashboard_state/hcn_dev_state.h"
#include "carlink_cb/hcn_carlink_cb.h"
#include  "ota_manage/hcn_tcp_client.h"
#include "vehicle_param/vehicle_param.h"

static TaskHandle_t ota_wifi_task = NULL;

static wifi_user_e wifi_usr = WIFI_USER_OTA;
static bool ota_task_running = false;
#if 0
#define ucNumNetworks		12

static WIFIDeviceMode_t wifi_cur_mode = eWiFiModeNotSupported;


static char g_StaSsid[33] = {0};
static char g_StaPasswd[33] = {0};

extern WIFIDeviceMode_t get_current_wifi_mode(void);
extern void set_current_wifi_mode(WIFIDeviceMode_t mode);
extern WIFIDeviceMode_t g_current_mode;

int start_wifi_sta(const char* ssid, const char* passwd, char need_passwd) {
	if (ssid == NULL || strlen(ssid) == 0) {
		hcn_log_error("ssid is invalid \n");
		return -1;
	}

	WIFINetworkParams_t xNetworkParams = {0};
    WIFIReturnCode_t xWifiStatus;

	setDhcpClientState(1);
	vDHCPProcess(1, eInitialWait);
	xSendDHCPEvent();
	WIFI_Context_init();
    
	if (g_current_mode != eWiFiModeStation) {
		g_current_mode = eWiFiModeStation;
		WIFI_SetMode(eWiFiModeStation);
		hcn_log_info("Current mode is not sta, so reboot wifi\r\n");
		WIFI_Off();
		vTaskDelay(pdMS_TO_TICKS(200));
	}

	xWifiStatus = WIFI_On();

	if (xWifiStatus == eWiFiSuccess) {
		hcn_log_info("WiFi module initialized.\r\n");
	} else {
		hcn_log_info("WiFi module failed to initialize.\r\n");
		return -1;
	}

    extern bool get_wifi_start_state();
	hcn_log_info("g wifi start state:%d\r\n", get_wifi_start_state());

	while (0) {
		hcn_log_info("Starting scan\r\n");
		WIFIScanResult_t xScanResults[ucNumNetworks] = {0};
		xWifiStatus = WIFI_Scan(xScanResults, ucNumNetworks); // Initiate scan

		hcn_log_info("Scan started!!\r\n");

		///< For each scan result, print out the SSID and RSSI
		if (xWifiStatus == eWiFiSuccess) {
			hcn_log_info("Scan success\r\n");
			for (uint8_t i = 0; i < ucNumNetworks; i++) {
				hcn_log_info("%s : %d \r\n", xScanResults[i].ucSSID, xScanResults[i].cRSSI);
			}
			break;
		} else {
			hcn_log_info("Scan failed, status code: %d\n", (int)xWifiStatus);
			goto exit;
			// return -1;
		}

		vTaskDelay(200);
	}

    /* Setup parameters. */
    memset(&xNetworkParams, 0, sizeof(xNetworkParams));
    xNetworkParams.ucSSIDLength = strlen( ssid );
    memcpy(xNetworkParams.ucSSID, ssid, xNetworkParams.ucSSIDLength);
    memcpy(g_StaSsid, ssid, strlen( ssid ));
	g_StaSsid[strlen( ssid )] = '\0';
    xNetworkParams.xPassword.xWPA.ucLength = strlen( passwd );
    memcpy(xNetworkParams.xPassword.xWPA.cPassphrase, passwd, xNetworkParams.xPassword.xWPA.ucLength);
    memcpy(g_StaPasswd, passwd, strlen( passwd ));
	g_StaPasswd[strlen( passwd )] = '\0';
    if (need_passwd)
        xNetworkParams.xSecurity = eWiFiSecurityWPA2;
    else
        xNetworkParams.xSecurity = eWiFiSecurityOpen;

retry:
    // Connect!
	if (g_current_mode != eWiFiModeStation) {
		hcn_log_info("wifi mode is not station");
		return -1;
	}
	
	if (strcmp(g_StaSsid, (char *)xNetworkParams.ucSSID) != 0) {
		hcn_log_info("strcmp gStaSsid:%s, ssid:%s\n", g_StaSsid, xNetworkParams.ucSSID);
		return -1;
	} else {
		hcn_log_info("equal gStaSsid:%s, ssid:%s\n", g_StaSsid, xNetworkParams.ucSSID);
	}

	if (wifi_usr == WIFI_USER_OTA) {
		xWifiStatus = WIFI_ConnectAP(&(xNetworkParams));
		if (xWifiStatus == eWiFiSuccess) {
			hcn_log_info("WiFi Connected to AP:%s\r\n", xNetworkParams.ucSSID);
		} else {
			hcn_log_info("WiFi failed to connect to AP:%s\r\n", xNetworkParams.ucSSID);
			vTaskDelay(3000);
			goto retry;
		}
	} else {
		return -1;
	}
	vDHCPProcess(1, eWaitingSendFirstDiscover);
    xSendDHCPEvent();
    
exit:
    return 0;
}
#endif

extern int start_sta_ota_proc(const char* ssid, const char* passwd, char need_passwd);
extern int start_sta_ext(const char* ssid, const char* passwd, char need_passwd);

static void ota_wifi_task_proc(void *param) {
    while (vehicle_get_data(VEH_CARLINK_URL_STATUS) == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

	hcn_log_info("ota ssid:%s pwd:%s\r\n", hcn_get_ota_ssid(), hcn_get_ota_ap_pwd());
	
	start_sta_ota_proc(hcn_get_ota_ssid(), hcn_get_ota_ap_pwd(), 1);

	if (wifi_usr == WIFI_USER_OTA) {
		hcn_log_info("start tcp client\r\n");
		ota_task_running = true;
		start_tcp_client();
	} else {
		hcn_log_info("stop tcp client\r\n");
		stop_tcp_client();
	}

	hcn_log_info("ota_wifi_task_proc exit\n");

	ota_wifi_task = NULL;
	vTaskDelete(NULL);
}

int start_sta_init(void) {
	hcn_log_info("start ota wifi task\r\n");
    if (xTaskCreate(ota_wifi_task_proc, "start_ota_sta", 2048, 
                    NULL, 4, &ota_wifi_task) != pdPASS) {
        hcn_log_error("Create ota_wifi_task_proc failed!\n");
        return -1;
    }

    return 0;
}

bool ota_task_started(void) {
	return ota_task_running;
}

void stop_sta_task(void) {
	ota_task_running = false;
	stop_tcp_client();
	printf("ota sta task stopped\r\n");
}

extern int restart_p2p();
void wifi_mode_switching(void) {
	if (vehicle_get_data(VEH_ENTER_OTA_PAGE_STATE) == 1) {
		vehicle_set_data(VEH_ENTER_OTA_PAGE_STATE, 0);
		if (start_sta_init() != 0) {
			hcn_log_info("Start ota task failed!");
			return;
		}
	} else if (vehicle_get_data(VEH_ENTER_OTA_PAGE_STATE) == 2) {\
		vehicle_set_data(VEH_ENTER_OTA_PAGE_STATE, 0);
		vehicle_set_data(VEH_OTA_START_STATUS, 0);
		wifi_usr = WIFI_USER_EC;
		ota_task_running = false;
		hcn_log_info("start p2p step1\r\n");
		stop_sta_task();
		vTaskDelay(pdMS_TO_TICKS(500));
		hcn_log_info("start p2p switch..\r\n");
		restart_p2p();
	}
}



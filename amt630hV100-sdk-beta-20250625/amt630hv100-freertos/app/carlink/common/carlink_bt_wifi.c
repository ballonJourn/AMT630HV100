//#include "os_adapt.h"
#include <FreeRTOS_POSIX.h>
#include <task.h>
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_DHCP_Server.h"
#include <FreeRTOS_IP.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include "board.h"
#include "timer.h"
#include "iot_wifi.h"
#include "mmcsd_core.h"
#include "carlink_utils.h"
#include "console.h"
#include "fsc_bt.h"
#include "iap.h"
#include "config/hcn_config.h"


#if (CARLINK_EC == 1) || (CARLINK_CP == 1) || (CARLINK_AA == 1)
#if !defined(USE_LWIP) || !USE_LWIP
#error "Carlink need lwip!"
#endif

#include "ethernet.h"
#include "tcpip.h"
#include "lwip/apps/lwiperf.h"
#include "dhcp.h"

#include "carlink_video.h"
#include "mycommon.h"
#include "wifi_conf.h"
#include "carlink_common.h"
#include "utils/hcn_utils.h"
#include "config/hcn_config.h"
#include "bt_module/hcn_bt_parse.h"

#define DEV_NAME_PREFIX   "AP630_CARLINK"

static char g_cp_bt_mac[13] = {0};
static char g_cp_ble_mac[13] = {0};
static bool g_cp_bt_mac_ready = false;
static bool g_cp_ble_mac_ready = false;

static uint8_t                carlink_p2p_name[64]     = {0};
static uint8_t                carlink_ap_ssid[64]     = {"ap63011"};
static uint8_t                carlink_ap_passwd[16]  = {"88888888"};
static uint8_t                carlink_wifi_mac[32] = {0};

extern int wps_connect_done;

static const uint8_t        ucIPAddress[4] = {192, 168, 13, 1};
static const uint8_t        ucNetMask[4] = {255, 255, 255, 0};
static const uint8_t        ucGatewayAddress[4] = {0, 0, 0, 0};

static int                  lwip_tcpip_init_done_flag = 0;

static int g_bt_wifi_init_flag = 0;
static pthread_mutex_t btwifiLocker =
{
        .xIsInitialized = pdFALSE,
        .xMutex = { { 0 } },
        .xTaskOwner = NULL,
        .xAttr = { .iType = 0 }
};


static struct netif  gnetif[4];

extern int wifi_add_custom_ie(void *cus_ie, int ie_num);
extern int mmcsd_wait_sdio_ready(int32_t timeout);

static void carlink_reset_wifi_ap_info(const char *prefex);
static void carlink_start_wlan();
extern err_t dhcp_server_start(struct netif *netif, ip4_addr_t *start, ip4_addr_t *end);
extern err_t wlan_ethernetif_init(struct netif *netif);
#define lwip_ipv4_addr(addr) ((addr[0]) | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24))

const char *carlink_get_bt_mac()
{
	if (!g_cp_bt_mac_ready)
		return NULL;
	return (const char *)g_cp_bt_mac;
}

const char *carlink_get_ble_mac()
{
	if (!g_cp_ble_mac_ready)
		return NULL;
	return (const char *)g_cp_ble_mac;
}

const char *carlink_get_wifi_p2p_name()
{
	return (const char *)carlink_p2p_name;
}

const char *carlink_get_wifi_ssid()
{
	return (const char *)carlink_ap_ssid;
}


const char *carlink_get_wifi_mac()
{
	wifi_get_mac_address((char *)carlink_wifi_mac);
	return (const char *)carlink_wifi_mac;
}

const char *carlink_get_wifi_passwd()
{
	return (const char *)carlink_ap_passwd;
}

void carlink_get_ap_ip_addr(char ip[4])
{
	memcpy((void*)ip, (void*)ucIPAddress, 4);
}

///< 取蓝牙地址mac后4位作为wifi的ssid后缀
static void format_ssid_name(char *ssid_name, char *bt_addr_suffix) {
	if (ssid_name == NULL || bt_addr_suffix == NULL)
		return;

	sprintf(ssid_name, "%s-%s", HCN_CUSTOMER_NAME, bt_addr_suffix);

#ifdef HCN_STRING_LOWER_ENABLE
	sting_2_lower(ssid_name);
#endif
}


static void tcpip_init_done(void *arg)
{
	(void)arg;
}

static void carlink_lwiperf_report_cb_impl(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
	(void)arg;

	printf("lwiperf_report_cb_impl bytes:%d %d ms \r\n", bytes_transferred, ms_duration);
}

int carlink_wlan_tcp_ip_is_ready()
{
	return lwip_tcpip_init_done_flag;
}

//dd3000a0400000020022020961726B6D6963726F0003064C696E75780004030102030606ffffffffffff070666fadde250c0
static u8 carplay_vendor_ie[] = {
	0xdd, 0x30, 0x00, 0xa0, 0x40, 0x00, 0x00, 0x02, 0x00, 0x22, 0x02, 0x09, 0x61, 0x72, 0x6B, 0x6D, 
	0x69, 0x63, 0x72, 0x6F, 0x00, 0x03, 0x06, 0x4C, 0x69, 0x6E, 0x75, 0x78, 0x00, 0x04, 0x03, 0x01, 
	0x02, 0x03, 0x06, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x06, 0x66, 0xfa, 0xdd, 0xe2, 
	0x50, 0xc0
};
static rtw_custom_ie_t carplay_ie[1] = {{carplay_vendor_ie, PROBE_RSP | BEACON}};

void carlink_carplay_add_vendor_ie()
{
	wifi_add_custom_ie((void *)carplay_ie, 1);
}

void carlink_carplay_ie_replace_bt_mac(const char* btmac_str, int btmac_str_len)
{
	char btmac[6] = {0};
	string2hex((char *)btmac_str, btmac_str_len, btmac, 6);
	//memcpy((void*)g_link_info->btmac, (void*)btmac, 6);
	//sscanf(btmac_str, "%02x%02x%02x%02x%02x%02x", btmac[0], btmac[1], btmac[2], btmac[3], btmac[4], btmac[5]);
	carplay_vendor_ie[36] = btmac[0];
	carplay_vendor_ie[37] = btmac[1];
	carplay_vendor_ie[38] = btmac[2];
	carplay_vendor_ie[39] = btmac[3];
	carplay_vendor_ie[40] = btmac[4];
	carplay_vendor_ie[41] = btmac[5];
	//memcpy((void*)(carplay_vendor_ie + 36), (void*)btmac, 6);

	{
		int i;
		for (i = 0; i < sizeof(carplay_vendor_ie); i++) {
			printf("0x%02x, ", carplay_vendor_ie[i]);
		}printf("\r\n");
	}
}

#define ENABLE_REBOOT_WIFI  0
void carlink_restart_bt_wifi()
{
	carlink_bt_close();
#if ENABLE_REBOOT_WIFI
	netif_set_down(&gnetif[0]);
	vTaskDelay(pdMS_TO_TICKS(1000));
	WIFI_Off();
#endif
	vTaskDelay(pdMS_TO_TICKS(2000));
#if ENABLE_REBOOT_WIFI
	//start_ap(36, (const char *)carlink_ap_ssid, (const char *)carlink_ap_passwd, 1);
#if (CARLINK_EC)
	start_p2p((const char *)carlink_p2p_name, (const char *)carlink_ap_ssid, (const char *)carlink_ap_passwd);
#else
	start_ap(36, (const char *)carlink_ap_ssid, (const char *)carlink_ap_passwd, 1);
#endif
	carlink_carplay_add_vendor_ie();
	netif_set_default(&gnetif[0]);

	netif_set_up(&gnetif[0]);
#endif
	printf("reopen bt\r\n");
	carlink_bt_open();
}

static void carlink_start_wlan()
{
	char ap_prefix[5] = {0};

	ip4_addr_t ip_addr;
	ip4_addr_t netmask;
	ip4_addr_t gw;
	ip4_addr_t dhcp_addr_start;
	ip4_addr_t dhcp_addr_end;

	while(!g_cp_bt_mac_ready) {
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	memcpy(ap_prefix, g_cp_bt_mac + 8, 4);

#ifdef HCN_WIFI_NAME_FORMAT_ENABLE
	char ssid_name[20] = {0};
	format_ssid_name(ssid_name, ap_prefix);
	carlink_reset_wifi_ap_info(ssid_name);
#else
	carlink_reset_wifi_ap_info(ap_prefix);
#endif

#if (CARLINK_EC)

	printf("start p2p\r\n");
	printf("p2p name:%s, ssid:%s, passwd:%s\r\n", carlink_p2p_name, carlink_ap_ssid, carlink_ap_passwd);

	start_p2p((const char *)carlink_p2p_name, (const char *)carlink_ap_ssid, (const char *)carlink_ap_passwd);
#else
	start_ap(36, (const char *)carlink_ap_ssid, (const char *)carlink_ap_passwd, 1);
#endif

	ip_addr.addr = lwip_ipv4_addr(ucIPAddress);
	netmask.addr = lwip_ipv4_addr(ucNetMask);
	gw.addr      = lwip_ipv4_addr(ucGatewayAddress);
	tcpip_init(tcpip_init_done, NULL);
	netif_add(&gnetif[0],
#if LWIP_IPV4
		&ip_addr, &netmask, &gw,
#endif
		NULL, wlan_ethernetif_init, tcpip_input);
	
	netif_set_default(&gnetif[0]);

	uint8_t 	 addr_start[4] = {192, 168, 13, 20};
	uint8_t 	 addr_end[4] = {192, 168, 13, 30};
	dhcp_addr_start.addr 	= lwip_ipv4_addr(addr_start);
	dhcp_addr_end.addr 		= lwip_ipv4_addr(addr_end);
	dhcp_server_start(&gnetif[0], &dhcp_addr_start, &dhcp_addr_end);


	netif_set_up(&gnetif[0]);
	lwiperf_start_tcp_server_default(carlink_lwiperf_report_cb_impl, NULL);
	lwip_tcpip_init_done_flag = 1;
	carlink_carplay_add_vendor_ie();
}

static void dump_ip_addr(const char *msg, const ip_addr_t *server_addr)
{
	printf("%s %d.%d.%d.%d \r\n", msg,
              ip4_addr1_16(ip_2_ip4(server_addr)), ip4_addr2_16(ip_2_ip4(server_addr)), ip4_addr3_16(ip_2_ip4(server_addr)), ip4_addr4_16(ip_2_ip4(server_addr)));
}


static void dhcp_client_status_callback(struct netif *netif, int status, const ip_addr_t *server_addr)
{
	if (&gnetif[0] == netif) {
		dump_ip_addr("dhcp server ip     :", (const ip_addr_t *)server_addr);
		dump_ip_addr("dhcp client ip     :", (const ip_addr_t *)&netif->ip_addr);
		dump_ip_addr("dhcp client netmask:", (const ip_addr_t *)&netif->netmask);
		dump_ip_addr("dhcp client gw     :", (const ip_addr_t *)&netif->gw);
	}
}

int start_sta_ext(const char* ssid, const char* passwd, char need_passwd)
{
	int ret = -1;
	ip4_addr_t ip_addr;
	ip4_addr_t netmask;
	ip4_addr_t gw;
	
	ret = start_sta(ssid, passwd, need_passwd);
	if (ret != 0) {
		printf("start wifi sta failed\r\n");
		return ret;
	}

	if (lwip_tcpip_init_done_flag) {
		netif_remove(&gnetif[0]);
		netif_add(&gnetif[0],
#if LWIP_IPV4
			&ip_addr, &netmask, &gw,
#endif
			NULL, wlan_ethernetif_init, tcpip_input);
		netif_set_default(&gnetif[0]);
		dhcp_regisger_status_callback(dhcp_client_status_callback);
		netif_set_up(&gnetif[0]);
		dhcp_start(&gnetif[0]);
	}

	return ret;
}

int __restart_p2p(const char *dev_name, const char *ssid, const char *passwd)
{
	int ret = -1;
	ip4_addr_t ip_addr;
	ip4_addr_t netmask;
	ip4_addr_t gw;

	ip_addr.addr = lwip_ipv4_addr(ucIPAddress);
	netmask.addr = lwip_ipv4_addr(ucNetMask);
	gw.addr      = lwip_ipv4_addr(ucGatewayAddress);

	ret = start_p2p(dev_name, ssid, passwd);
	if (ret != 0) {
		printf("restart wifi p2p failed\r\n");
		return ret;
	}
	if (!lwip_tcpip_init_done_flag) {
		printf("lwip tcpip is not inited\r\n");
		return ret;
	}

	dhcp_stop(&gnetif[0]);
	netif_remove(&gnetif[0]);

	netif_add(&gnetif[0],
#if LWIP_IPV4
		&ip_addr, &netmask, &gw,
#endif
		NULL, wlan_ethernetif_init, tcpip_input);

	netif_set_up(&gnetif[0]);
	ret = 0;
	return ret;
}

int restart_p2p()
{
	if (!g_cp_bt_mac_ready) {
		printf("bt is not ready at %s", __func__);
		return -1;
	}
	return __restart_p2p((const char *)carlink_p2p_name, (const char *)carlink_ap_ssid, (const char *)carlink_ap_passwd);
}

static void carlink_reset_wifi_ap_info(const char *prefex)
{
	if (prefex) {
#ifndef HCN_WIFI_NAME_FORMAT_ENABLE
		memset(carlink_ap_ssid, 0, sizeof(carlink_ap_ssid));
    	sprintf((char *)carlink_ap_ssid,   "%s_%s", DEV_NAME_PREFIX, prefex);
		memset(carlink_p2p_name, 0, sizeof(carlink_p2p_name));
		sprintf((char *)carlink_p2p_name, "DIRECT-%s_p2p_%s", DEV_NAME_PREFIX, prefex);//do not delete "DIRECT-" !!!
#else
		memset(carlink_ap_ssid, 0, sizeof(carlink_ap_ssid));
		memset(carlink_p2p_name, 0, sizeof(carlink_p2p_name));

	 	snprintf((char *)carlink_ap_ssid, sizeof(carlink_ap_ssid), "%s", prefex);
		snprintf((char *)carlink_p2p_name, sizeof(carlink_p2p_name), "%s-p2p", prefex);
#endif
	}
}

#ifdef HCN_WIFI_NAME_FORMAT_ENABLE
static void hcn_wifi_info_init(void)
{
	memset(carlink_p2p_name, 0, sizeof(carlink_p2p_name));
	memset(carlink_ap_ssid, 0, sizeof(carlink_ap_ssid));
	memset(carlink_ap_passwd, 0, sizeof(carlink_ap_passwd));
	snprintf((char *)carlink_ap_passwd, sizeof(carlink_ap_passwd), "%s", HCN_CUSTOMER_AP_PASSWD);	
	snprintf((char *)carlink_ap_ssid, sizeof(carlink_ap_ssid), "%s", HCN_DEFAULT_AP_NAME);
}
#endif

static void carlink_bt_callback(char * cAtStr)
{
	char* cmd_para = NULL;
	//printf("\r\nfsc_bt_callback_ec %s %d\r\n", cAtStr, strlen(cAtStr));
	struct carlink_event ev;
	memset((void*)&ev, 0, sizeof(ev));
	ev.type = CARLINK_EVENT_NONE;
    
	if (0 == strncmp(cAtStr, "ERR", 3) || 0 == strncmp(cAtStr, "OK", 2)) {
        return;
	} else if (0 == strncmp(cAtStr, "+IAPDATA=",      9)) {
        char ble_buf[256] = {0};
		int data_len, i;
		int data_str_len = (strlen(cAtStr) - 9);
		cmd_para = cAtStr + 9;
		data_len = data_str_len / 2;
		string2hex(cmd_para, data_str_len, ble_buf, data_len);
		for (i = 0; i < data_len; i++) {
			printf("%02x ", ble_buf[i]);
		}printf("\r\n");
		//iap2_read_data_proc(ble_buf, data_len);
		carlink_rfcomm_data_read_hook(ble_buf, data_len);
	} else if (0 == strncmp(cAtStr, "+AAPDATA=",      9)) {
        char ble_buf[256] = {0};
		int data_len, i;
		int data_str_len = (strlen(cAtStr) - 9);
		cmd_para = cAtStr + 9;
		data_len = data_str_len / 2;
		string2hex(cmd_para, data_str_len, ble_buf, data_len);
		for (i = 0; i < data_len; i++) {
			printf("%02x ", ble_buf[i]);
		}printf("\r\n");
		//iap2_read_data_proc(ble_buf, data_len);
		carlink_rfcomm_data_read_hook(ble_buf, data_len);
	} else if (0 == strncmp(cAtStr, "+GATTDATA=",      10)) {
	} else if (0 == strncmp(cAtStr, "+GATTSTAT=1",11)) {
        printf("TRACE[%s][%d]:EC_EVENT_BT_DISCONNECT\r\n",__func__ ,__LINE__);
		ev.type = CARLINK_EVENT_BT_DISCONNECT;
		ev.disable_filter = true;
		carlink_notify_event(&ev);
	} else if (0 == strncmp(cAtStr, "+GATTSTAT=3",11)) {
        printf("TRACE[%s][%d]:EC_EVENT_BT_CONNECT\r\n",__func__ ,__LINE__);

		ev.type = CARLINK_EVENT_BT_CONNECT;
		ev.disable_filter = true;
		carlink_notify_event(&ev);
	} else if (0 == strncmp(cAtStr, "+ADDR=",       6)) {
		char cmd_str[64] = {0};

#ifdef HCN_WIFI_NAME_FORMAT_ENABLE
		char bt_name[20] = {0};
		format_ssid_name(bt_name, (cAtStr + 6 + 8));
        sprintf(cmd_str, "AT+NAME=%s\r\n", bt_name);
#else
		sprintf(cmd_str, "AT+NAME=%s_%s\r\n", DEV_NAME_PREFIX, (cAtStr + 6 + 8));
#endif

		printf("ADDR:%s\r\n", cAtStr + 6);
		printf("set bt name:%s\r\n", cmd_str);

		console_send_atcmd(cmd_str, strlen(cmd_str));//get mac addr
#if 0	
		///< leddr处设置ble name
		memset(cmd_str, 0, sizeof(cmd_str));
		sprintf(cmd_str, "AT+LENAME=%s_%s\r\n", DEV_NAME_PREFIX, (cAtStr + 6 + 8));
		console_send_atcmd(cmd_str, strlen(cmd_str));//get mac addr
#endif

		memcpy(g_cp_bt_mac, (cAtStr + 6), 12);
		carlink_carplay_ie_replace_bt_mac(cAtStr + 6, 12);
		g_cp_bt_mac_ready = true;
	} else if (0 == strncmp(cAtStr, "+VER",         4)) {
		char* cmd = "AT+ADDR\r\n";
		console_send_atcmd(cmd, strlen(cmd));//get mac addr
    } else if (0 == strncmp(cAtStr, "+NAME=", 6)) {
		char cmd_str[64] = {0};
		sprintf(cmd_str, "AT+LEADDR\r\n");
		console_send_atcmd(cmd_str, strlen(cmd_str));//get LE addr
	} else if (0 == strncmp(cAtStr, "+LEADDR=", 6)) {
      char hexMacAddr[6] = {0};
	  char m_tmp_buf[64] = {0};
      string2hex(&cAtStr[8], 12, hexMacAddr, sizeof(hexMacAddr));
      sprintf(m_tmp_buf, "AT+ADVDATA=%s\r\n", hexMacAddr);
      console_send_atcmd(m_tmp_buf, 19);

	  ///< 设置ble name
	  g_cp_ble_mac_ready = true;
	  memcpy(g_cp_ble_mac, cAtStr + 8, 12);
	  char cmd_str[64] = {0};
	
#ifdef HCN_WIFI_NAME_FORMAT_ENABLE
	  memset(cmd_str, 0, sizeof(cmd_str));

	  char bt_ssid_name_ble[20] = {0};
      format_ssid_name(bt_ssid_name_ble,  (cAtStr + 8));
      sprintf(cmd_str, "AT+LENAME=%s\r\n", bt_ssid_name_ble);
#else
	   memset(cmd_str, 0, sizeof(cmd_str));
	   sprintf(cmd_str, "AT+LENAME=%s_%s\r\n", DEV_NAME_PREFIX, (cAtStr + 6 + 8));
#endif
	   	
	   printf("BLE_ADDR:%s\r\n", cAtStr + 8);
	   printf("set ble name:%s\r\n", cmd_str);

	   console_send_atcmd(cmd_str, strlen(cmd_str)); ///<set ble name
    } else if (0 == strncmp(cAtStr, "+GATTSENT=", 10)) {
        return;
    } else if (0 == strncmp(cAtStr, "+IAPSTAT=3", 10)) {
		ev.type = CARLINK_EVENT_BT_IAP_READY;
		ev.link_type = CARPLAY_WIRELESS;
		carlink_notify_event(&ev);
		
    } else if (0 == strncmp(cAtStr, "+IAPSTAT=1", 10)) {
		ev.type = CARLINK_EVENT_BT_DISCONNECT;
		ev.link_type = CARPLAY_WIRELESS;
		carlink_notify_event(&ev);
		
    } else if (0 == strncmp(cAtStr, "+AAPSTAT=3", 10)) {
		ev.type = CARLINK_EVENT_BT_AA_RFCOMM_READY;
		ev.link_type = AUTO_WIRELESS;
		carlink_notify_event(&ev);
		
    } else if (0 == strncmp(cAtStr, "+AAPSTAT=1", 10)) {
		ev.type = CARLINK_EVENT_BT_DISCONNECT;
		ev.link_type = AUTO_WIRELESS;
		carlink_notify_event(&ev);
    } else {
		if (0 != strncmp(cAtStr, "+PBDATA=1", 9)) {
            printf("bt_callback_ec %s\r\n", cAtStr);
        }

		if (bt_msg_task_add(cAtStr, strlen(cAtStr)) != 0) {
			printf("bt_callback_ec msg full!\r\n");
		}
	}
}

int carlink_iap_data_write(unsigned char *data, int len)
{
    uint8_t *at_str = NULL, *at_str_ptr = NULL;
    uint8_t *at_prefix = "AT+IAPSEND=";
    int prefix_len = strlen((char *)at_prefix);
    int cmd_len = (len * 2 + prefix_len + 2);

    at_str = (uint8_t *)malloc(cmd_len + 1);
    if (NULL == at_str)
        return -1;
	memset(at_str, 0, cmd_len + 1);
    at_str_ptr = at_str;
    sprintf((char*)at_str_ptr, "%s", (char*)at_prefix);
    at_str_ptr += prefix_len;
    hex2str(data, len, (char *)at_str_ptr);
    at_str_ptr 			+= (len * 2);
    *at_str_ptr++ 		= '\r';
    *at_str_ptr++		= '\n';
    console_send_atcmd((char *)at_str, cmd_len);
    free(at_str);
    return len;
}

int carlink_auto_rfcomm_data_write(unsigned char *data, int len)
{
    uint8_t *at_str = NULL, *at_str_ptr = NULL;
    uint8_t *at_prefix = "AT+AAPSEND=";
    int prefix_len = strlen((char *)at_prefix);
    int cmd_len = (len * 2 + prefix_len + 2);

    if (0) {
		int i;
		printf("AAPSend: ");
		for (i = 0; i < len; i++) {
			printf("%02x ", data[i]);
		}printf("\r\n");
	}

    at_str = (uint8_t *)malloc(cmd_len + 1);
    memset(at_str, 0, cmd_len + 1);
    if (NULL == at_str)
        return -1;
    at_str_ptr = at_str;
    sprintf((char*)at_str_ptr, "%s", (char*)at_prefix);
    at_str_ptr += prefix_len;
    hex2str(data, len, (char *)at_str_ptr);
    at_str_ptr 			+= (len * 2);
    *at_str_ptr++ 		= '\r';
    *at_str_ptr++		= '\n';
    console_send_atcmd((char *)at_str, cmd_len);
    free(at_str);
    return len;
}

void carlink_bt_open_nolock()
{
	char at_str[] = {"AT+BTEN=1\r\n"};
	console_send_atcmd((char *)at_str, strlen(at_str));
}


void carlink_bt_open()
{
	pthread_mutex_lock(&btwifiLocker);
	if (0 == g_bt_wifi_init_flag) {
		goto exit;
	}
	carlink_bt_open_nolock();
exit:
	pthread_mutex_unlock(&btwifiLocker);
}

void carlink_bt_close()
{
	char at_str[] = {"AT+BTEN=0\r\n"};
	pthread_mutex_lock(&btwifiLocker);
	if (0 == g_bt_wifi_init_flag) {
		goto exit;
	}

	console_send_atcmd((char *)at_str, strlen(at_str));
exit:
	pthread_mutex_unlock(&btwifiLocker);

}


static  void carlink_wifi_event_handler( WIFIEvent_t * xEvent )
{
    WIFIEventType_t xEventType = xEvent->xEventType;
	char mac_str[32] = {0};
	char *mac = NULL;

	if (xEvent) {
		mac = (char *)xEvent->xInfo.xAPStationConnected.ucMac;
		sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	wps_connect_done = 0;

    if (0) {
    } else if (eWiFiEventConnected                    == xEventType) {// meter is sta
		struct carlink_event ev = {0};
		ev.type = CARLINK_EVENT_WIFI_CONNECT;
		ev.link_type = CARLINK_ECLINK;

		carlink_notify_event(&ev);

        printf("\r\n The meter is connected to ap \r\n");
    } else if (eWiFiEventDisconnected                == xEventType) {// meter is sta
		struct carlink_event ev = {0};
		ev.type = CARLINK_EVENT_WIFI_DISCONNECT;
		ev.link_type = CARLINK_ECLINK;

		carlink_notify_event(&ev);

        printf("\r\n The meter is disconnected from ap \r\n");
    } else if (eWiFiEventAPStationConnected       == xEventType) {// meter is ap
		struct carlink_event ev = {0};
		ev.type = CARLINK_EVENT_AP_CONNECT;
		ev.link_type = CARLINK_ECLINK;

		carlink_notify_event(&ev);

        printf("\r\n The meter in AP is connected by sta %s \r\n", mac_str);
    } else if (eWiFiEventAPStationDisconnected   == xEventType) {// meter is ap
		struct carlink_event ev = {0};
		ev.type = CARLINK_EVENT_AP_DISCONNECT;
		ev.link_type = CARLINK_ECLINK;

		carlink_notify_event(&ev);

        printf("\r\n The sta %s is disconnected from the meter \r\n", mac_str);
#if 0
		ev.type = CARLINK_EVENT_WIFI_DISCONNECT;
		ev.disable_filter = true;
		memcpy((void*)ev.u.para, (void*)mac_str, strlen(mac_str));
		carlink_notify_event(&ev);
#endif
	
	}
}

#if ( ipconfigUSE_DHCP_HOOK != 0 )
eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase,
                                            uint32_t ulIPAddress )
{
	eDHCPCallbackAnswer_t eReturn;
	char g_ip_str[32] = {0};
	struct carlink_event ev;

	sprintf(g_ip_str, "%d.%d.%d.%d\r\n", (ulIPAddress >> 0) & 0xFF, 
	(ulIPAddress >> 8) & 0xFF, (ulIPAddress >> 16) & 0xFF, (ulIPAddress >> 24) & 0xFF);
	printf("\r\n  eDHCPPhase:%d  ulIPAddress:%s state:%d \r\n", eDHCPPhase, g_ip_str, getDhcpClientState());
	if (getDhcpClientState() == 0)
		return eDHCPStopNoChanges;

	switch( eDHCPPhase )
	{
	case eDHCPPhaseFinished: {
			printf("[ouchunhua]dhcp client get ip:%s\r\n", g_ip_str);
			ev.type = CARLINK_EVENT_CLIENT_DHCP_READY;
			ev.link_type = CARLINK_ECLINK;
			carlink_notify_event(&ev);
		}

		eReturn = eDHCPContinue;
	break;
	case eDHCPPhasePreDiscover  :
		eReturn = eDHCPContinue;
	break;

	case eDHCPPhasePreRequest  :
		eReturn = eDHCPContinue;
	break;

	case 0xFF:
		{
			printf("[ouchunhua]dhcp test 0xFF\r\n");
		}
		eReturn = eDHCPContinue;
	break;
	
	default :
		eReturn = eDHCPContinue;
	break;
	}

	return eReturn;
}
#endif

#ifndef HCN_WIFI_INIT_DELAY_ENABLE
static BaseType_t carlink_wifi_init()
{
	static int wifi_sdio_status = MMCSD_HOST_UNPLUGED;

	if (wifi_sdio_status == MMCSD_HOST_PLUGED)
		return 0;

	printf("wifi_init_sdio\n");

	WIFI_Context_init();
	WIFI_RegisterEvent(eWiFiEventMax, carlink_wifi_event_handler);
	for (;;) {
		wifi_sdio_status = mmcsd_wait_sdio_ready((int32_t)portMAX_DELAY);
		if (wifi_sdio_status == MMCSD_HOST_PLUGED) {
			printf("detect sdio device\r\n");
			break;
		}
	}
	return 0;
}
#else

int carlink_wifi_init() {
   	static int wifi_sdio_status = MMCSD_HOST_UNPLUGED;
	if (wifi_sdio_status == MMCSD_HOST_PLUGED)
		return 0;

    printf("wifi_init_sdio\n");
	WIFI_Context_init();
	WIFI_RegisterEvent(eWiFiEventMax, carlink_wifi_event_handler);
	for (;;) {
		wifi_sdio_status = mmcsd_wait_sdio_ready(pdMS_TO_TICKS(2000));
		if (wifi_sdio_status == MMCSD_HOST_PLUGED) {
			printf("detect sdio device\r\n");
			break;
		} else {
            return -1;
        }
	}

	return 0;
}

#endif

#if 0
static void bt_set_support_carplay() // cp
{
	char cmd_str[64] = {0};

	sprintf(cmd_str, "AT+PROFILE=33962\r\n");
	console_send_atcmd(cmd_str, strlen(cmd_str));
}
#endif

static void bt_set_support_carplay_android_auto()//auto + cp
{
	char cmd_str[64] = {0};

	sprintf(cmd_str, "AT+PROFILE=50346\r\n");
	console_send_atcmd(cmd_str, strlen(cmd_str));
}

static void  taskInitCarlinkWlanThread(void* param)
{
	carlink_start_wlan();
	vTaskDelete(NULL);
}

int carlink_bt_wifi_init()
{
	int retry_count = 0;
	pthread_mutex_lock(&btwifiLocker);
	if (g_bt_wifi_init_flag) {
		pthread_mutex_unlock(&btwifiLocker);
		return 0;
	}
	
#ifndef HCN_WIFI_INIT_DELAY_ENABLE
	carlink_wifi_init();
#endif

#ifdef HCN_WIFI_NAME_FORMAT_ENABLE
	hcn_wifi_info_init();
#endif

	console_register_cb(NULL, carlink_bt_callback);
	fsc_bt_main();
	//bt_set_support_carplay();
	bt_set_support_carplay_android_auto();
	carlink_bt_open_nolock();
	lwip_tcpip_init_done_flag = 0;
	xTaskCreate(taskInitCarlinkWlanThread, "initThread", 2048 * 4, NULL, 1, NULL);

	while(lwip_tcpip_init_done_flag == 0) {
		if (retry_count++ > 50)
			break;
		vTaskDelay(pdMS_TO_TICKS(300));
	}

	if (lwip_tcpip_init_done_flag) {
		g_bt_wifi_init_flag = 1;
		printf("bt wlan init ok\r\n");
	} else {
		g_bt_wifi_init_flag = 0;
		printf("bt wlan init failed\r\n");
	}
	pthread_mutex_unlock(&btwifiLocker);
	printf("bt wlan init is ok\r\n");
	return 0;
}
#endif

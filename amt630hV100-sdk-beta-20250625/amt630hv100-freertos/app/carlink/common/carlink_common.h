#ifndef __CARLINK_COMMON_H
#define __CARLINK_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

typedef enum
{
	CARLINK_CARPLAY = 0x00,
	CARLINK_CARLIFE,
	CARLINK_ANDROID_CARLIFE,
	CARLINK_ANDROID_MIRROR = 3,
	CARLINK_IOS_CARLIFE,
	CARLINK_ANDROID_AUTO = 5,
	CARLINK_ECLINK = 0x06,
	CARLINK_CARPLAY_WIRELESS,
	CARLINK_AUTO_WIRELESS,
	CARLINK_ECLINK_WIRELESS,
}CARLink_TYPE;



#define CARLINK_VIDEO_WIDTH              1024
#define CARLINK_VIDEO_HEIGHT             600



enum CARLINK_EVENT_TYPE
{
	CARLINK_EVENT_NONE = 0,
	CARLINK_EVENT_BT_CONNECT,
	CARLINK_EVENT_BT_IAP_READY,
	CARLINK_EVENT_BT_AA_RFCOMM_READY,
	CARLINK_EVENT_BT_DISCONNECT,
	CARLINK_EVENT_WIFI_DISCONNECT,
	CARLINK_EVENT_WIFI_CONNECT,
	CARLINK_EVENT_CLIENT_DHCP_READY,
	CARLINK_EVENT_BUILD_NET_OK,
	CARLINK_EVENT_AP_CONNECT,
	CARLINK_EVENT_AP_DISCONNECT,

	CARLINK_EVENT_INIT_DONE,
	CARLINK_EVENT_MSG_SESSION_CONNECT,
	CARLINK_EVENT_MSG_SESSION_STOP,
	CARLINK_EVENT_MSG_DISABLE_BT,
	CARLINK_EVENT_KEY_EVENT
};

struct carlink_event
{
	enum CARLINK_EVENT_TYPE type;
	CARLink_TYPE link_type;
	bool          disable_filter;
	union {
		int len;
		void* data;
		uint8_t para[32];
	}u;
};

struct ICalinkEventCallbacks
{
    void (*onEvent)(void* ctx, const struct carlink_event *ev);
	void (*rfcomm_data_read)(void* ctx, const void* buf, int len);
    void* cb_ctx;                                  
};


void carlink_notify_event(struct carlink_event *ev);
void carlink_notify_event_isr(struct carlink_event *ev);

void carlink_register_event_callbacks(const struct ICalinkEventCallbacks *pcb);
void carlink_rfcomm_data_read_hook(void* buf, int len);
int carlink_iap_data_write(unsigned char *data, int len);
int carlink_auto_rfcomm_data_write(unsigned char *data, int len);
int carlink_bt_wifi_init();
int carlink_wlan_tcp_ip_is_ready();
void carlink_bt_open();
void carlink_bt_close();
void carlink_restart_bt_wifi();
const char *carlink_get_bt_mac();
const char *carlink_get_ble_mac();
const char *carlink_get_wifi_p2p_name();
const char *carlink_get_wifi_ssid();
const char *carlink_get_wifi_passwd();
const char *carlink_get_wifi_mac();
void carlink_get_ap_ip_addr(char ip[4]);


int carlink_common_init();


#ifdef __cplusplus
}
#endif
#endif

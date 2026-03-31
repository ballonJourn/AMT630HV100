#ifndef __IAP_H_H
#define __IAP_H_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _IAP2_LINK_STATUS
{
	IAP2_INIT = -1,
	IAP2_USB_HOST_INSERTED,
	IAP2_CONNECT,
	IAP2_ID_OK,
	IAP2_USB_HOST_REMOVE,
	IAP2_USB_SWITCH_FAIL,
	IAP2_ID_FAIL,
	IAP2_DISCONNECT
}IAP2_LINK_STATUS;

typedef void (*iap2_link_status_cb_f)(void *ctx, IAP2_LINK_STATUS status);
typedef void (*iap2_msg_time_update_cb_f)(void *ctx, long long time, int zone_offset);
typedef void (*iap2_msg_time_zone_update_cb_f)(void *ctx, long long time, int16_t zone, int8_t daylightOff);
typedef void (*iap2_msg_gps_cb_f)(void *ctx, unsigned char session, int start);
typedef void (*iap2_msg_gps_gprmc_data_status_cb_f)(void *ctx, int value_a, int value_v, int value_x);
typedef void (*iap2_msg_identify_cb_f)(void *ctx, int type, int ok);
typedef void (*iap2_msg_wl_carplay_update_cb_f)(void *ctx, int status);
typedef int (*iap2_usb_switch_cb_f)(void *ctx, int state);
typedef int (*iap2_write_data_cb_f)(void *ctx, char *buf, int len);
typedef void (*iap2_msg_language_update_cb_f)(void *ctx, const char *lang);
typedef void (*iap2_msg_call_state_update_cb_f)(void *ctx, 
						const char *remoteId,
						const char *displayName,
						int status,
						int direction,
						const char *uuid,
						const char *addrBookId,
						const char *label,
						int service);

typedef void (*iap2_msg_nowplaying_update_cb_f)(void *ctx, 
						int playback_status, int elapsed_time,
						uint8_t* media_item_title, int media_item_title_len,
						uint8_t* media_item_album_title, int media_item_album_title_len, 
						uint8_t* media_item_artist, int media_item_artist_len, 
						int song_length_ms);

typedef void (*iap2_msg_msg_json_cb_f)(void *ctx, const char* buf, int len);
typedef struct iap2_callbacks_st
{
	iap2_link_status_cb_f 					iap2_link_status_cb;
	iap2_msg_time_update_cb_f 				iap2_msg_time_update_cb;
	iap2_msg_gps_cb_f						iap2_msg_gps_cb;
	iap2_msg_gps_gprmc_data_status_cb_f     iap2_msg_gps_gprmc_data_status_cb;
	iap2_msg_identify_cb_f					iap2_msg_identify_cb;
	iap2_msg_wl_carplay_update_cb_f			iap2_msg_wl_carplay_update_cb;
	iap2_msg_language_update_cb_f			iap2_msg_language_update_cb;
	iap2_msg_call_state_update_cb_f			iap2_msg_call_state_update_cb;
	iap2_usb_switch_cb_f 					iap2_usb_switch_cb;
	iap2_write_data_cb_f					iap2_write_data_cb;
	iap2_msg_time_zone_update_cb_f          iap2_msg_time_zone_update_cb;
	iap2_msg_nowplaying_update_cb_f         iap2_msg_nowplaying_update_cb;
	iap2_msg_msg_json_cb_f                  iap2_msg_msg_json_cb;
	
	void *ctx;
}iap2_callbacks;

void iap2_register_callbacks(iap2_callbacks *pcb);
int iap2_start();
int iap2_start_wifi_session();
int force_iap2_bt_start();
void iap2_stop();
int apple_send_pascd(unsigned char session, char *data);
int iap2_read_data_proc(char *buf, int len);
int apple_start_update_call_state(int start);


#ifdef __cplusplus
}
#endif
#endif

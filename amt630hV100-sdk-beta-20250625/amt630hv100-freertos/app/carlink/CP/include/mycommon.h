#ifndef MYCOMMON_H_H
#define MYCOMMON_H_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#ifndef CARLINK_LINK_TYPE
#define CARLINK_LINK_TYPE
typedef enum
{
	CARPLAY = 0x00,
	CARLIFE,
	ANDROID_CARLIFE,
	ANDROID_MIRROR = 3,
	IOS_CARLIFE,
	ANDROID_AUTO = 5,
	ECLINK = 0x06,
	CARPLAY_WIRELESS,
	AUTO_WIRELESS
}Link_TYPE;
#endif

//general function
typedef enum __USB_MODE
{
	UNDEFINED = 0,
	HOST,
	PERIPHERAL,
	OTG
}USB_MODE;

#define LOGV(...)  printf(__VA_ARGS__); printf("\r\n");

struct view_area
{
	short w;
	short h;
	short x;
	short y;
};

typedef struct __carplay_cfg_info
{
	char 	*iap2_name;
	char 	*iap2_modelIdentifier;
	char	*iap2_manfacturer;
	char	*iap2_serialnumber;
	char	*iap2_sw_ver;
	char	*iap2_hw_ver;
	char 	*iap2_vehicleInfo_name;
	char 	*iap2_product_uuid;
	char 	*iap2_usb_serial_num;

	char	*manfacturer;
	char	*oem_icon_label;
	char	*oem_icon_path;
	char	*os_info;
	char	*iOSVersionMin;
	char 	*limited_ui_elements;
	char	*guuid;
	char 	*devid;
	char    link_type;
	char	btmac[6];
	bool	oem_icon_visible;
	bool 	limited_ui;
	bool	right_hand_driver;
	bool 	night_mode;
	bool	has_knob;
	bool	has_telbutton;
	bool	has_mediabutton;
	bool	has_proxsensor;
	bool	has_EnhancedReqCarUI;
	bool	has_ETCSupported;
	bool	HiFiTouch;
	bool	LoFiTouch;
	unsigned short	usb_country_code;
	unsigned short	tp_verndor_code;
	unsigned short	tp_product_code;
	unsigned short	tel_verndor_code;
	unsigned short	tel_product_code;
	unsigned short	knob_verndor_code;
	unsigned short	knob_product_code;
	unsigned short	proxsensor_verndor_code;
	unsigned short	proxsensor_product_code;
	short		width;//pixel
	short		height;
	short		fps;
	short		screen_width_phy;//mm
	short		screen_height_phy;

	char 	encrypt_ic_i2c_bus_num;
	char	encrypt_ic_addr;
	char	usb_idx;
	char 	need_sw_aec;
	char	aec_delay;
	bool	tvout_enable;

	bool	use_remote_audio;
	char 	video_type;
	short	icurrent;
	int 	duck_vol;

	char    enable_iap_carplay_sess;
	char    *keychain_path_dir;

	char*   wifi_ssid;
	char*   wifi_passwd;
	char*   public_key;
	char*   src_version;
	char    ip_v4_addr[4];
	char   wifi_channel;
	short  net_port;

	char    carplay_net_ready;
	char    disable_bonjour;
	char    iap_carplay_rej;

	bool enable_enhanced_siri;
	bool enable_single_ui;
	bool disable_carplay_audio;

	short 	mfi_ic_i2c_bus_num;
	short	mfi_ic_addr;

	bool is_old_carplay_ver;

	struct view_area area[3];
	char  view_area_index;
	
}carplay_cfg_info;

typedef struct _carlink_flash_io
{
	uint32_t (*get_data_arae_size)(void* ctx);
	uint32_t (*get_flash_block_erase_size)(void* ctx);
	int32_t  (*op_flash)(void* flash_handle, void* ctx, int open);
    int32_t  (*read_data)(void* flash_handle, void *data, uint32_t length, uint32_t offset, void* ctx);
    int32_t  (*write_data)(void* flash_handle, void *data, uint32_t length, uint32_t offset, void* ctx);
	void* ctx;
	void* flash_handle;
} carlink_flash_io;
void register_carlink_flash_io_interface(carlink_flash_io *handle);

#if 0
typedef struct __auto_cfg_info
{
	short		width;//pixel
	short		height;
	short		fps;
	char*   wifi_ssid;
	char*   wifi_passwd;
	char   wifi_channel;
	bool disable_carplay_audio;

} auto_cfg_info;
#endif

extern carplay_cfg_info  *g_link_info;
//extern auto_cfg_info  *g_auto_link_info;

#ifdef __cplusplus
}
#endif
#endif

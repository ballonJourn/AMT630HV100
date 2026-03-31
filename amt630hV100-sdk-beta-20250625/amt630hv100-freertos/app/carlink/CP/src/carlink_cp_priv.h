#ifndef __CARLINK_CP_PRI_H
#define __CARLINK_CP_PRI_H

#ifdef __cplusplus
extern "C" {
#endif

#define IAP2NAME                    "KY CAR"
#define IAP2MODEID                  "Linux"
#define SERIALNUMBER                "0123456789ABCDEF"
#define SWVER                       "sw0.1"
#define HWVER                       "hw0.1"
#define VEHICLENAME                 "audi"

#define MANFACTURER					"ARK"
#define OEMICONLABEL				"Home"
#define OEMICONPATH					"/sf/carplay_icon.png"
#define OSINFO						"rtos"
#define iOS_VER_MIN					"11D257"
//#define LIMITEDUIELEMENTS			"softKeyboard"
#define LIMITEDUIELEMENTS			"japanMaps"
#define DEFAULTUUID					"e5f7b72d-9b7f-4305-954b-973f612a150b"
#define DEFAULTDEVID				"10:13:52:33:67:09"
#define ISOEMICONVISIBLE			1
#define ISRIGHTHANDDRIVER			0
#define ISLIMITEDUI					1
#define HAS_KNOB					1
#define HAS_PROXSENSOR				0
#define HAS_ETC						0
#define HAS_EnhancedRequestCarUI	0

#define kUSBCountryCodeUS                   33
#define kUSBVendorTouchScreen               0x0525
#define kUSBProductTouchScreen              0xa4a1
#define kUSBVendorTeleButtons               0x0525
#define kUSBProductTeleButtons              0xa4a2
#define kUSBVendorKnobButtons               0x0525
#define kUSBProductKnobButtons              0xa4a2
#define kUSBVendorProxSensor                0
#define kUSBProductProxSensor               0

#define PHYSICAL_WIDTH				160
#define PHYSICAL_HEIGHT				80

void carplay_modules_test();
int carlink_cp_audio_start(int handle, int type, int rate, int bits, int channels);
void carlink_cp_audio_stop(int handle, int type);


#ifdef __cplusplus
}
#endif
#endif

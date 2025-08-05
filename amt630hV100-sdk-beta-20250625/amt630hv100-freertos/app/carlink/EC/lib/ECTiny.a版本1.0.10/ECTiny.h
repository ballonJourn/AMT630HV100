#ifndef __ECTINY_API_H__
#define __ECTINY_API_H__

#include "ECTypes.h"

#define ECSDK_VERSION         "1.0.10"

typedef struct {

    void      (*onECConnectStatus)(ECConnectedStatus status, ECConnectedType type);

    void      (*onMirrorStatus)(ECMirrorStatus status);
    /**
    * @brief  Called when EasyConnected status changed.
    *
    * @param  status  The changed EasyConnected message.
    */
    void      (*onECStatusMessage)(ECStatusMessage status);

    /**
    * @brief  Called when the phone app sends down HUD information.
    *
    * @param  data  HUD information.
    */
    void      (*onPhoneAppHUD)(const ECNavigationHudInfo *data);

    /**
     * @brief  Called when the phone app sends down HUD Road Junction Picture.
     * @param data
     */
    void      (*onPhoneAppHUDRoadJunctionPicture)(const ECHudRoadJunctionPictureInfo* data);

	/*
	* @brief  Called when phone app tell the music info.
	*
	* @param  data The information of music.
	*/
	void      (*onPhoneAppMusicInfo)(const ECAppMusicInfo *data);

    /**
    * @brief  Called when the phone app sends down some information.
    *
    * @param  data    Buffer of app information.
    *
    * @param  length  Buffer length.
    *
    * @note  data is json string, the fields includes os, osVersion and ip. 
	*        Called when ECSDK::openTransport succeed.		 
    */
    void        (*onPhoneAppInfo)(const void *data, uint32_t length);

    /**
    * @brief  Called when ECSDK wants car to do call operations(dial or hang up) via Bluetooth.
    *
    * @param  type    Operation type.
    *
	* @param  name    The person's name of corresponding number.
	*	
    * @param  number  Phone numbers.
    *
    * @note   Phone app is not able to dial or hang up automatically due to the latest system access limitation,
    *         however, car is able to do it via Bluetooth. Therefore, ECSDK moves the call operations
    *         to car, which can dial or hang up when this method is called.
    */
    void        (*onCallAction)(ECCallType type, const char *name, const char *number);
        
    /**
    * @brief  Called when bulk data is received.
    *
    * @param  data    Buffer of bulk data.
    *
    * @param  length  Buffer length.
    *
    */
    void        (*onBulkDataReceived)(const void *data, uint32_t length);


    /**
     * @brief onRealMirrorSizeChanged
     * @param realWidth
     * @param realHeight
     *
     * \note The actual size of the projection screen does not equal the size of the video stream in some cases.
     * The surrounding area is filled with black. This message calls back the actual size of the projection screen
     */
    void        (*onMirrorInfoChanged)(const ECVideoInfo *info);

    /**
    * @brief  Called when the license authorization failed. After this interface was called, 
    *         all connections would be forced closed.
    *
    * @param  errCode Error code.
    *
    * @param  errMsg Error message.
    */
    void        (*onLicenseAuthFail)(int32_t errCode, const char *errMsg);

	/**
	* @brief  Called when the license authorization succeed. 
	*
	* @param  code success code. The code can gain specific meaning by ECAuthSuccessCode.
	*
	* @param  msg success information.
	*
	* @param  msg the description information.
	*
	*/
	void		(*onLicenseAuthSuccess)(int32_t code, const char *msg);
 
	/**
	* @brief  Called when registered command was triggered by VR.
	* 
	* @param  carCmd The triggered command.
	*
	* @note   Voice control can be implemented with this method by VR.
	*
	* @see    ECSDK::registerCarCmds
	*/
	void        (*onCarCmdNotified)(const ECCarCmd *carCmd);

	/**
	 * @brief  Called when phone app request the HU to start input.
	 *
	 * @param info relevant parameters about the input.
	 */
	void        (*onInputStart)(const ECInputInfo *info);

    /**
     * @brief  Called when phone app request the HU to cancel input.
     */
	void        (*onInputCancel)();

    /**
     * @brief  Called when phone app tell the selection of input.
     */
	void        (*onInputSelection)(int32_t start, int32_t stop);

    /**
     * @brief  Called when phone app tell the text of input.
     */
    void        (*onInputText)(const char *text);    

    /**
     * @brief  Called when phone app send the text of VR or TTS.
     */
    void        (*onVRTextReceived)(const ECVRTextInfo *info);

    /**
     * @brief  Called when phone app tell the page list.     
     * 
     * @param  pages Array of the struct ECPageInfo.
     * 
     * @param  length The length of the array.       
     */
    void        (*onPageListReceived)(const ECPageInfo *pages, int32_t length);

    /**
     * @brief  Called when phone app tell the icons.   
     * 
     * @param  icons Array of the struct ECIconInfo.
     * 
     * @param  length The length of the array.
     */
    void        (*onPageIconReceived)(const ECIconInfo *icons, int32_t length);

    /**
     * @brief  Called when phone app tell weather.
     * 
     * @param  data Buffer of weather information.
     * 
     * @param  length Buffer length.
     * 
     * @note   data pointed to a json string buffer.
     */
    void        (*onWeatherReceived)(const char *data, int32_t length);

    /**
     * @brief  Called when phone app tell vr tips.
     * 
     * @param  data Buffer of tips information.
     * 
     * @param  length Buffer length.
     * 
     * @note   data pointed to a json string buffer.
     */
    void        (*onVRTipsReceived)(const char *data, int32_t length);

    /**
     * @brief  Called when the app requests networking
     *
     * @param  clientInfo Mobile phone related information
     *
     * @note
     */
    void        (*onRequestBuildNet)(const ECBTClientInfo *clientInfo);

    /**
     * @brief  Called when canceling networking
     *
     * @note
     */
    void        (*onRequestBuildNetCancel)();

    /**
     * @brief  Called when networking is completed
     *
     * @note
     */
    void        (*onPhoneBuildNetFinish)(const char* ip);

    /**
     * @brief  Called when app sends AP information
     *
     * @param  netDeviceInfo AP information
     *
     * @note
     */
    void        (*onPhoneAPInfo)(const ECBTNetInfo* netDeviceInfo);

    /**
     * @brief Called when mobile phone has a notification message.
     * @param notification
     */
    void        (*onPhoneNotification)(const ECPhoneNotification* notification);

    /**
     * @brief Called when the phone app sends down HUD lane guidance Picture.
     * @param notification
     */
    void        (*onPhoneAppHUDLaneGuidancePicture)(const ECHudLaneGuidancePictureInfo * data);

    /**
    * @brief  Called when checkOTAUpdate was called, it will tell the result of checkOTAUpdate.
    *
    * @param  downloadableSoftwares It pointer to a array of ECOTAUpdateSoftware, which is downloadable software.
    *
    * @param  downloadableLength  The length of the downloadable array, if downloadableLength < 0, means check occur error, downloadableLength is error value of ECOTAUpdateErrorCode.
    *
    * @param  downloadedSoftwares It pointer to a array of ECOTAUpdateSoftware, which is downloaded software.
    *
    * @param  downloadedLength  The length of the downloaded array.
    */
    void        (*onOTAUpdateCheckResult)(const ECOTAUpdateSoftware* downloadableSoftwares, const int32_t downloadableLength, const ECOTAUpdateSoftware* downloadedSoftwares, const uint32_t downloadedLength);

    /**
	* @brief  Called when remote downloadable software has been downloaded to phone.
	*
	* @param  downloadableSoftwares It pointer to a array of ECOTAUpdateSoftware, which has been in phone, can be downloaded from phone to HU.
	*
	* @param  downloadableLength  The length of the downloadable array.
 	*/
    void        (*onOTAUpdateRequestDownload)(const ECOTAUpdateSoftware* downloadableSoftwares, const uint32_t downloadableLength);

    /**
    * @brief Called when startOTAUpdate is called, it will notify the progress of downloading.
    *
    * @param downloadingSoftwareId The id of the downloading software.
    *
    * @param progress The progress of the downloading software,which is a percentage.
    *
    * @param softwareLeftTime The left time of the downloading software.
    *
    * @param otaLeftTime The left time of all the specified software by startOTAUpdate.
    */
    void        (*onOTAUpdateProgress)(const char* downloadingSoftwareId, float progress, uint32_t softwareLeftTime, uint32_t otaLeftTime);

    /**
    * @brief Called when startOTAUpdate is called, it will notify software is downloaded.
    *
    * @param downloadedSoftwareId The id of the downloaded software.
    *
    * @param md5Path The md5 file path.
    *
    * @param packagePath The software path.
    *
    * @param iconPath The icon path.
    *
    * @param leftSoftwareNum The amount of software remaining to be downloaded.
    */
    void        (*onOTAUpdateCompleted)(const char* downloadedSoftwareId, const char* md5Path, const char* packagePath, const char* iconPath, uint32_t leftSoftwareNum);

    /**
    * @brief  Called when checkOTAUpdate or startOTAUpdate failed.
    *
    * @param  errCode error code, see ECOTAUpdateErrorCode.
    *
    * @param  softwarId  the id of software.
    */
    void        (*onOTAUpdateError)(int32_t errCode, const char* softwareId);

} IECCallBack;

typedef struct
{
    uint32_t (*size)(const char* name);
    int32_t  (*read)(const char* name,void *data, uint32_t length, uint32_t offset);
    int32_t  (*write)(const char* name,void *data, uint32_t length, uint32_t offset);
    void     (*clear)(const char* name);
}IECAccessFile;

typedef struct 
{
    int32_t  (*open)();
    int32_t  (*read)(void *data, uint32_t length);
    int32_t  (*write)(void *data, uint32_t length);
    void     (*close)();
}IECAccessDevice;

typedef struct
{
    uint32_t    (*registHid)(uint32_t deviceId,uint32_t descriptorSize);
    void        (*unRegistHid)(uint32_t deviceId);
    uint32_t    (*sendHiDDescriptor)(uint32_t deviceId, const unsigned char* descriptor, uint32_t len);
    uint32_t    (*sendHidEvent)(uint32_t deviceId, const unsigned char* event, uint32_t len);
}IECHidAccessDev;

typedef struct
{
    int32_t (*open)();
    int32_t (*read)(uint8_t* data, uint32_t len);
    int32_t (*write)(const uint8_t* data, uint32_t len);
    int32_t (*close)();
}IECBTAccessDev;

typedef struct
{
    void    (*start)(int32_t width, int32_t height);
    void    (*stop)();
    void    (*play)(const void *data, uint32_t len);
}IECVideoPlayer;

typedef struct
{
    void    (*start)(ECAudioType type, const ECAudioInfo *info);
    void    (*stop)(ECAudioType type);
    void    (*play)(ECAudioType type, const void *data, uint32_t len);
    void    (*setVolume)(ECAudioType type, uint32_t vol);
}IECAudioPlayer;

typedef struct
{
    int32_t (*start)(const ECAudioInfo *info);
    void    (*stop)();
    int32_t (*record)(void *data, uint32_t len);
}IECAudioRecorder;

typedef void *ECConfigHandle;

ECConfigHandle  EC_createECConfig();
void    EC_destroyECConfig(ECConfigHandle config);
void    EC_setBaseConfig(ECConfigHandle config,const char *uuid, const char *version,const char *writableDir);
void    EC_setCommonConfig(ECConfigHandle config, const char *cfgName, const char *value);
void    EC_setCommonConfig1(ECConfigHandle config, const char *cfgName, int32_t value);

int32_t EC_initialize(ECConfigHandle config, IECCallBack *listener);
int32_t EC_release();
int32_t EC_setLogInfo(const ECLogLevel level,const ECLogOutputType type,const char *logDirectory, int32_t module);
const char* EC_getVersion();
int32_t EC_getVersionCode();
int32_t EC_start();
int32_t EC_stop();
int32_t EC_bindAccessFile(IECAccessFile *handle);
int32_t EC_bindUSBDevice(ECTransportType type, IECAccessDevice *dev);
int32_t EC_bindWIFIDevice(ECTransportType type, const char *ip);
int32_t EC_unbindDevice(ECTransportType type);
int32_t EC_bindHidDevice(IECHidAccessDev* dev);
int32_t EC_unBindHidDevice();
int32_t EC_bindBTDevice( IECAccessDevice* ioHandle);
int32_t EC_unBindBTDevice();
int32_t EC_setMirrorConfig(const ECMirrorConfig *mirrorCfg);
int32_t EC_setVideoPlayer(IECVideoPlayer* video);
int32_t EC_setAudioPlayer(IECAudioPlayer* audio);
int32_t EC_setAudioRecorder(IECAudioRecorder* audioRecorde);
int32_t EC_notifyWifiStateChanged(ECWifiStateAction action, const ECNetWorkInfo* netInfo);
int32_t EC_setRequestBuildNetRly(const char *phoneID, const ECBTRequestBuildNetRly *rly);
int32_t EC_setNetInterfaceInfo(const ECNetInterfaceInfo *info,const int32_t num);
int32_t EC_startMirror();
void    EC_stopMirror();
int32_t EC_pauseMirror();
int32_t EC_resumeMirror();    
int32_t EC_sendTouchEvent(const ECTouchEventData *touch, ECTouchEventType type);
int32_t EC_sendBtnEvent(int32_t btnCode, int32_t type);
int32_t EC_stopPhoneNavigation();
int32_t EC_stopPhoneVR();
int32_t EC_uploadNightModeStatus(uint32_t isNightModeOn);
int32_t EC_uploadDrivingStatus(ECDrivingStatus status);
int32_t EC_enableDownloadPhoneAppAudio(uint32_t supportType, uint32_t autoChangeToBT);
void    EC_disableDownloadPhoneAppAudio();
int32_t EC_enableDownloadPhoneAppHud(uint32_t supportFunction);
void    EC_disableDownloadPhoneAppHud();
int32_t EC_setConnectedBTAddress(const char *carBtMac, const char *phoneBtMac);
int32_t EC_sendCarBluetooth(const char *name, const char *adddress, const char *pin);
int32_t EC_openAppPage(int32_t page);
int32_t EC_queryGPS(uint32_t* status, ECGPSInfo* gps);
int32_t EC_queryTime(uint64_t *gmtTime, uint64_t *localTime, char *timeZone, uint32_t len, char* dateTime, uint32_t dateTimeLen);
int32_t	EC_sendCarStatus(ECCarStatusType carStatus, ECCarStatusValue value);
int32_t EC_registerCarCmds(const ECCarCmd *carCmds, uint32_t length);
int32_t EC_playCarTTS(const char *text, uint32_t level);
int32_t EC_registerSimilarSoundingWords(const char *data, uint32_t length);
int32_t EC_sendInputText(const char* text);
int32_t EC_sendInputAction(int32_t actionId, int32_t keyCode);
int32_t EC_sendInputSelection(int32_t start, int32_t stop);
int32_t EC_queryPageList();
int32_t EC_queryPageIcon(int32_t* pages, int32_t length);
int32_t EC_queryWeather();
int32_t EC_queryVRTips();
const char*  EC_generateQRCodeUrl(ECQRInfo* info);
int32_t EC_requestPhoneNotification(int32_t enable);

int32_t EC_checkOTAUpdate(const ECOTAConfig* cfg, const ECLanguage language, const ECOTAUpdateCheckMode mode);
int32_t EC_startOTAUpdate(const char** softwareIds, const int32_t softwareNum);
void    EC_stopOTAUpdate();

int32_t EC_startIperfTcpServer(const char* ip, int port);
void EC_stopIperfTcpServer();
#endif


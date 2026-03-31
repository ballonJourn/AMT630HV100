#ifndef __ECTINY_API_H__
#define __ECTINY_API_H__

#include "ECTypes.h"

#define ECSDK_VERSION         "1.0.15.1"

typedef struct {
    /**
     * @brief 互联连接状态回调函数
     * @param status 互联连接状态
     * @param type 互联连接类型
     * @note 此函数是非常重要的回调函数。它会返回整个互联的状态。
     *       有部分互联功能接口，比如：EC_startMirror()/EC_enableDownloadPhoneAppHud()/EC_enableDownloadPhoneAppHud() 等，
     *       都需要 status为 EC_CONNECT_STATUS_CONNECT_SUCCEED 时，调用才能生效。
     *       因为这些接口都是在互联成功之后(ECTiny与手机app建立了通讯)，才能发指令给手机app，对应功能才能开启。
     */
    void      (*onECConnectStatus)(ECConnectedStatus status, ECConnectedType type);

    /**
     * @brief 投屏状态
     * @param status 互联投屏状态
     */
    void      (*onMirrorStatus)(ECMirrorStatus status);

    /**
    * @brief  互联状态变更通知
    * @param  status  变更的状态
    */
    void      (*onECStatusMessage)(ECStatusMessage status);

    /**
    * @brief  手机下发HUD信息时回调
    * @param  data  HUD信息
    */
    void      (*onPhoneAppHUD)(const ECNavigationHudInfo *data);

    /**
     * @brief  手机下发道路引导图时回调
     * @param data
     */
    void      (*onPhoneAppHUDRoadJunctionPicture)(const ECHudRoadJunctionPictureInfo* data);

    /**
     * @brief 手机音乐信息变化时回调
     * @param data
     */
	void      (*onPhoneAppMusicInfo)(const ECAppMusicInfo *data);

    /**
    * @brief  手机下发app信息时回调
    * @param  data    app信息
    * @param  length  app信息长度
    * @note  data 是json字符串, 包括手机类型、系统版本、ip地址等。
	*        ECTiny 与 手机app建立通讯后会回调此函数
    */
    void        (*onPhoneAppInfo)(const void *data, uint32_t length);

    /**
    * @brief  需要拨打或者挂断蓝牙电话时回调
    * @param  type    操作类型
	* @param  name    电话拨打的姓名
    * @param  number  电话号码
    * @note   受限于Android/iOS系统的权限，在车机上点击互联投屏的过来的拨打电话，ECTiny无法完成电话的拨打和接听操作，因此需要依赖于车机系统的蓝牙模块完成。
    *         车机的投屏界面上点击拨打蓝牙电话时，此回调函数会把拨打的用户姓名和电话号码传到车机上，由车机蓝牙模块完成电话的拨打
    *         1.需要车机端作为蓝牙免提设备（Hand-Free-Device）。
    *         2.亿连的蓝牙电话功能，需要在车机和手机的HFP保持连接后，才可以正常启用。
    */
    void        (*onCallAction)(ECCallType type, const char *name, const char *number);
        
    /**
    * @brief  接收手机端发送的数据块
    * @param  data    Buffer of bulk data.
    * @param  length  Buffer length.
    */
    void        (*onBulkDataReceived)(const void *data, uint32_t length);

    /**
     * @brief 投屏状态变更
     * @param ECVideoInfo 视屏参数
     * @note 投屏的实际宽高、横竖屏切换时回调此函数
     */
    void        (*onMirrorInfoChanged)(const ECVideoInfo *info);

    /**
    * @brief  鉴权失败时回调此函数。ECTiny功能全部不可用
    * @param  errCode 主要用于亿连后台进行排查的错误码，无需关心具体值。
    * @param  errMsg 错误信息。
    * @note 此回调函数需要处理，当出现激活失败时给与提示。
    *       该回调一般出现在互联建立过程中，出现时ECTiny会在内部进行释放动作。
    *       ECTiny使用者需要保证合法的激活校验流程，之后重新初始化ECSDK方可使用。
    */
    void        (*onLicenseAuthFail)(int32_t errCode, const char *errMsg);

	/**
	* @brief  鉴权成时回调此函数
	* @param  code 正常错误码，无需关心具体值。
	* @param  msg 正常激活log信息。
	* @note 该接口回调一般出现在互联建立过程中，在首次进行激活（手机IME号备案激活）时，回调会包含剩余数量等信息，SDK使用者可根据具体的使用场景进行相关展示。
	*/
	void		(*onLicenseAuthSuccess)(int32_t code, const char *msg);
 
	/**
	* @brief  当注册的控车指令被vr激活时回调
	* @param  carCmd 触发的控车指令
	* @see    EC_registerCarCmds()
	*/
	void        (*onCarCmdNotified)(const ECCarCmd *carCmd);

	/**
	 * @brief  监听手机端文字输入
	 * @param info 当前需要输入的文字信息，包括输入类型（文字、数字、电话号码等），最大输入长度，默认的文字，最大行数，以及Enter键默认的显示效果及动作。
	 * @note 监听手机端输入法的开始输入状态，并传递此次输入文字的相关信息。
	 */
	void        (*onInputStart)(const ECInputInfo *info);

    /**
     * @note  监听手机端输入法的结束或者取消的状态，此时车机端键盘也需要取消。
     */
	void        (*onInputCancel)();

    /**
     * @brief  监听手机端输入光标位置、选择状态信息
     * @param start 光标开始的位置
     * @param stop 光标结束的位置
     * @note 监听手机端输入文字的选择状态，光标状态，车机端并做状态展示。仅对安卓手机有效，苹果手机互联无此回调。
     */
	void        (*onInputSelection)(int32_t start, int32_t stop);

    /**
     * @brief  同步当前手机端的文字信息到车机
     * @param text 当前手机端输入的文字信息。
     * @note 如在手机端也对输入框内的文字进行输入，需要同步至车机端，保持车机端和手机端的状态同步。
     */
    void        (*onInputText)(const char *text);    

    /**
     * @brief  对识别内容进行展示
     * @param info 手机端语音引擎识别后的文字内容信息。
     */
    void        (*onVRTextReceived)(const ECVRTextInfo *info);

    /**
     * @brief  获取快捷方式列表信息
     * @param  pages 返回快捷方式的数组，参考 ECTypes.h 的 ECPageInfo 定义，主要包含图标的编号、名称、icon的唯一标识信息；
     * @param  length pages数组长度；
     * @note ECPageInfo::page 字段标识了每一个快捷方式的唯一标识，通过该标识，可以实现两个主要的功能：
     *       1. 通过page编号，可以通过调用 ECSDK::queryPageIcon 获取快捷方式的图标资源。
     *       2. 通过page编号，可以通过调用 ECSDK::openAppPage 实现快捷打开对应指定手机APP页面
     */
    void        (*onPageListReceived)(const ECPageInfo *pages, int32_t length);

    /**
     * @brief  手机app回调图标信息
     * @param  icons 返回快捷方式图标资源的数组，参考 ECTypes.h 的 ECIconInfo 定义，主要包含icon的编号、icon的格式、icon图像数据、icon的数据长度；
     * @param  length icons的数组长度。
     */
    void        (*onPageIconReceived)(const ECIconInfo *icons, int32_t length);

    /**
     * @brief  回调天气信息
     * @param  data 天气信息字符串
     * @param  length 字符串长度
     * @note   data 是一个固定格式的json字符串
     */
    void        (*onWeatherReceived)(const char *data, int32_t length);

    /**
     * @brief  请求VR提醒文字
     * @param  data  VR文字
     * @param  length data长度
     * @note   data是一个json字符串
     *         在使用车机端本地语音助手时，一般需要有一些常驻提示类的使用帮助，这些文字主要通过手机端传输至车机端，由车机端系统完成展示。
     */
    void        (*onVRTipsReceived)(const char *data, int32_t length);

    /**
     * @brief  手机app发送到车机的请求组网
     * @param  clientInfo 手机app的相关信息
     * @note 此回调用于BLE组网
     */
    void        (*onRequestBuildNet)(const ECBTClientInfo *clientInfo);

    /**
     * @brief  手机app取消组网时回调
     * @note 此回调用于BLE组网
     */
    void        (*onRequestBuildNetCancel)();

    /**
     * @brief  手机app组网完成时回调
     * @note 此回调用于BLE组网
     */
    void        (*onPhoneBuildNetFinish)(const char* ip);

    /**
     * @brief  手机app通知车机，手机创建的ap信息
     * @param  netDeviceInfo AP信息
     * @note 此回调用于BLE组网
     */
    void        (*onPhoneAPInfo)(const ECBTNetInfo* netDeviceInfo);

    /**
     * @brief 收到手机消息通知时回调
     * @param notification 消息通知
     * @note 允许下发手机消息通知功能开启后，当收到短信、微信等消息时，此函数会回调消息到车机
     * @see  EC_requestPhoneNotification()
     */
    void        (*onPhoneNotification)(const ECPhoneNotification* notification);

    /**
     * @brief HUD道路引导图
     * @param notification 引导图信息
     * @note 下发HUD导航功能开启后，导航时，有道路引导图时，此函数会回调
     * @see EC_enableDownloadPhoneAppHud()
     */
    void        (*onPhoneAppHUDLaneGuidancePicture)(const ECHudLaneGuidancePictureInfo * data);

    /**
    * @brief  检测更新函数 EC_checkOTAUpdate()调用后, 此回调函数返回结果
    * @param  downloadableSoftwares 可下载的软件包数组
    * @param  downloadableLength  可下载的软件包数组大小, 如果 downloadableLength < 0 标识出现错误, 错误码参考：ECOTAUpdateErrorCode.
    * @param  downloadedSoftwares 已下载的软件包数组
    * @param  downloadedLength  已下载的软件包数组大小
    */
    void        (*onOTAUpdateCheckResult)(const ECOTAUpdateSoftware* downloadableSoftwares, const int32_t downloadableLength, const ECOTAUpdateSoftware* downloadedSoftwares, const uint32_t downloadedLength);

    /**
	* @brief  有软件包请求下载手机时回调
	* @param  downloadableSoftwares 可下载的软件包数组
	* @param  downloadableLength  可下载软件包数组大小
    * @note 软件包已存在手机，请求下载到车机
 	*/
    void        (*onOTAUpdateRequestDownload)(const ECOTAUpdateSoftware* downloadableSoftwares, const uint32_t downloadableLength);

    /**
    * @brief EC_startOTAUpdate()调用之后, 此回调函数会回调下载进度
    * @param downloadingSoftwareId 下载的软件ID
    * @param progress 下载的进度
    * @param softwareLeftTime 剩余下载时间
    * @param otaLeftTime OTA剩余时间
    */
    void        (*onOTAUpdateProgress)(const char* downloadingSoftwareId, float progress, uint32_t softwareLeftTime, uint32_t otaLeftTime);

    /**
    * @brief EC_startOTAUpdate()调用之后, 下载完成时回调此函数
    * @param downloadedSoftwareId 下载的软件ID
    * @param md5Path md5文件路径
    * @param packagePath 升级包路径
    * @param iconPath 图标路径
    * @param leftSoftwareNum 剩余下载数量
    */
    void        (*onOTAUpdateCompleted)(const char* downloadedSoftwareId, const char* md5Path, const char* packagePath, const char* iconPath, uint32_t leftSoftwareNum);

    /**
    * @brief  EC_checkOTAUpdate() 或者 EC_startOTAUpdate() 调用过程出错回调此函数
    * @param  errCode 错误码, 参考：ECOTAUpdateErrorCode.
    * @param  softwarId  软件id
    */
    void        (*onOTAUpdateError)(int32_t errCode, const char* softwareId);

    /**
    * @brief  手机通知车机page状态 
    * @param  appPageStatus  手机app页面信息
    */
    void        (*onAppPageStatus)(const ECAppPageStutus* appPageStatus);

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

typedef struct 
{
    uint32_t (*size)(const char *path,int64_t *mtime);  // get file size
    void *(*op)(const char *path);                      // Open file
    void (*cl)(void *fd);                                 // Close file
    uint32_t (*rd)(void *fd, void *buf, uint32_t len);        // Read file
    uint32_t (*wr)(void *fd, const void *buf, uint32_t len);  // Write file
    uint32_t (*sk)(void *fd, uint32_t offset);                // Set file position
    int (*rm)(const char *path);                         // Delete file
}IECHttpAccessFile;

/**
* @brief  http请求回调,所有回复都应该在此回调中完成
* @param  c 客户端发起的某个请求
* @param  url  http请求的url
* @param  method http请求的方式
* @param  requestData 请求的body数据
* @param  requestQury 请求的参数
* @param  filePath 请求访问某个文件，filePath返回文件所在服务器上的完整路径,使用strdup返回字符
* @return 返回1表示自定义回复http消息，返回0表示不处理
*/
typedef int (*ECHttpRequestCallBack)(void *c,const char *url,const char *requestData,const char *requestQury);

typedef void *ECConfigHandle;

ECConfigHandle  EC_createECConfig();
void    EC_destroyECConfig(ECConfigHandle config);
void    EC_setBaseConfig(ECConfigHandle config,const char *uuid, const char *version,const char *writableDir);
void    EC_setCommonConfig(ECConfigHandle config, const char *cfgName, const char *value);
/**
 * @brief 设置 ECTiny 连接的app版本
 * @param type 0:国内版；  1:海外版
 */
void    EC_setLinkPhoneApp(ECConfigHandle config,int32_t type);
void    EC_setCommonConfig1(ECConfigHandle config, const char *cfgName, int32_t value);
// This function must be called after the EC_start() function.
int32_t EC_resetECConfig(const char* config);

/**
 * @brief ECTiny 初始化函数
 * @param config 项目配置
 * @param listener 回调接口
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此接口为互联必调用的接口。
 *       1.除了调用 EC_setBaseConfig() 设置基本的运行配置之外，config 一般不需要额外配置。
 *       2.listener 的生命周期大于 ECTiny 生命周期。即 listener 在 EC_initialize() 调用前就需要初始化，EC_release()之后才可以释放。
 */
int32_t EC_initialize(ECConfigHandle config, IECCallBack *listener);

/**
 * @brief ECTiny 释放函数
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此函数调用后，后面声明的函数调用都会失效。
 *       没有互联时，ECTiny线程处于block或者waiting状态，占用系统资源不多，所以ECTiny的一般使用场景不需要调用此函数。
 */
int32_t EC_release();

/**
 * @brief 设置ECTiny日志
 * @param level 日志级别。调试时设置成：EC_LOG_LEVEL_ALL，生产时设置成：EC_LOG_LEVEL_ERROR或者其他高级别
 * @param type 日志输出类型，设置成 EC_LOG_OUT_STD标准输出。其余类型暂不支持。
 * @param logDirectory 日志保存位置，暂不支持
 * @param module 日志模块，通常设置成：EC_LOG_MODULE_SDK | EC_LOG_MODULE_APP
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此接口为互联必调用的接口。
 */
int32_t EC_setLogInfo(const ECLogLevel level,const ECLogOutputType type,const char *logDirectory, int32_t module);

/**
  * @return 获取 ECTiny 版本号
 */
const char* EC_getVersion();

/**
 * @return 获取 ECTiny 升级版本号
 */
int32_t EC_getVersionCode();

/**
 * @brief 开启互联服务
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此接口为互联必调用的接口。
 */
int32_t EC_start();

/**
 * @brief 停止互联服务
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 没有投屏的情况下， ECTiny线程出于阻塞状态，占用的系统资源很少。
 *       一般使用场景，调用 EC_start() 之后，不需要调用 EC_stop()。
 */
int32_t EC_stop();

/**
 * @brief 绑定http server服务
 * @param webRootDir httpServer根目录
 * @param cb http请求回调
 * @param handle http静态资源文件操作接口,可以为空，默认使用posix的文件操作接口
 */
void    EC_bindHttpServer(const char* webRootDir,ECHttpRequestCallBack cb,IECHttpAccessFile* handle);

/**
 * @brief 绑定文件读写设备
 * @param handle 文件操作接口
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 对于某些 RTOS 系统，没有标准的文件操作接口，因此需要实现此接口，把基本文件操作的接口传给 ECTiny。
 *       ECTiny会使用此接口读写license和OTA。
 */
int32_t EC_bindAccessFile(IECAccessFile *handle);

/**
 * @brief 绑定usb设备
 * @param type 互联类型
 * @param dev usb设备操作接口
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 绑定usb设备读写操作，用于实现usb互联。
 * @see EC_unbindDevice()
 */
int32_t EC_bindUSBDevice(ECTransportType type, IECAccessDevice *dev);

/**
 * @brief 绑定wifi互联ip
 * @param type 互联类型
 * @param ip 对端ip地址
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此函数调用之后， ECTiny会直接连接ip，进行互联。
 * @see EC_unbindDevice()
 */
int32_t EC_bindWIFIDevice(ECTransportType type, const char *ip);

/**
 * @brief 释放绑定的设备
 * @param type 互联类型
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此函数与 EC_bindUSBDevice()，EC_bindWIFIDevice()，有绑定与解绑定关系。
 */
int32_t EC_unbindDevice(ECTransportType type);

/**
 * @brief 绑定HID设备
 * @param dev HID设备
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @see EC_unBindHidDevice()
 */
int32_t EC_bindHidDevice(IECHidAccessDev* dev);

/**
 * @brief 解除HID设备绑定
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @see EC_bindHidDevice()
 */
int32_t EC_unBindHidDevice();

/**
 * @brief 绑定蓝牙ble设备操作接口
 * @param ioHandle 蓝牙ble设备操作接口
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此接口用于BLE组网方式的互联。
 * @see EC_unBindBTDevice()
 */
int32_t EC_bindBTDevice( IECAccessDevice* ioHandle);

/**
 * @brief 蓝牙ble设备解绑定
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @see EC_bindBTDevice()
 */
int32_t EC_unBindBTDevice();

/**
 * @brief 设置投屏参数
 * @param mirrorCfg 投屏参数
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此接口为互联必执行的接口。
 */
int32_t EC_setMirrorConfig(const ECMirrorConfig *mirrorCfg);

/**
 * @brief 设置解码显示设备
 * @param video 解码显示器
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 解码显示器 IECVideoPlayer 通过此接口注册给 ECTiny之后，由 ECTiny主动调用解码器的开始和停止。
 */
int32_t EC_setVideoPlayer(IECVideoPlayer* video);

/**
 * @brief 设置音频播放器
 * @param audio 音频播放器
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 声音数据走usb或者wifi时才需要设置此接口。一般项目声音都是走蓝牙a2dp，不需要设置此接口。
 */
int32_t EC_setAudioPlayer(IECAudioPlayer* audio);

/**
 * @brief 设置录音设备
 * @param audioRecorde 录音设备
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 需要手机app回声降噪的项目才需要设置此接口。
 */
int32_t EC_setAudioRecorder(IECAudioRecorder* audioRecorde);

/**
 * @brief 仪表wifi状态通知接口
 * @param action 仪表wifi模式：ap/sta。 目前此值不做要求，填写此枚举任意值即可
 * @param netInfo 网络状态。ECNetWorkInfo::state 必须填写，其余字段不需要填。
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 仪表/车机wifi发生变化时，通过此接口通知给 ECTiny。wifi连接，此接口必调用
 */
int32_t EC_notifyWifiStateChanged(ECWifiStateAction action, const ECNetWorkInfo* netInfo);

/**
 * @brief 请求组网接口
 * @param phoneID 手机id
 * @param rly 组网信息
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 只有BLE组网的连接方式，才用到此接口。
 */
int32_t EC_setRequestBuildNetRly(const char *phoneID, const ECBTRequestBuildNetRly *rly);

/**
 * @brief 仪表ip地址上报给手机
 * @param info 仪表网络信息
 * @param num 仪表网络信息数量
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 只有BLE组网的连接方式，才用到此接口。
 */
int32_t EC_setNetInterfaceInfo(const ECNetInterfaceInfo *info,const int32_t num);

/**
 * @brief 开始投屏
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此接口为互联必执行的接口。
 *       此接口调用后，手机app会把 H264 数据发送给 ECTiny。 ECTiny会自动调用 IECVideoPlayer 解码器解码显示。
 *       onECConnectStatus 回调函数，status==EC_CONNECT_STATUS_CONNECT_SUCCEED 时才可以调用此接口，否则无效。
 * @see EC_stopMirror()
 */
int32_t EC_startMirror();

/**
 * @brief 停止投屏
 * @note 此接口为互联必执行的接口。
 * @see EC_startMirror()
 */
void    EC_stopMirror();

/**
 * @brief 暂停/开启 投屏
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 目前不调用。调用 EC_startMirror()/EC_stopMirror()
 */
int32_t EC_pauseMirror();
int32_t EC_resumeMirror();

/**
 * @brief 发送触控消息
 * @param touch 触控数据
 * @param type 触控类型
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 发送仪表/车机屏幕的触控消息给手机app，互联程序会映射到手机的坐标，让手机app做出响应。
 */
int32_t EC_sendTouchEvent(const ECTouchEventData *touch, ECTouchEventType type);

/**
 * @brief 发送按键消息
 * @param btnCode 按键键值，取值于：ECBtnCode
 * @param type 按键类型，取值与：ECBtnEventType
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 发送按键消息给手机app，让手机作出响应。
 */
int32_t EC_sendBtnEvent(int32_t btnCode, int32_t type);

/**
 * @brief 结束手机导航
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 */
int32_t EC_stopPhoneNavigation();

/**
 * @brief 结束手机vr语音
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 */
int32_t EC_stopPhoneVR();

/**
 * @brief 上传汽车的夜间模式信息到手机
 * @param isNightModeOn 1:夜间模式; 0: 非夜间模式
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 手机app的显示模式设置为自动时，此接口才能让手机app响应
 */
int32_t EC_uploadNightModeStatus(uint32_t isNightModeOn);

/**
 * @brief 上传汽车的驾驶信息到手机
 * @param status 驾驶状态
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 */
int32_t EC_uploadDrivingStatus(ECDrivingStatus status);

/**
 * @brief 允许手机下发声音数据到仪表/车机
 * @param supportType 下载的声音类型，取值于 ECAudioType 类型，可以通过或运算下载多种声音
 * @param autoChangeToBT 是否启用蓝牙优先的策略，当蓝牙连接后，声音自动走蓝牙通道。
 *                       也可通过该接口控制手机端蓝牙连接提示框的弹出，当该参数设置为false后，手机端不再弹出连接蓝牙提示框。
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 只有在连接层成功建立后，接口调用才能成功。参考 onECConnectStatus()
 *       该接口调用之后，手机app才会下传相应声音，停止音频下传接口为EC_disableDownloadPhoneAppAudio()
 */
int32_t EC_enableDownloadPhoneAppAudio(uint32_t supportType, uint32_t autoChangeToBT);

/**
 * @brief 停止手机下发声音数据
 */
void    EC_disableDownloadPhoneAppAudio();

/**
 * @brief 允许手机下发HUD导航信息
 * @param supportFunction hud导航功能，取值于 ECAPPHUDSupportFunction 类型，可以通过或运算开启多种hud功能
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 只有在连接层成功建立后，接口调用才能成功。参考 onECConnectStatus()
 *       此接口调用一次即可生效
 */
int32_t EC_enableDownloadPhoneAppHud(uint32_t supportFunction);

/**
 * @brief 停止手机下发HUD导航信息
 */
void    EC_disableDownloadPhoneAppHud();

/**
 * @brief 设置连接的蓝牙
 * @param carBtMac 车机自己的蓝牙地址
 * @param phoneBtMac 车机连接的蓝牙Mac地址
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 * @note 此接口的目的是用于对比手机和车机两者连接的蓝牙是不是对方。受限于手机系统限制，无法取得手机的蓝牙物理地址，目前此接口功能已经失效。
 */
int32_t EC_setConnectedBTAddress(const char *carBtMac, const char *phoneBtMac);

/**
 * @brief 发送车机的蓝牙信息给手机
 * @param name 蓝牙名称
 * @param adddress 蓝牙地址
 * @param pin 蓝牙pin码
 * @return EC_OK 为成功，其余值为失败。此返回值一般不处理
 */
int32_t EC_sendCarBluetooth(const char *name, const char *adddress, const char *pin);

/**
 * @brief 打开手机界面
 * @param page 快速访问手机APP的相关界面或功能，如导航、音乐、对讲等，具体取值参考：ECAppPage
 * @return EC_OK 为成功，其余值为失败。
 * @note 该接口主要用于如下场景，譬如车机端系统桌面集成快捷按钮，可直达互联的指定页面。
 */
int32_t EC_openAppPage(int32_t page);

/**
 * @brief 查询GPS信息
 * @param status 当前的请求结果是否有效，0：无效；其余值有效
 * @param gps GPS信息
 * @return EC_OK 为成功，其余值为失败。
 * @note 该接口主要用于车机需要使用到手机端GPS的场景，请求网络当前位置。
 */
int32_t EC_queryGPS(uint32_t* status, ECGPSInfo* gps);

/**
 * @brief 查询手机时间
 * @param gmtTime 返回GMT(UTC)时间，单位毫秒。
 * @param localTime 返回当前时区的时间，单位毫秒
 * @param timeZone 返回当前的时区的字符串。
 * @param len 当前时区字符串的长度
 * @param dateTime 返回手机app当前时间字符串
 * @param dateTimeLen 手机app当前时间字符串长度
 * @return EC_OK 为成功，其余值为失败。
 * @note 由于GMT时间涉及到系统函数和时区的计算，建议直接使用后两个参数获取当前时间字符串，然后解析字符串获取到时间
 */
int32_t EC_queryTime(uint64_t *gmtTime, uint64_t *localTime, char *timeZone, uint32_t len, char* dateTime, uint32_t dateTimeLen);

/**
 * @brief 同步车机状态到手机
 * @param carStatus 车机状态类型
 * @param value 部分status需要携带状态值
 * @return EC_OK 为成功，其余值为失败。
 * @note 部分场景下，需要将车机的部分状态信息通知到手机APP，手机APP需要根据状态做互联的相关逻辑处理。
 *       如手机APP可根据行车状态，在不同的地区做不同的使用限制。该接口主要用于行车模式功能等功能。
 */
int32_t	EC_sendCarStatus(ECCarStatusType carStatus, ECCarStatusValue value);

/**
 * @brief 注册控车指令
 * @param carCmds 控车指令
 *            .type : 指令类型，是一个全局指令或是一个和页面绑定的指令，见枚举 ECCarCmdEFFECTType 。
 *            .id   : 指令id，每个需要识别的指令，都有一个唯一的标识。
 *            .cmd : 期望语音识别的指令，当时一个全局指令时，可以是正则表达式；如"(打开|开启)空调"，等价于"打开空调" ，"开启空调"；而当是一个和页面绑定的指令词，不支持正则表达式，仅支持明确的指令，如"放大地图"。
 *            .vrText : 当前成功匹配的语义，仅用于当 IECCallback::onCarCmdNotified 回调时，可根据匹配的内容处理相关的动作。
 *            .pauseMusic : 该参数仅用于 ECSDK::registerCarCmds ，告知手机如果触发语音识别时，是否需要暂停当前正在播放的音乐。
 *            .responser : 语音指令执行的结果是由车机端播报，还是由手机端播报。0 车机端播报，1，手机端播报。
 *            .thresholdLevel : 可选项，默认值为0，由手机端指定默认的门限值， 范围参考值 1~9999，识别门限值，值越大识别率越低，误唤醒率越低，开发者在使用语音识别功能时，可在项目中调试设置合适的值。该参数仅在 type= EC_CAR_CMD_TYPE_EFFECTIVE_PAGE 生效；
 * @param length 注册的指令列表的长度。
 * @return EC_OK 为成功，其余值为失败。
 * @note 当发起语音识别后，通过手机端的VR引擎识别后，会转化后具体的指令，通过回调接口 IECCallback::onCarCmdNotified 响应，须在此进行相关处理。
 */
int32_t EC_registerCarCmds(const ECCarCmd *carCmds, uint32_t length);

/**
 * @brief 播放指定的文字
 * @param text 文字信息
 * @param level 优先级，0~10，优先级越高，被播放的优先级也越高。
 * @return EC_OK 为成功，其余值为失败。
 * @note 与手机的通讯建立成功后，把需要播报的文字传输至手机APP。
 *       如果车机端配置了 ECSDK::enableDownloadPhoneAppAudio 通过车机端播放手机APP音频，此时音频将以TTS类型，通过USB/wifi传输车机端。
 */
int32_t EC_playCarTTS(const char *text, uint32_t level);

/**
 * @brief 注册近似指令
 * @param data 需要注册的近音词组，数据格式为JSON格式字符串
 *             {
 *                 words:
 *                     [
 *                         ["词1", ..., "词m"], ///< 数据类型为string数组，近音词集合，数组中的第一个词为显示词。
 *                         ...
 *                         ["词a", ..., "词n"] ///< 数据类型为string数组，近音词集合，数组中的第一个词为显示词。
 *                     ]
 *             }
 * @param length 注册的指令Json字符串的长度。
 * @return EC_OK 为成功，其余值为失败。
 * @note 该接口主要是对 EC_registerCarCmds() 接口的补充，常见使用场景提高语音控车指令的准确度，如“上身车窗”、“上升车窗”可以正确的被识别为同一个含义。
 */
int32_t EC_registerSimilarSoundingWords(const char *data, uint32_t length);

/**
 * @brief 发送给手机端当前已输入的文字
 * @param text 当前输入的车机端文字
 * @return EC_OK 为成功，其余值为失败。
 * @note 此函数用于车机键盘输入功能
 */
int32_t EC_sendInputText(const char* text);

/**
 * @brief 发送给手机端键盘按键事件
 * @param actionId 当前输入的动作
 * @param keyCode 键值
 * @return EC_OK 为成功，其余值为失败。
 * @note 如车机端输入法点击Enter键，此时需把事件发送给手机端。所有keycode及ActionId对安卓手机都有效；
 *       苹果手机仅响应actionid=0 keycode=4，苹果手机处理为隐藏手机端输入法，并把输入状态置为inActive状态。
 *       此函数用于车机键盘输入功能
 */
int32_t EC_sendInputAction(int32_t actionId, int32_t keyCode);

/**
 * @brief 发送给手机端当前光标位置及选中状态
 * @param start 光标开始的位置。
 * @param stop 光标结束的位置。
 * @return EC_OK 为成功，其余值为失败。
 * @note 如果车机端的光标产生变化，需把对应状态发送至手机端同步。仅对安卓手机有效，苹果手机互联无此回调。
 *       此函数用于车机键盘输入功能
 */
int32_t EC_sendInputSelection(int32_t start, int32_t stop);

/**
 * @brief 获取快捷方式列表信息
 * @return EC_OK 为成功，其余值为失败。
 * @note ECTiny与手机app传输建立之后。通过 IECCallback::onPageListReceived 回调接口完快捷方式列表信息的接收。
 */
int32_t EC_queryPageList();

/**
 * @brief 获取快捷方式图标资源
 * @param pages 待请求的page编号的数组，通过 EC_queryPageList 获取。
 * @param length page编号的数组长度。
 * @return EC_OK 为成功，其余值为失败。
 * @note ECTiny与手机app传输建立之后。通过 IECCallback::onPageIconReceived 回调接口完快捷方式图标资源的的接收。
 *       为了避免重复的请求，占用带宽资源，以及做到车机端的快捷方式快速显示；车机端系统需要做好缓存策略，对icon的资源信息做到增量更新：
 *       通过每次互联后 EC_queryPageList 获取最新的列表信息，然后比对车机端缓存的列表信息，仅对增量的资源进行请求更新；
 */
int32_t EC_queryPageIcon(int32_t* pages, int32_t length);

/**
 * @brief 查询天气
 * @return EC_OK 为成功，其余值为失败。
 * @note 查询结果通过 IECCallback::onWeatherReceived 回调函数返回
 */
int32_t EC_queryWeather();

/**
 * @brief 车机端语音提醒轮播
 * @return EC_OK 为成功，其余值为失败。
 * @note 在使用车机端本地语音助手时，一般需要有一些常驻提示类的使用帮助，这些文字主要通过手机端传输至车机端，由车机端系统完成展示。
 */
int32_t EC_queryVRTips();

/**
 * @brief 生成二维码url
 * @param info 二维码信息
 * @return EC_OK 为成功，其余值为失败。
 */
const char*  EC_generateQRCodeUrl(ECQRInfo* info);

/**
 * @brief 允许手机下发手机消息
 * @param enable 0：禁止； 1：允许
 * @return EC_OK 为成功，其余值为失败。
 */
int32_t EC_requestPhoneNotification(uint32_t enable);

/**
 * @brief 检测OTA升级
 * @param cfg  ota配置
 * @param language 语言
 * @param mode ota升级模式
 * @return EC_OK 为成功，其余值为失败。
 */
int32_t EC_checkOTAUpdate(const ECOTAConfig* cfg, const ECLanguage language, const ECOTAUpdateCheckMode mode);

/**
 * @brief 开始OTA升级
 * @param softwareIds 需要升级的软件id
 * @param softwareNum 需要升级的软件id数量
 * @return EC_OK 为成功，其余值为失败。
 */
int32_t EC_startOTAUpdate(const char** softwareIds, const int32_t softwareNum);

/**
 * @brief 停止OTA升级
 */
void    EC_stopOTAUpdate();

/**
 * @brief 回复http response
 * @param c 回复的某个请求连接
 * @param status http回复的状态码
 * @param headers 自定义的http回复headers
 * @param body 自定义的http回复body
 * @note 此接口必须在http请求回调中使用
 */
void    EC_httpReplyContent(void *c,HttpReplyStatus status, HttpReplyContentType type,
                   const char *body);

/**
 * @brief 开启iperf服务端
 * @param ip
 * @param port
 * @return
 * @note 简单实现的iperf功能。车机/仪表进行iperf测试时，建议使用标准iperf
 */
int32_t EC_startIperfTcpServer(const char* ip, int port);
void EC_stopIperfTcpServer();
#endif


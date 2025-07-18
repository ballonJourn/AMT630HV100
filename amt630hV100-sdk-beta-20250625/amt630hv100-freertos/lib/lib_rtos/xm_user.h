//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_user.h
//	  constant，macro & basic typedef definition of user
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_USER_H_
#define _XM_USER_H_

#include <xm_type.h>
#if defined (__cplusplus)
	extern "C"{
#endif

// Macro definition

// 定时器资源
#define	XM_MAX_TIMER				16			// 系统最大可分配定时器资源

#define	XM_MAX_MSG					32			// 系统消息队列大小

#define	MAX_HWND_STACK				8			// 视窗栈最大层次

#define	MAX_HWND_WIDGET_COUNT	4			// 一个窗口中最大允许定义的控件个数

#define	MAX_STACK_WIDGET_COUNT	8		// 视窗栈最大允许同时定义的控件个数 (所有视窗中的控件累加个数)



// 消息类型定义

#define	XM_KEYDOWN		0x01
										// wp 按键键值，lp 按键状态
										
#define	XM_KEYUP			0x02
										// wp 按键键值，lp 按键状态
										
#define	XM_QUIT			0x03	// 消息循环退出消息
										// wp保留为0, lp为程序设置的返回值
										
#define	XM_CHAR			0x04	// 字符消息
										// wp =  0, lp为16为Unicode16编码
										// wp != 0, 字符编码值为Unicode编码 (wp << 16) | lp,
										
#define	XM_TIMER			0x05	// 定时器消息
										// wp 定时器ID，lp 保留为0
										
#define	XM_PAINT			0x06	// 屏幕刷新消息
										// wp,lp保留为0
										
#define	XM_COMMAND		0x07	// 命令消息
										// wp为命令ID，lp与命令ID相关
										
#define	XM_ALARM			0x08	// 闹铃消息


#define	XM_CLOCK			0x09	// 每秒消息
										// wp,lp保留为0
										
#define	XM_MCI			0x0A	// 流媒体播放控制消息
										// wp MCI通知码
										// lp 与wp相关的附属信息

#define	XM_ENTER			0x0B	// 视窗进入
										// wp = 0, 新视窗进入 (该视窗未创建)
										//		lp  保留为0
										// wp = 1, 从其他视窗返回到该视窗 (该视窗已创建)
										//    lp  
										//		   bit 0     1  表示视窗重进入是因为分辨率修改
										//					    0  其他原因进入
										//			bit 1~7   0	 保留为0
										//

#define	XM_LEAVE			0x0C	// 视窗退出
										// lp保留为0
										// wp = 0, 视窗彻底退出 (该视窗被摧毁)
										// wp = 1, 视窗临时离开，如压入新的其他视窗 (该视窗当前状态保留)

#define	XM_SYSTEMEVENT	0x10	// 系统事件定义
										// lp 保留为0
										// wp 参考 XM_SYSTEMEVENT 的 WP 参数定义
										//		1)	一般情况下视窗view无需处理该事件，由系统缺省处理
										//		2)	若视窗view需要处理某个事件，处理完后需判断是否继续将事件传递给系统缺省处理。
										//			通过调用API XM_BreakSystemEventDefaultProcess 可终止系统缺省处理(即系统不再处理该事件)			

#define	XM_TOUCHDOWN	0x11	// 触摸事件按下
										//	lp	 
										//		bit 0  ~ 15		触摸点x坐标
										//		bit 16 ~ 31		触摸点y坐标
										// wp  保留为0

#define	XM_TOUCHUP		0x12	// 触摸事件释放
										//	lp	 保留为0
										// wp  保留为0

#define	XM_TOUCHMOVE	0x13	// 触摸事件移动
										//	lp	 
										//		bit 0  ~ 15		触摸点x坐标
										//		bit 16 ~ 31		触摸点y坐标
										// wp  保留为0
										
#define	XM_TOUCHREPEATED	0x14	// 触摸事件长按
										//	lp	 
										//		bit 0  ~ 15		触摸点x坐标
										//		bit 16 ~ 31		触摸点y坐标
										// wp  保留为0
										
// XM_SYSTEMEVENT 的 WP 参数定义

// 卡、文件系统事件
#define SYSTEM_EVENT_CARD_DETECT							0		// SD卡准备中(SD卡检测中)
																			//		"SD卡已插入，正在识别中"
#define SYSTEM_EVENT_CARD_UNPLUG							1		// SD卡拔出事件
																			//		"SD卡已移除"
#define SYSTEM_EVENT_CARD_INSERT_WRITE_PROTECT		2		// SD卡插入(写保护)
																			//		"SD卡已写保护，请将'写保护'开关关闭"
#define SYSTEM_EVENT_CARD_INSERT							3		// SD卡插入(读写允许)
#define SYSTEM_EVENT_CARD_FS_ERROR						4		// SD卡插入,数据卡已识别(读写模式)，卡文件系统已安装
																			//		"SD卡文件系统错误，请重新格式化"
																			//		无法在数据卡上创建保存视频记录的目录， 
																			//		存在与目录同名的文件时，将其重命名(自动修复)
																			//		无法自动修复时需要提示"格式化或者换卡"
#define SYSTEM_EVENT_CARD_VERIFY_ERROR					5		// SD卡读写校验检查失败，
																			//		"SD卡已损坏, 请尝试重新格式化"
#define SYSTEM_EVENT_CARD_INVALID						6		// SD卡无法识别 (检测到卡插入，但卡没有应答卡控的任何指令)
																			//		"SD卡无法识别，请重新插入"
#define SYSTEM_EVENT_CARD_DISK_FULL						7		// SD卡磁盘已满
																			//		"SD卡已写满，请尝试重新格式化"
#define SYSTEM_EVENT_CARD_FS_UNSUPPORT					8		// 无法支持的其他文件系统格式exFAT或者NTFS
																			//		"SD卡文件系统不支持，请重新格式化"	
		
// 显示外设接入、断开事件
#define SYSTEM_EVENT_AVOUT_PLUGOUT						20		// AVOUT拔出
#define SYSTEM_EVENT_AVOUT_PLUGIN						21		// AVOUT插入
#define SYSTEM_EVENT_HDMI_PLUGIN							22		// HDMI设备插入
#define SYSTEM_EVENT_HDMI_PLUGOUT						23		// HDMI设备拔出
#define SYSTEM_EVENT_BL_OFF								24		// 关闭背光
#define SYSTEM_EVENT_BL_ON									25		// 开启背光
		

// 快捷按键事件
#define SYSTEM_EVENT_ADJUST_BELL_VOLUME				30		// 快捷按键事件，调整铃声音量、开启、关闭
#define SYSTEM_EVENT_ADJUST_MIC_VOLUME					31		// 快捷按键事件，调整录音音量、开启、关闭
#define SYSTEM_EVENT_ONE_KEY_PROTECT					32		// 快捷按键事件，紧急录像(一键锁定)
#define SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH				33		// 快捷按键事件，一键拍照
#define SYSTEM_EVENT_URGENT_RECORD					32		// 快捷按键事件，紧急录像(一键锁定)
#define SYSTEM_EVENT_ONE_KEY_REBOOT					34		// 快捷按键事件，一键重启
		
// 系统电池事件
#define SYSTEM_EVENT_MAIN_BATTERY						40		// 主电池变化事件
#define SYSTEM_EVENT_BACKUP_BATTERY						41		// 备份电池变化事件

// USB拔插事件
#define SYSTEM_EVENT_USB_DISCONNECT						50		// USB断开连接
#define SYSTEM_EVENT_USB_CONNECT_CHARGE				51		// USB作为充电线使用
#define SYSTEM_EVENT_USB_CONNECT_UDISK					52		// USB已连接，作为其他设备的U盘使用
#define SYSTEM_EVENT_USB_CONNECT_CAMERA				53		// USB已连接，作为其他设备的Camera输入使用

// GPS北斗导航事件
#define SYSTEM_EVENT_GPSBD_DISCONNECT					60		// GPSBD已断开
#define SYSTEM_EVENT_GPSBD_CONNECT_ANTENNA_OPEN		61		// GPSBD已连接(天线未连接)
#define SYSTEM_EVENT_GPSBD_CONNECT_ANTENNA_SHORT	62		// GPSBD已连接(天线短路)
#define SYSTEM_EVENT_GPSBD_CONNECT_ANTENNA_OK		63		// GPSBD已连接(天线连接正常)
#define SYSTEM_EVENT_GPSBD_CONNECT_LOCATE_OK			64		// GPSBD已定位




// 视频项事件
#define SYSTEM_EVENT_VIDEOITEM_LOW_SPACE				80		// 视频项可循环空间低，可能是锁定项占用空间太多或者是其他格式的文件占用磁盘空间太多
																			//		这个事件在卡插入时、视频记录过程中发出。
																			//		"循环录像空间不足，请解锁视频或重新格式化"

#define SYSTEM_EVENT_VIDEOITEM_LOW_SPEED				81		// 视频项文件写入速度低(慢速卡、文件系统碎片过多均会导致)
																			//		这个事件在记录过程中发出。
																			//		当写入速度已无法满足实时记录时(存在严重丢帧现象)，提醒格式化SD卡	
																			//		"SD卡写入速度较慢，可能需要重新格式化"

#define SYSTEM_EVENT_VIDEOITEM_LOW_PREFORMANCE		82		// 文件系统的参数需要重新配置，确保满足视频记录的要求
																			//		一般指SD卡的簇大小太小，导致卡读写速度无法满足高清视频实时的写入
																			//		这个事件在卡插入时发出
																			//		即需要提醒格式化SD卡
																			//		"SD卡读写效率较低，可能需要重新格式化"
#define SYSTEM_EVENT_VIDEOITEM_ERROR					83		//	视频项数据库异常，需要格式化SD卡
																			//		这个事件在卡插入时发出

#define SYSTEM_EVENT_VIDEOITEM_RECYCLE_CONSUMED		84		//	视频项数据库异常，可循环使用的资源已耗尽，
																			//		即已锁定的视频文件总数量已达到系统定义的最大数量
																			//		需要手工删除视频或者格式化SD卡
																			//		这个事件在卡录制过程中发出
																			//		"锁定的录像文件已满, 录像已停止, 请删除文件或重新格式化"

#define SYSTEM_EVENT_VIDEOITEM_LOW_SPACE_ALARM		85		// 可循环录像时间低于指定的阈值报警
																			//		按照当前的录像视频平均波特率, SD卡可循环录像空间的时间低于定义的报警阈值
																			//		"可循环录像时间已低于报警阈值"

// 系统升级事件
#define SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED		90		// 找到合法的系统升级文件

#define SYSTEM_EVENT_SYSTEM_UPDATE_FILE_MISSED		91		// 无法找到系统升级文件。
																			//		当升级失败后(掉电异常)，下次启动时自动进入到系统升级模式。
																			//		若无法找到升级文件，投递该系统事件
#define SYSTEM_EVENT_SYSTEM_UPDATE_FILE_ILLEGAL		92		// 非法的系统升级文件
																			//		
#define SYSTEM_EVENT_SYSTEM_UPDATE_SUCCESS			93
#define SYSTEM_EVENT_SYSTEM_UPDATE_FAILURE			94


// 测速雷达报警事件
#define SYSTEM_EVENT_RADAR_ALARM							(100)		// 测速雷达信号报警


//#define SYSTEM_EVENT_RECORD_SPACE
// CCD断开事件
#define SYSTEM_EVENT_CCD0_LOST_CONNECT				110
#define SYSTEM_EVENT_CCD1_LOST_CONNECT				111
#define SYSTEM_EVENT_CCD2_LOST_CONNECT				112
#define SYSTEM_EVENT_CCD3_LOST_CONNECT				113
#define SYSTEM_EVENT_CCD4_LOST_CONNECT				114
#define SYSTEM_EVENT_CCD5_LOST_CONNECT				115
#define SYSTEM_EVENT_CCD6_LOST_CONNECT				116
#define SYSTEM_EVENT_CCD7_LOST_CONNECT				117
#define SYSTEM_EVENT_CCD8_LOST_CONNECT				118
#define SYSTEM_EVENT_CCD9_LOST_CONNECT				119
#define SYSTEM_EVENT_CCDA_LOST_CONNECT				120
#define SYSTEM_EVENT_CCDB_LOST_CONNECT				121
#define SYSTEM_EVENT_CCDC_LOST_CONNECT				122
#define SYSTEM_EVENT_CCDD_LOST_CONNECT				123
#define SYSTEM_EVENT_CCDE_LOST_CONNECT				124
#define SYSTEM_EVENT_CCDF_LOST_CONNECT				125

// CCD连接事件
#define SYSTEM_EVENT_CCD0_CONNECT					130
#define SYSTEM_EVENT_CCD1_CONNECT					131
#define SYSTEM_EVENT_CCD2_CONNECT					132
#define SYSTEM_EVENT_CCD3_CONNECT					133
#define SYSTEM_EVENT_CCD4_CONNECT					134
#define SYSTEM_EVENT_CCD5_CONNECT					135
#define SYSTEM_EVENT_CCD6_CONNECT					136
#define SYSTEM_EVENT_CCD7_CONNECT					137
#define SYSTEM_EVENT_CCD8_CONNECT					138
#define SYSTEM_EVENT_CCD9_CONNECT					139
#define SYSTEM_EVENT_CCDA_CONNECT					140
#define SYSTEM_EVENT_CCDB_CONNECT					141
#define SYSTEM_EVENT_CCDC_CONNECT					142
#define SYSTEM_EVENT_CCDD_CONNECT					143
#define SYSTEM_EVENT_CCDE_CONNECT					144
#define SYSTEM_EVENT_CCDF_CONNECT					145


#define	XM_BARCODE		0x11	// 条形码输入事件
										// wp, lp保留为0

#define	XM_USB			0x12	// USB输入事件
										// wp USB事件类型
										// lp 保留为0

#define	XM_VIDEOSTOP	0x20	// 视频播放结束消息
										// wp 退出码 (参考app_video.h定义)
// 视频播放消息(XM_VIDEOSTOP)的退出码(8bit)定义
#define	AP_VIDEOEXITCODE_FINISH			0x00		// 解码播放结束
#define	AP_VIDEOEXITCODE_LOWVOLTAGE	0x01		// 低压异常结束
#define	AP_VIDEOEXITCODE_STREAMERROR	0x02		// 流格式错误
#define	AP_VIDEOEXITCODE_OTHERERROR	0x03		// 其他异常(如SD卡异常、文件系统异常等)									

#define	XM_USER			0x80	// 用户自定义消息(0x80 ~ 0xFF)

// Window视窗标志定义
#define	HWND_DISPATCH	((BYTE)0x01)		// 消息派发中
#define	HWND_DIRTY		((BYTE)0x02)		// 标识窗口需要重绘(包括所有控件)
#define	HWND_ANIMATE	((BYTE)0x80)		// 标识视窗支持动画效果

// Widget控件标志定义
#define	WDGT_VISUAL		((BYTE)0x01)		// 标识控件处于可视状态
#define	WDGT_ENABLE		((BYTE)0x02)		// 标识控件处于使能状态
#define	WDGT_FOCUS		((BYTE)0x04)		// 标识控件具有可聚焦属性，即可被聚焦
#define	WDGT_SELECT		((BYTE)0x08)		// 标识控件处于选择状态 (CheckBox控件可具有该标志)
#define	WDGT_FOCUSED	((BYTE)0x10)		// 标识控件处于聚焦状态，即拥有输入焦点
#define	WDGT_DIRTY		((BYTE)0x20)		// 标识控件处于脏状态，需要重绘

#define	WIDGET_IS_FOCUS(flag)		((flag) & WDGT_FOCUS)
#define	WIDGET_IS_FOCUSED(flag)		((flag) & WDGT_FOCUSED)
#define	WIDGET_IS_SELECT(flag)		((flag) & WDGT_SELECT)
#define	WIDGET_IS_ENABLE(flag)		((flag) & WDGT_ENABLE)
#define	WIDGET_IS_VISUAL(flag)		((flag) & WDGT_VISUAL)
#define	WIDGET_IS_DIRTY(flag)		((flag) & WDGT_DIRTY)

#define	HWND_VIEW			0x01				// 视窗类型
#define	HWND_ALERT			0x02				// 弹窗类型
#define	HWND_EVENT			0x03				// 通知类型

#define	XM_VIEW_DEFAULT_ALPHA		255	// 视窗缺省透明度

// 缺省定制属性
#define	HWND_CUSTOM_DEFAULT			((DWORD)(0))

// 获取视窗回调函数
#define	XMPROC(hwnd)		HWND_##hwnd##_WindowProc

// 视窗回调函数声明
#define	XMPROC_DECLARE(hwnd)	\
extern void HWND_##hwnd##_WindowProc (XMMSG *msg);

// 视窗回调函数定义开始
#define	XM_MESSAGE_MAP_BEGIN(hwnd) \
void HWND_##hwnd##_WindowProc (XMMSG *msg) {

// 消息处理函数定义
#define	XM_ON_MESSAGE(event,proc) \
	if(msg->message == event) \
	{	\
		proc (msg); \
		return; \
	}

// 视窗回调函数定义结束
#define	XM_MESSAGE_MAP_END \
	XM_DefaultProc (msg);	\
}

// 视窗定义
#define	XMHWND_DEFINE(x,y,cx,cy,hwnd,erase,lpWidget,cbWidget,alpha,type) \
XMHWND hWnd_##hwnd## = {\
	x,y,cx,cy, \
	XMPROC(hwnd),	\
	lpWidget, cbWidget, \
	alpha, \
	erase, \
	type, \
	0, 0, 0, 0 \
};

#define	XMHWND_DECLARE(hwnd)		extern XMHWND hWnd_##hwnd##;

// 获取视窗回调函数
#define	XMWDGTPROC(hwnd)		WDGT_##hwnd##_WindowProc

// 视窗回调函数声明
#define	XMWDGTPROC_DECLARE(hwnd)	\
extern void WDGT_##hwnd##_WindowProc (const XMWDGT *pWidget, BYTE bWidgetFlag, void *pUserData, XMMSG *msg);

// 视窗回调函数定义开始
#define	WIDGET_MESSAGE_MAP_BEGIN(hwnd) \
void WDGT_##hwnd##_WindowProc (const XMWDGT *pWidget,BYTE bWidgetFlag,void *pUserData,XMMSG *msg) {

// 消息处理函数定义
#define	WIDGET_ON_MESSAGE(event,proc) \
	if(msg->message == event) \
	{	\
		proc (pWidget,bWidgetFlag,pUserData,msg); \
		return; \
	}

// 视窗回调函数定义结束
#define	WIDGET_MESSAGE_MAP_END \
}

// 消息传递控制选项
#define	XMMSG_OPTION_SYSTEMEVENT_DEFAULT_PROCESS		0x00000001		// 请求系统事件消息的缺省处理

// structure definition
// 消息结构定义
typedef struct tagXMMSG {
	WORD		message;			// 消息类型
	WORD		wp;				// 消息字节参数
	DWORD		lp;				// 消息字参数
	DWORD		option;			// 消息控制选项
} XMMSG;

// 视窗栈定义
typedef struct _HWND_NODE {
	HANDLE	hwnd;
	BYTE *	lpWidgetFlag;		// 视窗的子控件标志
	void **	UserData;			// 视窗子控件的用户数据
	void *	PrivateData;		// 窗口的私有数据
	BYTE		flag;					// 视窗的状态标志
	BYTE		cbWidget;			// 子控件个数

	BYTE		alpha;				// 视窗alpha因子


	UINT		animatingDirection;
	//xm_osd_framebuffer_t framebuffer;
} HWND_NODE;

// 获取视窗结构的句柄
#define	XMHWND_HANDLE(hwnd)						((HANDLE)(&(hWnd_##hwnd##)))

#define	ADDRESS_OF_HANDLE(handle)				((void *)(handle))

// typedef definition
typedef void (*XMWNDPROC)(XMMSG *);

typedef struct tagXMWND {
//	XMWNDPROC	lpfnWndProc;	// 控件消息回调函数
	XMCOORD		_x;				// 控件X坐标(绝对坐标)
	XMCOORD		_y;				// 控件X坐标(绝对坐标)
	XMCOORD		_cx;				// 控件宽度
	XMCOORD		_cy;				// 控件高度
} XMWND;

typedef struct tagXMWDGT {
//	XMWNDPROC	lpfnWndProc;	// 控件消息回调函数
	XMCOORD		_x;				// 控件X坐标(绝对坐标)
	XMCOORD		_y;				// 控件X坐标(绝对坐标)
	XMCOORD		_cx;				// 控件宽度
	XMCOORD		_cy;				// 控件高度
	
	DWORD			dwTitleID;		// 控件标题文本的资源ID.
										//		0 表示视窗定制，由视窗调用相应的控件文本设置函数完XM_XXXSetText
										//		(DWORD)(-1)表示无文字输出，如按钮为图片时

	WORD			wForBmpID;		// 前景图片ID, 0 表示无图片。在Button中对应于释放时的图片效果
	WORD			wBkgBmpID;		// 背景图片ID, 0 表示无图片。在Button中对应于按下时的图片效果
	WORD			wDisBmpID;		// 禁止状态图片ID, 0 表示无图片
	
	BYTE			bHotKey;			// 与控件对应的热键。0 表示无热键对应
	BYTE			bCommand;		// 控件命令，0 表示无命令。
										//		控件热键按下或聚焦状态下按下空格，控件将该命令投递到消息队列。
	BYTE			bFlag;			// 控件初始状态。见	Widget控件标志定义
	BYTE			bType;			// 子控件类型。通过控件类型获取控件的回调函数

} XMWDGT;


// 视窗结构定义 (32字节，Cache line aligned)
typedef struct tagXMHWND {
	XMCOORD			_x;				// 视窗X坐标(绝对坐标)		
	XMCOORD			_y;				// 视窗Y坐标(绝对坐标)
	XMCOORD			_cx;				// 视窗宽度
	XMCOORD			_cy;				// 视窗宽度

	XMWNDPROC		lpfnWndProc;	// 视窗消息回调函数

	

	const XMWDGT	*lpWidget;		// 视窗子控件列表
											//		控件定义在ROM中，减小RAM的需求，不可以修改
											//		但可以修改控件的标志(Flag), 见 Widget控件标志定义
	BYTE				cbWidget;		// 控件的个数
	BYTE				alpha;			// 透明因子Alpha，0 全透明 255 全覆盖
	BYTE				erase;			// 是否擦除背景, 0 表示不擦除背景 1 表示擦除背景
	BYTE				type;				// 窗口类型 (视窗、弹窗)

	XMCOORD			view_x;			// 视窗窗口的x坐标
	XMCOORD			view_y;			// 视窗窗口的y坐标
	XMCOORD			view_cx;			// 视窗窗口的宽度
	XMCOORD			view_cy;			// 视窗窗口的高度
	DWORD				scale_mode;		// 视窗缩放模式，
											//		0 --> 自动缩放模式(缺省属性)  
											//		1 --> 非缩放模式


} XMHWND, *PXMHWND;

// function protocol type

// 获取消息，当消息队列为空时，用户任务挂起
// 此函数仅允许内核使用，应用程序不允许使用。
// XMBOOL	XM_GetMessage (XMMSG *msg);

// 检索指定的消息。
// msg可为NULL，即删除指定的消息
// bMsgFilterMin = 0 && bMsgFilterMax = 0表示检索所有消息
// 成功返回TRUE，失败返回FALSE
XMBOOL 	XM_PeekMessage(XMMSG *msg,	BYTE bMsgFilterMin, BYTE bMsgFilterMax);

//
XMBOOL	XM_GetMessage (XMMSG *msg);

// 向消息队列投递消息
XMBOOL	XM_PostMessage (WORD message, WORD wp, DWORD lp);

// 直接调用视窗回调函数
// XMBOOL	XM_SendMessage (WORD message, WORD wp, DWORD lp);	// 已取消该函数

// 派发消息
XMBOOL	XM_DispatchMessage (XMMSG *msg);

// 终止系统事件消息的缺省处理
void		XM_BreakSystemEventDefaultProcess (XMMSG *msg);

// 投递消息循环结束消息
// wExitCode 定义
#define	XMEXIT_REBOOT					(1)	// 重启
#define	XMEXIT_SLEEP					(2)	// 关机
#define	XMEXIT_EXCEPT					(3)	// 异常
#define	XMEXIT_CHANGE_RESOLUTION	(3)	// 修改UI分辨率事件

XMBOOL	XM_PostQuitMessage (WORD wExitCode);

// 清除消息队列中的所有消息
void 		XM_FlushMessage (void);

// 删除队列中所有非COMMAND消息
void 		XM_FlushMessageExcludeCommandAndSystemEvent (void);

// 设置及获取私有数据
XMBOOL	XM_SetWindowPrivateData (HANDLE hWnd, void *PrivateData);
void *	XM_GetWindowPrivateData (HANDLE hWnd);

// 设置定时器（以ms为单位），idTimer为定时器ID
XMBOOL	XM_SetTimer		(BYTE idTimer , DWORD dwTimeout);

// 删除定时器
XMBOOL	XM_KillTimer	(BYTE idTimer);

// 系统缺省消息处理函数
void		XM_DefaultProc (XMMSG *msg);

// 系统事件消息缺省处理
// hWnd  当前视窗句柄
// msg	系统事件消息
void		XM_DefaultSystemEventProc (HANDLE hWnd, XMMSG *msg);

// 使整个窗口无效。系统投递XM_PAINT消息到消息队列尾部
XMBOOL	XM_InvalidateWindow (void);

// 使某个控件无效。系统投递XM_PAINT消息到消息队列尾部
XMBOOL XM_InvalidateWidget (BYTE bWidgetIndex);


// 系统检查消息队列中的XM_PAINT消息。若存在，将其移至消息队列首部，以便在下一个消息循环中XM_PAINT消息被处理
XMBOOL	XM_UpdateWindow (void);

// XM_PushWindow
// 在当前视窗上压入新的视窗。应用于需要保存其过程且可以逐级返回的情形。
// 执行Push操作后，消息队列被清空
// 桌面不需要压入到栈中。当栈为空时，自动将桌面压入到栈顶。
// 如字典应用中，检索初始界面-->检索结果界面-->字典正文界面-->正文属性界面，其视窗栈表示如下
//    栈顶-->          正文属性界面
//                     字典正文界面
//                     检索结果界面
//    栈底-->          检索初始界面
XMBOOL	XM_PushWindow	(HANDLE hWnd);

// 在压入新的视窗时，同时指定新窗口的定制信息
// 定制信息(dwCustomData)通过XM_ENTER消息的lp参数传入
XMBOOL	XM_PushWindowEx	(HANDLE hWnd, DWORD dwCustomData);


// XM_PullWindow
// 1) hWnd = 0, 弹出栈顶视窗,用于返回到前一个视窗。
//  如从 “正文属性界面” 返回到 “字典正文界面”
//
// 2) hWnd != 0, 循环弹出栈顶视窗，直到找到指定的视窗(hWnd)，并将该视窗置为栈顶视窗
// 如输出一个字符，导致从 “正文属性界面” 返回到 “检索初始界面”
XMBOOL	XM_PullWindow	(HANDLE hWnd);

// XM_JumpWindow
// 将视窗栈中所有视窗清除，然后跳到新的视窗。一般用于功能性跳转使用
// 如上面的视窗栈情况下，按下“系统时间”功能键，会直接调用XM_WndGoto，跳转到“系统时间”。
XMBOOL	XM_JumpWindow	(HANDLE hWnd);

// 跳转(JUMP)视窗弹出类型定义
typedef enum {
	XM_JUMP_POPDEFAULT = 0,			//	将视窗栈中所有视窗清除(除去桌面)，然后跳到新的视窗
	XM_JUMP_POPDESKTOP,				// 将视窗栈中所有视窗清除(包括桌面)，然后跳到新的视窗
	XM_JUMP_POPTOPVIEW,				// 将视窗栈的栈顶视窗清除，然后跳到新的视窗 
} XM_JUMP_TYPE;

// XM_JumpWindow
// 将视窗栈中栈顶视窗或所有视窗清除(取决于JumpType)，然后跳到新的视窗。一般用于功能性跳转使用
// 如上面的视窗栈情况下，按下“系统时间”功能键，会直接调用XM_WndGoto，跳转到“系统时间”。
XMBOOL	XM_JumpWindowEx	(HANDLE hWnd, DWORD dwCustomData, XM_JUMP_TYPE JumpType);

// XM_GetWindowID
// 获取当前视窗(即栈顶视窗)的唯一ID
// 当栈中存在多个同一视窗句柄的视窗时，可区分每个视窗。
BYTE		XM_GetWindowID	(void);

// 矩形缩放
XMBOOL XM_InflateRect (XMRECT *lprc, XMCOORD dx, XMCOORD dy);

// 矩形移动
XMBOOL XM_OffsetRect (XMRECT *lprc, XMCOORD dx, XMCOORD dy);

// 将窗口(视窗或控件)坐标转换为屏幕坐标
XMBOOL XM_ClientToScreen (HANDLE hWnd, XMPOINT *lpPoint);

// 将屏幕坐标转换为窗口(视窗或控件)坐标
XMBOOL XM_ScreenToClient (HANDLE hWnd, XMPOINT *lpPoint);

// 获取窗口(视窗或控件)的位置信息
XMBOOL XM_GetWindowRect (HANDLE hwnd, XMRECT* lpRect);

// 设置窗口实例的位置(相对显示区原点)
XMBOOL XM_SetWindowPos (HANDLE hWnd, 
								XMCOORD x, XMCOORD y,			// 屏幕坐标
								XMCOORD cx, XMCOORD cy
								);

// 获取桌面的视窗句柄
HANDLE XM_GetDesktop (void);

// 获取桌面的显示区域坐标
void XM_GetDesktopRect (XMRECT *lpRect);

XMBOOL XM_SetRect (XMRECT* lprc, XMCOORD xLeft, XMCOORD yTop, XMCOORD xRight, XMCOORD yBottom);

// 使能视窗创建、关闭时的动画效果
XMBOOL XM_EnableViewAnimate (HANDLE hWnd);
// 禁止视窗创建、关闭时的动画效果
XMBOOL XM_DisableViewAnimate (HANDLE hWnd);

// 设置当前聚焦控件
XMBOOL XM_SetFocus (BYTE bWidgetIndex);

// (BYTE)(-1)表示没有聚焦的控件
BYTE XM_GetFocus (void);

// 设置子控件的选择状态
XMBOOL XM_SetSelect (BYTE bWidgetIndex, XMBOOL bSelect);

// 设置子控件的使能状态
XMBOOL XM_SetEnable (BYTE bWidgetIndex, XMBOOL bEnable);

// 设置子控件的可视状态
XMBOOL XM_SetVisual (BYTE bWidgetIndex, XMBOOL bVisual);

// 获取控件的视窗子控件索引, 失败返回(BYTE)(-1)
BYTE XM_GetWidgetIndex (const XMWDGT *pWidget);

// 获取控件的用户数据
void *XM_GetWidgetUserData (BYTE bWidgetIndex);

// 设置控件的用户数据
XMBOOL XM_SetWidgetUserData (BYTE bWidgetIndex, void *pUserData);

// 获取当前视窗的句柄
HANDLE XM_GetWindow (void);

// 系统启动时由系统负责调用，只调用一次
void XM_AppInit (void);

// 系统启动时由系统负责调用，只调用一次
void XM_AppExit (void);

// 获取视窗关联的framebuffer对象
//	仅在XM_PAINT消息时视窗被自动关联一个framebuffer对象
//	其他消息处理过程中，若需要即刻刷新显示，执行以下流程
//	1)	创建一个与视窗关联的framebuffer对象，XM_osd_framebuffer_create
//	2)	设置framebuffer对象到视窗对象 XM_SetWindowFrameBuffer
// 3) 执行显示过程
// 4) 设置NULL framebuffer对象到视窗对象 XM_SetWindowFrameBuffer
//	5)	关闭framebuffer,将修改刷新到显示设备 XM_osd_framebuffer_close
//		
//		详细请参考 alert_view的CountDownPaint实现
//
//xm_osd_framebuffer_t XM_GetWindowFrameBuffer (HANDLE hWnd);

// 设置视窗关联的framebuffer对象
//		
//		详细请参考 alert_view的CountDownPaint实现
//
//XMBOOL XM_SetWindowFrameBuffer (HANDLE hWnd, xm_osd_framebuffer_t framebuffer);

// 获取视窗的全局Alpha因子
unsigned char XM_GetWindowAlpha (HANDLE hWnd);

// 设置视窗的全局Alpha因子
XMBOOL XM_SetWindowAlpha (HANDLE hWnd, unsigned char alpha);

// 设置视图切换时Animating效果方向
void XM_SetViewSwitchAnimatingDirection (UINT AnimatingDirection);

// ALERTVIEW视图的按键回调函数定义
// uKeyPressed 表示当前按下的物理按键键值
typedef void (*FPALERTCB) (void *UserPrivate, UINT uKeyPressed);

#define	XM_COMMAND_OK			0
#define	XM_COMMAND_CANCEL		1
typedef void (*FPOKCANCELCB) (UINT Command);

#define	XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW		0x00000001		// 弹出栈顶视窗
#define	XM_OKCANCEL_OPTION_SYSTEM_MODEL				0x00000002		// 系统模式, 忽略系统事件

// ALERTVIEW视图的控制选项
#define	XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN		0x00000001		// "倒计时显示"使能
#define	XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE		0x00000002		// "禁止按键操作"使能
#define	XM_ALERTVIEW_OPTION_ENABLE_CALLBACK			0x00000004		// "按键回调函数"使能
																						//		按钮按下时，调用按键回调函数
																						//		按钮个数可以为0或非0		
#define	XM_ALERTVIEW_OPTION_ADJUST_COUNTDOWN		0x00000008		// 使能倒计时计数调整


// 视窗显示对齐方式选项定义
#define	XM_VIEW_ALIGN_CENTRE			0x00000001			// 视窗居中对齐(相对显示区域)
#define	XM_VIEW_ALIGN_BOTTOM			0x00000002			// 视窗底部居中对齐(相对显示区域)

// 显示信息视图
// 0 视图参数错误，视图显示失败
// 1 视图创建并显示
XMBOOL XM_OpenAlertView ( 
								 DWORD dwInfoTextID,				// 信息文本资源ID
																		//		非0值，指定显示文字信息的资源ID
								 DWORD dwImageID,					// 图片信息资源ID
																		//		非0值，指定显示图片信息的资源ID	
								 DWORD dwButtonCount,			// 按钮个数
																		//		当超过一个按钮时，
																		//		第一个按钮，使用VK_F1(Menu)
																		//		第二个按键，使用VK_F2(Mode)
																		//		第三个按键，使用VK_F3(Switch)
								 DWORD dwButtonNormalTextID[],	//	按钮文字资源ID
																		//		0 
																		//			表示没有定义Button，Alert仅为一个信息提示窗口。
																		//			按任意键或定时器自动关闭
																		//		其他值
																		//			表示Button文字信息的资源ID，Alert需要显示一个按钮
								 DWORD dwButtonPressedTextID[],	//	按钮按下文字资源ID
																		//		0
																		//			表示没有定义该资源。
																		//			按钮按下时使用dwButtonNormalTextID资源
																		//		其他值
																		//			表示Button按下时文字信息的资源ID
								 DWORD dwBackgroundColor,		// 背景色
																		//		0
																		//			表示使用缺省背景色
																		//		其他值
																		//			表示有效填充背景色，背景色可指定Alpha分量
								 float fAutoCloseTime,			//	指定自动关闭时间 (秒单位)，
																		//		0.0	表示禁止自动关闭
								 float fViewAlpha,				// 信息视图的视窗Alpha因子，0.0 ~ 1.0
																		//		0.0	表示全透
																		//		1.0	表示全覆盖
								 
								 FPALERTCB alertcb,				// 按键用户回调函数
								 void *UserPrivate,				// 按键用户回调函数的私有参数

								 DWORD dwAlignOption,			//	视图的显示对齐设置信息
																		//		相对OSD显示的原点(OSD有效区域的左上角)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			视窗居中对齐(相对OSD显示区域)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			视窗底部居中对齐(相对OSD显示区域)
								 DWORD dwOption					// ALERTVIEW视图的控制选项
																		//		XM_ALERTVIEW_OPTION_ENABLE_COUNTDOWN
																		//			倒计时显示使能
																		//		XM_ALERTVIEW_OPTION_ENABLE_KEYDISABLE
																		//			"禁止按键操作"使能
								 );


// 打开铃声音量调节视窗
XMBOOL XM_OpenBellSoundVolumeSettingView (void);
// 打开MIC录音音量调节视窗
XMBOOL XM_OpenMicSoundVolumeSettingView (void);

int XM_Main (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XM_USER_H_

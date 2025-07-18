//****************************************************************************
//
//	Copyright (C) 2011 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_dev.h
//	  constant，macro & basic typedef definition of X-Mini System
//
//	Revision history
//
//		2011.05.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_DEV_H_
#define _XM_DEV_H_

#if defined (__cplusplus)
	extern "C"{
#endif


// Modifier属性定义
#define	XM_KEY_PRESSED			0x10		/* key pressed */
#define	XM_KEY_REPEAT			0x20		/* Key Repeat Status */
#define	XM_KEY_STROKE			0x40		/* indicate hardware stroke */
#define	XM_KEY_LONGTIME		0x80		/* long time pressed */

#define	XM_TP_TYPE_DOWN	1	// 触摸按下事件类型
#define	XM_TP_TYPE_UP		2	// 触摸释放事件类型
#define	XM_TP_TYPE_MOVE	3	// 触摸移动事件类型


// XM事件类型定义
#define	XM_EVENT_TYPE_KEY		1
#define	XM_EVENT_TYPE_TP		2

// XM按键事件定义
typedef struct {
	unsigned short		key;
	unsigned short		mod;
} XM_KEY_EVENT;

// XM触摸事件定义
typedef struct {
	unsigned short		x;
	unsigned short		y;
	unsigned int		type;
} XM_TP_EVENT;

typedef struct tag_XM_EVENT {
	unsigned int		event_type;
	union {
		XM_KEY_EVENT 	key_event;
		XM_TP_EVENT		tp_event;
	};
} XM_EVENT;

// 投递按键事件到事件消息队列
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
int XM_KeyEventProc (unsigned short key, unsigned short mod);



// 投递触摸事件到消息队列
// point		触摸位置
// type		触摸事件类型
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
int XM_TpEventProc (unsigned short x, unsigned short y, unsigned int type);

// 获取一个事件
//    delay_ms 超时时间，毫秒。
//       0表示不等待，0xffffffff表示永久等待
// 返回值
// 1 表示成功获取一个事件
// 0 表示获取事件失败
int XM_GetEvent (XM_EVENT *xm_event, unsigned int delay_ms);




#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_DEV_H_

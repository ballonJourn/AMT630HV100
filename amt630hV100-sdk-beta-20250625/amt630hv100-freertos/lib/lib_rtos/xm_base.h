//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_base.h
//	  constant，macro & basic typedef definition of kernel service
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_BASE_H_
#define _XM_BASE_H_

#include <xm_type.h>

#if defined (__cplusplus)
	extern "C"{
#endif


#ifndef _XMBOOL_DEFINED_
#define _XMBOOL_DEFINED_
typedef unsigned char		XMBOOL;		// BOOL类型
#endif


// typedef definition

// structure definition



// 系统时间
typedef struct tagXMSYSTEMTIME {
    WORD wYear; 
    WORD wMonth; 
    WORD wDayOfWeek; 
    WORD wDay; 
    WORD wHour; 
    WORD wMinute; 
    WORD wSecond; 
    WORD wMilliseconds; 
} XMSYSTEMTIME, *PXMSYSTEMTIME;


// 时间设置/读取函数
//		返回1表示系统时间已设置
//		返回0表示系统时间未设置，系统返回缺省定义时间
int	XM_GetLocalTime	(XMSYSTEMTIME* pSystemTime);
//		返回1设置系统时间成功
//    返回0设置系统时间失败
int	XM_SetLocalTime	(const XMSYSTEMTIME *pSystemTime);

// 获取机器启动后的滴答计数 （1ms为一滴答计数单位）
DWORD		XM_GetTickCount	(void);

// 获取机器启动后高精度的滴答计数，以微秒为计数单位
XMINT64 	XM_GetHighResolutionTickCount (void);


void		XM_Sleep (DWORD dwMilliseconds);		// 延时，毫秒单位



void XM_Delay (unsigned int ms);

void XM_lock (void);
void XM_unlock (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_BASE_H_

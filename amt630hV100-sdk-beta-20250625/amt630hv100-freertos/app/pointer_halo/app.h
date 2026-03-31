//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app.h
//	  D3视窗定义
//
//	Revision history
//
//		2010.09.06	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XSPACE_APP_H_
#define _XSPACE_APP_H_

#include <xm_user.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_assert.h>
#include <xm_printf.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined (__cplusplus)
	extern "C"{
#endif

// 窗口外部声明定义
// 定时器定义
#define	XMTIMER_DESKTOPVIEW					1		// 主界面(桌面)下的定时器, 用于光晕指针刷新
#define	XMTIMER_CIRCULAR_POINTER_VIEW		2		// 环状指针界面下的定时器, 用于环状指针刷新
#define	XMTIMER_CLOCK_VIEW					3		// 时钟界面下的定时器

// 桌面(光晕指针)视图
XMHWND_DECLARE(Desktop)

// 环状指针视图
XMHWND_DECLARE(CircularPointer)

// 时钟视图
XMHWND_DECLARE(ClockView)




void AppInit (void);
void AppExit (void);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XSPACE_APP_H_w

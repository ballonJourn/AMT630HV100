//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmtype.h
//	  constant，macro & basic typedef definition of X-Mini Window System
//
//	Revision history
//
//		2010.08.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_TYPE_H_
#define _XM_TYPE_H_


#if defined(_WINDOWS) && defined(WIN32)

// 以下用于VC编译器警告及错误处理
#pragma warning(disable : 4996)	// 编译器遇到了标记有 deprecated 的函数
#pragma warning(disable:4068) // 不显示4068号警告信息 (warning C4068: unknown pragma)
#pragma warning(disable:4100) // 不显示4100号警告信息 (unreferenced formal parameter)
#pragma warning(error:4090)   // 不允许 C4090: '=' : different 'const' qualifiers
//#pragma warning(error:4701)	// 不允许 C4701: local variable 'xxx' may be used without having been initialized
#pragma warning(error:4706)	// 不允许 C4706: assignment within conditional expression
#pragma warning(error:4716)	// 不允许 C4716: must return a value
#pragma warning(error:4013)	// 不允许 C4013: undefined; assuming extern returning int
#pragma warning(error:4028)	// formal parameter nnn different from declaration
#pragma warning(error:4245)	// 'function' : conversion from 'const int ' to 'unsigned short ', signed/unsigned mismatch
#pragma warning(error:4020)	// 不允许 C4020: 'xxx' : too many actual parameters
#pragma warning(error:4244)	// 不允许 C4244: warning C4244: '=' : conversion from 'short ' to 'unsigned char ', possible loss of data
#pragma warning(disable:4793)
#pragma warning(disable : 4996)

#endif	// #if defined(_WINDOWS) && defined(WIN32)


#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

typedef signed char			i8_t;
typedef signed short			i16_t;
typedef signed int			i32_t;
typedef unsigned char		u8_t;
typedef unsigned short		u16_t;
typedef unsigned int		u32_t;
typedef unsigned short	WORD;
typedef unsigned long		DWORD;
typedef unsigned char		BYTE;
typedef unsigned int		UINT;

#ifdef WIN32
typedef __int64				XMINT64;
#else
typedef signed long long	XMINT64;
#endif




#ifndef _XMCOORD_DEFINED_
#define _XMCOORD_DEFINED_
typedef signed int			XMCOORD;		// 位置类型
#endif

#ifndef _XMCOLOR_DEFINED_
#define _XMCOLOR_DEFINED_
typedef unsigned long		XMCOLOR;		// 颜色类型
#endif

#ifndef _XMBOOL_DEFINED_
#define _XMBOOL_DEFINED_
typedef unsigned char		XMBOOL;		// BOOL类型
#endif

typedef void *					HANDLE;

// 类型定义
#undef NULL
#define NULL    				(0)

/*
#ifndef _WCHAR_T_DEFINED
#undef wchar_t
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif*/

#ifndef _WINDEF_

// 宏定义
#define MAKEWORD(a, b)     ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)     ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)          ((WORD)(l))
#define HIWORD(l)          ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)          ((BYTE)(w))
#define HIBYTE(w)          ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#endif

#define XMPALETTEINDEX(i)   ((XMCOLOR)(0x01000000 | (DWORD)(WORD)(i)))

#define XM_MAX_PATH			127			/* maximum path */


// 数据结构定义
typedef struct tgXMPOINT {
	XMCOORD x;	// x坐标
	XMCOORD y;	// y坐标
} XMPOINT;

// 矩形结构
typedef struct tagXMRECT {
	XMCOORD left;		// 矩形左上角X坐标
	XMCOORD top;		// 矩形左上角Y坐标
	XMCOORD right;		// 矩形右下角X坐标
	XMCOORD bottom;	// 矩形右下角Y坐标
} XMRECT;

// 尺寸结构
typedef struct tagXMSIZE {
    XMCOORD cx;
    XMCOORD cy;
} XMSIZE;


#endif	// _XM_TYPE_H_

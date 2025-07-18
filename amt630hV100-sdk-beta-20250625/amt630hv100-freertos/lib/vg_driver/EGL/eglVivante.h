/****************************************************************************
*
*    Copyright (c) 2005 - 2010 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************
*
*    Auto-generated file on 4/22/2010. Do not edit!!!
*
*****************************************************************************/




/*
 * Vivante specific definitions and declarations for EGL library.
 */

#ifndef __eglvivante_h_
#define __eglvivante_h_

/*
    USE VDK

    This define enables the VDK linkage for EGL.
*/
#ifndef USE_VDK
# define USE_VDK	1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define EGLAPIENTRY

#if USE_VDK
#if defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#include <windows.h>
#endif
/* VDK platform independent. */
#include <vdkTypes.h>
typedef vdkDisplay	EGLNativeDisplayType;
typedef vdkWindow	EGLNativeWindowType;
typedef vdkPixmap	EGLNativePixmapType;

#elif defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
/* Win32 and Windows CE platforms. */
#include <windows.h>
typedef HDC				EGLNativeDisplayType;
typedef HWND		    EGLNativeWindowType;
typedef HBITMAP		    EGLNativePixmapType;

#elif defined(LINUX) && defined(EGL_API_FB)
/* Linux platform for FBDEV. */
typedef struct _FBDisplay * EGLNativeDisplayType;
typedef struct _FBWindow *  EGLNativeWindowType;
typedef struct _FBPixmap *  EGLNativePixmapType;

EGLNativeDisplayType
fbGetDisplay(
	void
	);

void
fbGetDisplayGeometry(
	EGLNativeDisplayType Display,
	int * Width,
	int * Height
	);

void
fbDestroyDisplay(
	EGLNativeDisplayType Display
	);

EGLNativeWindowType
fbCreateWindow(
    EGLNativeDisplayType Display,
	int X,
	int Y,
	int Width,
	int Height
	);

void
fbGetWindowGeometry(
    EGLNativeWindowType Window,
	int * X,
	int * Y,
	int * Width,
	int * Height
	);

void
fbDestroyWindow(
	EGLNativeWindowType Window
	);

EGLNativePixmapType
fbCreatePixmap(
	EGLNativeDisplayType Display,
	int Width,
	int Height
	);

void
fbGetPixmapGeometry(
    EGLNativePixmapType Pixmap,
    int * Width,
    int * Height
    );

void
fbDestroyPixmap(
	EGLNativePixmapType Pixmap
	);

#elif defined(EGL_API_ANDROID)

#if defined(ANDROID_VERSION_ECLAIR)

#include <ui/egl/android_natives.h>

typedef struct android_native_window_t* 	EGLNativeWindowType;
typedef struct egl_native_pixmap_t*			EGLNativePixmapType;
typedef void*								EGLNativeDisplayType;

#else
#include <EGL/eglnatives.h>

typedef struct egl_native_window_t*		EGLNativeWindowType;
typedef struct egl_native_pixmap_t*		EGLNativePixmapType;
typedef void*							EGLNativeDisplayType;
#endif

#else
/* X11 platform. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef Display *	EGLNativeDisplayType;
typedef Window		EGLNativeWindowType;

#ifdef CUSTOM_PIXMAP
typedef void *		EGLNativePixmapType;
#else
typedef Pixmap		EGLNativePixmapType;
#endif /* CUSTOM_PIXMAP */

#endif

#ifdef __EGL_EXPORTS
# if defined(_WIN32) && !defined(__SCITECH_SNAP__)
#  define EGLAPI	__declspec(dllexport)
# else
#  define EGLAPI
# endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* __eglvivante_h_ */


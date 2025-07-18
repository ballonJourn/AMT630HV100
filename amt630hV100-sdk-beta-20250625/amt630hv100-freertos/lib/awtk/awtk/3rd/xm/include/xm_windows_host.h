//****************************************************************************
//
//	Copyright (C) 2021 ShenZhen ExceedSpace 
//
//	Author	ZhuoYongHong
//
//	File name: xm_windows_host.h
//	  constant£¬macro & basic typedef definition of X-Mini System
//
//	Revision history
//
//		2021.04.11	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef XM_WINDOWS_HOST_H
#define XM_WINDOWS_HOST_H


#if defined (__cplusplus)
	extern "C"{
#endif

extern void * XM_WinHost_WindowCreate(const char* title,
                              const unsigned int width,
                              const unsigned int height);
                              
extern void XM_WinHost_WindowDestroy(void);

//extern void XM_WinHost_WindowBuffersSwap(native_window_xm_t *native_window);

extern void XM_WinHost_WindowBuffersSwap(char *surface_pixels, unsigned int width, unsigned int height, unsigned int bpp );

extern void XM_GetWidowSize(int *w, int *h);

                              
#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif /* xm_windows_host_h */



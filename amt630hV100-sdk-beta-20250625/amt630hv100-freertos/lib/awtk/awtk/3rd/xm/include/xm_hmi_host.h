//****************************************************************************
//
//	Copyright (C) 2021 ShenZhen ExceedSpace 
//
//	Author	ZhuoYongHong
//
//	File name: xm_hmi_host.h
//	  constant£¬macro & basic typedef definition of X-Mini System
//
//	Revision history
//
//		2021.04.11	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef XM_HMI_HOST_H
#define XM_HMI_HOST_H


#if defined (__cplusplus)
	extern "C"{
#endif

extern void * XM_HmiHost_WindowCreate(const char* title,
                              const unsigned int width,
                              const unsigned int height);
                              
extern void XM_HmiHost_WindowDestroy(void);


extern void XM_HmiHost_WindowBuffersSwap(char *surface_pixels, unsigned int width, unsigned int height, unsigned int bpp );

extern void XM_GetWidowSize(int *w, int *h);

                              
#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif /* xm_hmi_host_h */



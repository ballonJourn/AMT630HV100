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
 * Vivante specific definitions and declarations for all API library.
 */
#ifndef __khrvivante_h_

#include "gcHalUser.h"

/* EGL image type enum. */
typedef enum _khrIMAGE_TYPE
{
	KHR_IMAGE_TEXTURE_2D			= 1,
	KHR_IMAGE_TEXTURE_CUBE,
	KHR_IMAGE_TEXTURE_3D,
	KHR_IMAGE_RENDER_BUFFER,
	KHR_IMAGE_VG_IMAGE,
	KHR_IMAGE_PIXMAP,
#ifdef EGL_API_ANDROID
	KHR_IMAGE_ANDROID_NATIVE_BUFFER,
#endif
} khrIMAGE_TYPE;

#define KHR_EGL_IMAGE_MAGIC_NUM		gcmCC('I','M','A','G')

/* EGL Image */
typedef struct _khrEGL_IMAGE
{
	gctUINT						magic;
	khrIMAGE_TYPE				type;
	gcoSURF						surface;
	union
	{
		struct
		{
			gctUINT				width;
			gctUINT				height;

			/* Format defined in GLES. */
			gctUINT				format;

			gctINT				level;
			gctINT				face;
			gctINT				depth;

			/* Address offset in surface, for cubemap. */
			gctUINT32			offset;

			gctINT				texture;
			gctPOINTER			ojbect;
		} texture;

		struct
		{
			gctUINT				width;
			gctUINT				height;
			gceSURF_FORMAT		format;
			gctINT				stride;

			gctPOINTER			address;
		} pixmap;

		struct
		{
			gctUINT				width;
			gctUINT				height;
			gctUINT				offset_x;
			gctUINT				offset_y;

			gctUINT				format;
			gctUINT				allowedQuality;
		} vgimage;

#ifdef EGL_API_ANDROID
		struct
		{
			gctUINT				width;
			gctUINT				height;
			gceSURF_FORMAT		format;
			gctINT				stride;
			gctPOINTER			address;

			gctPOINTER			native;
		} androidNativeBuffer;
#endif
	} u;
}
khrEGL_IMAGE;

typedef khrEGL_IMAGE * khrEGL_IMAGE_PTR;

#define __khrvivante_h_
#endif


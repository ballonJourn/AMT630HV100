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






#ifndef __gcsdk_h_
#define __gcsdk_h_

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************\
***************************** Include all headers. ****************************
\******************************************************************************/

#ifdef LINUX
#	include <signal.h>
#	include <unistd.h>
#	include <string.h>
#	include <time.h>
#	include <sys/time.h>
#else
#	include <windows.h>
#	include <tchar.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gcHal.h>
#include <gcHAL_2D.h>

/******************************************************************************\
********************************* Definitions. ********************************
\******************************************************************************/

#define gcvLEFTBUTTON	(1 << 0)
#define gcvMIDDLEBUTTON	(1 << 1)
#define gcvRIGHTBUTTON	(1 << 2)
#define gcvCTRL			(1 << 3)
#define gcvALT			(1 << 4)
#define gcvSHIFT		(1 << 5)


/******************************************************************************\
****************************** Type declarations. *****************************
\******************************************************************************/

/*
	Window timer event function type.
*/
typedef gctBOOL (* gctTIMERFUNC) (
	gctPOINTER Argument
	);

/*
	Keyboard key press event function type.
*/
typedef gctBOOL (* gctKEYBOARDEVENT) (
	gctPOINTER Argument,
	gctUINT State,
	gctUINT Code
	);

/*
	Mouse button press event function type.
*/
typedef gctBOOL (* gctMOUSEPRESSEVENT) (
	gctPOINTER Argument,
	gctINT X,
	gctINT Y,
	gctUINT State,
	gctUINT Code
	);

/*
	Mouse move event function type.
*/
typedef gctBOOL (* gctMOUSEMOVEEVENT) (
	gctPOINTER Argument,
	gctINT X,
	gctINT Y,
	gctUINT State
	);

/*
	Thread routine.
*/
typedef void (* gctTHREADROUTINE) (
	gctPOINTER Argument
	);

/*
	Time structure.
*/
struct _gcsTIME
{
	gctINT year;
	gctINT month;
	gctINT day;
	gctINT weekday;
	gctINT hour;
	gctINT minute;
	gctINT second;
};

typedef struct _gcsTIME * gcsTIME_PTR;


/******************************************************************************\
******************************* API declarations. *****************************
\******************************************************************************/

int
vdkAppEntry(
	void
	);

void
vdkGetCurrentTime(
	gcsTIME_PTR CurrTime
	);


/******************************************************************************\
********************************** Visual API. ********************************
\******************************************************************************/

gctBOOL
vdkInitVisual(
	void
	);

void
vdkCleanupVisual(
	void
	);

void
vdkGetDesktopSize(
	IN OUT gctUINT* Width,
	IN OUT gctUINT* Height
	);

gctBOOL
vdkCreateMainWindow(
	IN gctINT X,
	IN gctINT Y,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctSTRING Title,
	IN gctPOINTER EventArgument,
	IN gctKEYBOARDEVENT KeyboardEvent,
	IN gctMOUSEPRESSEVENT MousePressEvent,
	IN gctMOUSEMOVEEVENT MouseMoveEvent,
	IN gctTIMERFUNC TimerEvent,
	IN gctUINT TimerDelay
	);

void
vdkDestroyMainWindow(
	void
	);

gctHANDLE
vdkGetDisplayContext(
	void
	);

void
vdkReleaseDisplayContext(
	gctHANDLE Context
	);

gctHANDLE
vdkGetMainWindowHandle(
	void
	);

void
vdkGetMainWindowSize(
	IN OUT gctUINT* Width,
	IN OUT gctUINT* Height
	);

void
vdkGetMainWindowColorBits(
	IN OUT gctUINT* RedCount,
	IN OUT gctUINT* GreenCount,
	IN OUT gctUINT* BlueCount
	);

void
vdkSetMainWindowPostTitle(
	gctSTRING PostTitle
	);

void
vdkSetMainWindowImage(
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctUINT AlignedWidth,
	IN gctUINT AlignedHeight,
	IN gctPOINTER Image
	);

int
vdkEnterMainWindowLoop(
	void
	);


/******************************************************************************\
*********************** Threading and Synchronization API. ********************
\******************************************************************************/

void
vdkSleep(
	gctUINT Milliseconds
	);

gctHANDLE
vdkCreateThread(
	gctTHREADROUTINE ThreadRoutine,
	gctPOINTER Argument
	);

void
vdkCloseThread(
	gctHANDLE ThreadHandle
	);

gctHANDLE
vdkCreateEvent(
	gctBOOL ManualReset,
	gctBOOL InitialState
	);

void
vdkCloseEvent(
	gctHANDLE EventHandle
	);

void
vdkSetEvent(
	gctHANDLE EventHandle
	);

void
vdkResetEvent(
	gctHANDLE EventHandle
	);

void
vdkWaitForEvent(
	gctHANDLE EventHandle,
	gctUINT Timeout
	);

gctHANDLE
vdkCreateMutex(
	void
	);

void
vdkCloseMutex(
	gctHANDLE MutexHandle
	);

void
vdkAcquireMutex(
	gctHANDLE MutexHandle,
	gctUINT Timeout
	);

void
vdkReleaseMutex(
	gctHANDLE MutexHandle
	);


#ifdef __cplusplus
}
#endif

#endif


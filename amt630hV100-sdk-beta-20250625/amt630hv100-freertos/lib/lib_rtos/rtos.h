//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: rtos.h
//	  constant£¬macro, data structure£¬function protocol definition of lowlevel rtos interface 
//
//	Revision history
//
//		2011.09.08	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_RTOS_H_
#define _XM_RTOS_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#include <string.h>         // required for memset() etc.
#include <intrinsics.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "task.h"


#ifndef   OS_I8
  #define OS_I8 signed char
#endif

#ifndef   OS_U8
  #define OS_U8 unsigned char
#endif

#ifndef   OS_I16
  #define OS_I16 signed short
#endif

#ifndef   OS_U16
  #define OS_U16 unsigned short
#endif

#ifndef   OS_I32
  #define OS_I32 long
#endif

#ifndef   OS_U32
  #define OS_U32 unsigned OS_I32
#endif

#ifndef   OS_INT
  #define OS_INT       int
#endif

#ifndef   OS_UINT
  #define OS_UINT      unsigned OS_INT
#endif

#ifndef   OS_TIME
  #define OS_TIME      int
#endif

#ifndef   OS_STAT
  #define OS_STAT      OS_U8
#endif

#ifndef   OS_PRIO
  #define OS_PRIO      OS_U8
#endif

#ifndef   OS_BOOL
  #define OS_BOOL      OS_U8
#endif

#define	OS_ERR_ISR_INDEX                      (100 )
#define	OS_ERR_ISR_VECTOR                     (101 )
#define	OS_ERR_ISR_PRIO                       (102 )


#define	OS_ERR_STACK                          (120 )

#define	OS_ERR_CSEMA_OVERFLOW                 (121 )

#define	OS_ERR_INV_TASK                       (128 )
#define	OS_ERR_INV_TIMER                      (129 )
#define	OS_ERR_INV_MAILBOX                    (130 )
#define	OS_ERR_INV_CSEMA                      (132 )
#define	OS_ERR_INV_RSEMA                      (133 )

#define	OS_ERR_MAILBOX_NOT1                   (135 )

#define	OS_ERR_MAILBOX_DELETE                 (136 )
#define	OS_ERR_CSEMA_DELETE                   (137 )
#define	OS_ERR_RSEMA_DELETE                   (138 )

#define	OS_ERR_MAILBOX_NOT_IN_LIST            (140 )
#define	OS_ERR_TASKLIST_CORRUPT               (142 )

#define	OS_ERR_UNUSE_BEFORE_USE               (150 )
#define	OS_ERR_LEAVEREGION_BEFORE_ENTERREGION (151 )
#define	OS_ERR_LEAVEINT                       (152 )
#define	OS_ERR_DICNT                          (153 )
#define	OS_ERR_INTERRUPT_DISABLED             (154 )
#define	OS_ERR_TASK_ENDS_WITHOUT_TERMINATE    (155 )
#define	OS_ERR_RESOURCE_OWNER                 (156 )
                                                 
#define	OS_ERR_ILLEGAL_IN_ISR                 (160 )

#define	OS_ERR_ILLEGAL_IN_TIMER               (161 )

#define	OS_ERR_ILLEGAL_OUT_ISR                (162 ) 

#define	OS_ERR_NOT_IN_ISR                     (163 )  //*** OS_EnterInterrupt() has been called, but CPU is not in ISR state
#define	OS_ERR_IN_ISR                         (164 )  //*** OS_EnterInterrupt() has not been called, but CPU is in ISR state

#define	OS_ERR_INIT_NOT_CALLED                (165 )  //*** OS_InitKern() was not called

#define	OS_ERR_2USE_TASK                      (170 )
#define	OS_ERR_2USE_TIMER                     (171 )
#define	OS_ERR_2USE_MAILBOX                   (172 )
#define	OS_ERR_2USE_BSEMA                     (173 )
#define	OS_ERR_2USE_CSEMA                     (174 )
#define	OS_ERR_2USE_RSEMA                     (175 )
#define	OS_ERR_2USE_MEMF                      (176 )

#define	OS_ERR_NESTED_RX_INT                  (180 )

#define	OS_ERR_MEMF_INV                       (190 )
#define	OS_ERR_MEMF_INV_PTR                   (191 )
#define	OS_ERR_MEMF_PTR_FREE                  (192 )
#define	OS_ERR_MEMF_RELEASE                   (193 )
#define	OS_ERR_POOLADDR                       (194 )
#define	OS_ERR_BLOCKSIZE                      (195 )

#define	OS_ERR_SUSPEND_TOO_OFTEN              (200 )
#define	OS_ERR_RESUME_BEFORE_SUSPEND          (201 )

#define	OS_ERR_TASK_PRIORITY                  (202 )

#define	OS_ERR_EVENTOBJ_INV                   (210 )
#define	OS_ERR_2USE_EVENTOBJ                  (211 )
#define	OS_ERR_EVENT_DELETE                   (212 )


#define	OS_ERR_NON_ALIGNED_INVALIDATE         (230 )      // Cache invalidation needs to be cache line aligned

typedef union {
  int Dummy;            // Make sure a full integer (32 bit on 32 bit CPUs) is used.
  struct {
    OS_U8 Region;
    OS_U8 DI;
  } Cnt;
} OS_COUNTERS;

typedef struct OS_GLOBAL {
  OS_COUNTERS    Counters;
} OS_GLOBAL;

#define OS_Counters      OS_Global.Counters

extern    OS_GLOBAL     OS_Global;

#if (__CPU_MODE__== 1)  // if THUMB mode
  #define OS_INTERWORK  __interwork

  #define OS_DI() OS_DisableInt()
  #define OS_EI() OS_EnableInt()
#else
  #define OS_INTERWORK

  #define OS_DI() __set_CPSR(__get_CPSR() | (1uL << 7))     // Optimization for ARM mode
  #define OS_EI() __set_CPSR(__get_CPSR() & ~(1uL << 7))
#endif

#if OS_DEBUG
  #define OS_ASSERT(Exp, ErrCode) { if (!(Exp)) OS_Error(ErrCode); }
#else
  #define OS_ASSERT(Exp, ErrCode)
#endif

#if 0
#define OS_DICnt     OS_Counters.Cnt.DI

#define OS_ASSERT_DICnt()         OS_ASSERT(((OS_DICnt & 0xf0) == 0), OS_ERR_DICNT)

#define OS_IncDI()       { OS_ASSERT_DICnt(); OS_DI(); OS_DICnt++; }
#define OS_DecRI()       { OS_ASSERT_DICnt(); if (--OS_DICnt==0) OS_EI(); }
#define OS_RESTORE_I()   { OS_ASSERT_DICnt(); if (OS_DICnt==0)   OS_EI(); }
#else
#define OS_DICnt     OS_Counters.Cnt.DI

#define OS_ASSERT_DICnt()         OS_ASSERT(((OS_DICnt & 0xf0) == 0), OS_ERR_DICNT)

#define OS_IncDI()       	{ OS_ASSERT_DICnt(); OS_DI(); OS_DICnt++; }
#define OS_DecRI()       	{ OS_ASSERT_DICnt(); if (--OS_DICnt==0) OS_EI(); }
#define OS_RESTORE_I()   	{ OS_ASSERT_DICnt(); if (OS_DICnt==0)   OS_EI(); }

void OS_HandleTickEx ( void );
#define	OS_HandleTickEx	FreeRTOS_Tick_Handler
#endif

#define	RTOS_ID_TASK		0x5441534B
typedef struct OS_TASK OS_TASK;
struct OS_TASK 
{
	StaticTask_t			task;
	unsigned int			dummy1[4];
	unsigned int			id;
	TaskHandle_t			handle;
	unsigned int			dummy2[2];
} ;

typedef void voidRoutine(void);
typedef void OS_TIMERROUTINE(void);
typedef void OS_TIMER_EX_ROUTINE(void *);

void OS_TICK_Config ( unsigned FractPerInt, unsigned FractPerTick );


#ifndef OS_STACKPTR
  #define OS_STACKPTR
#endif

#ifndef OS_ROM_DATA
  #define OS_ROM_DATA
#endif

#define CTPARA_TIMESLICE ,2
#define OS_CREATE_TASK_PARA_TS   ,OS_UINT TimeSlice

  #define OS_CREATETASK(pTask, Name, Hook, Priority, pStack) \
  OS_CreateTask (pTask,                                      \
                  Name,                                      \
                  Priority,                                  \
                  Hook,                                      \
                  (void OS_STACKPTR*)pStack,                 \
                  sizeof(pStack)                             \
                  CTPARA_TIMESLICE                           \
               )

  #define OS_CREATETASK_EX(pTask, Name, Hook, Priority, pStack, pContext) \
  OS_CreateTaskEx  (pTask,                                                \
                    Name,                                                 \
                    Priority,                                             \
                    Hook,                                                 \
                    (void OS_STACKPTR*)pStack,                            \
                    sizeof(pStack)                                        \
                    CTPARA_TIMESLICE,                                     \
                    pContext                                              \
               )

void OS_CreateTask  ( OS_TASK * pTask,
                      OS_ROM_DATA const char* Name,
                      OS_U8 Priority,
                      void (*pRoutine)(void),
                      void OS_STACKPTR *pStack,
                      OS_UINT StackSize
                      OS_CREATE_TASK_PARA_TS
        );

#define OS_CREATE_TASK_PARA_NAME      OS_ROM_DATA const char* Name,

void OS_CreateTaskEx  ( OS_TASK * pTask,
                        OS_CREATE_TASK_PARA_NAME
                        OS_U8 Priority,
                        void (*pRoutine)(void *),
                        void OS_STACKPTR *pStack,
                        OS_UINT StackSize
                        OS_CREATE_TASK_PARA_TS,
                        void * pContext
        );
        

// Ends (terminates) a task.
void OS_Terminate (OS_TASK* pTask);

OS_TASK* OS_GetpCurrentTask (void);

// Suspends the calling task until a specified time.
// The calling task will be put into the TS_DELAY state until the time specified.
// The OS_DelayUntil() function delays until the value of the time-variable OS_Time has reached a certain value. It
// is very useful if you have to avoid accumulating delays.
void OS_DelayUntil (int t);		


// Waits for the specified events for a given time, and clears the event memory after an event occurs.
char OS_WaitEventTimed (char EventMask, OS_TIME TimeOut);

// Waits for one of the events specified in the bitmask and clears the event memory after an event occurs.
// If none of the specified events are signaled, the task is suspended. The first of the specified events will wake the task.
// These events are signaled by another task, a S/W timer or an interrupt handler. Any bit in the 8-bit event mask may
// enable the corresponding event.
char OS_WaitEvent (char EventMask);

// Waits for one of the events specified by the bitmask and clears only that event after it occurs.
//		Return value		All masked events that have actually occurred.
// If none of the specified events are signaled, the task is suspended. The first of the specified events will wake the task.
// These events are signaled by another task, a S/W timer, or an interrupt handler. Any bit in the 8-bit event mask may
// enable the corresponding event. All unmasked events remain unchanged.
char OS_WaitSingleEvent (char EventMask);

// Signals event(s) to a specified task.
// If the specified task is waiting for one of these events, it will be put in the READY state and activated according to the rules of the scheduler.
void OS_SignalEvent (char Event, OS_TASK* pTask);

// Returns a list of events that have occurred for a specified task.
// The event mask of the events that have actually occurred.
// By calling this function, the actual events remain signaled. The event memory is not cleared. This is one way for a task
// to find out which events have been signaled. The task is not suspended if no events are available.
char OS_GetEventsOccurred (OS_TASK* pTask);

// Returns the actual state of events and then clears the events of a specified task.
// pTask					The task who's event mask is to be returned,
//							NULL means current task.
//
// Return value		The events that were actually signaled before clearing.
char OS_ClearEvents (OS_TASK* pTask);

#define	RTOS_ID_RSEMA		0x5253454D

typedef struct OS_RSEMA OS_RSEMA;
struct OS_RSEMA 
{
	StaticSemaphore_t		sema;
	unsigned int			id;
	SemaphoreHandle_t		handle;
} ;

//typedef	StaticSemaphore_t	OS_RSEMA;

int OS_Use (OS_RSEMA* pRSema);

void OS_Unuse (OS_RSEMA* pRSema);
char OS_Request (OS_RSEMA* pRSema);
void OS_CREATERSEMA (OS_RSEMA* pRSema);
void OS_DeleteRSema (OS_RSEMA* pRSema);

#define	RTOS_ID_EVENT		0x45564E54	// "EVNT"

typedef struct  OS_EVENT OS_EVENT;

struct OS_EVENT 
{
	StaticEventGroup_t		event;
	unsigned int				id;
	EventGroupHandle_t		handle;
} ;

void OS_EVENT_Create (OS_EVENT* pEvent);
void OS_EVENT_Delete (OS_EVENT* pEvent);
void OS_EVENT_Set (OS_EVENT* pEvent);
void OS_EVENT_Reset (OS_EVENT* pEvent);
void OS_EVENT_Wait (OS_EVENT* pEvent);
char OS_EVENT_WaitTimed (OS_EVENT* pEvent, OS_TIME Timeout);	


#define	RTOS_ID_CSEMA		0x4353454D	// "CSEM"
typedef struct OS_CSEMA OS_CSEMA;
struct OS_CSEMA 
{
	StaticSemaphore_t		sema;
	unsigned int			id;
	SemaphoreHandle_t		handle;
} ;

void OS_CreateCSema (OS_CSEMA* pCSema, OS_UINT InitValue);

void OS_CREATECSEMA (OS_CSEMA* pCSema);
#define	OS_CREATECSEMA(pCSema)	OS_CreateCSema(pCSema,0)
// Increments the counter of a semaphore.
void OS_SignalCSema (OS_CSEMA * pCSema);

// Decrements the counter of a semaphore.
void OS_WaitCSema (OS_CSEMA* pCSema);

void OS_DeleteCSema (OS_CSEMA* pCSema);

#define	RTOS_ID_MAILBOX		0x4D41494C	// "MAIL"

typedef struct OS_MAILBOX OS_MAILBOX;
struct OS_MAILBOX
{
	StaticMessageBuffer_t	message;
	unsigned int				id;
	void *	handle;
	char 							min_msg[4];
	unsigned int				size;		// message size
} ;

void OS_CREATEMB (OS_MAILBOX* pMB, unsigned char sizeofMsg, unsigned int maxnofMsg, void* pMsg);
void OS_DeleteMB (OS_MAILBOX* pMB);
void OS_ClearMB (OS_MAILBOX* pMB);
void OS_PutMail (OS_MAILBOX* pMB,void* pMail);
void OS_PutMail1 (OS_MAILBOX* pMB, const char* pMail);
char OS_GetMailTimed (OS_MAILBOX* pMB, void* pDest, OS_TIME Timeout);
void OS_GetMail (OS_MAILBOX* pMB, void* pDest);
void OS_GetMail1 (OS_MAILBOX* pMB,char* pDest);

void OS_EnterRegion(void);
void OS_LeaveRegion(void);

#define	OS_INTERWORK

#define	RTOS_ID_TIMER		0x54494D52	// "TIMR"
typedef struct OS_TIMER OS_TIMER;
struct OS_TIMER 
{
	StaticTimer_t			timer;
	unsigned int			id;
	TimerHandle_t			handle;
} ;

typedef struct {
  OS_TIMER Timer;
  OS_TIMER_EX_ROUTINE * pfUser;
  void * pData;
} OS_TIMER_EX;


// Creates a software timer (but does not start it).
void OS_CreateTimer (OS_TIMER* pTimer, OS_TIMERROUTINE* Callback, OS_TIME Timeout);

// Starts a specified timer.
void OS_StartTimer (OS_TIMER* pTimer);

// Stops and deletes a specified timer.
void OS_DeleteTimer (OS_TIMER* pTimer);

// Restarts a specified timer with its initial time value.
void OS_RetriggerTimer (OS_TIMER* pTimer);


#define OS_CREATETIMER(pTimer,c,d)  \
        OS_CreateTimer(pTimer,c,d); \
        OS_StartTimer(pTimer);

void* OS_malloc(unsigned int n);
void  OS_free  (void* pMemBlock);

typedef void      OS_ISR_HANDLER(void);

OS_ISR_HANDLER* OS_ARM_InstallISRHandler (int ISRIndex, OS_ISR_HANDLER* pISRHandler);

void OS_Delay (int ms);		// Suspends the calling task for a specified period of time
int OS_GetTime (void);
unsigned int OS_GetTime32 (void);

void XM_Lock (void);
void XM_Unlock (void);
void OS_InitKern(void); 
void OS_INIT_SYS_LOCKS(void);
void OS_InitHW(void);
void OS_Start (void);

unsigned char OS_GetPriority (OS_TASK* pTask);
void OS_SetPriority (OS_TASK* pTask, unsigned char Priority);


void OS_Error(int ErrCode);

void OS_ARM_EnableISRSource(int SourceIndex);
void OS_ARM_DisableISRSource(int SourceIndex);

/*
*********************************************************************************************************
*                            TIMER OPTIONS (see OSTmrStart() and OSTmrStop())
*********************************************************************************************************
*/
#define  OS_TMR_OPT_NONE              0u    /* No option selected                                      */

#define  OS_TMR_OPT_ONE_SHOT          1u    /* Timer will not automatically restart when it expires    */
#define  OS_TMR_OPT_PERIODIC          2u    /* Timer will     automatically restart when it expires    */

#define  OS_TMR_OPT_CALLBACK          3u    /* OSTmrStop() option to call 'callback' w/ timer arg.     */
#define  OS_TMR_OPT_CALLBACK_ARG      4u    /* OSTmrStop() option to call 'callback' w/ new   arg.     */

/*
*********************************************************************************************************
*                                            TIMER STATES
*********************************************************************************************************
*/
#define  OS_TMR_STATE_UNUSED          0u
#define  OS_TMR_STATE_STOPPED         1u
#define  OS_TMR_STATE_COMPLETED       2u
#define  OS_TMR_STATE_RUNNING         3u

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_RTOS_H_
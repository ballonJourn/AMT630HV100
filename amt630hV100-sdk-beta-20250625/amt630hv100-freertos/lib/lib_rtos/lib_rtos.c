#include <stdio.h>
#include <stdarg.h>

#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "task.h"
#include "rtos.h"
#include "xm_base.h"
#include "rtc.h"

extern volatile uint32_t ulPortInterruptNesting;
OS_GLOBAL     OS_Global;

void XM_lock (void)
{
	OS_IncDI();
}

void XM_unlock (void)
{
	OS_DecRI();
}

void OS_InitKern (void)
{
}

void OS_INIT_SYS_LOCKS (void)
{
}

void OS_CreateTaskEx  ( OS_TASK * pTask,
                        OS_CREATE_TASK_PARA_NAME
                        OS_U8 Priority,
                        void (*pRoutine)(void *),
                        void OS_STACKPTR *pStack,
                        OS_UINT StackSize
                        OS_CREATE_TASK_PARA_TS,
                        void * pContext
        )
{
	int priority;
	TaskFunction_t pvTaskCode = (TaskFunction_t)pRoutine;
	TaskHandle_t handle;
	StackSize /= sizeof(StackType_t);

	priority = Priority;
	priority -= 224;
	if(priority < 0)
		priority = 0;
	else if(priority > configMAX_PRIORITIES )
		priority = configMAX_PRIORITIES - 1;
	
	
	vTaskSuspendAll ();
	//XM_printf ("sizeof(pTask->task) = %d\n", sizeof(pTask->task));
	handle = xTaskCreateStatic 
							(pvTaskCode,		// TaskFunction_t pvTaskCode,
							 Name,				// const char * const pcName,
							 StackSize,			// uint32_t ulStackDepth
							 pContext,				// void *pvParameters,
							 priority,			// UBaseType_t uxPriority,
							 pStack,				// StackType_t * const puxStackBuffer,
							 &pTask->task		// StaticTask_t * const pxTaskBuffer
							 );
	if(handle)
	{
		pTask->id = RTOS_ID_TASK;
		pTask->handle = handle;
		vTaskSetApplicationTaskTag(handle, (TaskHookFunction_t)pTask);
	}
	xTaskResumeAll ();
	if(handle == NULL)
	{
		OS_Error (OS_ERR_STACK);
	}
}


void OS_CreateTask  ( OS_TASK * pTask,
                      OS_ROM_DATA const char* Name,
                      OS_U8 Priority,
                      void (*pRoutine)(void),
                      void OS_STACKPTR *pStack,
                      OS_UINT StackSize
                      OS_CREATE_TASK_PARA_TS
        )
{
	int priority;
	TaskFunction_t pvTaskCode = (TaskFunction_t)pRoutine;
	TaskHandle_t handle;
	StackSize /= sizeof(StackType_t);

	priority = Priority;
	priority -= 224;
	if(priority < 0)
		priority = 0;
	else if(priority > configMAX_PRIORITIES )
		priority = configMAX_PRIORITIES - 1;
	
	
	vTaskSuspendAll ();
	//XM_printf ("sizeof(pTask->task) = %d\n", sizeof(pTask->task));
	handle = xTaskCreateStatic 
							(pvTaskCode,		// TaskFunction_t pvTaskCode,
							 Name,				// const char * const pcName,
							 StackSize,			// uint32_t ulStackDepth
							 NULL,				// void *pvParameters,
							 priority,			// UBaseType_t uxPriority,
							 pStack,				// StackType_t * const puxStackBuffer,
							 &pTask->task		// StaticTask_t * const pxTaskBuffer
							 );
	if(handle)
	{
		pTask->id = RTOS_ID_TASK;
		pTask->handle = handle;
		vTaskSetApplicationTaskTag(handle, (TaskHookFunction_t)pTask);
	}
	xTaskResumeAll ();
	if(handle == NULL)
	{
		OS_Error (OS_ERR_STACK);
	}
}

void OS_Terminate (OS_TASK* pTask)
{
	TaskHandle_t handle;
	if(pTask == NULL)
	{
		pTask = OS_GetpCurrentTask();
		pTask->id = 0;
		pTask->handle = 0;
		vTaskDelete(NULL);
	}
	else
	{
		if(pTask->id != RTOS_ID_TASK)
		{
			OS_Error (OS_ERR_INV_TASK);
			return;	
		}
		handle = pTask->handle;
		pTask->id = 0;
		pTask->handle = 0;
		vTaskDelete(handle);
	}
}

void OS_Delay (int ms)
{
	vTaskDelay(ms/portTICK_RATE_MS); 
	 //while(ms--);
}

OS_TASK* OS_GetpCurrentTask (void)
{
	OS_TASK *pTask = (OS_TASK *)xTaskGetApplicationTaskTag(NULL);
	if(pTask == NULL || pTask->id != RTOS_ID_TASK)
	{
		OS_Error (OS_ERR_INV_TASK);
		return NULL;	
	}
	return pTask;
}

unsigned char OS_GetPriority (OS_TASK* pTask)
{
	int priority;
	TaskHandle_t handle = 0;
	if(pTask && pTask->id != RTOS_ID_TASK)
	{
		OS_Error (OS_ERR_INV_TASK);
		return 0;			
	}
	if(pTask)
		handle = pTask->handle;
	priority = (unsigned char)uxTaskPriorityGet (handle);
	priority += 224;	// 0 ~ 32
	if(priority > 255)
		priority = 255;
	return (unsigned char)priority; 
}

void OS_SetPriority (OS_TASK* pTask, unsigned char Priority)
{
	int priority;
	TaskHandle_t handle = 0;
	if(pTask && pTask->id != RTOS_ID_TASK)
	{
		OS_Error (OS_ERR_INV_TASK);
		return;			
	}
	priority = Priority;
	priority -= 224;
	if(priority < 0)
		priority = 0;
	else if(priority > configMAX_PRIORITIES )
		priority = configMAX_PRIORITIES - 1;
		
	if(pTask)
		handle = pTask->handle;
	vTaskPrioritySet(handle, priority); 	
}

#include <stdlib.h>
void* OS_malloc(unsigned int n)
{
	char *mem =  (char *)pvPortMalloc(n);
	if(mem)
	{
		//memset (mem, 0, n);
	}
	return mem;
}

void  OS_free  (void* pMemBlock)
{
	vPortFree (pMemBlock);
}

void* OS_realloc (void * pv, unsigned int n )
{
	return pvPortRealloc (pv, n);
}

void OS_CREATERSEMA (OS_RSEMA* pRSema)
{
	SemaphoreHandle_t handle;
	if(pRSema == NULL)
	{
		OS_Error (OS_ERR_INV_RSEMA);
		return;	
	}
	if(pRSema->id == RTOS_ID_RSEMA)
	{
		OS_Error (OS_ERR_2USE_RSEMA);
		return;			
	}
	vTaskSuspendAll ();
	handle = xSemaphoreCreateRecursiveMutexStatic(&pRSema->sema);
	if(handle)
	{
		pRSema->handle = handle;
		pRSema->id = RTOS_ID_RSEMA;	
	}
	xTaskResumeAll ();
	if(handle == NULL)
	{
		OS_Error (OS_ERR_INV_RSEMA);
		return;
	}
}

void OS_DeleteRSema (OS_RSEMA* pRSema)
{
	if(pRSema == NULL || pRSema->id != RTOS_ID_RSEMA || pRSema->handle == NULL)
	{
		OS_Error (OS_ERR_RSEMA_DELETE);
		return;			
	}
	vTaskSuspendAll ();
	vSemaphoreDelete(pRSema->handle);
	pRSema->handle = NULL;
	pRSema->id = 0;
	xTaskResumeAll ();
}

int OS_Use (OS_RSEMA* pRSema)
{
	if(pRSema == NULL || pRSema->id != RTOS_ID_RSEMA)
	{
		OS_Error (OS_ERR_INV_RSEMA);
		return -1;
	}
	if(xSemaphoreTakeRecursive( pRSema->handle, portMAX_DELAY) == pdPASS)
		return 1;
	else
	{
		OS_Error (OS_ERR_INV_RSEMA);
		return -1;				
	}
}

void OS_Unuse (OS_RSEMA* pRSema)
{
	if(pRSema == NULL || pRSema->id != RTOS_ID_RSEMA)
	{
		OS_Error (OS_ERR_INV_RSEMA);
		return;
	}
	if(xSemaphoreGiveRecursive(pRSema->handle) != pdPASS)
	{
		OS_Error (OS_ERR_UNUSE_BEFORE_USE);
	}
}

// Requests a specified semaphore and blocks it for other tasks if it is available. Continues execution in any case.
//	1: Resource was available, now in use by calling task
//	0: Resource was not available.
char OS_Request (OS_RSEMA* pRSema)
{
	if(pRSema == NULL || pRSema->id != RTOS_ID_RSEMA)
	{
		OS_Error (OS_ERR_INV_RSEMA);
		return 0;
	}
	if(xSemaphoreTakeRecursive(pRSema->handle, 0) == pdPASS)
		return 1;
	else
		return 0;
}

void OS_CreateCSema (OS_CSEMA* pCSema, OS_UINT InitValue)
{
	SemaphoreHandle_t handle;
	if(pCSema == NULL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return;
	}
	memset (pCSema, 0, sizeof(OS_CSEMA));

	// ½ûÖ¹ÈÎÎñµ÷¶È
	vTaskSuspendAll ();	
	handle = xSemaphoreCreateCountingStatic( 0xFFFFFFFF, InitValue, &pCSema->sema);
	if(handle)
	{
		pCSema->handle = handle;
		pCSema->id = RTOS_ID_CSEMA;	
	}
	// Ê¹ÄÜÈÎÎñµ÷¶È
	xTaskResumeAll ();
	
	if(handle == NULL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return;		
	}
}

void OS_DeleteCSema (OS_CSEMA* pCSema)
{
	SemaphoreHandle_t handle;
	if(pCSema == NULL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return;		
	}
	if(pCSema->id != RTOS_ID_CSEMA || pCSema->handle == NULL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return;				
	}
	// ½ûÖ¹ÈÎÎñµ÷¶È
	vTaskSuspendAll ();	
	handle = pCSema->handle;
	pCSema->id = 0;
	pCSema->handle = 0;
	vSemaphoreDelete (handle);
	// Ê¹ÄÜÈÎÎñµ÷¶È
	xTaskResumeAll ();
}

void OS_WaitCSema (OS_CSEMA* pCSema)
{
	if(pCSema == NULL || pCSema->id != RTOS_ID_CSEMA || pCSema->handle == NULL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return;						
	}
	if(xSemaphoreTake(pCSema->handle, portMAX_DELAY) == pdFAIL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return;						
	}
}

int OS_WaitCSemaTimed (OS_CSEMA* pCSema, int TimeOut)
{
	if(pCSema == NULL || pCSema->id != RTOS_ID_CSEMA || pCSema->handle == NULL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return 0;						
	}
	if(xSemaphoreTake(pCSema->handle, TimeOut) == pdFAIL)
	{
		//printf ("OS_WaitCSemaTimed failed\n");
		return 0;						
	}	
	
     //  printf ("OS_WaitCSemaTimed 0x%08x OK\n", pCSema);
	return 1;
}

// Increments the counter of a semaphore.
void OS_SignalCSema (OS_CSEMA * pCSema)
{
	BaseType_t ret = pdFAIL;
	
	if(ulPortInterruptNesting)
	{
		// ´ÓÖÐ¶Ï³ÌÐòÖÐµ÷ÓÃ
		if(pCSema && pCSema->id == RTOS_ID_CSEMA)
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			ret = xSemaphoreGiveFromISR(pCSema->handle, &xHigherPriorityTaskWoken);		
			if(ret == pdTRUE)
			{
				//XM_printf ("OS_SignalCSema 0x%08x\n", pCSema);
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}
			else
			{
				//XM_printf ("OS_SignalCSema failed\n");
			}
		}
		else
		{
			//XM_printf ("OS_SignalCSema error\n");
		}
	}
	else
	{
		vTaskSuspendAll ();
		if(pCSema && pCSema->id == RTOS_ID_CSEMA)
		{
			ret = xSemaphoreGive(pCSema->handle);		
		}
		xTaskResumeAll ();
	}
	if(ret == pdFAIL)
	{
		OS_Error (OS_ERR_INV_TASK);
	}		
}

int OS_GetCSemaValue (OS_CSEMA* pCSema)
{
	if(pCSema == NULL || pCSema->id != RTOS_ID_CSEMA || pCSema->handle == NULL)
	{
		OS_Error (OS_ERR_INV_CSEMA);
		return 0;						
	}
	return uxSemaphoreGetCount (pCSema->handle);	
}

// 0: If the value could be set.
// != 0: In case of error.
int OS_SetCSemaValue (OS_CSEMA* pCSema, OS_UINT Value)
{
	while(Value)
	{
		OS_SignalCSema (pCSema);
		Value --;
	}
	return 0;	
}


// Waits for one of the events specified in the bitmask and clears the event memory after an event occurs.
char OS_WaitEvent (char EventMask)
{
	uint32_t pulNotificationValue;
	while(1)
	{
		if(xTaskNotifyWait(0, (unsigned char)0xffffffff, &pulNotificationValue, portMAX_DELAY) == pdTRUE)
		{
			// ¼ì²éÖ¸¶¨µÄÊÂ¼þÊÇ·ñ²úÉú
			if(pulNotificationValue & EventMask)
				break;
		}
		else
		{
			// Òì³£
			OS_Error (OS_ERR_INV_TASK);
			return 0;
		}
	}
	return (char)(pulNotificationValue & EventMask);
}

// Waits for one of the events specified by the bitmask and clears only that event after it occurs.
char OS_WaitSingleEvent (char EventMask)
{
	unsigned int mask;
	int i;
	uint32_t events = 0;
	uint32_t pulNotificationValue = 0;
	uint32_t event_to_write_back = 0;	// »ØÐ´ÊÂ¼þ
	uint32_t event_to_write_back_mask = (uint8_t)(~EventMask);
	while(1)
	{
		if(xTaskNotifyWait(0, (unsigned char)0xffffffff, &pulNotificationValue, portMAX_DELAY) == pdTRUE)
		{
			pulNotificationValue |= pulNotificationValue & event_to_write_back_mask;
			// ¼ì²éÖ¸¶¨µÄÊÂ¼þÊÇ·ñ²úÉú
			events = pulNotificationValue & EventMask;
			if(events)
				break;
		}
		else
		{
			// Òì³£
			OS_Error (OS_ERR_INV_TASK);
			return 0;
		}
	}
	
	// ´Óbit0¿ªÊ¼É¨Ãè, bit0~bit7
	mask = 0x01;
	for (i = 0; i < 8; i ++)
	{
		if(events & mask)
		{
			events &= ~mask;
			break;
		}
		mask = mask << 1;
	}
	
	event_to_write_back |= events;
	
	// ¼ì²éÊÇ·ñ´æÔÚ»ØÐ´µÄÊÂ¼þ
	if(event_to_write_back)
	{
		xTaskNotify(xTaskGetCurrentTaskHandle(), event_to_write_back, eSetBits);
	}
	return (char)(pulNotificationValue & mask);
	
}

void OS_SignalEvent (char Event, OS_TASK* pTask)
{
	BaseType_t ret = pdFAIL;

	if(ulPortInterruptNesting)
	{
		if(pTask && pTask->id == RTOS_ID_TASK)
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			ret = xTaskNotifyFromISR(pTask->handle, (unsigned char)Event, eSetBits, &xHigherPriorityTaskWoken);	
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	}
	else
	{
		vTaskSuspendAll ();
		if(pTask && pTask->id == RTOS_ID_TASK)
		{
			ret = xTaskNotify(pTask->handle, (unsigned char)Event, eSetBits);		
		}
		xTaskResumeAll ();
	}
	if(ret == pdFAIL)
	{
		OS_Error (OS_ERR_INV_TASK);
	}
}

// ÒÔÏÂº¯ÊýµÄ·ÂÕæ·ÇÍêÕû, µ«²»Ó°Ïìµ±Ç°ÏîÄ¿µÄÊ¹ÓÃ.
// ¼Ù¶¨pTaskÎªµ±Ç°ÈÎÎñ
char OS_GetEventsOccurred (OS_TASK* pTask)
{
	uint32_t ulNotifiedValue = 0;
	if(pTask)
	{
		if(pTask->id != RTOS_ID_TASK)
		{
			OS_Error (OS_ERR_INV_TASK);
			return 0;
		}
		// ¼Ù¶¨±ØÐëÎªµ±Ç°ÈÎÎñ
		if(pTask->handle != xTaskGetCurrentTaskHandle())
		{
			OS_Error (OS_ERR_INV_TASK);
			return 0;		
		}
	}
	xTaskNotifyWait(0, 0, &ulNotifiedValue, 0);
	return (char)ulNotifiedValue;
}

// ÒÔÏÂº¯ÊýµÄ·ÂÕæ·ÇÍêÕû, µ«²»Ó°Ïìµ±Ç°ÏîÄ¿µÄÊ¹ÓÃ.
// Returns the actual state of events and then clears the events of a specified task.
char OS_ClearEvents (OS_TASK* pTask)
{
	if(pTask == NULL || pTask->id !=  RTOS_ID_TASK)
	{
		OS_Error (OS_ERR_INV_TASK);
		return 0;
	}
	xTaskNotifyStateClear(pTask->handle);
	return 0; 
}

void OS_Start (void)
{
	configASSERT(OS_DICnt == 1);
	OS_DICnt = 0;
	vTaskStartScheduler ();
}

void OS_EnterRegion (void)
{
	XM_lock ();
	//configASSERT(ulPortInterruptNesting == 0);
	//taskENTER_CRITICAL ();
}

void OS_LeaveRegion (void)
{
	//configASSERT(ulPortInterruptNesting == 0);
	//taskEXIT_CRITICAL ();
	XM_unlock ();
}

int OS_GetTime (void)
{
	return xTaskGetTickCount ();
}

unsigned int OS_GetTime32 (void)
{
	return xTaskGetTickCount ();
}

void OS_CreateTimer (OS_TIMER* pTimer, OS_TIMERROUTINE* Callback, OS_TIME Timeout)
{
	TimerHandle_t handle;
	if(pTimer == NULL)
	{
		OS_Error (OS_ERR_INV_TIMER);
		return;
	}
	if(pTimer->id == RTOS_ID_TIMER)
	{
		OS_Error (OS_ERR_2USE_TIMER);
		return;
	}
	handle = xTimerCreateStatic (NULL, Timeout, 0, 0,  (TimerCallbackFunction_t)Callback, &pTimer->timer);
	if(handle)
	{
		pTimer->id = RTOS_ID_TIMER;
		pTimer->handle = handle;
	}
	else
	{
		OS_Error (OS_ERR_INV_TIMER);
		return;		
	}
}

void OS_DeleteTimer (OS_TIMER* pTimer)
{
	TimerHandle_t handle;
	if(pTimer == NULL)
	{
		OS_Error (OS_ERR_INV_TIMER);
		return;
	}
	if(pTimer->id != RTOS_ID_TIMER)
	{
		OS_Error (OS_ERR_INV_TIMER);
		return;
	}
	
	handle = pTimer->handle;
	pTimer->id = 0;
	pTimer->handle = 0;
	xTimerDelete (handle, portMAX_DELAY);
}

void OS_StartTimer (OS_TIMER* pTimer)
{
	if(pTimer == NULL || pTimer->id != RTOS_ID_TIMER)
	{
		OS_Error (OS_ERR_INV_TIMER);
		return;
	}
	if(ulPortInterruptNesting)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		if( xTimerStartFromISR( pTimer->handle, &xHigherPriorityTaskWoken ) == pdPASS )
		{
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	}
	else
	{
		xTimerStart (pTimer->handle, portMAX_DELAY);
	}
}

void OS_StopTimer (OS_TIMER* pTimer)
{
	if(pTimer == NULL || pTimer->id != RTOS_ID_TIMER)
	{
		OS_Error (OS_ERR_INV_TIMER);
		return;
	}
	if(ulPortInterruptNesting)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		if(xTimerStopFromISR( pTimer->handle, &xHigherPriorityTaskWoken) == pdPASS)
		{
			if(xHigherPriorityTaskWoken != pdFALSE)
			{
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}
		}
		else
		{
			OS_Error (OS_ERR_INV_TIMER);
			return;
		}
	}
	else
	{
		xTimerStop (pTimer->handle, portMAX_DELAY);
	}
}
			
void OS_RetriggerTimer (OS_TIMER* pTimer)
{
	if(pTimer == NULL || pTimer->id != RTOS_ID_TIMER)
	{
		OS_Error (OS_ERR_INV_TIMER);
		return;
	}
	if(ulPortInterruptNesting)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		if( xTimerStartFromISR( pTimer->handle, &xHigherPriorityTaskWoken ) == pdPASS )
		{
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}		
	}
	else
	{
		xTimerStart (pTimer->handle, portMAX_DELAY);
	}
}
			
void OS_EVENT_Create (OS_EVENT* pEvent)
{
	EventGroupHandle_t handle;
	if(pEvent == NULL || pEvent->id == RTOS_ID_EVENT)
	{
		OS_Error (OS_ERR_EVENTOBJ_INV);
		return;
	}
	handle = xEventGroupCreateStatic(&pEvent->event);
	pEvent->handle = handle;
	pEvent->id = RTOS_ID_EVENT;
}
			
void OS_EVENT_Delete (OS_EVENT* pEvent)
{
	if(pEvent == NULL || pEvent->id != RTOS_ID_EVENT)
	{
		OS_Error (OS_ERR_EVENTOBJ_INV);
		return;		
	}
	pEvent->id = 0;
	vEventGroupDelete (pEvent->handle);
	pEvent->handle = 0;
}
			
void OS_EVENT_Wait (OS_EVENT* pEvent)
{
	if(pEvent == NULL || pEvent->id != RTOS_ID_EVENT)
	{
		OS_Error (OS_ERR_EVENTOBJ_INV);
		return;		
	}
	// Ê¹ÓÃbit0Ä£ÄâOS_EVENT
	xEventGroupWaitBits( pEvent->handle, 0x01, pdTRUE, pdFALSE, portMAX_DELAY);
}

// 0: Success, the event was signaled within the specified time.
// 1: The event was not signaled and a timeout occurred.
char OS_EVENT_WaitTimed (OS_EVENT* pEvent, OS_TIME Timeout)
{
	if(pEvent == NULL || pEvent->id != RTOS_ID_EVENT)
	{
		OS_Error (OS_ERR_EVENTOBJ_INV);
		return 1;		
	}
	// Ê¹ÓÃbit0Ä£ÄâOS_EVENT
	if(xEventGroupWaitBits( pEvent->handle, 0x01, pdTRUE, pdFALSE, Timeout) & 0x01)
		return 0;
	else
		return 1;	
}
			
void OS_EVENT_Reset (OS_EVENT* pEvent)
{
	if(pEvent == NULL || pEvent->id != RTOS_ID_EVENT)
	{
		OS_Error (OS_ERR_EVENTOBJ_INV);
		return;		
	}
	xEventGroupClearBits (pEvent->handle, 0x01);
}
			
void OS_EVENT_Set (OS_EVENT* pEvent)
{
	if(pEvent == NULL || pEvent->id != RTOS_ID_EVENT)
	{
		OS_Error (OS_ERR_EVENTOBJ_INV);
		return;		
	}
	if(ulPortInterruptNesting)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		if(xEventGroupSetBitsFromISR( pEvent->handle, 0x01, &xHigherPriorityTaskWoken) == pdPASS)
		{
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
		else
		{
			//XM_printf ("OS_EVENT_Set failed\n");
		}
	}
	else
	{
		xEventGroupSetBits (pEvent->handle, 0x01);	
	}
}



void OS_CREATEMB (OS_MAILBOX* pMB, unsigned char sizeofMsg, unsigned int maxnofMsg, void* pMsg)
{
	MessageBufferHandle_t handle;
	unsigned int size;
	if(pMB == NULL || pMsg == NULL || sizeofMsg*maxnofMsg == 0)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return;
	}
	if(pMB->id == RTOS_ID_MAILBOX)
	{
		OS_Error (OS_ERR_2USE_MAILBOX);
		return;		
	}
	size = sizeofMsg * maxnofMsg;
	if(size < 4)
	{
		// special one MB
		handle = xMessageBufferCreateStatic(sizeof(pMB->min_msg), (uint8_t *)pMB->min_msg, &pMB->message);
	}
	else
	{
		// ±ØÐëÎª4×Ö½ÚµÄ±¶Êý
		if(size & 3)
		{
			OS_Error (OS_ERR_INV_MAILBOX);
			return;
		}
		handle = xMessageBufferCreateStatic(sizeofMsg * maxnofMsg, pMsg, &pMB->message);
	}
	pMB->handle = handle;
	pMB->id = RTOS_ID_MAILBOX;
	pMB->size = sizeofMsg;
}

// Clears all messages in a specified mailbox
void OS_ClearMB (OS_MAILBOX* pMB)
{
	if(pMB == NULL || pMB->id != RTOS_ID_MAILBOX || pMB->handle == NULL)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return;		
	}
	if(xMessageBufferReset(pMB->handle) == pdFAIL)
	{
		//OS_Error (OS_ERR_INV_MAILBOX);
		return;				
	}
}

char OS_GetMailTimed (OS_MAILBOX* pMB, void* pDest, OS_TIME Timeout)
{
	size_t length;
	if(pMB == NULL || pMB->id != RTOS_ID_MAILBOX || pMB->handle == NULL || pDest == NULL)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return 1;		
	}
	length = xMessageBufferReceive(pMB->handle, pDest, pMB->size, Timeout);
	if(length == pMB->size)
		return 0;
	else
		return 1;
}

void OS_PutMail (OS_MAILBOX* pMB, void* pMail)
{
	if(pMB == NULL || pMB->id != RTOS_ID_MAILBOX || pMB->handle == NULL || pMail == NULL)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return;		
	}
	configASSERT (ulPortInterruptNesting == 0);
	if(xMessageBufferSend(pMB->handle, pMail, pMB->size, portMAX_DELAY) == 0)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return;
	}
}

void OS_PutMail1 (OS_MAILBOX* pMB, const char* pMail)
{
	if(pMB == NULL || pMB->id != RTOS_ID_MAILBOX || pMB->handle == NULL || pMail == NULL)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return;		
	}
	if(pMB->size != 1)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return;		
	}
	if(xMessageBufferSend(pMB->handle, pMail, pMB->size, portMAX_DELAY) == 0)
	{
		OS_Error (OS_ERR_INV_MAILBOX);
		return;
	}	
}

void OS_TICK_Config ( unsigned FractPerInt, unsigned FractPerTick )
{
	
}


void OS_Error(int ErrCode) 
{
#if (DEBUG == 1)  
	// Êä³ö´íÎóÂë
	printf ("OS_Error CODE: 0x%02x\r\n", ErrCode);
#endif
	OS_DICnt = 0;         /* Allow interrupts so we can communicate */
	OS_EI();
  while (1);
}

void trace_error (char *fmt, ...)
{
}

void xm_do_change_task (void)
{
}


#define	XM_PRINTF_SIZE		128
void XM_printf (char *fmt, ...)
{
	static char xm_info[XM_PRINTF_SIZE + 4];

	va_list ap;
	xm_info[XM_PRINTF_SIZE] = 0;
	va_start(ap, fmt);
	//vsprintf (xm_info, fmt, ap);
	vsnprintf (xm_info, XM_PRINTF_SIZE, fmt, ap);
	va_end(ap);

	printf ("%s", xm_info);
}

void XM_printf_ (char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	printf(fmt, ap);
	va_end(ap);
}

// æ—¶é—´è®¾ç½®/è¯»å–å‡½æ•°
//		è¿”å›ž1è¡¨ç¤ºç³»ç»Ÿæ—¶é—´å·²è®¾ç½®
//		è¿”å›ž0è¡¨ç¤ºç³»ç»Ÿæ—¶é—´æœªè®¾ç½®ï¼Œç³»ç»Ÿè¿”å›žç¼ºçœå®šä¹‰æ—¶é—´
int	XM_GetLocalTime	(XMSYSTEMTIME* pSystemTime)
{
	SystemTime_t t;
	iGetLocalTime (&t);
	pSystemTime->wDay = t.tm_mday;
	pSystemTime->wDayOfWeek = t.tm_wday;
	pSystemTime->wHour = t.tm_hour;
	pSystemTime->wMilliseconds = 0;
	pSystemTime->wMinute = t.tm_min;
	pSystemTime->wMonth = t.tm_mon;
	pSystemTime->wSecond = t.tm_sec;
	pSystemTime->wYear = t.tm_year;
	return 1;
}
//		è¿”å›ž1è®¾ç½®ç³»ç»Ÿæ—¶é—´æˆåŠŸ
//    è¿”å›ž0è®¾ç½®ç³»ç»Ÿæ—¶é—´å¤±è´¥
int	XM_SetLocalTime	(const XMSYSTEMTIME *pSystemTime)
{
	SystemTime_t t;
	t.tm_mday = pSystemTime->wDay;
	t.tm_wday = pSystemTime->wDayOfWeek ;
	t.tm_hour = pSystemTime->wHour;
	t.tm_min = pSystemTime->wMinute;
	t.tm_mon = pSystemTime->wMonth;
	t.tm_sec = pSystemTime->wSecond;
	t.tm_year = pSystemTime->wYear;
	vSetLocalTime (&t);
	return 1;
}

DWORD XM_GetTickCount (void)
{
	return OS_GetTime();
}

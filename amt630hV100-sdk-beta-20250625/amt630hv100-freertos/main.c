/*
 * FreeRTOS V202104.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting (defined in this file) is used to
 * select between the two.  The simply blinky demo is implemented and described
 * in main_blinky.c.  The more comprehensive test and demo application is
 * implemented and described in main_full.c.
 *
 * This file implements the code that is not demo specific, including the
 * hardware setup, standard FreeRTOS hook functions, and the ISR hander called
 * by the RTOS after interrupt entry (including nesting) has been taken care of.
 *
 * ENSURE TO READ THE DOCUMENTATION PAGE FOR THIS PORT AND DEMO APPLICATION ON
 * THE http://www.FreeRTOS.org WEB SITE FOR FULL INFORMATION ON USING THIS DEMO
 * APPLICATION, AND ITS ASSOCIATE FreeRTOS ARCHITECTURE PORT!
 *
 */

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Standard demo includes. */
#include "partest.h"
#include "TimerDemo.h"
#include "QueueOverwrite.h"
#include "EventGroupsDemo.h"

/* Library includes. */
#include "board.h"
#include "chip.h"
#ifdef ADC_TOUCH
#include "touch.h"
#endif
#ifdef ADC_KEY
#include "keypad.h"
#endif
//#define mainCREATE_SIMPLE_BLINKY_DEMO

#if defined(AWTK)
#define mainCREATE_AWTK_DEMO
#elif defined(VG_ONLY)
#define mainCREATE_OPENVG_DEMO
#else
#define mainCREATE_LVGL_DEMO
#endif

#if defined(mainCREATE_LVGL_DEMO)
extern int main_lvgl(void);
#elif defined(mainCREATE_AWTK_DEMO)
extern int main_awtk(void);
#elif defined(mainCREATE_OPENVG_DEMO)
extern void main_openvg( void );
#elif defined(mainCREATE_SIMPLE_BLINKY_DEMO)
extern void main_blinky( void );
#endif
#if defined(AUDIO_REPLAY) || defined(AUDIO_RECORD)
extern int sound_init(void);
#endif

/*-----------------------------------------------------------*/

/*
 * Configure the hardware as necessary to run this demo.
 */
static void prvSetupHardware( void );


/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file. */
void vApplicationMallocFailedHook( size_t size );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

/* Prototype for the IRQ handler called by the generic Cortex-A5 RTOS port
layer. */
void vApplicationIRQHandler( void );


#ifdef WIFI_SUPPORT
#if 0
static void wifi_reset(void)
{
	gpio_direction_output(WIFI_BT_PWR_GPIO, 1);
	gpio_direction_output(WIFI_RESET_IO, 1);
	mdelay(10);
	gpio_direction_output(WIFI_RESET_IO, 0);
	mdelay(20);
	gpio_direction_output(WIFI_RESET_IO, 1);
	mdelay(10);
}

static void wifi_config(void)
{
	// wifi power off	
	gpio_direction_output(WIFI_BT_PWR_GPIO, 1);
	// set the wifi IOs to input mode 	
	gpio_direction_output(WIFI_BT_SDO_CMD_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_CLK_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D3_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D2_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D1_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D0_GPIO, 0);
	gpio_direction_output(WIFI_BT_UART_TX_GPIO, 0);
	gpio_direction_output(WIFI_BT_UART_RX_GPIO, 0);
	gpio_direction_output(WIFI_BT_UART_RX_GPIO, 0);
	gpio_direction_output(WIFI_RESET_IO, 0);
	gpio_direction_output(BT_RESET_IO, 0);
	vTaskDelay(pdMS_TO_TICKS(500));

	gpio_direction_output(WIFI_BT_SDO_CMD_GPIO, 1);
	gpio_direction_output(WIFI_BT_SDO_D3_GPIO,  1);
	gpio_direction_output(WIFI_BT_SDO_D2_GPIO,  1);
	gpio_direction_output(WIFI_BT_SDO_D1_GPIO,  1);
	gpio_direction_output(WIFI_BT_SDO_D0_GPIO,  1);
	gpio_direction_output(WIFI_BT_UART_TX_GPIO, 1);
	gpio_direction_output(WIFI_BT_UART_RX_GPIO, 1);
	gpio_direction_output(WIFI_RESET_IO, 1);
	gpio_direction_output(BT_RESET_IO, 1);
	vPinctrlSetup();
	mdelay(250);
}
#endif

#endif

static void prvBoardLateInitTask( void *pvParameters )
{
	ark_lcd_enable(1);

#ifdef SDMMC_SUPPORT
	mmcsd_core_init();
#endif

#ifdef WIFI_SUPPORT
	gpio_direction_output(WIFI_BT_SDO_CMD_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_CLK_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D3_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D2_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D1_GPIO, 0);
	gpio_direction_output(WIFI_BT_SDO_D0_GPIO, 0);
	gpio_direction_output(WIFI_BT_UART_TX_GPIO, 0);
	gpio_direction_output(WIFI_BT_UART_RX_GPIO, 0);
	gpio_direction_output(WIFI_BT_UART1_CTS_GPIO, 0);
	gpio_direction_output(WIFI_BT_UART1_RTS_GPIO, 0);
#else
	mmc_init();
#endif

#ifdef ADC_TOUCH
	adc_channel_enable(ADC_CH_TP);
	/* tp enable */
	vSysctlConfigure(SYS_ANA1_CFG, 26, 1, 1);
	if (LoadTouchConfigure()) {
		AdjustTouch();
	}
#endif

#ifdef ADC_KEY
	KeypadInit();
#endif

#if defined(AUDIO_REPLAY) || defined(AUDIO_RECORD)
	sound_init();
#endif

#ifdef USB_SUPPORT
	extern int ark_usb_init(void);
	ark_usb_init();
#endif

/* you'd better init the rtc driver at last */
#ifdef RTC_SUPPORT
	rtc_init();
	SystemTime_t tm;
	iGetLocalTime(&tm);
	printf("Time: %d-%.2d-%.2d %.2d:%.2d:%.2d.\n", tm.tm_year, tm.tm_mon,
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif

	for (;;) {
		vTaskDelay(portMAX_DELAY);
	}
}

/*-----------------------------------------------------------*/
int main( void )
{
	/* Configure the hardware ready to run the demo. */
	prvSetupHardware();

	/* lateinit the hardware which needs call task funcs */
	xTaskCreate(prvBoardLateInitTask, "lateinit", configMINIMAL_STACK_SIZE * 10, NULL,
		configMAX_PRIORITIES - 1, NULL);

#if defined(mainCREATE_LVGL_DEMO) && LCD_INTERFACE_TYPE != LCD_INTERFACE_CPU
	main_lvgl();
#elif defined(mainCREATE_AWTK_DEMO)
	main_awtk();
#elif defined(mainCREATE_OPENVG_DEMO)
	main_openvg();
#elif defined(mainCREATE_SIMPLE_BLINKY_DEMO)
	main_blinky();
#endif

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details. */
	for( ;; );
}
/*-----------------------------------------------------------*/

#if defined(__ICCARM__) || defined(__ICCRX__)         /* for IAR compiler */
#pragma data_alignment=16*1024
static uint32_t MMUTable[4*1024];
#else
static uint32_t MMUTable[4*1024] __attribute__((aligned(16*1024)));
#endif
static void prvSetupHardware( void )
{
    /* Disable watchdog */
    //WDT_Disable( WDT );

	/* Setup pinctrl */
	vPinctrlSetup();

	/* Setup clock */
	vClkInit();

	/* Configure a timer for delay */
	vInitialiseTimerForDelay();

	/* Initialize interrupt contorller */
	AIC_Initialize();

	/* Initialize Debug Uart */
	vDebugConsoleInitialise();

	/* Configure ports used by LEDs. */
	//vParTestInitialise();

	/* enable cache */
	MMU_Initialize(MMUTable);
	CP15_EnableMMU();
	CP15_EnableDcache();
	CP15_EnableIcache();
#if 1
	printf("sys_pll=%d\n", ulClkGetRate(CLK_SYSPLL));
	printf("cpu_pll=%d\n", ulClkGetRate(CLK_CPUPLL));
	printf("vpu_pll=%d\n", ulClkGetRate(CLK_VPUPLL));
	printf("ddr_pll=%d\n", ulClkGetRate(CLK_DDRPLL));
	printf("cpu_clk=%d\n", ulClkGetRate(CLK_CPU));
	printf("ddr_clk=%d\n", ulClkGetRate(CLK_DDR));
	printf("ahb_clk=%d\n", ulClkGetRate(CLK_AHB));
	printf("apb_clk=%d\n", ulClkGetRate(CLK_APB));
	printf("mmc_clk=%d\n", ulClkGetRate(CLK_SDMMC0));
	printf("mfc_clk=%d\n", ulClkGetRate(CLK_MFC));
#endif

	dma_init();
	spi_init();
	i2c_init();
	wdt_init();
	vdec_init();
	itu_init();
	pxp_init();
	VideoDisplayBufInit();
	adc_init();
#if LCD_INTERFACE_TYPE == LCD_INTERFACE_CPU
	Cpulcd_Init();
#elif LCD_INTERFACE_TYPE == LCD_INTERFACE_MIPI
	extern void mipi_init(void);
	mipi_init();
	lcd_init();
#else
	lcd_init();
#endif
#ifdef ADC_TOUCH
	TouchInit();
#endif
#ifdef REMOTE_SUPPORT
	RemoteKeyInit();
#endif
	blend2d_init();
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( size_t size )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	HeapStats_t heapStat;
	vPortGetHeapStats(&heapStat);
	printf("ERROR! Malloc %d bytes fail.\n", size);
	printf("xAvailableHeapSpaceInBytes %d.\n", heapStat.xAvailableHeapSpaceInBytes);
	printf("xSizeOfLargestFreeBlockInBytes %d.\n", heapStat.xSizeOfLargestFreeBlockInBytes);

	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */

	printf("ERROR! task %s stack over flow.\n", pcTaskName);

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	static uint32_t idletick = 0;
	volatile size_t xFreeHeapSpace;

	/* This is just a trivial example of an idle hook.  It is called on each
	cycle of the idle task.  It must *NOT* attempt to block.  In this case the
	idle task just queries the amount of FreeRTOS heap that remains.  See the
	memory management section on the http://www.FreeRTOS.org web site for memory
	management options.  If there is a lot of heap memory free then the
	configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
	RAM. */
	xFreeHeapSpace = xPortGetFreeHeapSize();

	/* Remove compiler warning about xFreeHeapSpace being set but never used. */
	( void ) xFreeHeapSpace;

	if (xTaskGetTickCount() - idletick > configTICK_RATE_HZ * 10) {
		printf("Memory free: %d bytes.\n", xFreeHeapSpace);
		idletick = xTaskGetTickCount();
	}
}
/*-----------------------------------------------------------*/

void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
volatile unsigned long ul = 0;

	( void ) pcFile;
	( void ) ulLine;
	printf("crash %s : %d", pcFile, ulLine);
	taskENTER_CRITICAL();
	{
		/* Set ul to a non-zero value using the debugger to step out of this
		function. */
		while( ul == 0 )
		{
			portNOP();
		}
	}
	taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{

}
/*-----------------------------------------------------------*/

/* The function called by the RTOS port layer after it has managed interrupt
entry. */
void vApplicationIRQHandler( void )
{
	AIC_IrqHandler();
	__DSB();
	__ISB();
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}


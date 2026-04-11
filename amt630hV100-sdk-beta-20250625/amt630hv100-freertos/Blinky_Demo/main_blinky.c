/*
 * FreeRTOS Kernel V10.3.1
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
 * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.  Console output
 * is used in place of the normal LED toggling.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 milliseconds (please read the notes
 * above regarding the accuracy of timing under Windows).
 *
 * The Queue Send Software Timer:
 * The timer is a one-shot timer that is reset by a key press.  The timer's
 * period is set to two seconds - if the timer expires then its callback
 * function writes the value 200 to the queue.  The callback function is
 * implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 *
 * Expected Behaviour:
 * - The queue send task writes to the queue every 200ms, so every 200ms the
 *   queue receive task will output a message indicating that data was received
 *   on the queue from the queue send task.
 * - The queue send software timer has a period of two seconds, and is reset
 *   each time a key is pressed.  So if two seconds expire without a key being
 *   pressed then the queue receive task will output a message indicating that
 *   data was received on the queue from the queue send software timer.
 *
 * NOTE:  Console input and output relies on Windows system calls, which can
 * interfere with the execution of the FreeRTOS Windows port.  This demo only
 * uses Windows system call occasionally.  Heavier use of Windows system calls
 * can crash the port.
 */

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "chip.h"
#include "sfud.h"

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 50000UL )
#define mainTIMER_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 20000UL )

/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH					( 2 )

/* The values sent to the queue receive task from the queue send task and the
queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK			( 100UL )
#define mainVALUE_SENT_FROM_TIMER			( 200UL )

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );

/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/

/*
 * Register commands that can be used with FreeRTOS+CLI.  The commands are
 * defined in CLI-Commands.c and File-Related-CLI-Command.c respectively.
 */
extern void vRegisterSampleCLICommands( void );

/*
 * The task that manages the FreeRTOS+CLI input and output.
 */
extern void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );


/*-----------------------------------------------------------*/

static int im32x_i2c_write (struct i2c_adapter *adap, unsigned int addr, unsigned char data)
{
	struct i2c_msg msg;
	int ret = -1;
	u8 retries = 0;
	u8 buf[3];

	buf[0] = ((addr>>8)&0xFF);
	buf[1] = (addr&0xFF);
	buf[2] = (data&0xFF);

	msg.flags = !I2C_M_RD;
	msg.addr  = 0x1a;
	msg.len   = sizeof(buf);
	msg.buf   = buf;
	
	while(retries < 5)
	{
		ret = i2c_transfer(adap, &msg, 1);
		if (ret == 1)
			break;
		retries++;
	}
		
	if (retries >= 5)
	{
		printf("%s timeout\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

static unsigned int im32x_i2c_read(struct i2c_adapter *adap, unsigned int addr)
{
	struct i2c_msg msgs[2];
	int retries = 0;
	int ret = -1;
	u8 buf[2];

	buf[0] = (addr>>8)&0xFF;
	buf[1] = addr&0xFF;
	
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = 0x1a;
	msgs[0].len   = 2;
	msgs[0].buf   = buf;
	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = 0x1a;
	msgs[1].len   = 1;
	msgs[1].buf   = buf;
	
	while(retries < 5)
	{
		ret = i2c_transfer(adap, msgs, 2);
		if(ret == 2)
			break;
		retries++;
	}
	
	if (retries >= 5)
	{
		printf( "%s timeout\n", __FUNCTION__);
		return 0;
	}

	return buf[0];
}



static u8 spi_rdata[0x10000];
static u8 spi_wdata[0x10000];
static int dma_finished = 0;

static void tx_callback(void *dma_async_param)
{
	//DEBUG_PRINT("callback here\n");
	printf("tx_callback.\n");
	dma_finished = 1;
}

static void prvDriverTestTask( void *pvParameters )
{
	struct i2c_adapter *adap;
	SystemTime_t tm;
	struct dma_chan *chan;
	struct dma_async_tx_descriptor *tx = NULL;
	dma_cookie_t dma_cookie;
	sfud_flash *sflash;
	int i;
	
	/* initialize the spi flash */
	sfud_init();

	//lcd_init();
	//rtc_init();
	//dw_dma_init();

	tm.tm_year = 2020;
	tm.tm_mon = 9;
	tm.tm_mday = 1;
	tm.tm_hour = 13;
	tm.tm_min = 40;
	tm.tm_sec = 0;
	vSetLocalTime(&tm);

	sflash = sfud_get_device(0);
	sfud_erase(sflash, 0, 0x10000);
	sfud_read(sflash, 0x0, 0x10000, spi_rdata);
	for (i = 0; i < 0x10000; i++)
		spi_wdata[i] = rand();
	sfud_write(sflash, 0, 0x10000, spi_wdata);
	sfud_read(sflash, 0, 0x10000, spi_rdata);
	for (i = 0; i < 0x10000; i++) {
		if (spi_rdata[i] != spi_wdata[i]) {
			printf("spi data compare err 0x%x, 0x%x.\n", spi_wdata[i], spi_rdata[i]);
		}
	}
#if 0
	for (i = 0; i < 0x10000; i++) {
		spi_rdata[i] = rand();
		spi_wdata[i] = 0;
	}
	chan = dma_request_channel(NULL);
	CP15_clean_dcache_for_dma((u32)spi_rdata, (u32)spi_rdata + 0x10000);
	CP15_invalidate_dcache_for_dma((u32)spi_wdata, (u32)spi_wdata + 0x10000);
	tx = dmaengine_prep_dma_memcpy(chan, (u32)spi_wdata, (u32)spi_rdata, 
		0x10000, DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if(NULL == tx) {
		dma_release_channel(chan);
	} else {
		tx->callback = tx_callback;
		dma_cookie = dmaengine_submit(tx);
		if (dma_submit_error(dma_cookie)) {
			printf("Failed to do DMA tx_submit");
		}
		dma_async_issue_pending(chan);	
		while(!dma_finished)
			vTaskDelay(pdMS_TO_TICKS(10));
		dma_finished = 0;
		dma_release_channel(chan);
	}
	for (i = 0; i < 0x10000; i++) {
		if (spi_rdata[i] != spi_wdata[i]) {
			printf("dma data compare err 0x%x, 0x%x.\n", spi_wdata[i], spi_rdata[i]);
		}
	}

	gpio_request(114);
	gpio_direction_output(114, 0);
	vTimerMdelay(1);
	gpio_direction_output(114, 1);
	vTimerMdelay(1);

	for ( ; ; ) {
		adap = i2c_open("i2c0");
		im32x_i2c_write(adap, 0x302C, 0x0);
		im32x_i2c_write(adap, 0x0100, 0x0);
		im32x_i2c_write(adap, 0x0112, 0xc);
		printf("imx322 read 0x%x.\n", im32x_i2c_read(adap, 0x0112));
		i2c_close(adap);
		vTaskDelay(pdMS_TO_TICKS(1000));
		iGetLocalTime(&tm);
		printf("time %d-%.2d-%.2d %.2d:%.2d:%.2d.\n", tm.tm_year, tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
#endif
	for (;;) {
		ark_lcd_wait_for_vsync();
		/* iGetLocalTime(&tm);
		printf("time %d-%.2d-%.2d %.2d:%.2d:%.2d.\n", tm.tm_year, tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec); */
	}
}


/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_blinky( void )
{
const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

	/* Start the task that manages the command console for FreeRTOS+CLI. */
	vUARTCommandConsoleStart( ( configMINIMAL_STACK_SIZE * 3 ), tskIDLE_PRIORITY );

	/* Register the standard CLI commands. */
	vRegisterSampleCLICommands();

	/* Create the queue. */
	xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

	if( xQueue != NULL )
	{
		/* Start the two tasks as described in the comments at the top of this
		file. */
		xTaskCreate( prvQueueReceiveTask,			/* The function that implements the task. */
					"Rx", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
					configMINIMAL_STACK_SIZE, 		/* The size of the stack to allocate to the task. */
					NULL, 							/* The parameter passed to the task - not used in this simple case. */
					mainQUEUE_RECEIVE_TASK_PRIORITY,/* The priority assigned to the task. */
					NULL );							/* The task handle is not required, so NULL is passed. */

		xTaskCreate( prvQueueSendTask, "TX", configMINIMAL_STACK_SIZE, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );

		/* Create the software timer, but don't start it yet. */
		xTimer = xTimerCreate( "Timer",				/* The text name assigned to the software timer - for debug only as it is not used by the kernel. */
								xTimerPeriod,		/* The period of the software timer in ticks. */
								pdFALSE,			/* xAutoReload is set to pdFALSE, so this is a one-shot timer. */
								NULL,				/* The timer's ID is not used. */
								prvQueueSendTimerCallback );/* The function executed when the timer expires. */

		xTimerStart( xTimer, 0 ); /* The scheduler has not started so use a block time of 0. */

		/* Create a task to test driver */
		xTaskCreate( prvDriverTestTask, "test", configMINIMAL_STACK_SIZE * 10, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTask( void *pvParameters )
{
TickType_t xNextWakeTime;
const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil( &xNextWakeTime, xBlockTime );

		/* Send to the queue - causing the queue receive task to unblock and
		write to the console.  0 is used as the block time so the send operation
		will not block - it shouldn't need to block as the queue should always
		have at least one space at this point in the code. */
		xQueueSend( xQueue, &ulValueToSend, 0U );

		uint32_t xTime = xTaskGetTickCount();
		while(xTaskGetTickCount() - xTime < 3000);
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	( void ) xTimerHandle;

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	xQueueSend( xQueue, &ulValueToSend, 0U );
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask( void *pvParameters )
{
uint32_t ulReceivedValue;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h.  It will not use any CPU time while it is in the
		Blocked state. */
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

		/*  To get here something must have been received from the queue, but
		is it an expected value?  Normally calling printf() from a task is not
		a good idea.  Here there is lots of stack space and only one task is
		using console IO so it is ok.  However, note the comments at the top of
		this file about the risks of making Windows system calls (such as
		console output) from a FreeRTOS task. */
		if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
		{
			printf( "Message received from task\r\n" );
		}
		else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
		{
			printf( "Message received from software timer\r\n" );
		}
		else
		{
			printf( "Unexpected message\r\n" );
		}

		/* Reset the timer if a key has been pressed.  The timer will write
		mainVALUE_SENT_FROM_TIMER to the queue when it expires. */

		/* Reset the software timer. */
		xTimerReset( xTimer, portMAX_DELAY );
	}
}
/*-----------------------------------------------------------*/


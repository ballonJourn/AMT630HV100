/*
 * FreeRTOS V202104.00
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 *
 * http://www.FreeRTOS.org/cli
 *
 ******************************************************************************/


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "board.h"
#ifndef  configINCLUDE_TRACE_RELATED_CLI_COMMANDS
	#define configINCLUDE_TRACE_RELATED_CLI_COMMANDS 0
#endif

#ifndef configINCLUDE_QUERY_HEAP_COMMAND
	#define configINCLUDE_QUERY_HEAP_COMMAND 0
#endif

/*
 * The function that registers the commands that are defined within this file.
 */
void vRegisterSampleCLICommands( void );

/*
 * Implements the task-stats command.
 */
static BaseType_t prvTaskStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the run-time-stats command.
 */
#if( configGENERATE_RUN_TIME_STATS == 1 )
	static BaseType_t prvRunTimeStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif /* configGENERATE_RUN_TIME_STATS */

/*
 * Implements the echo-three-parameters command.
 */
static BaseType_t prvThreeParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the echo-parameters command.
 */
static BaseType_t prvParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

#ifdef WIFI_SUPPORT
static BaseType_t prvParameterStartWifiCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t prvParameterPingCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t prvParameterStartBtcoLogCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t prvParameterStartIwprivCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif
static BaseType_t prvParameterIperfCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * open or close system log command.
 */
static BaseType_t prvParameterSysLogCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the "query heap" command.
 */
#if( configINCLUDE_QUERY_HEAP_COMMAND == 1 )
	static BaseType_t prvQueryHeapCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif

/*
 * Implements the "trace start" and "trace stop" commands;
 */
#if( configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1 )
	static BaseType_t prvStartStopTraceCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif

/* Structure that defines the "task-stats" command line command.  This generates
a table that gives information on each task in the system. */
static const CLI_Command_Definition_t xTaskStats =
{
	"task-stats", /* The command string to type. */
	"\r\ntask-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n",
	prvTaskStatsCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "echo_3_parameters" command line command.  This
takes exactly three parameters that the command simply echos back one at a
time. */
static const CLI_Command_Definition_t xThreeParameterEcho =
{
	"echo-3-parameters",
	"\r\necho-3-parameters <param1> <param2> <param3>:\r\n Expects three parameters, echos each in turn\r\n",
	prvThreeParameterEchoCommand, /* The function to run. */
	3 /* Three parameters are expected, which can take any value. */
};

/* Structure that defines the "echo_parameters" command line command.  This
takes a variable number of parameters that the command simply echos back one at
a time. */
static const CLI_Command_Definition_t xParameterEcho =
{
	"echo-parameters",
	"\r\necho-parameters <...>:\r\n Take variable number of parameters, echos each in turn\r\n",
	prvParameterEchoCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

#if( configGENERATE_RUN_TIME_STATS == 1 )
	/* Structure that defines the "run-time-stats" command line command.   This
	generates a table that shows how much run time each task has */
	static const CLI_Command_Definition_t xRunTimeStats =
	{
		"run-time-stats", /* The command string to type. */
		"\r\nrun-time-stats:\r\n Displays a table showing how much processing time each FreeRTOS task has used\r\n",
		prvRunTimeStatsCommand, /* The function to run. */
		0 /* No parameters are expected. */
	};
#endif /* configGENERATE_RUN_TIME_STATS */

#if( configINCLUDE_QUERY_HEAP_COMMAND == 1 )
	/* Structure that defines the "query_heap" command line command. */
	static const CLI_Command_Definition_t xQueryHeap =
	{
		"query-heap",
		"\r\nquery-heap:\r\n Displays the free heap space, and minimum ever free heap space.\r\n",
		prvQueryHeapCommand, /* The function to run. */
		0 /* The user can enter any number of commands. */
	};
#endif /* configQUERY_HEAP_COMMAND */

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
	/* Structure that defines the "trace" command line command.  This takes a single
	parameter, which can be either "start" or "stop". */
	static const CLI_Command_Definition_t xStartStopTrace =
	{
		"trace",
		"\r\ntrace [start | stop]:\r\n Starts or stops a trace recording for viewing in FreeRTOS+Trace\r\n",
		prvStartStopTraceCommand, /* The function to run. */
		1 /* One parameter is expected.  Valid values are "start" and "stop". */
	};
#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */

#ifdef WIFI_SUPPORT
	static const CLI_Command_Definition_t xStartWifi =
	{
		"startwifi",
		"\r\nstartwifi [ap | sta]:\r\n Starts AP or STA in FreeRTOS+Trace\r\n",
		prvParameterStartWifiCommand, /* The function to run. */
		0 /* One parameter is expected.  Valid values are "start" and "stop". */
	};
    static const CLI_Command_Definition_t xPingCmd =
	{
		"ping",
		"\r\nping [192.168.13.20]:\r\n ping remote device in FreeRTOS+Trace\r\n",
		prvParameterPingCommand, /* The function to run. */
		1 /* One parameter is expected.  Valid values are "start" and "stop". */
	};
	static const CLI_Command_Definition_t xStartBtCoLog =
	{
		"startbtcolog",
		"\r\nstartbtcolog:\r\n Starts btco log in FreeRTOS+Trace\r\n",
		prvParameterStartBtcoLogCommand, /* The function to run. */
		0 /* One parameter is expected.  Valid values are "start" and "stop". */
	};

	static const CLI_Command_Definition_t xStartIwprivCmd =
	{
		"iwpriv",
		"\r\niwpriv dbg [flow | rx] :\r\n Starts btco log in FreeRTOS+Trace\r\n",
		prvParameterStartIwprivCommand, /* The function to run. */
		0
	};
#endif
	static const CLI_Command_Definition_t xIperfCmd =
	{//��iperfֻ����������ͨwifi��Ƶ����
		"iperf",
		"\r\n iperf [ip] [port]:\r\n Starts iperf client\r\n",
		prvParameterIperfCommand, /* The function to run. */
		1 /* One parameter is expected.  Valid values are "start" and "stop". */
	};

	static const CLI_Command_Definition_t xSysLogCmd =
	{
		"sysLog",
		"\r\n syslog [0|1], open or close syslog\r\n",
		prvParameterSysLogCommand, /* The function to run. */
		1 /* One parameter is expected.*/
	};
/*-----------------------------------------------------------*/

void vRegisterSampleCLICommands( void )
{
	/* Register all the command line commands defined immediately above. */
	FreeRTOS_CLIRegisterCommand( &xTaskStats );	
	FreeRTOS_CLIRegisterCommand( &xThreeParameterEcho );
	FreeRTOS_CLIRegisterCommand( &xParameterEcho );
	
	#if( configGENERATE_RUN_TIME_STATS == 1 )
	{
		FreeRTOS_CLIRegisterCommand( &xRunTimeStats );
	}
	#endif
	
	#if( configINCLUDE_QUERY_HEAP_COMMAND == 1 )
	{
		FreeRTOS_CLIRegisterCommand( &xQueryHeap );
	}
	#endif

	#if( configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1 )
	{
		FreeRTOS_CLIRegisterCommand( &xStartStopTrace );
	}
	#endif
#ifdef WIFI_SUPPORT
    FreeRTOS_CLIRegisterCommand( &xStartWifi );
	FreeRTOS_CLIRegisterCommand( &xPingCmd );
	FreeRTOS_CLIRegisterCommand( &xStartBtCoLog);
	FreeRTOS_CLIRegisterCommand( &xStartIwprivCmd);
#endif
	FreeRTOS_CLIRegisterCommand( &xIperfCmd);
	FreeRTOS_CLIRegisterCommand( &xSysLogCmd );
}
/*-----------------------------------------------------------*/

static BaseType_t prvTaskStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *const pcHeader = "     State   Priority  Stack    #\r\n************************************************\r\n";
BaseType_t xSpacePadding;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Generate a table of task stats. */
	strcpy( pcWriteBuffer, "Task" );
	pcWriteBuffer += strlen( pcWriteBuffer );

	/* Minus three for the null terminator and half the number of characters in
	"Task" so the column lines up with the centre of the heading. */
	configASSERT( configMAX_TASK_NAME_LEN > 3 );
	for( xSpacePadding = strlen( "Task" ); xSpacePadding < ( configMAX_TASK_NAME_LEN - 3 ); xSpacePadding++ )
	{
		/* Add a space to align columns after the task's name. */
		*pcWriteBuffer = ' ';
		pcWriteBuffer++;

		/* Ensure always terminated. */
		*pcWriteBuffer = 0x00;
	}
	strcpy( pcWriteBuffer, pcHeader );
	vTaskList( pcWriteBuffer + strlen( pcHeader ) );

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

#if( configINCLUDE_QUERY_HEAP_COMMAND == 1 )

	static BaseType_t prvQueryHeapCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
	{
		/* Remove compile time warnings about unused parameters, and check the
		write buffer is not NULL.  NOTE - for simplicity, this example assumes the
		write buffer length is adequate, so does not check for buffer overflows. */
		( void ) pcCommandString;
		( void ) xWriteBufferLen;
		configASSERT( pcWriteBuffer );

		sprintf( pcWriteBuffer, "Current free heap %d bytes, minimum ever free heap %d bytes\r\n", ( int ) xPortGetFreeHeapSize(), ( int ) xPortGetMinimumEverFreeHeapSize() );

		/* There is no more data to return after this single string, so return
		pdFALSE. */
		return pdFALSE;
	}

#endif /* configINCLUDE_QUERY_HEAP */
/*-----------------------------------------------------------*/

#if( configGENERATE_RUN_TIME_STATS == 1 )
	
	static BaseType_t prvRunTimeStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
	{
	const char * const pcHeader = "  Abs Time      % Time\r\n****************************************\r\n";
	BaseType_t xSpacePadding;

		/* Remove compile time warnings about unused parameters, and check the
		write buffer is not NULL.  NOTE - for simplicity, this example assumes the
		write buffer length is adequate, so does not check for buffer overflows. */
		( void ) pcCommandString;
		( void ) xWriteBufferLen;
		configASSERT( pcWriteBuffer );

		/* Generate a table of task stats. */
		strcpy( pcWriteBuffer, "Task" );
		pcWriteBuffer += strlen( pcWriteBuffer );

		/* Pad the string "task" with however many bytes necessary to make it the
		length of a task name.  Minus three for the null terminator and half the
		number of characters in	"Task" so the column lines up with the centre of
		the heading. */
		for( xSpacePadding = strlen( "Task" ); xSpacePadding < ( configMAX_TASK_NAME_LEN - 3 ); xSpacePadding++ )
		{
			/* Add a space to align columns after the task's name. */
			*pcWriteBuffer = ' ';
			pcWriteBuffer++;

			/* Ensure always terminated. */
			*pcWriteBuffer = 0x00;
		}

		strcpy( pcWriteBuffer, pcHeader );
		vTaskGetRunTimeStats( pcWriteBuffer + strlen( pcHeader ) );

		/* There is no more data to return after this single string, so return
		pdFALSE. */
		return pdFALSE;
	}
	
#endif /* configGENERATE_RUN_TIME_STATS */
/*-----------------------------------------------------------*/

static BaseType_t prvThreeParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *pcParameter;
BaseType_t xParameterStringLength, xReturn;
static UBaseType_t uxParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	if( uxParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		sprintf( pcWriteBuffer, "The three parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		uxParameterNumber = 1U;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
						(
							pcCommandString,		/* The command string itself. */
							uxParameterNumber,		/* Return the next parameter. */
							&xParameterStringLength	/* Store the parameter string length. */
						);

		/* Sanity check something was returned. */
		configASSERT( pcParameter );

		/* Return the parameter string. */
		memset( pcWriteBuffer, 0x00, xWriteBufferLen );
		sprintf( pcWriteBuffer, "%d: ", ( int ) uxParameterNumber );
		strncat( pcWriteBuffer, pcParameter, ( size_t ) xParameterStringLength );
		strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

		/* If this is the last of the three parameters then there are no more
		strings to return after this one. */
		if( uxParameterNumber == 3U )
		{
			/* If this is the last of the three parameters then there are no more
			strings to return after this one. */
			xReturn = pdFALSE;
			uxParameterNumber = 0;
		}
		else
		{
			/* There are more parameters to return after this one. */
			xReturn = pdTRUE;
			uxParameterNumber++;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *pcParameter;
BaseType_t xParameterStringLength, xReturn;
static UBaseType_t uxParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	if( uxParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		sprintf( pcWriteBuffer, "The parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		uxParameterNumber = 1U;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
						(
							pcCommandString,		/* The command string itself. */
							uxParameterNumber,		/* Return the next parameter. */
							&xParameterStringLength	/* Store the parameter string length. */
						);

		if( pcParameter != NULL )
		{
			/* Return the parameter string. */
			memset( pcWriteBuffer, 0x00, xWriteBufferLen );
			sprintf( pcWriteBuffer, "%d: ", ( int ) uxParameterNumber );
			strncat( pcWriteBuffer, ( char * ) pcParameter, ( size_t ) xParameterStringLength );
			strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

			/* There might be more parameters to return after this one. */
			xReturn = pdTRUE;
			uxParameterNumber++;
		}
		else
		{
			/* No more parameters were found.  Make sure the write buffer does
			not contain a valid string. */
			pcWriteBuffer[ 0 ] = 0x00;

			/* No more data to return. */
			xReturn = pdFALSE;

			/* Start over the next time this command is executed. */
			uxParameterNumber = 0;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1

	static BaseType_t prvStartStopTraceCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
	{
	const char *pcParameter;
	BaseType_t lParameterStringLength;

		/* Remove compile time warnings about unused parameters, and check the
		write buffer is not NULL.  NOTE - for simplicity, this example assumes the
		write buffer length is adequate, so does not check for buffer overflows. */
		( void ) pcCommandString;
		( void ) xWriteBufferLen;
		configASSERT( pcWriteBuffer );

		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
						(
							pcCommandString,		/* The command string itself. */
							1,						/* Return the first parameter. */
							&lParameterStringLength	/* Store the parameter string length. */
						);

		/* Sanity check something was returned. */
		configASSERT( pcParameter );

		/* There are only two valid parameter values. */
		if( strncmp( pcParameter, "start", strlen( "start" ) ) == 0 )
		{
			/* Start or restart the trace. */
			vTraceStop();
			vTraceClear();
			vTraceStart();

			sprintf( pcWriteBuffer, "Trace recording (re)started.\r\n" );
		}
		else if( strncmp( pcParameter, "stop", strlen( "stop" ) ) == 0 )
		{
			/* End the trace, if one is running. */
			vTraceStop();
			sprintf( pcWriteBuffer, "Stopping trace recording.\r\n" );
		}
		else
		{
			sprintf( pcWriteBuffer, "Valid parameters are 'start' and 'stop'.\r\n" );
		}

		/* There is no more data to return after this single string, so return
		pdFALSE. */
		return pdFALSE;
	}

#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */

#ifdef WIFI_SUPPORT
#include "iot_wifi.h"
static BaseType_t prvParameterStartWifiCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,		/* The command string itself. */
    				1,						/* Return the first parameter. */
    				&lParameterStringLength	/* Store the parameter string length. */
    				);

    /* Sanity check something was returned. */
    configASSERT( pcParameter );

	/* There are only two valid parameter values. */
    if( strncmp( pcParameter, "ap", strlen( "ap" ) ) == 0 )
    {
        //printf("\r\nstart ap\r\n");
        start_ap(36, "ap63011", "88888888", 1);
    }
    else if( strncmp( pcParameter, "sta", strlen( "sta" ) ) == 0 )
    {
        int pssid_len = 0, ppwd_len = 0;
		char ssid[64] = {0}, passwd[64] = {0};
		const char *pssid = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,
    				2,
    				(BaseType_t *)&pssid_len
    				);
		memcpy(ssid, pssid, pssid_len);
		const char *ppwd = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,
    				3,
    				(BaseType_t *)&ppwd_len
    				);
		memcpy(passwd, ppwd, ppwd_len);
		//printf("\r\nssid:[%s] len:%d passwd:[%s] ppwd_len:%d\r\n", pssid, pssid_len, ppwd, ppwd_len);
        start_sta(ssid, passwd, 1);
    }
    else
    {
        printf("\r\ninvalid parameters\r\n");
    }

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

static BaseType_t prvParameterPingCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,		/* The command string itself. */
    				1,						/* Return the first parameter. */
    				&lParameterStringLength	/* Store the parameter string length. */
    				);

    /* Sanity check something was returned. */
    configASSERT( pcParameter );

    start_ping(pcParameter);

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

static BaseType_t prvParameterStartBtcoLogCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    //const char *pcParameter;
    //BaseType_t lParameterStringLength;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );
#if 0
    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,		/* The command string itself. */
    				1,						/* Return the first parameter. */
    				&lParameterStringLength	/* Store the parameter string length. */
    				);

    /* Sanity check something was returned. */
    configASSERT( pcParameter );
#endif
    enable_btco_log();

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

void cmd_test(const char* temp_uart_buf);//for wifi iwpriv2a��?
static BaseType_t prvParameterStartIwprivCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    //const char *pcParameter;
    //BaseType_t lParameterStringLength;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );
#if 0
    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,		/* The command string itself. */
    				2,						/* Return the first parameter. */
    				&lParameterStringLength	/* Store the parameter string length. */
    				);

    /* Sanity check something was returned. */
    configASSERT( pcParameter );
#endif
	cmd_test(pcCommandString);

	sprintf( pcWriteBuffer, "\r\n %s started.\r\n" , pcCommandString);
    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}
#endif

#if !USE_LWIP
void start_iperf_client(const char* ip, int port);
#else
#include "ethernet.h"
#include "tcpip.h"
#include "lwip/apps/lwiperf.h"

static void* iperf_client_handle = NULL;

#define lwip_addr_converter( ucOctet0, ucOctet1, ucOctet2, ucOctet3 ) \
    ( ( ( ( uint32_t ) ( ucOctet3 ) ) << 24UL ) |                                  \
      ( ( ( uint32_t ) ( ucOctet2 ) ) << 16UL ) |                                  \
      ( ( ( uint32_t ) ( ucOctet1 ) ) << 8UL ) |                                   \
      ( ( uint32_t ) ( ucOctet0 ) ) )
static void lwiperf_client_report_cb_impl(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
	printf("lwiperf_report_cb_impl bytes:%d %d ms \r\n", bytes_transferred, ms_duration);
}
static void start_iperf_client(const char* ip, short port)
{
	ip_addr_t  remote_addr;
	char addr_buf[4] = {0};

	sscanf(ip, "%d.%d.%d.%d", (int*)&addr_buf[0], (int*)&addr_buf[1], (int*)&addr_buf[2], (int*)&addr_buf[3]);

	remote_addr.addr = lwip_addr_converter(addr_buf[0], addr_buf[1], addr_buf[2], addr_buf[3]);
	iperf_client_handle = lwiperf_start_tcp_client(&remote_addr, (u16_t)port, LWIPERF_CLIENT, lwiperf_client_report_cb_impl, NULL);
}
#endif
static BaseType_t prvParameterIperfCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,		/* The command string itself. */
    				1,						/* Return the first parameter. */
    				&lParameterStringLength	/* Store the parameter string length. */
    				);

    /* Sanity check something was returned. */
    //configASSERT( pcParameter );

	if( strncmp( pcParameter, "client", strlen( "client" ) ) == 0 )
    {
        printf("\r\n iperf client test start \r\n");
		int addr_str_len = 0, port_str_len = 0;
		char addr_str[64] = {0}, port_str[64] = {0};
		int port = 0;
		const char *paddr_str = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,
    				2,
    				(BaseType_t *)&addr_str_len
    				);
		memcpy(addr_str, paddr_str, addr_str_len);
		const char *pport_str = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,
    				3,
    				(BaseType_t *)&port_str_len
    				);
		memcpy(port_str, pport_str, port_str_len);
		port = atoi(port_str);
		printf("\r\n addr:[%s] len:%d port str:[%s] port len:%d port:%d\r\n", addr_str, addr_str_len, port_str, port_str_len, port);

		start_iperf_client(addr_str, port);
        
        printf("\r\n iperf client test end\r\n");
    }
	else if( strncmp( pcParameter, "stop", strlen( "stop" ) ) == 0 )
    {
    	#if USE_LWIP
		if (iperf_client_handle)
			lwiperf_abort(iperf_client_handle);
		#endif
	}
    else
    {
        printf("\r\ninvalid parameters\r\n");
    }
    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

static BaseType_t prvParameterSysLogCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t lParameterStringLength;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter
    				(
    				pcCommandString,		/* The command string itself. */
    				1,						/* Return the first parameter. */
    				&lParameterStringLength	/* Store the parameter string length. */
    				);


    /* Sanity check something was returned. */
    configASSERT( pcParameter );
	pcWriteBuffer[ 0 ] = 0x00;  //clear out buffer
	/* There are only two valid parameter values. */

	extern void set_system_log(uint8_t status);

	if (strncmp(pcParameter, "0", strlen("0")) == 0)
	{
		printf("\r\ncmd system log close\r\n");
		set_system_log(0);
	}
	else if (strncmp(pcParameter, "1", strlen("1")) == 0)
	{
		set_system_log(1);
		printf("\r\ncmd system log open\r\n");
	}
    else 	 
    {
		printf("Invalid cmd:%s\n", pcCommandString);
    }

    /* There is no more data to return after this single string, so return
    pdFALSE. */
    return pdFALSE;
}

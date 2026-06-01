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

#if ENABLE_BD_USB_DVR_FUNC
static BaseType_t prvDVRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t prvDVRStatusCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t prvDVRViewCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t prvDVRDisplayCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t prvDVRPreviewCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif

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
		0 /* One parameter is expected.  Valid values are "start" and "stop". */
	};

#if ENABLE_BD_USB_DVR_FUNC
/* Structure that defines the "dvr" command line command. */
static const CLI_Command_Definition_t xDVRCommand =
{
	"dvr",
	"\r\ndvr [getid|recstart|recstop|snap|sos|getlist|pbstart|pbpause|pbstop|getsts] [mode] [index]:\r\n DVR control\r\n",
	prvDVRCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

/* Structure that defines the "dvrstatus" command line command. */
static const CLI_Command_Definition_t xDVRStatusCommand =
{
	"dvrstatus",
	"\r\ndvrstatus:\r\n Display DVR status (SD, recording, MIC, etc)\r\n",
	prvDVRStatusCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "dvrview" command line command. */
static const CLI_Command_Definition_t xDVRViewCommand =
{
	"dvrview",
	"\r\ndvrview [0|1|2|3|4]:\r\n Switch DVR view mode: 0=front, 1=rear, 2=f+r, 3=r+f, 4=hzh\r\n",
	prvDVRViewCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/* Structure that defines the "dvrdsp" command line command. */
static const CLI_Command_Definition_t xDVRDisplayCommand =
{
	"dvrdsp",
	"\r\ndvrdsp [x] [y] [w] [h]:\r\n Set DVR display window position and size\r\n",
	prvDVRDisplayCommand, /* The function to run. */
	4 /* Four parameters are expected: x y width height */
};

/* Structure that defines the "dvrpreview" command line command. */
static const CLI_Command_Definition_t xDVRPreviewCommand =
{
	"dvrpreview",
	"\r\ndvrpreview [0|1]:\r\n Enable/disable DVR preview mode (0=disable jpeg decode, 1=enable)\r\n",
	prvDVRPreviewCommand, /* The function to run. */
	1 /* One parameter is expected. */
};
#endif
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
#if ENABLE_BD_USB_DVR_FUNC
	FreeRTOS_CLIRegisterCommand( &xDVRCommand );
	FreeRTOS_CLIRegisterCommand( &xDVRStatusCommand );
	FreeRTOS_CLIRegisterCommand( &xDVRViewCommand );
	FreeRTOS_CLIRegisterCommand( &xDVRDisplayCommand );
	FreeRTOS_CLIRegisterCommand( &xDVRPreviewCommand );
#endif
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
        printf("\r\nstart ap\r\n");
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

#if ENABLE_BD_USB_DVR_FUNC
/* External DVR API functions from main_awtk.c */
extern void dvr_api_get_id(void);
extern void dvr_api_rec_start(void);
extern void dvr_api_rec_stop(void);
extern void dvr_api_snap(void);
extern void dvr_api_sos(void);
extern void dvr_api_get_list(uint8_t mode);
extern void dvr_api_pb_start(uint8_t mode, uint16_t index);
extern void dvr_api_pb_pause(void);
extern void dvr_api_pb_stop(void);
extern void dvr_api_get_status(void);
extern void dvr_api_view_switch(uint8_t mode);
extern uint8_t dvr_get_sd_status(void);
extern uint8_t dvr_get_rec_status(void);
extern uint8_t dvr_get_lock_status(void);
extern uint8_t dvr_get_mic_status(void);
extern uint8_t dvr_get_sd_error_status(void);
extern uint8_t dvr_get_sd_full_status(void);
extern uint16_t dvr_get_video_list_count(void);
extern uint16_t dvr_get_photo_list_count(void);
extern uint8_t dvr_get_view_mode(void);
extern int dvr_api_check_exists(void);
extern int dvr_api_is_running(void);
extern void dvr_api_set_display_window(int32_t x, int32_t y, int32_t width, int32_t height);
extern void dvr_api_get_display_window(int32_t *x, int32_t *y, int32_t *width, int32_t *height);
extern void dvr_api_set_preview_enable(uint8_t enable);
extern uint8_t dvr_api_get_preview_enable(void);

/*
 * Implements the "dvr" command line command.
 */
static BaseType_t prvDVRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the first parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &xParameterStringLength );

    if( pcParameter != NULL )
    {
        if( strncmp( pcParameter, "getid", strlen( "getid" ) ) == 0 )
        {
            dvr_api_get_id();
            sprintf( pcWriteBuffer, "DVR: Get ID command sent\r\n" );
        }
        else if( strncmp( pcParameter, "recstart", strlen( "recstart" ) ) == 0 )
        {
            dvr_api_rec_start();
            sprintf( pcWriteBuffer, "DVR: Record start command sent\r\n" );
        }
        else if( strncmp( pcParameter, "recstop", strlen( "recstop" ) ) == 0 )
        {
            dvr_api_rec_stop();
            sprintf( pcWriteBuffer, "DVR: Record stop command sent\r\n" );
        }
        else if( strncmp( pcParameter, "snap", strlen( "snap" ) ) == 0 )
        {
            dvr_api_snap();
            sprintf( pcWriteBuffer, "DVR: Snap command sent\r\n" );
        }
        else if( strncmp( pcParameter, "sos", strlen( "sos" ) ) == 0 )
        {
            dvr_api_sos();
            sprintf( pcWriteBuffer, "DVR: SOS/Emergency lock command sent\r\n" );
        }
        else if( strncmp( pcParameter, "getlist", strlen( "getlist" ) ) == 0 )
        {
            /* Get second parameter: mode (0=video, 1=photo) */
            const char *pcModeParam = FreeRTOS_CLIGetParameter( pcCommandString, 2, &xParameterStringLength );
            uint8_t mode = 0;
            if( pcModeParam != NULL && xParameterStringLength > 0 )
            {
                mode = (uint8_t)atoi( pcModeParam );
            }
            dvr_api_get_list( mode );
            sprintf( pcWriteBuffer, "DVR: Get file list command sent (mode=%s)\r\n", mode ? "photo" : "video" );
        }
        else if( strncmp( pcParameter, "pbstart", strlen( "pbstart" ) ) == 0 )
        {
            /* Get second parameter: mode, third parameter: index */
            const char *pcModeParam = FreeRTOS_CLIGetParameter( pcCommandString, 2, &xParameterStringLength );
            const char *pcIdxParam = FreeRTOS_CLIGetParameter( pcCommandString, 3, &xParameterStringLength );
            uint8_t mode = 0;
            uint16_t index = 0;
            if( pcModeParam != NULL && xParameterStringLength > 0 )
            {
                mode = (uint8_t)atoi( pcModeParam );
            }
            if( pcIdxParam != NULL && xParameterStringLength > 0 )
            {
                index = (uint16_t)atoi( pcIdxParam );
            }
            dvr_api_pb_start( mode, index );
            sprintf( pcWriteBuffer, "DVR: Playback start command sent (mode=%s, index=%d)\r\n",
                     mode ? "photo" : "video", index );
        }
        else if( strncmp( pcParameter, "pbpause", strlen( "pbpause" ) ) == 0 )
        {
            dvr_api_pb_pause();
            sprintf( pcWriteBuffer, "DVR: Playback pause command sent\r\n" );
        }
        else if( strncmp( pcParameter, "pbstop", strlen( "pbstop" ) ) == 0 )
        {
            dvr_api_pb_stop();
            sprintf( pcWriteBuffer, "DVR: Playback stop command sent\r\n" );
        }
        else if( strncmp( pcParameter, "getsts", strlen( "getsts" ) ) == 0 )
        {
            dvr_api_get_status();
            sprintf( pcWriteBuffer, "DVR: Get status command sent\r\n" );
        }
        else
        {
            sprintf( pcWriteBuffer, "DVR: Unknown command. Use: getid|recstart|recstop|snap|sos|getlist|pbstart|pbpause|pbstop|getsts\r\n" );
        }
    }
    else
    {
        sprintf( pcWriteBuffer, "DVR: Usage: dvr [getid|recstart|recstop|snap|sos|getlist|pbstart|pbpause|pbstop|getsts]\r\n" );
    }

    return pdFALSE;
}

/*
 * Implements the "dvrstatus" command line command.
 */
static BaseType_t prvDVRStatusCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    if( !dvr_api_check_exists() )
    {
        sprintf( pcWriteBuffer, "DVR: elene file not found (USB DVR not connected)\r\n" );
    }
    else if( !dvr_api_is_running() )
    {
        sprintf( pcWriteBuffer, "DVR: Task not running\r\n" );
    }
    else
    {
        uint8_t sd = dvr_get_sd_status();
        uint8_t rec = dvr_get_rec_status();
        uint8_t lock = dvr_get_lock_status();
        uint8_t mic = dvr_get_mic_status();
        uint8_t err = dvr_get_sd_error_status();
        uint8_t full = dvr_get_sd_full_status();
        uint16_t video_cnt = dvr_get_video_list_count();
        uint16_t photo_cnt = dvr_get_photo_list_count();
        uint8_t view = dvr_get_view_mode();

        sprintf( pcWriteBuffer,
                 "DVR Status:\r\n"
                 "  SD Card: %s\r\n"
                 "  Recording: %s\r\n"
                 "  File Locked: %s\r\n"
                 "  MIC: %s\r\n"
                 "  SD Error: %s\r\n"
                 "  SD Full: %s\r\n"
                 "  Video Files: %d\r\n"
                 "  Photo Files: %d\r\n"
                 "  View Mode: %d\r\n",
                 sd ? "Present" : "Not Present",
                 rec ? "Yes" : "No",
                 lock ? "Yes" : "No",
                 mic ? "On" : "Off",
                 err ? "Yes" : "No",
                 full ? "Yes" : "No",
                 video_cnt,
                 photo_cnt,
                 view );
    }

    return pdFALSE;
}

/*
 * Implements the "dvrview" command line command.
 */
static BaseType_t prvDVRViewCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the first parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &xParameterStringLength );

    if( pcParameter != NULL )
    {
        uint8_t mode = (uint8_t)atoi( pcParameter );
        if( mode > 4 )
        {
            mode = 0;
        }
        dvr_api_view_switch( mode );
        sprintf( pcWriteBuffer, "DVR: View switch command sent (mode=%d: %s)\r\n",
                 mode,
                 mode == 0 ? "front" : (mode == 1 ? "rear" : (mode == 2 ? "f+r" : (mode == 3 ? "r+f" : "hzh"))) );
    }
    else
    {
        sprintf( pcWriteBuffer, "DVR: Usage: dvrview [0|1|2|3|4] (0=front, 1=rear, 2=f+r, 3=r+f, 4=hzh)\r\n" );
    }

    return pdFALSE;
}

/*
 * Implements the "dvrdsp" command line command.
 * Usage: dvrdsp [x] [y] [w] [h] - Set display window position and size
 *        dvrdsp -1 - Get current display window settings
 */
static BaseType_t prvDVRDisplayCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;
    int32_t x, y, w, h;

    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the first parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &xParameterStringLength );

    if( pcParameter != NULL )
    {
        /* Check if it's a get status request (-1) */
        if( strncmp( pcParameter, "-1", strlen( "-1" ) ) == 0 )
        {
            dvr_api_get_display_window( &x, &y, &w, &h );
            sprintf( pcWriteBuffer, "DVR Display Window: x=%d y=%d w=%d h=%d\r\n", x, y, w, h );
        }
        else
        {
            /* Get x parameter */
            x = atoi( pcParameter );

            /* Get y parameter */
            pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, 2, &xParameterStringLength );
            if( pcParameter == NULL )
            {
                sprintf( pcWriteBuffer, "DVR: Usage: dvrdsp [x] [y] [w] [h] or dvrdsp -1 to get current settings\r\n" );
                return pdFALSE;
            }
            y = atoi( pcParameter );

            /* Get w parameter */
            pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, 3, &xParameterStringLength );
            if( pcParameter == NULL )
            {
                sprintf( pcWriteBuffer, "DVR: Usage: dvrdsp [x] [y] [w] [h] or dvrdsp -1 to get current settings\r\n" );
                return pdFALSE;
            }
            w = atoi( pcParameter );

            /* Get h parameter */
            pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, 4, &xParameterStringLength );
            if( pcParameter == NULL )
            {
                sprintf( pcWriteBuffer, "DVR: Usage: dvrdsp [x] [y] [w] [h] or dvrdsp -1 to get current settings\r\n" );
                return pdFALSE;
            }
            h = atoi( pcParameter );

            /* Set display window */
            dvr_api_set_display_window( x, y, w, h );
            sprintf( pcWriteBuffer, "DVR: Display window set to x=%d y=%d w=%d h=%d\r\n", x, y, w, h );
        }
    }
    else
    {
        sprintf( pcWriteBuffer, "DVR: Usage: dvrdsp [x] [y] [w] [h] or dvrdsp -1 to get current settings\r\n" );
    }

    return pdFALSE;
}

/*
 * Implements the "dvrpreview" command line command.
 * Usage: dvrpreview [0|1] - 0=disable preview (skip jpeg decode), 1=enable preview
 *        dvrpreview -1 - Get current preview status
 */
static BaseType_t prvDVRPreviewCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Obtain the first parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &xParameterStringLength );

    if( pcParameter != NULL )
    {
        /* Check if it's a get status request (-1) */
        if( strncmp( pcParameter, "-1", strlen( "-1" ) ) == 0 )
        {
            uint8_t enable = dvr_api_get_preview_enable();
            sprintf( pcWriteBuffer, "DVR Preview: %s (dvr_preview_enable=%d)\r\n",
                     enable ? "Enabled" : "Disabled", enable );
        }
        else
        {
            uint8_t enable = (uint8_t)atoi( pcParameter );
            dvr_api_set_preview_enable( enable ? 1 : 0 );
            sprintf( pcWriteBuffer, "DVR Preview: %s\r\n", enable ? "Enabled" : "Disabled" );
        }
    }
    else
    {
        sprintf( pcWriteBuffer, "DVR: Usage: dvrpreview [0|1] or dvrpreview -1 to get status\r\n" );
    }

    return pdFALSE;
}
#endif /* ENABLE_BD_USB_DVR_FUNC */

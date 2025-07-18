/***********************************************************************************************************************
* Copyright (C) 2021 Arkmicro Corporation. All rights reserved.
***********************************************************************************************************************/

/***********************************************************************************************************************
* File Name    : NetworkInterface.c
* Device(s)    : RTL8189FTV
* Description  : Interfaces FreeRTOS TCP/IP stack to RX Ethernet driver.
***********************************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
/*#include "FreeRTOS_DNS.h" */
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"
#include "wifi_constants.h"
#include "net_stack_intf.h"

#define USE_AP   0
//#define DUMP_NETWORK_DATA
#undef DUMP_NETWORK_DATA



typedef enum
{
    eMACInit,   /* Must initialise MAC. */
    eMACPass,   /* Initialisation was successful. */
    eMACFailed, /* Initialisation failed. */
} eMAC_INIT_STATUS_TYPE;

static eMAC_INIT_STATUS_TYPE xMacInitStatus = eMACInit;

extern void cmd_test(const char* temp_uart_buf);

static int InitializeNetwork( void );
static int scan_comp_flag = 0;


/***********************************************************************************************************************
 * Function Name: xNetworkInterfaceInitialise ()
 * Description  : Initialization of Ethernet driver.
 * Arguments    : none
 * Return Value : pdPASS, pdFAIL
 **********************************************************************************************************************/
BaseType_t xNetworkInterfaceInitialise( void )
{
    BaseType_t xReturn;

    if( xMacInitStatus == eMACInit )
    {
    	//rltk_wlan_set_netif_info(0, NULL, "00:0c:29:5d:2e:05");
        /*
         * Perform the hardware specific network initialization here using the Ethernet driver library to initialize the
         * Ethernet hardware, initialize DMA descriptors, and perform a PHY auto-negotiation to obtain a network link.
         *
         * InitialiseNetwork() uses Ethernet peripheral driver library function, and returns 0 if the initialization fails.
         */
        if( InitializeNetwork() == pdFALSE )
        {
            xMacInitStatus = eMACFailed;
        }
        else
        {
            /* Indicate that the MAC initialisation succeeded. */
            xMacInitStatus = eMACPass;
        }

        FreeRTOS_printf( ( "InitializeNetwork returns %s\n", ( xMacInitStatus == eMACPass ) ? "OK" : " Fail" ) );
    }

    if( xMacInitStatus == eMACPass )
    {
        xReturn = pdPASS;
    }
    else
    {
        xReturn = pdFAIL;
    }

    FreeRTOS_printf( ( "xNetworkInterfaceInitialise returns %d\n", xReturn ) );

    return xReturn;
} /* End of function xNetworkInterfaceInitialise() */


/***********************************************************************************************************************
 * Function Name: xNetworkInterfaceOutput ()
 * Description  : Simple network output interface.
 * Arguments    : pxDescriptor, xReleaseAfterSend
 * Return Value : pdTRUE, pdFALSE
 **********************************************************************************************************************/
BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxDescriptor,
                                    BaseType_t xReleaseAfterSend )
{
    BaseType_t xReturn = pdFALSE;
	struct eth_drv_sg sg_list = {0};

	if (xCheckLoopback( pxDescriptor, xReleaseAfterSend ) != 0 ) {
		return pdTRUE;
	}

	if (!rltk_wlan_running(0)) {
		return pdFALSE;
	}

	sg_list.buf = (unsigned int)pxDescriptor->pucEthernetBuffer;
	sg_list.len = (unsigned int)pxDescriptor->xDataLength;
#ifdef DUMP_NETWORK_DATA
	if (1) {
		int i;
		char*  tmp = (char *)pxDescriptor->pucEthernetBuffer;
		printf("\r\n send:");
		for (i = 0; i < sg_list.len; i++) {
			printf("%02x ", tmp[i]);
		}printf("send end\r\n");
	}
#endif
	xReturn = (BaseType_t)rltk_wlan_send(0, &sg_list, 1, pxDescriptor->xDataLength);

	xReturn = pdTRUE;

    if( xReleaseAfterSend != pdFALSE )
    {
        /* It is assumed SendData() copies the data out of the FreeRTOS+TCP Ethernet
         * buffer.  The Ethernet buffer is therefore no longer needed, and must be
         * freed for re-use. */
        vReleaseNetworkBufferAndDescriptor( pxDescriptor );
    }

    return xReturn;
} /* End of function xNetworkInterfaceOutput() */


/***********************************************************************************************************************
 * Function Name: vNetworkInterfaceAllocateRAMToBuffers ()
 * Description  : .
 * Arguments    : pxNetworkBuffers
 * Return Value : none
 **********************************************************************************************************************/
void vNetworkInterfaceAllocateRAMToBuffers( NetworkBufferDescriptor_t pxNetworkBuffers[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ] )
{
} /* End of function vNetworkInterfaceAllocateRAMToBuffers() */

/***********************************************************************************************************************
 * Function Name: InitializeNetwork ()
 * Description  :
 * Arguments    : none
 * Return Value : pdTRUE, pdFALSE
 **********************************************************************************************************************/
#include "wifi_structures.h"


static rtw_result_t ark_rtw_scan_result_handler( rtw_scan_handler_result_t* malloced_scan_result )
{
	char bssid[32] = {0};
	char *ptr = malloced_scan_result->ap_details.BSSID.octet;
	sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
	printf("\r\nSSID:%s Bssid:%s Signal strength:%d DB\r\n", malloced_scan_result->ap_details.SSID.val, bssid,
		malloced_scan_result->ap_details.signal_strength);
	if (malloced_scan_result->scan_complete != 0) {
		printf("scan complete!\r\n");
		scan_comp_flag = 1;
	}
	
}

extern struct sdio_func *wifi_sdio_func;

int ark_wlan_init(void)
{
	int return_code = 0;
	char *ssid = "ap630";
	
	//rltk_wlan_ctx_init();
	
#if USE_AP
	/*if (wifi_on(RTW_MODE_AP) < 0) {
		printf("\r\nopen wifi failed \r\n");
		return return_code;
	}
	printf("\r\n wifi init finished!\r\n");*/
	//wifi_start_ap(ssid, RTW_SECURITY_OPEN, NULL, strlen(ssid), 0, 1);
	cmd_test("wifi_ap ap630  1");
	printf("\r\n wifi start now!\r\n");
#else
	//cmd_test("wifi_debug set_mac 000C295D2E05");//0x00, 0x0c, 0x29, 0x5d, 0x2e, 0x05
	if (wifi_on(RTW_MODE_STA) < 0) {
		printf("\r\nopen wifi failed \r\n");
		return return_code;
	}
	scan_comp_flag = 0;

	//wifi_set_mac_address("000c295d2e03");
	
	wifi_scan_networks(ark_rtw_scan_result_handler, NULL);

	while(scan_comp_flag == 0)
		mdelay(500);
	ssid = "ark-9528";

	cmd_test("wifi_connect ark-9528 02345678");
	
#endif
	return_code = 1;
	return return_code;
}

static int InitializeNetwork( void )
{
    BaseType_t return_code = pdFALSE;
	
    return pdTRUE;
} /* End of function InitializeNetwork() */


/***********************************************************************************************************************
 * End of file "NetworkInterface.c"
 **********************************************************************************************************************/
static UBaseType_t ulNextRand;
BaseType_t xTotalSuccess = 0;


UBaseType_t uxRand( void )
{
	const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

	ulNextRand = portGET_RUN_TIME_COUNTER_VALUE();

	ulNextRand = ( ulMultiplier * ulNextRand ) + ulIncrement;
	return( ( int ) ( ulNextRand >> 16UL ) & 0x7fffUL );
}
/*-----------------------------------------------------------*/

static void prvSRand( UBaseType_t ulSeed )
{
	ulNextRand = ulSeed;
}


 extern uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
													 uint16_t usSourcePort,
													 uint32_t ulDestinationAddress,
													 uint16_t usDestinationPort )
 {
	 ( void ) ulSourceAddress;
	 ( void ) usSourcePort;
	 ( void ) ulDestinationAddress;
	 ( void ) usDestinationPort;
 
	 return uxRand();
 }
 
 BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
 {
	 *( pulNumber ) = uxRand();
	 return pdTRUE;
 }
 
 
 void vApplicationPingReplyHook( ePingReplyStatus_t eStatus,
								 uint16_t usIdentifier )
 {
	 //if( eStatus == eSuccess )
	 {
		 FreeRTOS_printf( ( "Ping response received. ID: %d\r\n", usIdentifier ) );
		 printf("Ping response received. ID: %d\r\n", usIdentifier);
 
		 /* Increment successful ping replies. */
		 xTotalSuccess++;
	 }
 }


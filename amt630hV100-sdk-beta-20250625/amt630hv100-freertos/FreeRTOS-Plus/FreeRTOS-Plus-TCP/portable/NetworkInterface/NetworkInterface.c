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
#include "FreeRTOS_ARP.h"
#include "wifi_conf.h"
#include "board.h"

//#define USE_AP   0
//#define DUMP_NETWORK_DATA
//#undef DUMP_NETWORK_DATA

#ifdef WIFI_SUPPORT
#if CONFIG_USB_DEVICE_CDC_NCM
#error "Do not choose USE_USB_DEVICE when use WIFI_SUPPORT"
#endif
#endif

typedef enum
{
    eMACInit,   /* Must initialise MAC. */
    eMACPass,   /* Initialisation was successful. */
    eMACFailed, /* Initialisation failed. */
} eMAC_INIT_STATUS_TYPE;

static eMAC_INIT_STATUS_TYPE xMacInitStatus = eMACInit;

extern void cmd_test(const char* temp_uart_buf);

static int InitializeNetwork( void );

int g_ncm_register(const char *name);

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
#if CONFIG_USB_DEVICE_CDC_NCM
void gether_send(NetworkBufferDescriptor_t * const pxDescriptor);
#endif
BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxDescriptor,
                                    BaseType_t xReleaseAfterSend )
{
    	BaseType_t xReturn = pdFALSE;
#ifdef WIFI_SUPPORT
	struct eth_drv_sg sg_list = {0};
#endif

#ifdef DUMP_NETWORK_DATA
	if (1) {
		int i;
		char*  tmp = (char *)pxDescriptor->pucEthernetBuffer;
		printf("\r\nsend len:%d--> ", pxDescriptor->xDataLength);
		for (i = 0; i < pxDescriptor->xDataLength; i++) {
			printf("%02x ", tmp[i]);
		}printf("send end\r\n");
	}
#endif
#ifdef WIFI_SUPPORT
	if (!rltk_wlan_running(0)) {
		goto exit;
	}

	sg_list.buf = (unsigned int)pxDescriptor->pucEthernetBuffer;
	sg_list.len = (unsigned int)pxDescriptor->xDataLength;
	xReturn = (BaseType_t)rltk_wlan_send(0, &sg_list, 1, pxDescriptor->xDataLength);
#endif

	xReturn = pdTRUE;
#if CONFIG_USB_DEVICE_CDC_NCM
	NetworkBufferDescriptor_t *pxDescriptor_eth = pxDescriptor;
	if (!xReleaseAfterSend) {
		pxDescriptor_eth = pxDuplicateNetworkBufferWithDescriptor(pxDescriptor, pxDescriptor->xDataLength);
		xReleaseAfterSend = pdFALSE;
	} else {
		xReleaseAfterSend = pdFALSE;//release at usb net driver
	}
	//print_hex(pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength, NULL);
	gether_send(pxDescriptor_eth);
#else
#ifdef WIFI_SUPPORT
exit:
#endif
#endif
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

#if 0
static rtw_result_t ark_rtw_scan_result_handler( rtw_scan_handler_result_t* malloced_scan_result )
{
	char bssid[32] = {0};
	char *ptr = (char *)malloced_scan_result->ap_details.BSSID.octet;
	sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
	printf("\r\nSSID:%s Bssid:%s Signal strength:%d DB\r\n", malloced_scan_result->ap_details.SSID.val, bssid,
		malloced_scan_result->ap_details.signal_strength);
	if (malloced_scan_result->scan_complete != 0) {
		printf("scan complete!\r\n");
		scan_comp_flag = 1;
	}
	return 0;
}
#endif

static int InitializeNetwork( void )
{
	BaseType_t return_code = pdTRUE;

#if CONFIG_USB_DEVICE_CDC_NCM
	g_ncm_register("ncm");
#endif

	return return_code;
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
 
 #if 0
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
 #endif


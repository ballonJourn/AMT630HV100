/* mbed Microcontroller Library
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *                                        
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
//#define _NET_STACK_INTF_C_
#include <stdint.h>
#include <autoconf.h>
#include <net_stack_intf.h>

#if (CONFIG_LWIP_LAYER == 1)
#include <lwip/netif.h>
#include <lwip_netconf.h>
#include <ethernetif.h>
#endif
#include <osdep_service.h>
#include <wifi/wifi_util.h>
//----- ------------------------------------------------------------------
// External Reference
//----- ------------------------------------------------------------------
#if (CONFIG_LWIP_LAYER == 1)
extern struct netif xnetif[];			//LWIP netif
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
/*#include "FreeRTOS_DNS.h" */
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

//#define DUMP_NETWORK_RX_DATA

/**
 *      rltk_wlan_set_netif_info - set netif hw address and register dev pointer to netif device
 *      @idx_wlan: netif index
 *			    0 for STA only or SoftAP only or STA in STA+SoftAP concurrent mode, 
 *			    1 for SoftAP in STA+SoftAP concurrent mode
 *      @dev: register netdev pointer to LWIP. Reserved.
 *      @dev_addr: set netif hw address
 *
 *      Return Value: None
 */     
void rltk_wlan_set_netif_info(int idx_wlan, void * dev, unsigned char * dev_addr)
{
#if (CONFIG_LWIP_LAYER == 1)
	rtw_memcpy(xnetif[idx_wlan].hwaddr, dev_addr, 6);
	xnetif[idx_wlan].state = dev;
#endif
	printf("\r\ndev_addr:  %02x:%02x:%02x:%02x:%02x:%02x\r\n", dev_addr[0], dev_addr[1], dev_addr[2], dev_addr[3], dev_addr[4], dev_addr[5]);
	FreeRTOS_UpdateMACAddress(dev_addr);
}

/**
 *      rltk_wlan_send - send IP packets to WLAN. Called by low_level_output().
 *      @idx: netif index
 *      @sg_list: data buffer list
 *      @sg_len: size of each data buffer
 *      @total_len: total data len
 *
 *      Return Value: None
 */     
int rltk_wlan_send(int idx, struct eth_drv_sg *sg_list, int sg_len, int total_len)
{
//#if (CONFIG_LWIP_LAYER == 1)
	struct eth_drv_sg *last_sg;
	struct sk_buff *skb = NULL;
	int ret = 0;

#ifdef CONFIG_TX_ZERO_COPY
	u8 *data = NULL;
	u8 *Mac_data = NULL;
#endif
	
	if(idx == -1){
		DBG_INFO("netif is DOWN");
		return -1;
	}
	DBG_TRACE("%s is called", __FUNCTION__);
	
	save_and_cli();
	if (rltk_wlan_check_isup(idx)) {
		rltk_wlan_tx_inc(idx);
	} else {
		DBG_INFO("netif is DOWN");printf("%s is called\r\n", __FUNCTION__);
		restore_flags();
		return -1;
	}
	restore_flags();

#ifdef CONFIG_TX_ZERO_COPY
	data = rtw_malloc(1640);
	if(data == NULL){
		DBG_INFO("###########ERR_MEM at %s\r\n",__func__);
		ret = -1;
		goto exit;
	}

	Mac_data = data + 80;

	for (last_sg = &sg_list[sg_len]; sg_list < last_sg; ++sg_list) {
		rtw_memcpy(Mac_data, (void *)(sg_list->buf), sg_list->len);
		Mac_data+=sg_list->len;
	}

	skb = (struct sk_buff *)rltk_wlan_alloc_skb_0copy();
	if (skb == NULL) {
		DBG_INFO("rltk_wlan_alloc_skb()failed!\r\n");
		goto exit;
	}
	skb->data = data + 80;
	skb->head = data;
	skb->end = data + 1640;
	skb->tail = data + 80 + total_len;
	skb->len = total_len;
	ret = 0;
#else
	skb = rltk_wlan_alloc_skb(total_len);
#endif

	if (skb == NULL) {
		DBG_INFO("rltk_wlan_alloc_skb() for data len=%d failed!", total_len);
		ret = -1;
		goto exit;
	}

#ifndef CONFIG_TX_ZERO_COPY
	for (last_sg = &sg_list[sg_len]; sg_list < last_sg; ++sg_list) {
		//printf("sg_list len :%d \r\n", sg_list->len);
		rtw_memcpy(skb->tail, (void *)(sg_list->buf), sg_list->len);
		skb_put(skb,  sg_list->len);
	}
#endif

	rltk_wlan_send_skb(idx, skb);

exit:
	save_and_cli();
	rltk_wlan_tx_dec(idx);
	restore_flags();
	return ret;
//#endif
}

/**
 *      rltk_wlan_recv - indicate packets to LWIP. Called by ethernetif_recv().
 *      @idx: netif index
 *      @sg_list: data buffer list
 *      @sg_len: size of each data buffer
 *
 *      Return Value: None
 */     
int rltk_wlan_recv(int idx, struct eth_drv_sg *sg_list, int sg_len)
{
//#if (CONFIG_LWIP_LAYER == 1)
	struct eth_drv_sg *last_sg;
	struct sk_buff *skb;
	
	DBG_TRACE("%s is called", __FUNCTION__);
	if(idx == -1){
		DBG_INFO("skb is NULL");
		return 0;
	}//printf("\r\n%s is called at:%d\r\n", __FUNCTION__, __LINE__);
	skb = rltk_wlan_get_recv_skb(idx);
	if (NULL == skb) {printf("\r\n%s is called at:%d\r\n", __FUNCTION__, __LINE__);
		//mdelay(10);
		return 0;
	}
	DBG_ASSERT(skb, "No pending rx skb");

	for (last_sg = &sg_list[sg_len]; sg_list < last_sg; ++sg_list) {
		if (sg_list->buf != 0) {
			rtw_memcpy((void *)(sg_list->buf), skb->data, sg_list->len);
			skb_pull(skb, sg_list->len);
		}
	}
	return 1;
//#endif
}

int netif_is_valid_IP(int idx, unsigned char *ip_dest)
{
#if CONFIG_LWIP_LAYER == 1
	struct netif * pnetif = &xnetif[idx];

	ip_addr_t addr = { 0 };

#if CONFIG_MEMORY_ACCESS_ALIGNED
	unsigned int temp;
	memcpy(&temp, ip_dest, sizeof(unsigned int));
	u32_t *ip_dest_addr = &temp;
#else
	u32_t *ip_dest_addr  = (u32_t*)ip_dest;
#endif

#if LWIP_VERSION_MAJOR >= 2
	ip_addr_set_ip4_u32(&addr, *ip_dest_addr);
#else
	addr.addr = *ip_dest_addr;
#endif
	
#if LWIP_VERSION_MAJOR >= 2
	if((ip_addr_get_ip4_u32(netif_ip_addr4(pnetif))) == 0)
		return 1;
#else
	
	if(pnetif->ip_addr.addr == 0)
		return 1;
#endif
	
	if(ip_addr_ismulticast(&addr) || ip_addr_isbroadcast(&addr,pnetif)){
		return 1;
	}
		
	//if(ip_addr_netcmp(&(pnetif->ip_addr), &addr, &(pnetif->netmask))) //addr&netmask
	//	return 1;

	if(ip_addr_cmp(&(pnetif->ip_addr),&addr))
		return 1;

	DBG_TRACE("invalid IP: %d.%d.%d.%d ",ip_dest[0],ip_dest[1],ip_dest[2],ip_dest[3]);
#endif
	//printf("\r\nnet id:%d  IP: %d.%d.%d.%d \r\n", idx, ip_dest[0], ip_dest[1], ip_dest[2], ip_dest[3]);
	return 1;
}

int netif_get_idx(struct netif* pnetif)
{
#if (CONFIG_LWIP_LAYER == 1)
	int idx = pnetif - xnetif;

	switch(idx) {
	case 0:
		return 0;
	case 1:
		return 1;
	default:
		return -1;
	}
#else	
	return 0;
#endif
}

unsigned char *netif_get_hwaddr(int idx_wlan)
{
#if (CONFIG_LWIP_LAYER == 1)
	return xnetif[idx_wlan].hwaddr;
#else
	return (unsigned char *)FreeRTOS_GetMACAddress();
#endif
}

void netif_rx(int idx, unsigned int len)
{
	//printf("%s idx:%d len:%d\r\n", __func__, idx, len);
#if (CONFIG_LWIP_LAYER == 1)
	ethernetif_recv(&xnetif[idx], len);
#else

	NetworkBufferDescriptor_t * pxBufferDescriptor;
	IPStackEvent_t xRxEvent;
	int32_t xBytesReceived = len;
	int ret = -1;

	pxBufferDescriptor = pxGetNetworkBufferWithDescriptor( ( size_t ) xBytesReceived, 0 );

    if( pxBufferDescriptor != NULL )
    {
    	struct eth_drv_sg sg_list = {0};
		sg_list.buf = (unsigned int)pxBufferDescriptor->pucEthernetBuffer;
		sg_list.len = (unsigned int)xBytesReceived;
		ret = rltk_wlan_recv(0, &sg_list, 1);
		if (ret == 0) {
			printf("no rcv data\r\n");
			vReleaseNetworkBufferAndDescriptor( pxBufferDescriptor );
			return;
		}
#ifdef DUMP_NETWORK_RX_DATA
		if (1) {
			int i;
			char*  tmp = (char *)pxBufferDescriptor->pucEthernetBuffer;
			printf("\r\n recv:");
			for (i = 0; i < sg_list.len; i++) {
				printf("%02x ", tmp[i]);
			}printf("recv end\r\n");
		}
#endif
        //memcpy( pxBufferDescriptor->pucEthernetBuffer, buffer_pointer, ( size_t ) xBytesReceived );
        pxBufferDescriptor->xDataLength = ( size_t ) xBytesReceived;
        if( eConsiderFrameForProcessing( pxBufferDescriptor->pucEthernetBuffer ) == eProcessBuffer )
        {//printf("send eNetworkRxEvent\r\n"); 
            xRxEvent.eEventType = eNetworkRxEvent;
            xRxEvent.pvData = ( void * ) pxBufferDescriptor;
            if( xSendEventStructToIPTask( &xRxEvent, 0 ) == pdFALSE )
            {
                vReleaseNetworkBufferAndDescriptor( pxBufferDescriptor );
                iptraceETHERNET_RX_EVENT_LOST();
            }
            else
            {
                iptraceNETWORK_INTERFACE_RECEIVE();
            }
        }
        else
        {
            vReleaseNetworkBufferAndDescriptor( pxBufferDescriptor );
        }
    }
    else
    {
        iptraceETHERNET_RX_EVENT_LOST();
        FreeRTOS_printf( ( "R_ETHER_Read_ZC2: Cleared descriptors\n" ) );
    }
#endif
	
}

void netif_post_sleep_processing(void)
{
#if (CONFIG_LWIP_LAYER == 1)
	lwip_POST_SLEEP_PROCESSING();	//For FreeRTOS tickless to enable Lwip ARP timer when leaving IPS - Alex Fang
#endif
}

void netif_pre_sleep_processing(void)
{
#if (CONFIG_LWIP_LAYER == 1)
	lwip_PRE_SLEEP_PROCESSING();
#endif
}

#if CONFIG_WOWLAN
unsigned char *rltk_wlan_get_ip(int idx){
#if (CONFIG_LWIP_LAYER == 1)
	return LwIP_GetIP(&xnetif[idx]);
#else
	printf("rltk_wlan_get_ip\r\n");
	return NULL;
#endif
}
unsigned char *rltk_wlan_get_gw(int idx){
#if (CONFIG_LWIP_LAYER == 1)
	return LwIP_GetGW(&xnetif[idx]);
#else
	printf("rltk_wlan_get_gw\r\n");
	return NULL;
#endif
}

unsigned char *rltk_wlan_get_gwmask(int idx){
#if (CONFIG_LWIP_LAYER == 1)
	return LwIP_GetMASK(&xnetif[idx]);
#else
	printf("rltk_wlan_get_gwmask\r\n");
	return NULL;
#endif
}
#endif



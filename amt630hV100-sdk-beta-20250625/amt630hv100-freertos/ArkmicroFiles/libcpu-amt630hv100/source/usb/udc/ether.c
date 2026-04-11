#include "usb_os_adapter.h"
#include "board.h"
#include <linux/usb/cdc.h>
#include "linux/usb/composite.h"
#include "linux/usb/ether.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "trace.h"
#include "list.h"
#include "task.h"
#include "ark_dwc2.h"
#if !USE_LWIP
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"
#else
#include "queue.h"
#include "lwip/pbuf.h"
#include "ethernet.h"
struct netif *ncm_netif = NULL;
void ncm_net_set_intf(void* intf)
{
	ncm_netif = (struct netif *)intf;
}
extern void ncm_ethernetif_input(void *h, struct pbuf* p);

#endif

#define UETH__VERSION	"29-May-2008"
#define	WORK_RX_MEMORY		0

struct eth_dev {
	spinlock_t		lock;
	struct gether		*port_usb;

	struct usb_gadget	*gadget;

	spinlock_t		req_lock;
	List_t			tx_reqs;
	atomic_t		tx_qlen;
#if !USE_LWIP
	List_t                  rx_frames;
#else
	struct pbuf            rx_frames;
#endif
	struct usb_request *out_req;

	unsigned		header_len;
	bool			zlp;
	u8			host_mac[ETH_ALEN];
	TaskHandle_t       usb_ether_task;
	QueueHandle_t     usb_ether_event_queue;
	int                start;
	int                tx_err_count;
};

struct eth_event
{
	int 		type;
	void*   	priv;
};

#define USB_ETH_EVENT_USB_CONNECT      	0
#define USB_ETH_EVENT_USB_DISCONNECT      	1
#define USB_ETH_EVENT_USB_DATA_RX      	2  // usb recv data
#define USB_ETH_EVENT_USB_DATA_TX      	3  // usb data tx ok notify
#define USB_ETH_EVENT_NET_DATA_TX      	4  // data from tcpip
/*-------------------------------------------------------------------------*/
#undef atomic_read
#define atomic_read(v)  ((v)->counter)
#undef atomic_set
#define atomic_set(v,i) ((v)->counter = (i))
#undef atomic_inc
#define atomic_inc(v) ((v)->counter++)

#define RX_EXTRA	20	/* bytes guarding against rx overflows */

#define DEFAULT_QLEN	4/* double buffering by default */

static unsigned qmult = 2;

static struct eth_dev *the_dev;

/* for dual-speed hardware, use deeper queues at high/super speed */
static inline int qlen(struct usb_gadget *gadget)
{
	if (gadget_is_dualspeed(gadget) && (gadget->speed == USB_SPEED_HIGH ||
					    gadget->speed == USB_SPEED_SUPER))
		return qmult * DEFAULT_QLEN;
	else
		return DEFAULT_QLEN * 8;
}


/*-------------------------------------------------------------------------*/
#undef DBG
#undef VDBG
#undef ERROR
#undef INFO

#define xprintk(d, level, fmt, args...) \
	printk(level "%s: " fmt , (d)->net->name , ## args)

#ifdef DEBUG
#undef DEBUG
#define DBG(dev, fmt, args...) \
	xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define DBG(dev, fmt, args...) \
	do { } while (0)
#endif /* DEBUG */

#ifdef VERBOSE_DEBUG
#define VDBG	DBG
#else
#define VDBG(dev, fmt, args...) \
	do { } while (0)
#endif /* DEBUG */

#define ERROR(dev, fmt, args...) \
	xprintk(dev , KERN_ERR , fmt , ## args)
#define INFO(dev, fmt, args...) \
	xprintk(dev , KERN_INFO , fmt , ## args)


static void ether_disconnect(struct gether *link);
static int ether_connect(struct eth_dev	*dev, struct gether *link);
static int rx_submit(struct eth_dev *dev, struct usb_request *req, gfp_t gfp_flags);

static void rx_complete(struct usb_ep *ep, struct usb_request *req);

int ncm_get_mac_address(char mac[6])
{
	if (the_dev)
		memcpy(mac, the_dev->host_mac, 6);
	return 0;
}


static int prealloc(List_t *list, struct usb_ep *ep, unsigned n)
{
	unsigned		i;
	struct usb_request	*req;
	ListItem_t *pxListItem = NULL;

	if (!n)
		return -ENOMEM;

	i = n;
	list_for_each_entry(pxListItem, req, list) {
		if (i-- == 0) {
			printf("prealloc out!\r\n");
			goto extra;
		}
	}
	while (i--) {
		req = usb_ep_alloc_request(ep, GFP_ATOMIC);
		if (!req)
			return list_empty(list) ? -ENOMEM : 0;
		vListInitialiseItem(&req->list);
		listSET_LIST_ITEM_OWNER(&(req->list), req);
		printf("req:%x is alloc\n", req);
		req->buf = NULL;
		req->length = 0;
		list_add_tail(&req->list, list);
	}
	return 0;

extra:
	/* free extras */
	for (;;) {
		ListItem_t	*next;

		next = listGET_NEXT(&req->list);
		uxListRemove(&req->list);
		usb_ep_free_request(ep, req);

		if (next == listGET_END_MARKER(list))
			break;

		req = listGET_LIST_ITEM_OWNER(&req->list);
	}
	return 0;
}

static int alloc_requests(struct eth_dev *dev, struct gether *link, unsigned n)
{
	int	status;

	spin_lock(&dev->req_lock);
	status = prealloc(&dev->tx_reqs, link->in_ep, n);
	if (status < 0)
		goto fail;
	/*status = prealloc(&dev->rx_reqs, link->out_ep, n);
	if (status < 0)
		goto fail;*/
	goto done;
fail:
	DBG(dev, "can't alloc requests\n");
done:
	spin_unlock(&dev->req_lock);
	return status;
}

static void rx_fill(struct eth_dev *dev, gfp_t gfp_flags)
{
	//struct usb_request	*req;
	//unsigned long		flags;

	/* fill unused rxq slots with some skb */
	/*spin_lock_irqsave(&dev->req_lock, flags);
	while (!list_empty(&dev->rx_reqs)) {
		req = listGET_LIST_ITEM_OWNER(listGET_HEAD_ENTRY(&dev->rx_reqs));
		list_del_init(&req->list);
		spin_unlock_irqrestore(&dev->req_lock, flags);

		rx_submit(dev, req, 0);
		spin_lock_irqsave(&dev->req_lock, flags);
	}
	spin_unlock_irqrestore(&dev->req_lock, flags);*/
	rx_submit(dev, dev->out_req, 0);
}


static void tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct eth_dev	*dev = ep->driver_data;
	struct eth_event ev;

	ev.type 	= USB_ETH_EVENT_USB_DATA_TX;
	ev.priv		= (void*)req;
	xQueueSendFromISR(dev->usb_ether_event_queue, &ev, 0);

}

static void rx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct eth_event ev;
	struct eth_dev	*dev = ep->driver_data;
	if (req)
	//printf("%s:%d req->actual:%d\r\n", __func__, __LINE__, req->actual);
	ev.type 	= USB_ETH_EVENT_USB_DATA_RX;
	ev.priv		= (void*)req;

	if (!xPortIsInInterrupt())
		xQueueSend(dev->usb_ether_event_queue, &ev, 0);
	else
		xQueueSendFromISR(dev->usb_ether_event_queue, &ev, 0);
}

static inline int is_promisc(u16 cdc_filter)
{
	return cdc_filter & USB_CDC_PACKET_TYPE_PROMISCUOUS;
}

static int rx_submit(struct eth_dev *dev, struct usb_request *req, gfp_t gfp_flags)
{
	int		retval = -ENOMEM;
	size_t		size = 0;
	struct usb_ep	*out;
	unsigned long	flags;
	void *buf = NULL;

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->port_usb)
		out = dev->port_usb->out_ep;
	else
		out = NULL;
	spin_unlock_irqrestore(&dev->lock, flags);

	if (!out)
		return -ENOTCONN;

	size += ETH_ALEN + ipconfigNETWORK_MTU + RX_EXTRA;
	size += dev->port_usb->header_len;
	if (out->maxpacket <= 0) {
		printf("maxpacket err\r\n");
	}
	size += out->maxpacket - 1;
	size -= size % out->maxpacket;

	if (dev->port_usb->is_fixed)
		size = max_t(size_t, size, dev->port_usb->fixed_out_len);
	buf = req->buf;
	if (NULL == req->buf || req->length < size) {
		if (NULL != req->buf)
			vPortFree(req->buf);
		buf = pvPortMalloc(size);
		if (buf == NULL) {
			return -ENOMEM;
		}
	}
	//memset(req->buf, 0, req->length);

	req->buf = buf;
	req->length = size;
	req->complete = rx_complete;
	req->context = NULL;

	retval = usb_ep_queue(out, req, 0);

	if (retval) {
		printf("rx submit failed--> %d\r\n", retval);
		/*spin_lock_irqsave(&dev->req_lock, flags);
		list_add_tail(&req->list, &dev->rx_reqs);
		spin_unlock_irqrestore(&dev->req_lock, flags);*/
	}
	return retval;
}

int usb_data_rx_proc(struct eth_dev* dev, struct usb_request *req)
{
	int ret = -1;

	if (!dev->port_usb->connected) {
		printf("%s:%d usb is disconnected\r\n", __func__, __LINE__);
		return -1;
	}

	if (req->status != 0) {
		usb_ep_queue(dev->port_usb->out_ep, req, 0);
		return -1;
	}

	if (dev->port_usb && dev->port_usb->unwrap) {
#if !USE_LWIP
		dev->port_usb->unwrap(dev->port_usb, req->buf, req->actual, &dev->rx_frames);
#else
		dev->port_usb->unwrap(dev->port_usb, req->buf, req->actual, (List_t *)&dev->rx_frames);
#endif
	}

	if (dev->port_usb->connected) {
#if !USE_LWIP
		NetworkBufferDescriptor_t*  pxBufferDescriptor = NULL;
		IPStackEvent_t xRxEvent;
		ListItem_t *pxListItem, *pxListItem1;
		list_for_each_entry_safe(pxListItem, pxListItem1, pxBufferDescriptor, &dev->rx_frames) {
			list_del_init(&pxBufferDescriptor->xBufferListItem);
			xRxEvent.eEventType = eNetworkRxEvent;
			xRxEvent.pvData = ( void * ) pxBufferDescriptor;
			if (0) {
				int i, dump_len = pxBufferDescriptor->xDataLength;
				printf("recv len:%d\r\n", dump_len);
				for (i = 0; i < dump_len; i++) {
					printf("%02x ", pxBufferDescriptor->pucEthernetBuffer[i]);
					if ((i + 1) % 16 == 0)
						printf("\r\n");
				}printf("\r\n");
			}
			if( xSendEventStructToIPTask( &xRxEvent, 0 ) == pdFALSE ) {
				vReleaseNetworkBufferAndDescriptor( pxBufferDescriptor );
				iptraceETHERNET_RX_EVENT_LOST();
			} else {
				iptraceNETWORK_INTERFACE_RECEIVE();
			}
		}
#else
		if (dev->rx_frames.next) {
			struct pbuf *header = dev->rx_frames.next;
			while(header != NULL) {
		        struct pbuf *current = header;
		        header = header->next;
				current->next = NULL;
		        ncm_ethernetif_input(ncm_netif, current);
	    	}
			dev->rx_frames.next = NULL;
		}
#endif

	}

	if (req) {
		ret = rx_submit(dev, req, 0);
		if (ret != 0) {
			rx_fill(dev, 0);
		}
	}

       return 0;
}

void usb_data_tx_proc(struct eth_dev* dev, struct usb_request *req)
{
	if (1) {
#if !USE_LWIP
		NetworkBufferDescriptor_t *pxBufferDescriptor =  (NetworkBufferDescriptor_t *)req->context;

		if (pxBufferDescriptor) {
			if (pxBufferDescriptor->pucEthernetBuffer) {
				vPortFree(pxBufferDescriptor->pucEthernetBuffer);
				pxBufferDescriptor->pucEthernetBuffer = NULL;
			}
			vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
		}
#else
		struct pbuf* pxBufferDescriptor =  (struct pbuf*)req->context;//the buf is alloc by ncm, it must be freed.
		if (pxBufferDescriptor) {
			pbuf_free(pxBufferDescriptor);
		}

#endif
	}
	spin_lock(&dev->req_lock);
	if ((dev->port_usb == NULL || !dev->port_usb->connected)
		&& dev->gadget && dev->gadget->ep0) {
		req->buf = NULL;
		req->length = 0;
		usb_ep_free_request(dev->gadget->ep0, req);
		printf("%s #############.\n", __func__);
	} else {
		list_add_tail(&req->list, &dev->tx_reqs);
	}
	spin_unlock(&dev->req_lock);

}

static void free_net_buffer(void* desc_handle)
{
#if !USE_LWIP
	NetworkBufferDescriptor_t *pxBufferDescriptor = (NetworkBufferDescriptor_t*)desc_handle;

	if (NULL != pxBufferDescriptor)
		vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
#else
	struct pbuf* pxBufferDescriptor =  (struct pbuf*)desc_handle;
	if (pxBufferDescriptor) {
		pbuf_free(pxBufferDescriptor);
	}
#endif
}

int net_data_tx_proc(struct eth_dev* dev, void *pxBufferDescriptorHandle)
{
	int			length = 0;
	int			retval;
	struct usb_request	*req = NULL;
	unsigned long		flags;
	struct usb_ep		*in;
	u16			cdc_filter;
#if !USE_LWIP
	NetworkBufferDescriptor_t *pxBufferDescriptor = (NetworkBufferDescriptor_t*)pxBufferDescriptorHandle;
#else
	struct pbuf* pxBufferDescriptor =  (struct pbuf*)pxBufferDescriptorHandle;
#endif

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->port_usb) {
		in = dev->port_usb->in_ep;
		cdc_filter = dev->port_usb->cdc_filter;
	} else {
		in = NULL;
		cdc_filter = 0;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	if (!in || !dev->port_usb->connected) {
		free_net_buffer((void*)pxBufferDescriptor);
		return 0;
	}

	/* apply outgoing CDC or RNDIS filters */
	if (!is_promisc(cdc_filter)) {
#if !USE_LWIP
		u8		*dest = pxBufferDescriptor->pucEthernetBuffer;
#else
		u8		*dest = pxBufferDescriptor->payload;
#endif

		if (is_multicast_ether_addr(dest)) {
			u16	type;

			/* ignores USB_CDC_PACKET_TYPE_MULTICAST and host
			 * SET_ETHERNET_MULTICAST_FILTERS requests
			 */
			if (is_broadcast_ether_addr(dest))
				type = USB_CDC_PACKET_TYPE_BROADCAST;
			else
				type = USB_CDC_PACKET_TYPE_ALL_MULTICAST;
			if (!(cdc_filter & type)) {
				free_net_buffer((void*)pxBufferDescriptor);
				return 0;
			}
		}
		/* ignores USB_CDC_PACKET_TYPE_DIRECTED */
	}

	spin_lock_irqsave(&dev->req_lock, flags);
	/*
	 * this freelist can be empty if an interrupt triggered disconnect()
	 * and reconfigured the gadget (shutting down this queue) after the
	 * network stack decided to xmit but before we got the spinlock.
	 */
	if (list_empty(&dev->tx_reqs)) {
		spin_unlock_irqrestore(&dev->req_lock, flags);
		printf("tx reqs empty\r\n");
		free_net_buffer((void*)pxBufferDescriptor);
		if (dev->tx_err_count++ > 3) {
			usb_dwc2_reset(0, 1);
			dev->tx_err_count = 0;
		}
		return -1;
	}
	dev->tx_err_count = 0;
	req = listGET_LIST_ITEM_OWNER(listGET_HEAD_ENTRY(&dev->tx_reqs));
	list_del(&req->list);

	/* temporarily stop TX queue when the freelist empties */
	if (list_empty(&dev->tx_reqs)) {

	}
	spin_unlock_irqrestore(&dev->req_lock, flags);

	/* no buffer copies needed, unless the network stack did it
	 * or the hardware can't use skb buffers.
	 * or there's not enough space for extra headers we need
	 */
#if !USE_LWIP
	spin_lock_irqsave(&dev->lock, flags);
	if (dev->port_usb && dev->port_usb->wrap)
		pxBufferDescriptor = dev->port_usb->wrap(dev->port_usb, pxBufferDescriptor);
	spin_unlock_irqrestore(&dev->lock, flags);
	if (!pxBufferDescriptor)
		goto drop;

	length = pxBufferDescriptor->xDataLength;

	req->buf = pxBufferDescriptor->pucEthernetBuffer;
	req->context = pxBufferDescriptor;
	req->complete = tx_complete;
#else
	spin_lock_irqsave(&dev->lock, flags);
	if (dev->port_usb && dev->port_usb->wrap_ext)
		pxBufferDescriptor = (struct pbuf*)dev->port_usb->wrap_ext(dev->port_usb, (void*)pxBufferDescriptor);
	spin_unlock_irqrestore(&dev->lock, flags);
	if (!pxBufferDescriptor)
		goto drop;

	length = pxBufferDescriptor->len;
	req->buf = pxBufferDescriptor->payload;
	req->context = pxBufferDescriptor;
	req->complete = tx_complete;
#endif
	/* NCM requires no zlp if transfer is dwNtbInMaxSize */
	if (dev->port_usb->is_fixed &&
	    length == dev->port_usb->fixed_in_len &&
	    (length % in->maxpacket) == 0)
		req->zero = 0;
	else
		req->zero = 1;

	/* use zlp framing on tx for strict CDC-Ether conformance,
	 * though any robust network rx path ignores extra padding.
	 * and some hardware doesn't like to write zlps.
	 */
	if (req->zero && !dev->zlp && (length % in->maxpacket) == 0)
		length++;

	req->length = length;

	retval = usb_ep_queue(in, req, GFP_ATOMIC);
	switch (retval) {
	default:
		DBG(dev, "tx queue err %d\n", retval);
		break;
	case 0:
		atomic_inc(&dev->tx_qlen);
	}

	if (retval) {
#if !USE_LWIP
		pxBufferDescriptor =  (NetworkBufferDescriptor_t *)req->context;

		if (pxBufferDescriptor) {
			if (pxBufferDescriptor->pucEthernetBuffer) {
				vPortFree(pxBufferDescriptor->pucEthernetBuffer);
				pxBufferDescriptor->pucEthernetBuffer = NULL;
			}
			vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
		}
#else
		pxBufferDescriptor =  (struct pbuf*)req->context;
		if (pxBufferDescriptor) {
			pbuf_free(pxBufferDescriptor);
		}
#endif
drop:
		spin_lock_irqsave(&dev->req_lock, flags);
		list_add_tail(&req->list, &dev->tx_reqs);
		spin_unlock_irqrestore(&dev->req_lock, flags);
	}
	return 0;
}

static void usb_ether_task_proc(void* arg)
{
	struct eth_event ev;
	struct eth_dev		*dev = (struct eth_dev *)arg;

	while(dev->start) {
		ev.type = -1;
		if (xQueueReceive(dev->usb_ether_event_queue, &ev,  portMAX_DELAY) != pdPASS) {
			printf("%s xQueueReceive err!\r\n", __func__);
			continue;
		}

		if (ev.type == -1)
			continue;
		switch(ev.type) {
		case USB_ETH_EVENT_USB_DATA_RX:
			if (NULL != ev.priv)
				usb_data_rx_proc(dev, (struct usb_request *)ev.priv);
			break;
		case USB_ETH_EVENT_USB_DATA_TX:
			if (NULL != ev.priv)
				usb_data_tx_proc(dev, (struct usb_request *)ev.priv);
			break;
		case USB_ETH_EVENT_NET_DATA_TX:
			if (NULL != ev.priv)
				net_data_tx_proc(dev, ev.priv);
			break;
		case USB_ETH_EVENT_USB_DISCONNECT:
			if (NULL != ev.priv)
				ether_disconnect((struct gether *)ev.priv);
			break;
		case USB_ETH_EVENT_USB_CONNECT:
			if (NULL != ev.priv)
				ether_connect(dev, (struct gether *)ev.priv);
			break;
		default:
			break;
		}
	}

	vTaskDelete(NULL);
}

/**
 * gether_setup - initialize one ethernet-over-usb link
 * @g: gadget to associated with these links
 * @ethaddr: NULL, or a buffer in which the ethernet address of the
 *	host side of the link is recorded
 * Context: may sleep
 *
 * This sets up the single network link that may be exported by a
 * gadget driver using this framework.  The link layer addresses are
 * set up using module parameters.
 *
 * Returns negative errno, or zero on success
 */
#if 0
UBaseType_t uxRand( void );
static void random_ether_addr(u8 *addr)
{
	UBaseType_t val = uxRand();

	if (NULL == addr)
		return;
	addr [0] = ((val >> 0) & 0xff);
	addr [1] = ((val >> 8) & 0xff);
	addr [2] = ((val >> 16) & 0xff);
	addr [3] = ((val >> 24) & 0xff);
	val = uxRand();
	addr [4] = ((val >> 0) & 0xff);
	addr [5] = ((val >> 8) & 0xff);

	addr [0] &= 0xfe;	/* clear multicast bit */
	addr [0] |= 0x02;	/* set local assignment bit (IEEE802) */
}
#endif
int gether_setup(struct usb_gadget *g, u8 ethaddr[ETH_ALEN])
{
	struct eth_dev		*dev;
	BaseType_t			ret = pdFAIL;

	if (the_dev)
		return -EBUSY;
	dev = pvPortMalloc(sizeof(struct eth_dev));
	if (dev == NULL)
		return -ENOMEM;
	memset((void*)dev, 0, sizeof(struct eth_dev));

	spin_lock_init(&dev->lock);
	spin_lock_init(&dev->req_lock);

	INIT_LIST_HEAD(&dev->tx_reqs);
	//INIT_LIST_HEAD(&dev->rx_reqs);
#if !USE_LWIP
	INIT_LIST_HEAD(&dev->rx_frames);
#else
	dev->rx_frames.next = NULL;
	dev->rx_frames.len = 0;
	dev->rx_frames.tot_len = 0;
#endif

#if 1
	dev->host_mac[0] = 0x00;//00:0c:29:53:02:09
	dev->host_mac[1] = 0x0c;
	dev->host_mac[2] = 0x29;
	dev->host_mac[3] = 0x53;
	dev->host_mac[4] = 0x02;
	dev->host_mac[5] = 0x09;
#else
	random_ether_addr(dev->host_mac);
#endif
	//FreeRTOS_UpdateMACAddress(dev->host_mac);
	printf("\r\nncm mac: %02x %02x %02x %02x %02x %02x\r\n", dev->host_mac[0], dev->host_mac[1],
		dev->host_mac[2], dev->host_mac[3], dev->host_mac[4], dev->host_mac[5]);

	if (ethaddr)
		memcpy(ethaddr, dev->host_mac, ETH_ALEN);
	dev->usb_ether_event_queue = xQueueCreate(16, sizeof(struct eth_event));
	if (NULL == dev->usb_ether_event_queue) {
		goto exit;
	}
	dev->start = 1;
	ret = xTaskCreate(usb_ether_task_proc, "usb_ether_task", 2048, (void*)dev, configMAX_PRIORITIES - 3, &dev->usb_ether_task);
	if (ret != pdPASS) {
		goto exit;
	}

	dev->gadget = g;
	the_dev = dev;
	return 0;
exit:
	if (ret != pdPASS) {
		if (dev) {
			if (dev->usb_ether_event_queue) {
				vQueueDelete(dev->usb_ether_event_queue);
			}
			vPortFree(dev);
		}
	}
	return -1;
}

/**
 * gether_cleanup - remove Ethernet-over-USB device
 * Context: may sleep
 *
 * This is called to free all resources allocated by @gether_setup().
 */
void gether_cleanup(void)
{
	struct eth_event ev;
	struct eth_dev		*dev = the_dev;

	if (!dev)
		return;
	dev->start = 0;

	if (dev->usb_ether_event_queue) {
		ev.type = -1;
		xQueueSend(dev->usb_ether_event_queue, &ev, 0);
		vQueueDelete(dev->usb_ether_event_queue);
	}//maybe exist bug

	vPortFree(dev);

	the_dev = NULL;
}

static int ether_connect(struct eth_dev	*dev, struct gether *link)
{
	int			result = 0;

	if (!dev)
		return -EINVAL;

	dev->tx_err_count = 0;
	if (link && link->connected)
		return 0;

	link->in_ep->driver_data = dev;
	result = usb_ep_enable(link->in_ep, link->in);
	if (result != 0) {
		DBG(dev, "enable %s --> %d\n",
			link->in_ep->name, result);
		goto fail0;
	}

	link->out_ep->driver_data = dev;
	result = usb_ep_enable(link->out_ep, link->out);
	if (result != 0) {
		DBG(dev, "enable %s --> %d\n",
			link->out_ep->name, result);
		goto fail1;
	}

	if (result == 0) {
		result = alloc_requests(dev, link, qlen(dev->gadget));
		dev->out_req = NULL;
		dev->out_req = usb_ep_alloc_request(link->out_ep, GFP_ATOMIC);
		if (!dev->out_req) {
			printf("usb_ep_alloc_request %s --> %d\r\n",
				link->out_ep->name, result);
			goto fail1;
		}
		dev->out_req->buf = NULL;
		dev->out_req->length = 0;
	}

	if (result == 0 && dev->out_req) {
		dev->zlp = link->is_zlp_ok;
		DBG(dev, "qlen %d\n", qlen(dev->gadget));

		dev->header_len = link->header_len;

		spin_lock(&dev->lock);
		dev->port_usb = link;
		link->ctx = (void *)dev;
		spin_unlock(&dev->lock);

		rx_fill(dev, 0);
		atomic_set(&dev->tx_qlen, 0);
		link->connected = true;

	/* on error, disable any endpoints  */
	} else {
		(void) usb_ep_disable(link->out_ep);
fail1:
		(void) usb_ep_disable(link->in_ep);
	}
fail0:
	/* caller is responsible for cleanup on error */
	if (result < 0)
		return result;
	return result;
}

/**
 * gether_disconnect - notify network layer that USB link is inactive
 * @link: the USB link, on which gether_connect() was called
 * Context: irqs blocked
 *
 * This is called to deactivate endpoints and let the network layer know
 * the connection went inactive ("no carrier").
 *
 * On return, the state is as if gether_connect() had never been called.
 * The endpoints are inactive, and accordingly without active USB I/O.
 * Pointers to endpoint descriptors and endpoint private data are nulled.
 */
static void ether_disconnect(struct gether *link)
{
	struct eth_dev		*dev = (struct eth_dev *)link->ctx;
	struct usb_request	*req;

	if (!dev || !link->connected)
		return;
	dev->tx_err_count = 0;
	link->connected = false;
	printf("%s:%d start\r\n", __func__, __LINE__);
	usb_ep_disable(link->in_ep);
	spin_lock(&dev->req_lock);
	while (!list_empty(&dev->tx_reqs)) {
		req = listGET_LIST_ITEM_OWNER(listGET_HEAD_ENTRY(&dev->tx_reqs));
		list_del(&req->list);

		spin_unlock(&dev->req_lock);
		req->buf = NULL;
		req->length = 0;
		usb_ep_free_request(link->in_ep, req);
		printf("req:%x is released at disconnect\n", req);
		spin_lock(&dev->req_lock);
	}
	spin_unlock(&dev->req_lock);
	link->in_ep->driver_data = NULL;
	link->in_ep->desc = NULL;

	usb_ep_disable(link->out_ep);
	spin_lock(&dev->req_lock);
#if !USE_LWIP
	while (!list_empty(&dev->rx_frames)) {
		NetworkBufferDescriptor_t * pxNetworkBuffer = listGET_LIST_ITEM_OWNER(listGET_HEAD_ENTRY(&dev->rx_frames));
		list_del_init(&pxNetworkBuffer->xBufferListItem);

		vReleaseNetworkBufferAndDescriptor(pxNetworkBuffer);
	}
#else
	if (dev->rx_frames.next) {
		struct pbuf *header = dev->rx_frames.next;
		while(header != NULL) {
	        struct pbuf *current = header;
	        header = header->next;
	        pbuf_free(current);
    	}
		dev->rx_frames.next = NULL;
	}
#endif
	req = dev->out_req;

	if (req && req->buf) {
		vPortFree(req->buf);
		req->buf = NULL;
		req->length = 0;
	}
	usb_ep_free_request(link->out_ep, dev->out_req);
	spin_unlock(&dev->req_lock);
	link->out_ep->driver_data = NULL;
	link->out_ep->desc = NULL;

	/* finish forgetting about this USB link episode */
	dev->header_len = 0;
	spin_lock(&dev->lock);
	dev->port_usb = NULL;
	spin_unlock(&dev->lock);
	if (link && link->disconnect_cb)
		link->disconnect_cb(link);
	printf("%s:%d end\r\n", __func__, __LINE__);
}

int gether_connect(struct gether *link)
{
	struct eth_event ev;
	struct eth_dev	*dev = the_dev;

	if (dev && dev->usb_ether_event_queue) {
		ev.type 	= USB_ETH_EVENT_USB_CONNECT;
		ev.priv		= (void*)link;
		if (!xPortIsInInterrupt())
			xQueueSend(dev->usb_ether_event_queue, &ev, 0);
		else
			xQueueSendFromISR(dev->usb_ether_event_queue, &ev, 0);
	}
	return 0;
}

void gether_disconnect(struct gether *link)
{
	struct eth_event ev;
	struct eth_dev	*dev = (struct eth_dev *)link->ctx;

	if (dev && dev->usb_ether_event_queue) {
		ev.type 	= USB_ETH_EVENT_USB_DISCONNECT;
		ev.priv		= (void*)link;
		if (!xPortIsInInterrupt())
			xQueueSend(dev->usb_ether_event_queue, &ev, 0);
		else
			xQueueSendFromISR(dev->usb_ether_event_queue, &ev, 0);
	}
}

void gether_send(NetworkBufferDescriptor_t * const pxDescriptor)
{
	struct eth_event ev;
	struct eth_dev	*dev = the_dev;

	if (dev && dev->usb_ether_event_queue) {
		ev.type 	= USB_ETH_EVENT_NET_DATA_TX;
		ev.priv		= (void*)pxDescriptor;
		xQueueSend(dev->usb_ether_event_queue, &ev, 0);
	} else {
#if !USE_LWIP
		vReleaseNetworkBufferAndDescriptor( pxDescriptor );
#endif
	}
}

void gether_send_ext(void * const pxDescriptor)
{
	struct eth_event ev;
	struct eth_dev	*dev = the_dev;

	if (dev && dev->usb_ether_event_queue) {
		ev.type 	= USB_ETH_EVENT_NET_DATA_TX;
		ev.priv 	= (void*)pxDescriptor;
		xQueueSend(dev->usb_ether_event_queue, &ev, 0);
	} else {
#if !USE_LWIP
		vReleaseNetworkBufferAndDescriptor( pxDescriptor );
#endif
	}
}


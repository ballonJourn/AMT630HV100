#include "usb_os_adapter.h"
#include "trace.h"
#include <asm/dma-mapping.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "core.h"
#include "hcd.h"
#include "ark_dwc2.h"
#include "semphr.h"
#include "timers.h"
#include "usbroothubdes.h"
#include "sysctl.h"
#include "board.h"

struct dwc2_host_data {
	struct dwc2_hsotg  *host;
	struct usb_hcd hcd;
	enum usb_device_speed host_speed;
	int root_hub_devnum;
	struct usb_host_endpoint hep_in[16];
	struct usb_host_endpoint hep_out[16];
	struct hc_driver *dw2_hc_driver;
	List_t free_urb_list;
	spinlock_t lock;
};

struct urb_user_ctx {
	struct usb_device* dev;
	struct usb_host_endpoint *hep;
	unsigned long pipe;
	void *buffer;
	int length;
	int num_iso_packets;
	int interval;
	int alloc_flag;
};

int dwc2_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags);
int dwc2_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status);
extern int dwc2_driver_init(struct dwc2_hsotg **dev, struct usb_hcd *hcd);
extern int dwc2_driver_uninit(struct dwc2_hsotg *hsotg);
extern struct hc_driver* dwc2_get_driver();
static int dwc_otg_submit_rh_msg(struct usb_device *dev,
				 unsigned long pipe, void *buffer, int txlen,
				 struct devrequest *cmd);



struct dwc2_host_data dwc2_host;

struct urb_user_ctx urb_user_handle[8];

void *alloc_urb_user_ctx()
{
	int i;
	struct urb_user_ctx* handle = NULL;
	for (i = 0; i < 8; i++) {
		handle = &urb_user_handle[i];
		if (handle->alloc_flag == 0) {
			handle->alloc_flag = 1;
			break;
		}
	}
	return handle;
}

void free_urb_user_ctx(void *ctx)
{
	struct urb_user_ctx* handle = (struct urb_user_ctx*)ctx;
	handle->alloc_flag = 0;
}


static void dwc2_host_complete_urb(struct urb *urb)
{
	if (urb->status != 0) {
		printf("#urb transfer err:%d\r\n", urb->status);
	}
	urb->dev->status &= ~USB_ST_NOT_PROC;
	urb->dev->act_len = urb->actual_length;
	//printf("actual_length:%d\r\n", urb->actual_length);
	xQueueSendFromISR((QueueHandle_t)urb->queueHandle, NULL, 0);
}

static void usb_urb_release(struct urb *urb)
{
	if (NULL == urb)
		return;
	spin_lock(&dwc2_host.lock);
	list_add_tail(&urb->urb_list, &dwc2_host.free_urb_list);
	spin_unlock(&dwc2_host.lock);
}

static struct urb *usb_urb_alloc(struct dwc2_host_data *ctx,
					       int iso_desc_count,
					       gfp_t mem_flags)
{
	struct urb *urb = NULL;
	QueueHandle_t complete;
	u32 size = sizeof(*urb) + iso_desc_count *
		   sizeof(struct usb_iso_packet_descriptor);

	ListItem_t *pxListItem = NULL;
	int found = 0, flags;
	spin_lock_irqsave(&ctx->lock, flags);
	list_for_each_entry(pxListItem, urb, &ctx->free_urb_list) {
		if (urb->number_of_packets == iso_desc_count) {
			found = 1;
            break;
		}
	}
	if (found) {
		void *queueHandle = urb->queueHandle;
		list_del_init(&urb->urb_list);
		spin_unlock_irqrestore(&ctx->lock, flags);
		memset(urb, 0, sizeof(struct urb));
		urb->number_of_packets = iso_desc_count;
		INIT_LIST_ITEM(&urb->urb_list);
		listSET_LIST_ITEM_OWNER(&urb->urb_list, urb);
		urb->queueHandle = queueHandle;
		xQueueReset(urb->queueHandle);
		return urb;
	}
	spin_unlock_irqrestore(&ctx->lock, flags);

	urb = (struct urb *)kzalloc(size, mem_flags);
	if (urb) {
		urb->number_of_packets = iso_desc_count;
		INIT_LIST_ITEM(&urb->urb_list);
		listSET_LIST_ITEM_OWNER(&urb->urb_list, urb);
		complete = xQueueCreate(1, 0);
		urb->queueHandle = complete;
	}

	return urb;
}


static void construct_urb(struct urb *urb, struct usb_host_endpoint *hep,
			  struct usb_device *dev, int endpoint_type,
			  unsigned long pipe, void *buffer, int len,
			  struct devrequest *setup, int interval)
{
	int epnum = usb_pipeendpoint(pipe);
	int is_in = usb_pipein(pipe);

	INIT_LIST_HEAD(&hep->urb_list);
#ifndef NO_GNU
	INIT_LIST_HEAD(&urb->urb_list);
#else
	INIT_LIST_ITEM(&urb->urb_list);
	urb->urb_list.pvOwner = (void *)urb;
#endif
	urb->ep = hep;
	urb->complete = dwc2_host_complete_urb;
	urb->status = -EINPROGRESS;
	urb->dev = dev;
	urb->pipe = pipe;
	urb->transfer_buffer = buffer;
	urb->transfer_dma = (dma_addr_t)buffer;
	urb->transfer_buffer_length = len;
	urb->setup_packet = (unsigned char *)setup;
	urb->setup_dma = (dma_addr_t)setup;
	urb->transfer_flags &= ~(URB_DIR_MASK | URB_DMA_MAP_SINGLE |
			URB_DMA_MAP_PAGE | URB_DMA_MAP_SG | URB_MAP_LOCAL |
			URB_SETUP_MAP_SINGLE | URB_SETUP_MAP_LOCAL |
			URB_DMA_SG_COMBINED);
	urb->transfer_flags |= (is_in ? URB_DIR_IN : URB_DIR_OUT);

	urb->ep->desc.wMaxPacketSize =
		__cpu_to_le16(is_in ? dev->epmaxpacketin[epnum] :
				dev->epmaxpacketout[epnum]);
	urb->ep->desc.bmAttributes = endpoint_type;
	urb->ep->desc.bEndpointAddress =
		(is_in ? USB_DIR_IN : USB_DIR_OUT) | epnum;
	urb->ep->desc.bInterval = interval;
}

static int submit_urb(struct usb_hcd *hcd, struct urb *urb)
{
	int ret;

	ret = dwc2_urb_enqueue(hcd, urb, 0);
	if (ret < 0) {
		printf("Failed to enqueue URB to controller ret=%d \r\n", ret);
		return ret;
	}

	return ret;
}

static int _dwc2_submit_control_msg(struct dwc2_host_data *host,
	struct usb_device *dev, unsigned long pipe,
	void *buffer, int len, struct devrequest *setup, int timeout)
{
	struct urb *urb = NULL;
	struct usb_host_endpoint *hep = NULL;
	int epnum = usb_pipeendpoint(pipe);
	int is_in = usb_pipein(pipe);
	int ret = 0;

	if (epnum > 8 || epnum < 0)
		return -EINVAL;

	if (is_in) {
		hep = &host->hep_in[epnum];
	} else {
		hep = &host->hep_out[epnum];
	}
	urb = usb_urb_alloc(host, 0, IRQ_NONE);
	if (urb == NULL)
		return -ENOMEM;

	construct_urb(urb, hep, dev, USB_ENDPOINT_XFER_CONTROL,
		      pipe, buffer, len, setup, 0);

	/* Fix speed for non hub-attached devices */
	if (!usb_dev_get_parent(dev))
		dev->speed = host->host_speed;
	//unsigned char *a = urb->setup_packet;
	//if (a)
	//printf("setup-->%02x %02x %02x %02x %02x %02x %02x %02x\r\n", a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
	ret = submit_urb(&host->hcd, urb);
	if (ret < 0) {
		printf("%s:%d\r\n", __func__, __LINE__);
		//dwc2_urb_dequeue(&host->hcd, urb, 0);
		//printf("%s:%d\r\n", __func__, __LINE__);
		goto exit;
	}

	ret = xQueueReceive((QueueHandle_t)urb->queueHandle, NULL, timeout);
	if (ret != pdTRUE || 0 != urb->status) {//timeout
		printf("%s:%d\r\n", __func__, __LINE__);
		ret = urb->status;
		dwc2_urb_dequeue(&host->hcd, urb, 0);
		printf("%s:%d\r\n", __func__, __LINE__);
		if (0 != urb->status)
			printf("usb control urb error:%d\r\n", urb->status);
	} else {
		ret = urb->actual_length;
	}
exit:
	usb_urb_release(urb);
	return ret;
}

int submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer, int transfer_len, int *actual_length, int timeout)
{
	struct urb *urb = NULL;
	struct usb_host_endpoint *hep = NULL;
	int epnum = usb_pipeendpoint(pipe);
	int is_in = usb_pipein(pipe);
	int ret = 0;
	struct dwc2_host_data *host = &dwc2_host;

	if (epnum > 8 || epnum < 0)
		return -EINVAL;
	if (is_in) {
		hep = &host->hep_in[epnum];
	} else {
		hep = &host->hep_out[epnum];
	}

	urb = usb_urb_alloc(host, 0, IRQ_NONE);
	if (urb == NULL)
		return -ENOMEM;

	construct_urb(urb, hep, dev, USB_ENDPOINT_XFER_BULK,
		      pipe, buffer, transfer_len, NULL, 0);
	ret = submit_urb(&host->hcd, urb);
	if (ret < 0) {
		printf("%s:%d\r\n", __func__, __LINE__);
		//dwc2_urb_dequeue(&host->hcd, urb, 0);
		//printf("%s:%d\r\n", __func__, __LINE__);
		goto exit;
	}

	ret = xQueueReceive((QueueHandle_t)urb->queueHandle, NULL, timeout);
	if (ret != pdTRUE || 0 != urb->status) {//timeout
		printf("%s:%d\r\n", __func__, __LINE__);
		ret = urb->status;
		dwc2_urb_dequeue(&host->hcd, urb, 0);
		printf("%s:%d\r\n", __func__, __LINE__);
		if (actual_length)
			*actual_length = 0;
		if (0 != urb->status)
			printf("usb bulk urb error:%d\r\n", urb->status);
	} else {
		if (actual_length)
			*actual_length = urb->actual_length;
	}

exit:
	usb_urb_release(urb);

	return ret;
}

static int _dwc2_submit_int_msg(struct dwc2_host_data *host, struct usb_device *dev, unsigned long pipe, void *buffer, int len, int interval, int timeout)
{
	struct urb *urb = NULL;
	struct usb_host_endpoint *hep = NULL;
	int epnum = usb_pipeendpoint(pipe);
	int is_in = usb_pipein(pipe);
	int ret = 0;

	if (epnum > 8 || epnum < 0)
		return -EINVAL;

	if (is_in) {
		hep = &host->hep_in[epnum];
	} else {
		hep = &host->hep_out[epnum];
	}

	urb = usb_urb_alloc(host, 0, IRQ_NONE);
	if (urb == NULL)
		return -ENOMEM;

	construct_urb(urb, hep, dev, USB_ENDPOINT_XFER_INT,
		      pipe, buffer, len, NULL, interval);
	ret = submit_urb(&host->hcd, urb);
	if (ret < 0) {
		dwc2_urb_dequeue(&host->hcd, urb, 0);
		goto exit;
	}

	ret = xQueueReceive((QueueHandle_t)urb->queueHandle, NULL, timeout);
	if (ret != pdTRUE) {//timeout
		dwc2_urb_dequeue(&host->hcd, urb, 0);
	}

exit:
	usb_urb_release(urb);

	return ret;
}
#if 0
static int _dwc2_reset_root_port(struct dwc2_host_data *host,
	struct usb_device *dev)
{

	mdelay(50);
	host->host_speed = USB_SPEED_HIGH;
	mdelay((host->host_speed == USB_SPEED_LOW) ? 200 : 50);

	return 0;
}
#endif
void usb_reset_endpoint()
{
	int i;
	struct dwc2_host_data *host = &dwc2_host;

	if ((NULL == host->dw2_hc_driver) || (NULL == host->dw2_hc_driver->endpoint_reset))
		return;
	for (i = 0; i < 16; i++) {
		host->dw2_hc_driver->endpoint_reset(&(host->hcd), &host->hep_in[i]);
		host->dw2_hc_driver->endpoint_reset(&(host->hcd), &host->hep_out[i]);
	}
}

void usb_disable_endpoint()
{
	int i;
	struct dwc2_host_data *host = &dwc2_host;

	if ((NULL == host->dw2_hc_driver) || (NULL == host->dw2_hc_driver->endpoint_disable))
		return;
	for (i = 0; i < 16; i++) {
		host->dw2_hc_driver->endpoint_disable(&(host->hcd), &host->hep_in[i]);
		host->dw2_hc_driver->endpoint_disable(&(host->hcd), &host->hep_out[i]);
	}
}

void reset_usb_phy()
{
	vSysctlConfigure(SYS_SOFT_RST, 30, 1, 0);//phy reset
	mdelay(10);
	vSysctlConfigure(SYS_SOFT_RST, 30, 1, 1);//phy reset
}

void usb_sysctrl_init()
{
	vSysctlConfigure(SYS_ANA_CFG, 8, 0xff, 4);

	vSysctlConfigure(SYS_SOFT_RST, 30, 1, 0);//phy reset
	vSysctlConfigure(SYS_SOFT_RST, 3, 1, 0);
	vSysctlConfigure(SYS_SOFT1_RST, 5, 1, 0);
	mdelay(10);
	vSysctlConfigure(SYS_SOFT_RST, 30, 1, 1);//phy reset
	vSysctlConfigure(SYS_SOFT_RST, 3, 1, 1);
	vSysctlConfigure(SYS_SOFT1_RST, 5, 1, 1);
	if (!get_usb_mode())
		vSysctlConfigure(SYS_ANA_CFG, 24, 3, 1);// usb host
	else
    	vSysctlConfigure(SYS_ANA_CFG, 24, 3, 3);// usb dev
	mdelay(10);
}



int	hub_status_data(char buf[8])
{
	int ret = 1;
	struct dwc2_host_data *host = &dwc2_host;
	//printf("prvUsbPortScanTimerCallback\r\n");
	ret = host->dw2_hc_driver->hub_status_data(&(host->hcd), buf);
	if (ret != 0) {
		//printf("state:%d usb state changed now!\r\n", buf[0]);
	}
	return ret;
}

void usb_dwc2_lowlevel_restart()
{
	struct dwc2_host_data *host = &dwc2_host;

	if (host->dw2_hc_driver && host->dw2_hc_driver->stop) {
		host->dw2_hc_driver->stop(&(host->hcd));
	}
	msleep(50);
	if (host->dw2_hc_driver && host->dw2_hc_driver->start) {
		host->dw2_hc_driver->start(&(host->hcd));
	}
}

int usb_dwc2_reset(int inIrq, int isDev)
{
	struct dwc2_hsotg *hsotg = dwc2_host.host;
	struct wq_msg *pmsg = &hsotg->xmsg;
	if (isDev)
		pmsg->id = OTG_WQ_MSG_ID_DEV_RESET;
	else
		pmsg->id = OTG_WQ_MSG_ID_HOST_RESET;
	pmsg->delay = 50;

	if (inIrq) {
		xQueueSendFromISR(hsotg->wq_otg, (void*)pmsg, 0);
	} else {
		xQueueSend(hsotg->wq_otg, (void*)pmsg, 0);
	}
	return 0;
}

//#define mainTIMER_SCAN_FREQUENCY_MS			pdMS_TO_TICKS( 1000UL )

int usb_dwc2_lowlevel_init()
{
	struct dwc2_host_data *host = &dwc2_host;
	int ret;

	INIT_LIST_HEAD(&host->free_urb_list);
	spin_lock_init(&host->lock);

	ret = dwc2_driver_init(&host->host, &(host->hcd));
	if (!host->host) {
		printf("MUSB host is not registered\n");
		return -ENODEV;
	}
	host->dw2_hc_driver = dwc2_get_driver();


	return ret;
}

int usb_dwc2_lowlevel_uninit()
{
	if (!dwc2_host.host) {
		printf("MUSB host is not registered\n");
		return -ENODEV;
	}

	dwc2_driver_uninit(dwc2_host.host);
	return 0;
}

int submit_control_msg(struct usb_device *dev, unsigned long pipe,
		       void *buffer, int length, struct devrequest *setup, int timeout)
{
	if (dev->parent == NULL) {
		return dwc_otg_submit_rh_msg(dev, pipe, buffer, length, setup);
	} else {
		return _dwc2_submit_control_msg(&dwc2_host, dev, pipe, buffer, length, setup, timeout);
	}
}

int submit_int_msg(struct usb_device *dev, unsigned long pipe,
		   void *buffer, int length, int interval)
{
	return _dwc2_submit_int_msg(&dwc2_host, dev, pipe, buffer, length, interval, 0);
}

static void construct_iso_urb(struct urb *urb, struct usb_host_endpoint *hep,
			  struct usb_device *dev, int endpoint_type,
			  unsigned long pipe, void *buffer, int len,
			  int num_iso_packets, int interval)
{
	int epnum = usb_pipeendpoint(pipe);
	int is_in = usb_pipein(pipe);
	int i;
	int psize = 512;

	INIT_LIST_HEAD(&hep->urb_list);
#ifndef NO_GNU
	INIT_LIST_HEAD(&urb->urb_list);
#else
	INIT_LIST_ITEM(&urb->urb_list);
	urb->urb_list.pvOwner = (void *)urb;
#endif
	urb->ep = hep;
	urb->complete = dwc2_host_complete_urb;
	urb->status = -EINPROGRESS;
	urb->dev = dev;
	urb->pipe = pipe;
	urb->transfer_buffer = buffer;
	urb->transfer_dma = (dma_addr_t)buffer;
	urb->transfer_buffer_length = len;
	urb->transfer_flags &= ~(URB_DIR_MASK | URB_DMA_MAP_SINGLE |
			URB_DMA_MAP_PAGE | URB_DMA_MAP_SG | URB_MAP_LOCAL |
			URB_SETUP_MAP_SINGLE | URB_SETUP_MAP_LOCAL |
			URB_DMA_SG_COMBINED);
	urb->transfer_flags |= (is_in ? URB_DIR_IN : URB_DIR_OUT);

	urb->ep->desc.wMaxPacketSize =
		__cpu_to_le16(is_in ? dev->epmaxpacketin[epnum] :
				dev->epmaxpacketout[epnum]);
	urb->ep->desc.bmAttributes = endpoint_type;
	urb->ep->desc.bEndpointAddress =
		(is_in ? USB_DIR_IN : USB_DIR_OUT) | epnum;
	urb->ep->desc.bInterval = interval;

	if (num_iso_packets > 0)
		psize = len / num_iso_packets;
	for (i = 0; i < num_iso_packets; ++i) {
		urb->iso_frame_desc[i].offset = i * psize;
		urb->iso_frame_desc[i].length = psize;
	}
}

int submit_iso_msg(struct usb_device *dev, unsigned long pipe, void *buffer, int transfer_len, int *actual_length, int timeout)
{
	struct urb *urb = NULL;
	struct usb_host_endpoint *hep = NULL;
	int epnum = usb_pipeendpoint(pipe);
	int is_in = usb_pipein(pipe);
	int ret = 0;
	struct dwc2_host_data *host = &dwc2_host;
	int num_iso_packets = 1;

	if (epnum > 8 || epnum < 0)
		return -EINVAL;
	if (is_in) {
		hep = &host->hep_in[epnum];
	} else {
		hep = &host->hep_out[epnum];
	}

	urb = usb_urb_alloc(host, num_iso_packets, IRQ_NONE);
	if (urb == NULL)
		return -ENOMEM;

	construct_iso_urb(urb, hep, dev, USB_ENDPOINT_XFER_ISOC,
		      pipe, buffer, transfer_len, num_iso_packets, 0);
	ret = submit_urb(&host->hcd, urb);
	if (ret < 0) {
		dwc2_urb_dequeue(&host->hcd, urb, 0);
		if (actual_length)
			*actual_length = 0;
		goto exit;
	}

	ret = xQueueReceive((QueueHandle_t)urb->queueHandle, NULL, timeout);
	if (ret != pdTRUE || 0 != urb->status) {
		ret = urb->status;
		dwc2_urb_dequeue(&host->hcd, urb, 0);
		if (actual_length)
			*actual_length = 0;
		if (0 != urb->status)
			printf("usb iso urb error:%d\r\n", urb->status);
	} else {
		if (actual_length) {
			int i, tmp_len = 0;
			for (i = 0; i < urb->number_of_packets; i++) {
				tmp_len += urb->iso_frame_desc[i].actual_length;
			}
			*actual_length = tmp_len;
		}
	}

exit:
	usb_urb_release(urb);

	return ret;
}

/*
 * DWC2 to USB API interface
 */
/* Direction: In ; Request: Status */
static int dwc_otg_submit_rh_msg_in_status(struct usb_device *dev, void *buffer,
					   int txlen, struct devrequest *cmd)
{
	int len = 0;
	int stat = 0;

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		*(uint16_t *)buffer = cpu_to_le16(1);
		len = 2;
		break;
	case USB_RECIP_INTERFACE:
	case USB_RECIP_ENDPOINT:
		*(uint16_t *)buffer = cpu_to_le16(0);
		len = 2;
		break;
	case USB_TYPE_CLASS:
		*(uint32_t *)buffer = cpu_to_le32(0);
		len = 4;
		break;
	case USB_RECIP_OTHER | USB_TYPE_CLASS:
		len = 4;
		stat = dwc2_host.dw2_hc_driver->hub_control(&dwc2_host.hcd, (cmd->requesttype << 8) | (cmd->request),
			cmd->value, cmd->index, (char*)buffer, len);

		break;
	default:
		//puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In ; Request: Descriptor */

/* roothub.a masks */
#define RH_A_NDP	(0xff << 0)	/* number of downstream ports */
#define RH_A_PSM	(1 << 8)	/* power switching mode */
#define RH_A_NPS	(1 << 9)	/* no power switching */
#define RH_A_DT		(1 << 10)	/* device type (mbz) */
#define RH_A_OCPM	(1 << 11)	/* over current protection mode */
#define RH_A_NOCP	(1 << 12)	/* no over current protection */
#define RH_A_POTPGT	(0xffUL << 24)	/* power on to power good time */

/* roothub.b masks */
#define RH_B_DR		0x0000ffff	/* device removable flags */
#define RH_B_PPCM	0xffff0000	/* port power control mask */

static int dwc_otg_submit_rh_msg_in_descriptor(struct usb_device *dev,
					       void *buffer, int txlen,
					       struct devrequest *cmd)
{

	unsigned char data[32];
	uint32_t dsc;
	int len = 0;
	int stat = 0;
	uint16_t wValue = cpu_to_le16(cmd->value);
	uint16_t wLength = cpu_to_le16(cmd->length);

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
	  {
		switch (wValue & 0xff00) {
		case 0x0100:	/* device descriptor */
			len = min3(txlen, (int)sizeof(root_hub_dev_des), (int)wLength);
			memcpy(buffer, root_hub_dev_des, len);
			break;
		case 0x0200:	/* configuration descriptor */
			len = min3(txlen, (int)sizeof(root_hub_config_des), (int)wLength);
			memcpy(buffer, root_hub_config_des, len);
			break;
		case 0x0300:	/* string descriptors */
			switch (wValue & 0xff) {
			case 0x00:
				len = min3(txlen, (int)sizeof(root_hub_str_index0),
					   (int)wLength);
				memcpy(buffer, root_hub_str_index0, len);
				break;
			case 0x01:
				len = min3(txlen, (int)sizeof(root_hub_str_index1),
					   (int)wLength);
				memcpy(buffer, root_hub_str_index1, len);
				break;
			}
			break;
		default:
			stat = USB_ST_STALLED;
		}
		break;
	 }
	case USB_TYPE_CLASS:
		/* Root port config, set 1 port and nothing else. */
		dsc = 0x00000001;

		data[0] = 9;		/* min length; */
		data[1] = 0x29;
		data[2] = dsc & RH_A_NDP;
		data[3] = 0;
		if (dsc & RH_A_PSM)
			data[3] |= 0x1;
		if (dsc & RH_A_NOCP)
			data[3] |= 0x10;
		else if (dsc & RH_A_OCPM)
			data[3] |= 0x8;

		/* corresponds to data[4-7] */
		data[5] = (dsc & RH_A_POTPGT) >> 24;
		data[7] = dsc & RH_B_DR;
		if (data[2] < 7) {
			data[8] = 0xff;
		} else {
			data[0] += 2;
			data[8] = (dsc & RH_B_DR) >> 8;
			data[9] = 0xff;
			data[10] = data[9];
		}

		len = min3(txlen, (int)data[0], (int)wLength);
		memcpy(buffer, data, len);
		break;
	default:
		//puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In ; Request: Configuration */
static int dwc_otg_submit_rh_msg_in_configuration(struct usb_device *dev,
						  void *buffer, int txlen,
						  struct devrequest *cmd)
{
	int len = 0;
	int stat = 0;

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		*(uint8_t *)buffer = 0x01;
		len = 1;
		break;
	default:
		//puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In */
static int dwc_otg_submit_rh_msg_in(
				    struct usb_device *dev, void *buffer,
				    int txlen, struct devrequest *cmd)
{
	switch (cmd->request) {
	case USB_REQ_GET_STATUS:
		return dwc_otg_submit_rh_msg_in_status(dev, buffer,
						       txlen, cmd);
	case USB_REQ_GET_DESCRIPTOR:
		return dwc_otg_submit_rh_msg_in_descriptor(dev, buffer,
							   txlen, cmd);
	case USB_REQ_GET_CONFIGURATION:
		return dwc_otg_submit_rh_msg_in_configuration(dev, buffer,
							      txlen, cmd);
	default:
		//puts("unsupported root hub command\n");
		return USB_ST_STALLED;
	}
}

/* Direction: Out */
static int dwc_otg_submit_rh_msg_out(
				     struct usb_device *dev,
				     void *buffer, int txlen,
				     struct devrequest *cmd)
{
	int len = 0;
	int stat = 0;
	uint16_t bmrtype_breq = cmd->requesttype | (cmd->request << 8);
	uint16_t wValue = cpu_to_le16(cmd->value);
	switch (bmrtype_breq & ~USB_DIR_IN) {
	case (USB_REQ_CLEAR_FEATURE << 8) | USB_RECIP_ENDPOINT:
	case (USB_REQ_CLEAR_FEATURE << 8) | USB_TYPE_CLASS:
		break;

	case (USB_REQ_CLEAR_FEATURE << 8) | USB_RECIP_OTHER | USB_TYPE_CLASS:
	case (USB_REQ_SET_FEATURE << 8) | USB_RECIP_OTHER | USB_TYPE_CLASS:
		dwc2_host.dw2_hc_driver->hub_control(&dwc2_host.hcd, (cmd->requesttype << 8) | (cmd->request),
			cmd->value, cmd->index, (char*)buffer, txlen);
		break;
	case (USB_REQ_SET_ADDRESS << 8):
		dwc2_host.root_hub_devnum = wValue;
		dwc2_host.dw2_hc_driver->hub_control(&dwc2_host.hcd, (cmd->requesttype << 8) | (cmd->request),
			cmd->value, cmd->index, (char*)buffer, txlen);
		break;
	case (USB_REQ_SET_CONFIGURATION << 8):
		break;
	default:
		//puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	len = min(len, txlen);

	dev->act_len = len;
	dev->status = stat;

	return stat;
}

static int dwc_otg_submit_rh_msg(struct usb_device *dev,
				 unsigned long pipe, void *buffer, int txlen,
				 struct devrequest *cmd)
{
	int stat = 0;
	if (usb_pipeint(pipe)) {
		//puts("Root-Hub submit IRQ: NOT implemented\n");
		return 0;
	}

	if (cmd->requesttype & USB_DIR_IN)
		stat = dwc_otg_submit_rh_msg_in(dev, buffer, txlen, cmd);
	else
		stat = dwc_otg_submit_rh_msg_out(dev, buffer, txlen, cmd);

	stat = dev->act_len;
	return stat;
}


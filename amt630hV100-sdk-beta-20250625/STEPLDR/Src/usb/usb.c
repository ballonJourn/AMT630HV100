#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

#define CONFIG_USB_STORAGE

#include "usb_os_adapter.h"
#include "trace.h"
#include <linux/usb/ch9.h>
#include "usb.h"
#include "dwc2_compat.h"
#include <linux/errno.h>
#include "ark_dwc2.h"
#include "timer.h"

extern int	hub_status_data(char buf[8]);
void usb_stor_disconnect();
static char connect_status = 0;

#define USB_BUFSIZ	512
#define HUB_SHORT_RESET_TIME    20
#define HUB_LONG_RESET_TIME 200
#define PORT_OVERCURRENT_MAX_SCAN_COUNT     3
#ifndef CONFIG_USB_MAX_CONTROLLER_COUNT
#define CONFIG_USB_MAX_CONTROLLER_COUNT 1
#endif


static int asynch_allowed;
static struct usb_device usb_dev[USB_MAX_DEVICE];
static int dev_index;

struct usb_device_scan {
	ListItem_t list;
	struct usb_device *dev;		/* USB hub device to scan */
	struct usb_hub_device *hub;	/* USB hub struct */
	int port;			/* USB port to scan */
};

static List_t usb_scan_list;
static struct usb_hub_device hub_dev[USB_MAX_HUB];
static int usb_hub_index;

void usb_hub_reset(void)
{
	usb_hub_index = 0;

	/* Zero out global hub_dev in case its re-used again */
	memset(hub_dev, 0, sizeof(hub_dev));
}

static struct usb_hub_device *usb_hub_allocate(void)
{
	if (usb_hub_index < USB_MAX_HUB)
		return &hub_dev[usb_hub_index++];

	return NULL;
}

/*-------------------------------------------------------------------
 * Max Packet stuff
 */

/*
 * returns the max packet size, depending on the pipe direction and
 * the configurations values
 */
int usb_maxpacket(struct usb_device *dev, unsigned long pipe)
{
	/* direction is out -> use emaxpacket out */
	if ((pipe & USB_DIR_IN) == 0)
		return dev->epmaxpacketout[((pipe>>15) & 0xf)];
	else
		return dev->epmaxpacketin[((pipe>>15) & 0xf)];
}

/*
 * The routine usb_set_maxpacket_ep() is extracted from the loop of routine
 * usb_set_maxpacket(), because the optimizer of GCC 4.x chokes on this routine
 * when it is inlined in 1 single routine. What happens is that the register r3
 * is used as loop-count 'i', but gets overwritten later on.
 * This is clearly a compiler bug, but it is easier to workaround it here than
 * to update the compiler (Occurs with at least several GCC 4.{1,2},x
 * CodeSourcery compilers like e.g. 2007q3, 2008q1, 2008q3 lite editions on ARM)
 *
 * NOTE: Similar behaviour was observed with GCC4.6 on ARMv5.
 */
static void usb_set_maxpacket_ep(struct usb_device *dev, int if_idx, int ep_idx)
{
	int b;
	struct usb_endpoint_descriptor *ep;
	u16 ep_wMaxPacketSize;
	unsigned char *tmp;

	ep = &dev->config.if_desc[if_idx].ep_desc[ep_idx];

	b = ep->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	tmp = (unsigned char*)&ep->wMaxPacketSize;
	ep_wMaxPacketSize = (tmp[1] << 8) | tmp[0];//get_unaligned(&ep->wMaxPacketSize);
	USB_UNUSED(ep_wMaxPacketSize);
	
	if ((ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
						USB_ENDPOINT_XFER_CONTROL) {
		/* Control => bidirectional */
		dev->epmaxpacketout[b] = ep_wMaxPacketSize;
		dev->epmaxpacketin[b] = ep_wMaxPacketSize;
		/* debug("##Control EP epmaxpacketout/in[%d] = %d\n",
		      b, dev->epmaxpacketin[b]); */
	} else {
		if ((ep->bEndpointAddress & 0x80) == 0) {
			/* OUT Endpoint */
			if (ep_wMaxPacketSize > dev->epmaxpacketout[b]) {
				dev->epmaxpacketout[b] = ep_wMaxPacketSize;
				/* debug("##EP epmaxpacketout[%d] = %d\n",
				      b, dev->epmaxpacketout[b]); */
			}
		} else {
			/* IN Endpoint */
			if (ep_wMaxPacketSize > dev->epmaxpacketin[b]) {
				dev->epmaxpacketin[b] = ep_wMaxPacketSize;
				/* debug("##EP epmaxpacketin[%d] = %d\n",
				      b, dev->epmaxpacketin[b]); */
			}
		} /* if out */
	} /* if control */
}

/*
 * set the max packed value of all endpoints in the given configuration
 */
static int usb_set_maxpacket(struct usb_device *dev)
{
	int i, ii;

	for (i = 0; i < dev->config.desc.bNumInterfaces; i++)
		for (ii = 0; ii < dev->config.if_desc[i].desc.bNumEndpoints; ii++)
			usb_set_maxpacket_ep(dev, i, ii);

	return 0;
}

/*
 * submits a control message and waits for comletion (at least timeout * 1ms)
 * If timeout is 0, we don't wait for completion (used as example to set and
 * clear keyboards LEDs). For data transfers, (storage transfers) we don't
 * allow control messages with 0 timeout, by previousely resetting the flag
 * asynch_allowed (usb_disable_asynch(1)).
 * returns the transferred length if OK or -1 if error. The transferred length
 * and the current status are stored in the dev->act_len and dev->status.
 */
int usb_control_msg(struct usb_device *dev, unsigned int pipe,
			unsigned char request, unsigned char requesttype,
			unsigned short value, unsigned short index,
			void *data, unsigned short size, int timeout)
{
	ALLOC_CACHE_ALIGN_BUFFER(struct devrequest, setup_packet, 1);
	int err;

	if ((timeout == 0) && (!asynch_allowed)) {
		/* request for a asynch control pipe is not allowed */
		return -EINVAL;
	}

	/* set setup command */
	setup_packet->requesttype = requesttype;
	setup_packet->request = request;
	setup_packet->value = value;
	setup_packet->index = index;
	setup_packet->length = size;

	if (0 == connect_status && dev->parent != NULL) {
		return -1;
	}
	dev->status = USB_ST_NOT_PROC; /*not yet processed */

	err = submit_control_msg(dev, pipe, data, size, setup_packet, timeout);
	if (err < 0) {
		err = -1;
	}
	if (dev->status) {
		err = -1;
	}

	return err;

}

/*-------------------------------------------------------------------
 * submits bulk message, and waits for completion. returns 0 if Ok or
 * negative if Error.
 * synchronous behavior
 */
#define BULK_BUF_LEN  2048
int usb_bulk_msg(struct usb_device *dev, unsigned int pipe,
			void *data, int len, int *actual_length, int timeout)
{
#if 1
	int err;
	ALLOC_CACHE_ALIGN_BUFFER(char, dma_buf, BULK_BUF_LEN);
	int transfer_len = 0, total_len = 0, act_len = 0;
	unsigned int addr = 0;
	int is_in = usb_pipein(pipe);

	if (len < 0)
		return -EINVAL;
	while(len > 0) {
		dev->status = USB_ST_NOT_PROC; /*not yet processed */
		if (len < BULK_BUF_LEN)
			transfer_len = len;
		else
			transfer_len = BULK_BUF_LEN;
		if (is_in == 0) {
			memcpy(dma_buf, data, transfer_len);
		}
		if (0 == connect_status && dev->parent != NULL) {
			dev->status = -1;
			break;
		}
		err = submit_bulk_msg(dev, pipe, dma_buf, transfer_len, &act_len, timeout);
		if (err < 0)
			break;
		if (is_in) {
			memcpy(data, dma_buf, transfer_len);
		}
		total_len  += act_len;
		addr = (unsigned int)data + transfer_len;
		data       = (void *)addr;
		len        -= transfer_len;
	}
	if (len == 0)
		*actual_length = total_len;
	else
		*actual_length = 0;
	if (dev->status)
		return -1;
	return 0;
#else
	int err;

	if (len < 0)
		return -EINVAL;
	dev->status = USB_ST_NOT_PROC; /*not yet processed */
	err = submit_bulk_msg(dev, pipe, data, len, timeout);
	if (err < 0)
		return err;
	
	*actual_length = dev->act_len;
	if (dev->status)
		return -1;
	return 0;
#endif
}

/*******************************************************************************
 * Parse the config, located in buffer, and fills the dev->config structure.
 * Note that all little/big endian swapping are done automatically.
 * (wTotalLength has already been swapped and sanitized when it was read.)
 */
static int usb_parse_config(struct usb_device *dev,
			unsigned char *buffer, int cfgno)
{
	struct usb_descriptor_header *head;
	int index, ifno, epno, curr_if_num;
	u16 ep_wMaxPacketSize;
	unsigned char *tmp;
	struct usb_interface *if_desc = NULL;

	ifno = -1;
	epno = -1;
	curr_if_num = -1;

	dev->configno = cfgno;
	head = (struct usb_descriptor_header *) &buffer[0];
	if (head->bDescriptorType != USB_DT_CONFIG) {
		/* printf(" ERROR: NOT USB_CONFIG_DESC %x\n",
			head->bDescriptorType); */
		return -EINVAL;
	}
	if (head->bLength != USB_DT_CONFIG_SIZE) {
		//printf("ERROR: Invalid USB CFG length (%d)\n", head->bLength);
		return -EINVAL;
	}
	memcpy(&dev->config, head, USB_DT_CONFIG_SIZE);
	dev->config.no_of_if = 0;

	index = dev->config.desc.bLength;
	/* Ok the first entry must be a configuration entry,
	 * now process the others */
	head = (struct usb_descriptor_header *) &buffer[index];
	while (index + 1 < dev->config.desc.wTotalLength && head->bLength) {
		switch (head->bDescriptorType) {
		case USB_DT_INTERFACE:
			if (head->bLength != USB_DT_INTERFACE_SIZE) {
				/* printf("ERROR: Invalid USB IF length (%d)\n",
					head->bLength); */
				break;
			}
			if (index + USB_DT_INTERFACE_SIZE >
			    dev->config.desc.wTotalLength) {
				//puts("USB IF descriptor overflowed buffer!\n");
				break;
			}
			if (((struct usb_interface_descriptor *) \
			     head)->bInterfaceNumber != curr_if_num) {
				/* this is a new interface, copy new desc */
				ifno = dev->config.no_of_if;
				if (ifno >= USB_MAXINTERFACES) {
					//puts("Too many USB interfaces!\n");
					/* try to go on with what we have */
					return -EINVAL;
				}
				if_desc = &dev->config.if_desc[ifno];
				dev->config.no_of_if++;
				memcpy(if_desc, head,
					USB_DT_INTERFACE_SIZE);
				if_desc->no_of_ep = 0;
				if_desc->num_altsetting = 1;
				curr_if_num =
				     if_desc->desc.bInterfaceNumber;
			} else {
				/* found alternate setting for the interface */
				if (ifno >= 0) {
					if_desc = &dev->config.if_desc[ifno];
					if_desc->num_altsetting++;
				}
			}
			break;
		case USB_DT_ENDPOINT:
			if (head->bLength != USB_DT_ENDPOINT_SIZE &&
			    head->bLength != USB_DT_ENDPOINT_AUDIO_SIZE) {
				/* printf("ERROR: Invalid USB EP length (%d)\n",
					head->bLength); */
				break;
			}
			if (index + head->bLength >
			    dev->config.desc.wTotalLength) {
				//puts("USB EP descriptor overflowed buffer!\n");
				break;
			}
			if (ifno < 0) {
				//puts("Endpoint descriptor out of order!\n");
				break;
			}
			epno = dev->config.if_desc[ifno].no_of_ep;
			if_desc = &dev->config.if_desc[ifno];
			if (epno >= USB_MAXENDPOINTS) {
				/* printf("Interface %d has too many endpoints!\n",
					if_desc->desc.bInterfaceNumber); */
				return -EINVAL;
			}
			/* found an endpoint */
			if_desc->no_of_ep++;
			memcpy(&if_desc->ep_desc[epno], head,
				USB_DT_ENDPOINT_SIZE);
			/* ep_wMaxPacketSize = get_unaligned(&dev->config.\
							if_desc[ifno].\
							ep_desc[epno].\
							wMaxPacketSize); */
			tmp = (unsigned char*)&dev->config.if_desc[ifno].ep_desc[epno].wMaxPacketSize;
			ep_wMaxPacketSize = (tmp[1] << 8) | tmp[0];
			USB_UNUSED(ep_wMaxPacketSize);
			/* put_unaligned(le16_to_cpu(ep_wMaxPacketSize),
					&dev->config.\
					if_desc[ifno].\
					ep_desc[epno].\
					wMaxPacketSize); */
			//debug("if %d, ep %d\n", ifno, epno);
			break;
		case USB_DT_SS_ENDPOINT_COMP:
			if (head->bLength != USB_DT_SS_EP_COMP_SIZE) {
				/* printf("ERROR: Invalid USB EPC length (%d)\n",
					head->bLength); */
				break;
			}
			if (index + USB_DT_SS_EP_COMP_SIZE >
			    dev->config.desc.wTotalLength) {
				//puts("USB EPC descriptor overflowed buffer!\n");
				break;
			}
			if (ifno < 0 || epno < 0) {
				//puts("EPC descriptor out of order!\n");
				break;
			}
			if_desc = &dev->config.if_desc[ifno];
			memcpy(&if_desc->ss_ep_comp_desc[epno], head,
				USB_DT_SS_EP_COMP_SIZE);
			break;
		default:
			if (head->bLength == 0)
				return -EINVAL;

			/* debug("unknown Description Type : %x\n",
			      head->bDescriptorType); */

#ifdef DEBUG
			{
				unsigned char *ch = (unsigned char *)head;
				int i;

				for (i = 0; i < head->bLength; i++)
					debug("%02X ", *ch++);
				debug("\n\n\n");
			}
#endif
			break;
		}
		index += head->bLength;
		head = (struct usb_descriptor_header *)&buffer[index];
	}
	return 0;
}

struct usb_device *usb_get_dev_index(int index)
{
	if (usb_dev[index].devnum == -1)
		return NULL;
	else
		return &usb_dev[index];
}

void usb_free_device(void *controller)
{
	dev_index--;
	//debug("Freeing device node: %d\n", dev_index);
	memset(&usb_dev[dev_index], 0, sizeof(struct usb_device));
	usb_dev[dev_index].devnum = -1;
}

int usb_alloc_device(struct usb_device *udev)
{
	return 0;
}

/***********************************************************************
 * Clears an endpoint
 * endp: endpoint number in bits 0-3;
 * direction flag in bit 7 (1 = IN, 0 = OUT)
 */
int usb_clear_halt(struct usb_device *dev, int pipe)
{
	int result;
	int endp = usb_pipeendpoint(pipe)|(usb_pipein(pipe)<<7);

	result = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				 USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0,
				 endp, NULL, 0, USB_CNTL_TIMEOUT * 3);

	/* don't clear if failed */
	if (result < 0)
		return result;

	/*
	 * NOTE: we do not get status and verify reset was successful
	 * as some devices are reported to lock up upon this check..
	 */

	usb_endpoint_running(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));

	/* toggle is reset on clear */
	usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 0);
	return 0;
}

/**********************************************************************
 * get_descriptor type
 */
static int usb_get_descriptor(struct usb_device *dev, unsigned char type,
			unsigned char index, void *buf, int size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			       USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
			       (type << 8) + index, 0, buf, size,
			       USB_CNTL_TIMEOUT);
}

/**********************************************************************
 * gets len of configuration cfgno
 */
int usb_get_configuration_len(struct usb_device *dev, int cfgno)
{
	int result;
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, 9);
	struct usb_config_descriptor *config;

	config = (struct usb_config_descriptor *)&buffer[0];
	result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno, buffer, 9);
	if (result < 9) {
		if (result < 0)
			;/* printf("unable to get descriptor, error %lX\n",
				dev->status); */
		else
			;/* printf("config descriptor too short " \
				"(expected %i, got %i)\n", 9, result); */
		return -EIO;
	}
	return config->wTotalLength;
}

/**********************************************************************
 * gets configuration cfgno and store it in the buffer
 */
int usb_get_configuration_no(struct usb_device *dev, int cfgno,
			     unsigned char *buffer, int length)
{
	int result;
	struct usb_config_descriptor *config;

	config = (struct usb_config_descriptor *)&buffer[0];
	result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno, buffer, length);
	/* debug("get_conf_no %d Result %d, wLength %d\n", cfgno, result,
	      le16_to_cpu(config->wTotalLength)); */
	config->wTotalLength = result; /* validated, with CPU byte order */

	return result;
}

static int get_descriptor_len(struct usb_device *dev, int len, int expect_len)
{
	struct usb_device_descriptor *desc;
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, tmpbuf, USB_BUFSIZ);
	int err;

	desc = (struct usb_device_descriptor *)tmpbuf;

	err = usb_get_descriptor(dev, USB_DT_DEVICE, 0, desc, len);
	if (err < expect_len) {
		if (err < 0) {
			/* printf("unable to get device descriptor (error=%d)\n",
				err); */
			return err;
		} else {
			/* printf("USB device descriptor short read (expected %i, got %i)\n",
				expect_len, err); */
			return -EIO;
		}
	}
	memcpy(&dev->descriptor, tmpbuf, sizeof(dev->descriptor));

	return 0;
}

/********************************************************************
 * set address of a device to the value in dev->devnum.
 * This can only be done by addressing the device via the default address (0)
 */
static int usb_set_address(struct usb_device *dev)
{
	//debug("set address %d\n", dev->devnum);

	return usb_control_msg(dev, usb_snddefctrl(dev), USB_REQ_SET_ADDRESS,
			       0, (dev->devnum), 0, NULL, 0, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * set interface number to interface
 */
int usb_set_interface(struct usb_device *dev, int interface, int alternate)
{
	struct usb_interface *if_face = NULL;
	int ret, i;

	for (i = 0; i < dev->config.desc.bNumInterfaces; i++) {
		if (dev->config.if_desc[i].desc.bInterfaceNumber == interface) {
			if_face = &dev->config.if_desc[i];
			break;
		}
	}
	if (!if_face) {
		//printf("selecting invalid interface %d", interface);
		return -EINVAL;
	}
	/*
	 * We should return now for devices with only one alternate setting.
	 * According to 9.4.10 of the Universal Serial Bus Specification
	 * Revision 2.0 such devices can return with a STALL. This results in
	 * some USB sticks timeouting during initialization and then being
	 * unusable in U-Boot.
	 */
	if (if_face->num_altsetting == 1)
		return 0;

	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE,
				alternate, interface, NULL, 0,
				USB_CNTL_TIMEOUT * 5);
	if (ret < 0)
		return ret;

	return 0;
}

/********************************************************************
 * set configuration number to configuration
 */
static int usb_set_configuration(struct usb_device *dev, int configuration)
{
	int res;
	//debug("set configuration %d\n", configuration);
	/* set setup command */
	res = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_CONFIGURATION, 0,
				configuration, 0,
				NULL, 0, USB_CNTL_TIMEOUT);
	if (res == 0) {
		dev->toggle[0] = 0;
		dev->toggle[1] = 0;
		return 0;
	} else
		return -EIO;
}

static int usb_setup_descriptor(struct usb_device *dev, bool do_read)
{
	/*
	 * This is a Windows scheme of initialization sequence, with double
	 * reset of the device (Linux uses the same sequence)
	 * Some equipment is said to work only with such init sequence; this
	 * patch is based on the work by Alan Stern:
	 * http://sourceforge.net/mailarchive/forum.php?
	 * thread_id=5729457&forum_id=5398
	 */

	/*
	 * send 64-byte GET-DEVICE-DESCRIPTOR request.  Since the descriptor is
	 * only 18 bytes long, this will terminate with a short packet.  But if
	 * the maxpacket size is 8 or 16 the device may be waiting to transmit
	 * some more, or keeps on retransmitting the 8 byte header.
	 */

	if (dev->speed == USB_SPEED_LOW) {
		dev->descriptor.bMaxPacketSize0 = 8;
		dev->maxpacketsize = PACKET_SIZE_8;
	} else {
		dev->descriptor.bMaxPacketSize0 = 64;
		dev->maxpacketsize = PACKET_SIZE_64;
	}
	dev->epmaxpacketin[0] = dev->descriptor.bMaxPacketSize0;
	dev->epmaxpacketout[0] = dev->descriptor.bMaxPacketSize0;

	if (do_read && dev->speed == USB_SPEED_FULL) {
		int err;

		/*
		 * Validate we've received only at least 8 bytes, not that
		 * we've received the entire descriptor. The reasoning is:
		 * - The code only uses fields in the first 8 bytes, so
		 *   that's all we need to have fetched at this stage.
		 * - The smallest maxpacket size is 8 bytes. Before we know
		 *   the actual maxpacket the device uses, the USB controller
		 *   may only accept a single packet. Consequently we are only
		 *   guaranteed to receive 1 packet (at least 8 bytes) even in
		 *   a non-error case.
		 *
		 * At least the DWC2 controller needs to be programmed with
		 * the number of packets in addition to the number of bytes.
		 * A request for 64 bytes of data with the maxpacket guessed
		 * as 64 (above) yields a request for 1 packet.
		 */
		err = get_descriptor_len(dev, 64, 8);
		if (err)
			return err;
	}

	dev->epmaxpacketin[0] = dev->descriptor.bMaxPacketSize0;
	dev->epmaxpacketout[0] = dev->descriptor.bMaxPacketSize0;
	switch (dev->descriptor.bMaxPacketSize0) {
	case 8:
		dev->maxpacketsize  = PACKET_SIZE_8;
		break;
	case 16:
		dev->maxpacketsize = PACKET_SIZE_16;
		break;
	case 32:
		dev->maxpacketsize = PACKET_SIZE_32;
		break;
	case 64:
		dev->maxpacketsize = PACKET_SIZE_64;
		break;
	default:
		//printf("%s: invalid max packet size\n", __func__);
		return -EIO;
	}

	return 0;
}

int usb_alloc_new_device(void *controller, struct usb_device **devp)
{
	int i;

	if (dev_index == USB_MAX_DEVICE) {
		SendUartString("ERROR, too many USB Devices\n");
		return -ENOSPC;
	}
	/* default Address is 0, real addresses start with 1 */
	usb_dev[dev_index].devnum = dev_index + 1;
	usb_dev[dev_index].maxchild = 0;
	for (i = 0; i < USB_MAXCHILDREN; i++)
		usb_dev[dev_index].children[i] = NULL;
	usb_dev[dev_index].parent = NULL;
	usb_dev[dev_index].controller = controller;
	dev_index++;
	*devp = &usb_dev[dev_index - 1];

	return 0;
}

static int usb_prepare_device(struct usb_device *dev, int addr, bool do_read,
			      struct usb_device *parent)
{
	int err;

	/*
	 * Allocate usb 3.0 device context.
	 * USB 3.0 (xHCI) protocol tries to allocate device slot
	 * and related data structures first. This call does that.
	 * Refer to sec 4.3.2 in xHCI spec rev1.0
	 */
	err = usb_alloc_device(dev);
	if (err) {
		//printf("Cannot allocate device context to get SLOT_ID\n");
		return err;
	}
	err = usb_setup_descriptor(dev, do_read);
	if (err)
		return err;
	/* err = usb_hub_port_reset(dev, parent);
	if (err)
		return err; */

	dev->devnum = addr;

	err = usb_set_address(dev); /* set address */

	if (err < 0) {
		/* printf("\n      USB device not accepting new address " \
			"(error=%lX)\n", dev->status); */
		return err;
	}

	mdelay(10);	/* Let the SET_ADDRESS settle */

	/*
	 * If we haven't read device descriptor before, read it here
	 * after device is assigned an address. This is only applicable
	 * to xHCI so far.
	 */
	if (!do_read) {
		err = usb_setup_descriptor(dev, true);
		if (err)
			return err;
	}

	return 0;
}

int usb_select_config(struct usb_device *dev)
{
	unsigned char *rawtmpbuf = NULL, *tmpbuf = NULL;
	int err;

	err = get_descriptor_len(dev, USB_DT_DEVICE_SIZE, USB_DT_DEVICE_SIZE);
	if (err)
		return err;

	/*
	 * Kingston DT Ultimate 32GB USB 3.0 seems to be extremely sensitive
	 * about this first Get Descriptor request. If there are any other
	 * requests in the first microframe, the stick crashes. Wait about
	 * one microframe duration here (1mS for USB 1.x , 125uS for USB 2.0).
	 */
	mdelay(1);

	/* only support for one config for now */
	err = usb_get_configuration_len(dev, 0);
	if (err >= 0) {
		rawtmpbuf = (unsigned char *)malloc(err + ARCH_DMA_MINALIGN - 1);
		if (!rawtmpbuf)
			err = -ENOMEM;
		else {
			tmpbuf = (unsigned char*)ALIGN((uintptr_t)rawtmpbuf, ARCH_DMA_MINALIGN);
			err = usb_get_configuration_no(dev, 0, tmpbuf, err);
		}
	}
	if (err < 0) {
		/* printf("usb_new_device: Cannot read configuration, " \
		       "skipping device %04x:%04x\n",
		       dev->descriptor.idVendor, dev->descriptor.idProduct); */
		if (rawtmpbuf)
			free(rawtmpbuf);
		return err;
	}
	usb_parse_config(dev, tmpbuf, 0);
	free(rawtmpbuf);
	usb_set_maxpacket(dev);
	/*
	 * we set the default configuration here
	 * This seems premature. If the driver wants a different configuration
	 * it will need to select itself.
	 */
	err = usb_set_configuration(dev, dev->config.desc.bConfigurationValue);
	if (err < 0) {
		/* printf("failed to set default configuration " \
			"len %d, status %lX\n", dev->act_len, dev->status); */
		return err;
	}

	/*
	 * Wait until the Set Configuration request gets processed by the
	 * device. This is required by at least SanDisk Cruzer Pop USB 2.0
	 * and Kingston DT Ultimate 32GB USB 3.0 on DWC2 OTG controller.
	 */
	mdelay(10);

	return 0;
}

int usb_setup_device(struct usb_device *dev, bool do_read,
		     struct usb_device *parent)
{
	int addr;
	int ret;

	/* We still haven't set the Address yet */
	addr = dev->devnum;
	dev->devnum = 0;

	ret = usb_prepare_device(dev, addr, do_read, parent);
	if (ret)
		return ret;
	ret = usb_select_config(dev);

	return ret;
}

bool usb_device_has_child_on_port(struct usb_device *parent, int port)
{
	return parent->children[port] != NULL;
}


static inline bool usb_hub_is_superspeed(struct usb_device *hdev)
{
	return hdev->descriptor.bDeviceProtocol == 3;
}

static int usb_get_hub_descriptor(struct usb_device *dev, void *data, int size)
{
	unsigned short dtype = USB_DT_HUB;

	if (usb_hub_is_superspeed(dev))
		dtype = USB_DT_SS_HUB;

	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
		dtype << 8, 0, data, size, USB_CNTL_TIMEOUT);
}

static struct usb_hub_device *usb_get_hub_device(struct usb_device *dev)
{
	struct usb_hub_device *hub;

	/* "allocate" Hub device */
	hub = usb_hub_allocate();

	return hub;
}

static int usb_get_hub_status(struct usb_device *dev, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
			data, sizeof(struct usb_hub_status), USB_CNTL_TIMEOUT);
}

static int usb_clear_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_CLEAR_FEATURE, USB_RT_PORT, feature,
				port, NULL, 0, USB_CNTL_TIMEOUT);
}

static int usb_set_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_FEATURE, USB_RT_PORT, feature,
				port, NULL, 0, USB_CNTL_TIMEOUT);
}

int usb_get_port_status(struct usb_device *dev, int port, void *data)
{
	int ret;

	ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port,
			data, sizeof(struct usb_port_status), USB_CNTL_TIMEOUT);

	return ret;
}

static void usb_hub_power_on(struct usb_hub_device *hub)
{
	int i;
	struct usb_device *dev;
	unsigned pgood_delay = hub->desc.bPwrOn2PwrGood * 2;
	//const char *env;

	dev = hub->pusb_dev;

	//debug("enabling power on all ports\n");
	for (i = 0; i < dev->maxchild; i++) {
		usb_set_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);
		//debug("port %d returns %lX\n", i + 1, dev->status);
	}

	/*
	 * Wait for power to become stable,
	 * plus spec-defined max time for device to connect
	 * but allow this time to be increased via env variable as some
	 * devices break the spec and require longer warm-up times
	 */
	/* env = env_get("usb_pgood_delay");
	if (env)
		pgood_delay = max(pgood_delay,
			          (unsigned)simple_strtol(env, NULL, 0));
	debug("pgood_delay=%dms\n", pgood_delay); */

	/*
	 * Do a minimum delay of the larger value of 100ms or pgood_delay
	 * so that the power can stablize before the devices are queried
	 */
	hub->query_delay = get_timer(0) + max(100, (int)pgood_delay);

	/*
	 * Record the power-on timeout here. The max. delay (timeout)
	 * will be done based on this value in the USB port loop in
	 * usb_hub_configure() later.
	 */
	hub->connect_timeout = hub->query_delay + 1000;
	/* debug("devnum=%d poweron: query_delay=%d connect_timeout=%d\n",
	      dev->devnum, max(100, (int)pgood_delay),
	      max(100, (int)pgood_delay) + 1000); */
}

#define MAX_TRIES 5

/**
 * usb_hub_port_reset() - reset a port given its usb_device pointer
 *
 * Reset a hub port and see if a device is present on that port, providing
 * sufficient time for it to show itself. The port status is returned.
 *
 * @dev:	USB device to reset
 * @port:	Port number to reset (note ports are numbered from 0 here)
 * @portstat:	Returns port status
 */
static int usb_hub_port_reset(struct usb_device *dev, int port,
			      unsigned short *portstat)
{
	int err, tries;
	ALLOC_CACHE_ALIGN_BUFFER(struct usb_port_status, portsts, 1);
	unsigned short portstatus, portchange;
	int delay = HUB_SHORT_RESET_TIME; /* start with short reset delay */

	//debug("%s: resetting port %d...\n", __func__, port + 1);

	for (tries = 0; tries < MAX_TRIES; tries++) {
		err = usb_set_port_feature(dev, port + 1, USB_PORT_FEAT_RESET);
		if (err < 0)
			return err;

		mdelay(delay);

		if (usb_get_port_status(dev, port + 1, portsts) < 0) {
			/* debug("get_port_status failed status %lX\n",
			      dev->status); */
			return -1;
		}
		portstatus = le16_to_cpu(portsts->wPortStatus);
		portchange = le16_to_cpu(portsts->wPortChange);
		USB_UNUSED(portchange);
		/* debug("portstatus %x, change %x, %s\n", portstatus, portchange,
							portspeed(portstatus));

		debug("STAT_C_CONNECTION = %d STAT_CONNECTION = %d" \
		      "  USB_PORT_STAT_ENABLE %d\n",
		      (portchange & USB_PORT_STAT_C_CONNECTION) ? 1 : 0,
		      (portstatus & USB_PORT_STAT_CONNECTION) ? 1 : 0,
		      (portstatus & USB_PORT_STAT_ENABLE) ? 1 : 0); */

		/*
		 * Perhaps we should check for the following here:
		 * - C_CONNECTION hasn't been set.
		 * - CONNECTION is still set.
		 *
		 * Doing so would ensure that the device is still connected
		 * to the bus, and hasn't been unplugged or replaced while the
		 * USB bus reset was going on.
		 *
		 * However, if we do that, then (at least) a San Disk Ultra
		 * USB 3.0 16GB device fails to reset on (at least) an NVIDIA
		 * Tegra Jetson TK1 board. For some reason, the device appears
		 * to briefly drop off the bus when this second bus reset is
		 * executed, yet if we retry this loop, it'll eventually come
		 * back after another reset or two.
		 */

		if (portstatus & USB_PORT_STAT_ENABLE)
			break;

		/* Switch to long reset delay for the next round */
		delay = HUB_LONG_RESET_TIME;
	}

	if (tries == MAX_TRIES) {
		/* debug("Cannot enable port %i after %i retries, " \
		      "disabling port.\n", port + 1, MAX_TRIES);
		debug("Maybe the USB cable is bad?\n"); */
		return -1;
	}

	usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_C_RESET);
	if (portstat)
		*portstat = portstatus;
	return 0;
}

int usb_new_device(struct usb_device *dev);

int usb_hub_port_connect_change(struct usb_device *dev, int port)
{
	ALLOC_CACHE_ALIGN_BUFFER(struct usb_port_status, portsts, 1);
	unsigned short portstatus;
	int ret, speed;

	/* Check status */
	ret = usb_get_port_status(dev, port + 1, portsts);
	if (ret < 0) {
		//debug("get_port_status failed\n");
		return ret;
	}

	portstatus = portsts->wPortStatus;
	/* debug("portstatus %x, change %x, %s\n",
	      portstatus,
	      le16_to_cpu(portsts->wPortChange),
	      portspeed(portstatus)); */

	/* Clear the connection change status */
	usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_C_CONNECTION);

	/* Disconnect any existing devices under this port */
	if (((!(portstatus & USB_PORT_STAT_CONNECTION)) &&
	     (!(portstatus & USB_PORT_STAT_ENABLE))) ||
	    usb_device_has_child_on_port(dev, port)) {
		//debug("usb_disconnect(&hub->children[port]);\n");
		/* Return now if nothing is connected */
		if (!(portstatus & USB_PORT_STAT_CONNECTION))
			return -ENOTCONN;
	}

	/* Reset the port */
	ret = usb_hub_port_reset(dev, port, &portstatus);
	if (ret < 0) {
		if (ret != -ENXIO)
			;//printf("cannot reset port %i!?\n", port + 1);
		return ret;
	}

	switch (portstatus & USB_PORT_STAT_SPEED_MASK) {
	case USB_PORT_STAT_SUPER_SPEED:
		speed = USB_SPEED_SUPER;
		break;
	case USB_PORT_STAT_HIGH_SPEED:
		speed = USB_SPEED_HIGH;
		break;
	case USB_PORT_STAT_LOW_SPEED:
		speed = USB_SPEED_LOW;
		break;
	default:
		speed = USB_SPEED_FULL;
		break;
	}

	struct usb_device *usb;

	ret = usb_alloc_new_device(dev->controller, &usb);
	if (ret) {
		//printf("cannot create new device: ret=%d", ret);
		return ret;
	}

	dev->children[port] = usb;
	usb->speed = speed;
	usb->parent = dev;
	usb->portnr = port + 1;
	/* Run it through the hoops (find a driver, etc) */
	ret = usb_new_device(usb);
	if (ret < 0) {
		/* Woops, disable the port */
		usb_free_device(dev->controller);
		dev->children[port] = NULL;
	}

	if (ret < 0) {
		//debug("hub: disabling port %d\n", port + 1);
		usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_ENABLE);
	}

	return ret;
}


static int usb_scan_port(struct usb_device_scan *usb_scan)
{
	ALLOC_CACHE_ALIGN_BUFFER(struct usb_port_status, portsts, 1);
	unsigned short portstatus;
	unsigned short portchange;
	struct usb_device *dev;
	struct usb_hub_device *hub;
	int ret = 0;
	int i;

	dev = usb_scan->dev;
	hub = usb_scan->hub;
	i = usb_scan->port;

	/*
	 * Don't talk to the device before the query delay is expired.
	 * This is needed for voltages to stabalize.
	 */
	if (get_timer(0) < hub->query_delay)
		return 0;

	ret = usb_get_port_status(dev, i + 1, portsts);
	if (ret < 0) {
		//debug("get_port_status failed\n");
		if (get_timer(0) >= hub->connect_timeout) {
			/* debug("devnum=%d port=%d: timeout\n",
			      dev->devnum, i + 1); */
			/* Remove this device from scanning list */
			list_del(&usb_scan->list);
			free(usb_scan);			      
			return -1;
		}
		return 0;
	}

	portstatus = portsts->wPortStatus;
	portchange = portsts->wPortChange;
	USB_UNUSED(portchange);
	//debug("Port %d Status %X Change %X\n", i + 1, portstatus, portchange);

	/*
	 * No connection change happened, wait a bit more.
	 *
	 * For some situation, the hub reports no connection change but a
	 * device is connected to the port (eg: CCS bit is set but CSC is not
	 * in the PORTSC register of a root hub), ignore such case.
	 */
	if (!(portchange & USB_PORT_STAT_C_CONNECTION) &&
	    !(portstatus & USB_PORT_STAT_CONNECTION)) {
		if (get_timer(0) >= hub->connect_timeout) {
			/* debug("devnum=%d port=%d: timeout\n",
			      dev->devnum, i + 1); */
			/* Remove this device from scanning list */
			list_del(&usb_scan->list);
			free(usb_scan);			
			return -1;
		}
		return 0;
	}

	if (portchange & USB_PORT_STAT_C_RESET) {
		//debug("port %d reset change\n", i + 1);
		usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_C_RESET);
	}

	if ((portchange & USB_SS_PORT_STAT_C_BH_RESET) &&
	    usb_hub_is_superspeed(dev)) {
		//debug("port %d BH reset change\n", i + 1);
		usb_clear_port_feature(dev, i + 1, USB_SS_PORT_FEAT_C_BH_RESET);
	}

	/* A new USB device is ready at this point */
	//debug("devnum=%d port=%d: USB dev found\n", dev->devnum, i + 1);

	ret = usb_hub_port_connect_change(dev, i);
	if (ret < 0) {
		PrintVariableValueHex("usb port change err at line", __LINE__);
		goto exit;
	}

	if (portchange & USB_PORT_STAT_C_ENABLE) {
		//debug("port %d enable change, status %x\n", i + 1, portstatus);
		usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_C_ENABLE);
		/*
		 * The following hack causes a ghost device problem
		 * to Faraday EHCI
		 */

		/*
		 * EM interference sometimes causes bad shielded USB
		 * devices to be shutdown by the hub, this hack enables
		 * them again. Works at least with mouse driver
		 */
		if (!(portstatus & USB_PORT_STAT_ENABLE) &&
		    (portstatus & USB_PORT_STAT_CONNECTION) &&
		    usb_device_has_child_on_port(dev, i)) {
			/* debug("already running port %i disabled by hub (EMI?), re-enabling...\n",
			      i + 1); */
			ret = usb_hub_port_connect_change(dev, i);
			if (ret < 0) {
				PrintVariableValueHex("usb port change err at line", __LINE__);
				goto exit;
			}
		}
	}

	if (portstatus & USB_PORT_STAT_SUSPEND) {
		//debug("port %d suspend change\n", i + 1);
		usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_SUSPEND);
	}

	if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
		//debug("port %d over-current change\n", i + 1);
		usb_clear_port_feature(dev, i + 1,
				       USB_PORT_FEAT_C_OVER_CURRENT);
		/* Only power-on this one port */
		usb_set_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);
		hub->overcurrent_count[i]++;

		/*
		 * If the max-scan-count is not reached, return without removing
		 * the device from scan-list. This will re-issue a new scan.
		 */
		if (hub->overcurrent_count[i] <=
		    PORT_OVERCURRENT_MAX_SCAN_COUNT)
			return 0;

		/* Otherwise the device will get removed */
		/* printf("Port %d over-current occurred %d times\n", i + 1,
		       hub->overcurrent_count[i]); */
	}

	/*
	 * We're done with this device, so let's remove this device from
	 * scanning list
	 */
	ret = 0;
exit:
	list_del(&usb_scan->list);
	free(usb_scan);
	PrintVariableValueHex("usb_scan_port ret", ret);

	return ret;
}

static int usb_device_list_scan(void)
{
	struct usb_device_scan *usb_scan;
	//struct usb_device_scan *tmp;
	static int running;
	int ret = 0;
	ListItem_t *pxListItem, *nListItem;

	/* Only run this loop once for each controller */
	if (running)
		return 0;

	running = 1;

	while (1) {
		/* We're done, once the list is empty again */
		if (list_empty(&usb_scan_list))
			goto out;
		list_for_each_entry_safe(pxListItem, nListItem, usb_scan, &usb_scan_list) {
			int ret;

			/* Scan this port */
			ret = usb_scan_port(usb_scan);
			if (ret)
				goto out;
			ret = usb_stor_scan(0);
		}
	}

out:
	/*
	 * This USB controller has finished scanning all its connected
	 * USB devices. Set "running" back to 0, so that other USB controllers
	 * will scan their devices too.
	 */
	running = 0;

	return ret;
}

static int usb_hub_configure(struct usb_device *dev)
{
	int i, length;
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, USB_BUFSIZ);
	unsigned char *bitmap;
	short hubCharacteristics;
	struct usb_hub_descriptor *descriptor;
	struct usb_hub_device *hub;
	struct usb_hub_status *hubsts;
	unsigned char *tmp;
	unsigned short tmpval;
	int ret;

	hub = usb_get_hub_device(dev);
	if (hub == NULL)
		return -ENOMEM;
	hub->pusb_dev = dev;

	/* Get the the hub descriptor */
	ret = usb_get_hub_descriptor(dev, buffer, 4);
	if (ret < 0) {
		/* debug("usb_hub_configure: failed to get hub " \
		      "descriptor, giving up %lX\n", dev->status); */
		return ret;
	}
	descriptor = (struct usb_hub_descriptor *)buffer;

	length = min(descriptor->bLength,
		       sizeof(struct usb_hub_descriptor));

	ret = usb_get_hub_descriptor(dev, buffer, length);
	if (ret < 0) {
		/* debug("usb_hub_configure: failed to get hub " \
		      "descriptor 2nd giving up %lX\n", dev->status); */
		return ret;
	}
	memcpy((unsigned char *)&hub->desc, buffer, length);
	/* adjust 16bit values */
	/* put_unaligned(le16_to_cpu(get_unaligned(
			&descriptor->wHubCharacteristics)),
			&hub->desc.wHubCharacteristics); */
	tmp = (unsigned char*)&descriptor->wHubCharacteristics;
	tmpval = (tmp[1] << 8) | tmp[0];
	tmp = (unsigned char*)&hub->desc.wHubCharacteristics;
	tmp[0] = tmpval & 0xff;
	tmp[1] = (tmpval >> 8) & 0xff;
	
	/* set the bitmap */
	bitmap = (unsigned char *)&hub->desc.u.hs.DeviceRemovable[0];
	/* devices not removable by default */
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8);
	bitmap = (unsigned char *)&hub->desc.u.hs.PortPowerCtrlMask[0];
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8); /* PowerMask = 1B */

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++)
		hub->desc.u.hs.DeviceRemovable[i] =
			descriptor->u.hs.DeviceRemovable[i];

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++)
		hub->desc.u.hs.PortPowerCtrlMask[i] =
			descriptor->u.hs.PortPowerCtrlMask[i];

	dev->maxchild = descriptor->bNbrPorts;
	//debug("%d ports detected\n", dev->maxchild);

	tmp = (unsigned char*)&hub->desc.wHubCharacteristics;
	hubCharacteristics = (tmp[1] << 8) | tmp[0];//get_unaligned(&hub->desc.wHubCharacteristics);
	switch (hubCharacteristics & HUB_CHAR_LPSM) {
	case 0x00:
		//debug("ganged power switching\n");
		break;
	case 0x01:
		//debug("individual port power switching\n");
		break;
	case 0x02:
	case 0x03:
		//debug("unknown reserved power switching mode\n");
		break;
	}

	if (hubCharacteristics & HUB_CHAR_COMPOUND)
		;//debug("part of a compound device\n");
	else
		;//debug("standalone hub\n");

	switch (hubCharacteristics & HUB_CHAR_OCPM) {
	case 0x00:
		//debug("global over-current protection\n");
		break;
	case 0x08:
		//debug("individual port over-current protection\n");
		break;
	case 0x10:
	case 0x18:
		//debug("no over-current protection\n");
		break;
	}

	switch (dev->descriptor.bDeviceProtocol) {
	case USB_HUB_PR_FS:
		break;
	case USB_HUB_PR_HS_SINGLE_TT:
		//debug("Single TT\n");
		break;
	case USB_HUB_PR_HS_MULTI_TT:
		ret = usb_set_interface(dev, 0, 1);
		if (ret == 0) {
			//debug("TT per port\n");
			hub->tt.multi = true;
		} else {
			;//debug("Using single TT (err %d)\n", ret);
		}
		break;
	case USB_HUB_PR_SS:
		/* USB 3.0 hubs don't have a TT */
		break;
	default:
		/* debug("Unrecognized hub protocol %d\n",
		      dev->descriptor.bDeviceProtocol); */
		break;
	}

	/* Note 8 FS bit times == (8 bits / 12000000 bps) ~= 666ns */
	switch (hubCharacteristics & HUB_CHAR_TTTT) {
	case HUB_TTTT_8_BITS:
		if (dev->descriptor.bDeviceProtocol != 0) {
			hub->tt.think_time = 666;
			/* debug("TT requires at most %d FS bit times (%d ns)\n",
			      8, hub->tt.think_time); */
		}
		break;
	case HUB_TTTT_16_BITS:
		hub->tt.think_time = 666 * 2;
		/* debug("TT requires at most %d FS bit times (%d ns)\n",
		      16, hub->tt.think_time); */
		break;
	case HUB_TTTT_24_BITS:
		hub->tt.think_time = 666 * 3;
		/* debug("TT requires at most %d FS bit times (%d ns)\n",
		      24, hub->tt.think_time); */
		break;
	case HUB_TTTT_32_BITS:
		hub->tt.think_time = 666 * 4;
		/* debug("TT requires at most %d FS bit times (%d ns)\n",
		      32, hub->tt.think_time); */
		break;
	}

	/* debug("power on to power good time: %dms\n",
	      descriptor->bPwrOn2PwrGood * 2);
	debug("hub controller current requirement: %dmA\n",
	      descriptor->bHubContrCurrent);

	for (i = 0; i < dev->maxchild; i++)
		debug("port %d is%s removable\n", i + 1,
		      hub->desc.u.hs.DeviceRemovable[(i + 1) / 8] & \
		      (1 << ((i + 1) % 8)) ? " not" : ""); */

	if (sizeof(struct usb_hub_status) > USB_BUFSIZ) {
		/* debug("usb_hub_configure: failed to get Status - " \
		      "too long: %d\n", descriptor->bLength); */
		return -EFBIG;
	}

	ret = usb_get_hub_status(dev, buffer);
	if (ret < 0) {
		/* debug("usb_hub_configure: failed to get Status %lX\n",
		      dev->status); */
		return ret;
	}

	hubsts = (struct usb_hub_status *)buffer;
	USB_UNUSED(hubsts);
	/* debug("get_hub_status returned status %X, change %X\n",
	      le16_to_cpu(hubsts->wHubStatus),
	      le16_to_cpu(hubsts->wHubChange));
	debug("local power source is %s\n",
	      (le16_to_cpu(hubsts->wHubStatus) & HUB_STATUS_LOCAL_POWER) ? \
	      "lost (inactive)" : "good");
	debug("%sover-current condition exists\n",
	      (le16_to_cpu(hubsts->wHubStatus) & HUB_STATUS_OVERCURRENT) ? \
	      "" : "no "); */
	
	usb_hub_power_on(hub);

	return ret;
}

static int usb_hub_check(struct usb_device *dev, int ifnum)
{
	struct usb_interface *iface;
	struct usb_endpoint_descriptor *ep = NULL;

	iface = &dev->config.if_desc[ifnum];
	/* Is it a hub? */
	if (iface->desc.bInterfaceClass != USB_CLASS_HUB)
		goto err;
	/* Some hubs have a subclass of 1, which AFAICT according to the */
	/*  specs is not defined, but it works */
	if ((iface->desc.bInterfaceSubClass != 0) &&
	    (iface->desc.bInterfaceSubClass != 1))
		goto err;
	/* Multiple endpoints? What kind of mutant ninja-hub is this? */
	if (iface->desc.bNumEndpoints != 1)
		goto err;
	ep = &iface->ep_desc[0];
	/* Output endpoint? Curiousier and curiousier.. */
	if (!(ep->bEndpointAddress & USB_DIR_IN))
		goto err;
	/* If it's not an interrupt endpoint, we'd better punt! */
	if ((ep->bmAttributes & 3) != 3)
		goto err;
	/* We found a hub */
	//debug("USB hub found\n");
	return 0;

err:
	/* debug("USB hub not found: bInterfaceClass=%d, bInterfaceSubClass=%d, bNumEndpoints=%d\n",
	      iface->desc.bInterfaceClass, iface->desc.bInterfaceSubClass,
	      iface->desc.bNumEndpoints);
	if (ep) {
		debug("   bEndpointAddress=%#x, bmAttributes=%d",
		      ep->bEndpointAddress, ep->bmAttributes);
	} */

	return -ENOENT;
}

int usb_hub_probe(struct usb_device *dev, int ifnum)
{
	int ret;

	ret = usb_hub_check(dev, ifnum);
	if (ret)
		return 0;
	ret = usb_hub_configure(dev);
	return ret;
}

int usb_new_device(struct usb_device *dev)
{
	bool do_read = true;
	int err;

	err = usb_setup_device(dev, do_read, dev->parent);
	if (err)
		return err;

	/* Now probe if the device is a hub */
	err = usb_hub_probe(dev, 0);
	if (err < 0)
		return err;

	return 0;
}

void usb_connect_state_notify_cb(void* ctx, int state)
{
	
}

/*
 * disables the asynch behaviour of the control message. This is used for data
 * transfers that uses the exclusiv access to the control and bulk messages.
 * Returns the old value so it can be restored later.
 */
int usb_disable_asynch(int disable)
{
	int old_value = asynch_allowed;

	asynch_allowed = !disable;
	return old_value;
}

static void notify_new_dev_attach()
{
	int i;
	struct usb_hub_device *hub = NULL;
	struct usb_device *dev = NULL;
	//int ret;

	hub = &hub_dev[0];
	if (NULL == hub)
		return;
	dev = hub->pusb_dev;
	
	for (i = 0; i < dev->maxchild; i++) {
		struct usb_device_scan *usb_scan;

		usb_scan = malloc(sizeof(*usb_scan));
		if (!usb_scan) {
			//printf("Can't allocate memory for USB device!\n");
			return;
		}
		memset(usb_scan, 0, sizeof(struct usb_device_scan));
		usb_scan->dev = dev;
		usb_scan->hub = hub;
		usb_scan->port = i;
		INIT_LIST_ITEM(&usb_scan->list);
		//listSET_LIST_ITEM_OWNER(usb_scan->list.pvOwner, usb_scan);
        usb_scan->list.pvOwner = (void*)usb_scan;
		list_add_tail(&usb_scan->list, &usb_scan_list);
	}
}

static void hub_usb_dev_reset()
{
	int i;
	struct usb_device *dev;

	//usb_disable_endpoint();
	reset_usb_phy();
	usb_clear_port_feature(&usb_dev[0], 1, USB_PORT_FEAT_C_ENABLE);
	for (i = 1; i < USB_MAX_DEVICE; i++) {
		dev = &usb_dev[i];
		if (dev->devnum != -1) {
			memset(dev, 0, sizeof(struct usb_device));
			dev->devnum = -1;
			if (dev_index > 0)
				dev_index--;
		}
	}
}

int ark_usb_init(void)
{
	void *ctrl = NULL;
	struct usb_device *dev;
	int i, start_index = 0;
	int controllers_initialized = 0;
	int ret;
	unsigned int start_time;

	usb_sysctrl_init();
	dev_index = 0;
	asynch_allowed = 1;
	usb_hub_reset();

	USB_UNUSED(start_index);
	
	vListInitialise(&usb_scan_list);

	/* first make all devices unknown */
	for (i = 0; i < USB_MAX_DEVICE; i++) {
		memset(&usb_dev[i], 0, sizeof(struct usb_device));
		usb_dev[i].devnum = -1;
	}


	/* init low_level USB */
	ret = usb_dwc2_lowlevel_init();
	if (ret) {
		SendUartString("lowlevel init failed\n");
		return -ENODEV;
	}
	/*
	 * lowlevel init is OK, now scan the bus for devices
	 * i.e. search HUBs and configure them
	 */
	controllers_initialized++;
	start_index = dev_index;
	ret = usb_alloc_new_device(ctrl, &dev);//usb 0 for root hub
	if (ret)
		return -ENODEV;
	
	/*
	 * device 0 is always present
	 * (root hub, so let it analyze)
	 */
	ret = usb_new_device(dev);
	if (ret)
		usb_free_device(dev->controller);

	start_time = get_timer(0);
	while(1) {
		char buf[8] = {0};
		int ret = 1;
		char status = 0;

		if (get_timer(start_time) > 10000) //10s timeout
			break;
		
		hub_status_data(buf);
		status = (buf[0] >> 1) & 0x1;
		if (connect_status != status) {
			PrintVariableValueHex("usb state changed now!state", buf[0]);
			connect_status = status;
			if (status) {
				notify_new_dev_attach();
				usb_disable_endpoint();
				ret = usb_device_list_scan();
				if (!ret) {
					return 0;
				}
			} else {
				usb_stor_disconnect();
				hub_usb_dev_reset();
				return -1;
			}
		}

		mdelay(100);
	}
	
	return -1;
}

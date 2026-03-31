#include "usb_os_adapter.h"
#include "board.h"
#include <linux/usb/cdc.h>
#include "linux/usb/composite.h"
#include "linux/usb/ether.h"
#include <stdio.h>
#include "source/crc32.h"
#include "timers.h"
#include "trace.h"
#include "NetworkBufferManagement.h"
#if USE_LWIP
#include "lwip/pbuf.h"
#endif

/* to trigger crc/non-crc ndp signature */
#define VDBG(d, fmt, args...) \
	dev_vdbg(&(d)->gadget->dev , fmt , ## args)
#define DBG(d, fmt, args...) \
	dev_vdbg(&(d)->gadget->dev , fmt , ## args)
#define ERROR(d, fmt, args...) \
	dev_err(&(d)->gadget->dev , fmt , ## args)
#define INFO(d, fmt, args...) \
	dev_info(&(d)->gadget->dev , fmt , ## args)
#define NCM_NDP_HDR_CRC_MASK	0x01000000
#define NCM_NDP_HDR_CRC		0x01000000
#define NCM_NDP_HDR_NOCRC	0x00000000
#define	DEFAULT_FILTER	(USB_CDC_PACKET_TYPE_BROADCAST \
			|USB_CDC_PACKET_TYPE_ALL_MULTICAST \
			|USB_CDC_PACKET_TYPE_PROMISCUOUS \
			|USB_CDC_PACKET_TYPE_DIRECTED)
enum ncm_notify_state {
	NCM_NOTIFY_NONE,		/* don't notify */
	NCM_NOTIFY_CONNECT,		/* issue CONNECT next */
	NCM_NOTIFY_SPEED,		/* issue SPEED_CHANGE next */
};

struct f_ncm {
       struct gether			port;
	u8				ctrl_id, data_id;

	char				ethaddr[14];

	struct usb_ep			*notify;
	struct usb_request		*notify_req;
	u8				notify_state;
	bool				is_open;

	struct ndp_parser_opts		*parser_opts;
	bool				is_crc;

	/*
	 * for notification, it is accessed from both
	 * callback and ethernet open/close
	 */
	spinlock_t			lock;

	bool                       connect;
	TimerHandle_t        timer;
};

static inline struct f_ncm *func_to_ncm(struct usb_function *f)
{
	return (struct f_ncm *)f->powner;
}

/* peak (theoretical) bulk transfer rate in bits-per-second */
static inline unsigned ncm_bitrate(struct usb_gadget *g)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return 13 * 512 * 8 * 1000 * 8;
	else
		return 19 *  64 * 1 * 1000 * 8;
}

/*-------------------------------------------------------------------------*/

/*
 * We cannot group frames so use just the minimal size which ok to put
 * one max-size ethernet frame.
 * If the host can group frames, allow it to do that, 16K is selected,
 * because it's used by default by the current linux host driver
 */
#define NTB_DEFAULT_IN_SIZE	2048//USB_CDC_NCM_NTB_MIN_IN_SIZE
#define NTB_OUT_SIZE		3072

/*
 * skbs of size less than that will not be aligned
 * to NCM's dwNtbInMaxSize to save bus bandwidth
 */

#define	MAX_TX_NONFIXED		(512 * 3)

#define FORMATS_SUPPORTED	(USB_CDC_NCM_NTB16_SUPPORTED |	\
				 USB_CDC_NCM_NTB32_SUPPORTED)

static struct usb_cdc_ncm_ntb_parameters ntb_parameters = {
	.wLength = sizeof ntb_parameters,
	.bmNtbFormatsSupported = cpu_to_le16(FORMATS_SUPPORTED),
	.dwNtbInMaxSize = cpu_to_le32(NTB_DEFAULT_IN_SIZE),
	.wNdpInDivisor = cpu_to_le16(4),
	.wNdpInPayloadRemainder = cpu_to_le16(0),
	.wNdpInAlignment = cpu_to_le16(4),

	.dwNtbOutMaxSize = cpu_to_le32(NTB_OUT_SIZE),
	.wNdpOutDivisor = cpu_to_le16(4),
	.wNdpOutPayloadRemainder = cpu_to_le16(0),
	.wNdpOutAlignment = cpu_to_le16(4),
	//.wNtbOutMaxDatagrams = cpu_to_le16(32),
};

/*
 * Use wMaxPacketSize big enough to fit CDC_NOTIFY_SPEED_CHANGE in one
 * packet, to simplify cancellation; and a big transfer interval, to
 * waste less bandwidth.
 */

#define LOG2_STATUS_INTERVAL_MSEC	5	/* 1 << 5 == 32 msec */
#define NCM_STATUS_BYTECOUNT		16	/* 8 byte header + data */

static struct usb_interface_assoc_descriptor ncm_iad_desc = {
	.bLength =		sizeof ncm_iad_desc,
	.bDescriptorType =	USB_DT_INTERFACE_ASSOCIATION,

	/* .bFirstInterface =	DYNAMIC, */
	.bInterfaceCount =	2,	/* control + data */
	.bFunctionClass =	USB_CLASS_COMM,
	.bFunctionSubClass =	USB_CDC_SUBCLASS_NCM,
	.bFunctionProtocol =	USB_CDC_PROTO_NONE,
	/* .iFunction =		DYNAMIC */
};

/* interface descriptor: */

static struct usb_interface_descriptor ncm_control_intf = {
	.bLength =		sizeof ncm_control_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_COMM,
	.bInterfaceSubClass =	USB_CDC_SUBCLASS_NCM,
	.bInterfaceProtocol =	USB_CDC_PROTO_NONE,
	/* .iInterface = DYNAMIC */
};

static struct usb_cdc_header_desc ncm_header_desc = {
	.bLength =		sizeof ncm_header_desc,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_HEADER_TYPE,

	.bcdCDC =		cpu_to_le16(0x0110),
};

static struct usb_cdc_union_desc ncm_union_desc = {
	.bLength =		sizeof(ncm_union_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_UNION_TYPE,
	/* .bMasterInterface0 =	DYNAMIC */
	/* .bSlaveInterface0 =	DYNAMIC */
};

static struct usb_cdc_ether_desc ecm_desc = {
	.bLength =		sizeof ecm_desc,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_ETHERNET_TYPE,

	/* this descriptor actually adds value, surprise! */
	/* .iMACAddress = DYNAMIC */
	.bmEthernetStatistics =	cpu_to_le32(0), /* no statistics */
	.wMaxSegmentSize =	cpu_to_le16(ETH_FRAME_LEN * 8),
	.wNumberMCFilters =	cpu_to_le16(0),
	.bNumberPowerFilters =	0,
};

#define NCAPS	(USB_CDC_NCM_NCAP_ETH_FILTER | USB_CDC_NCM_NCAP_CRC_MODE)

static struct usb_cdc_ncm_desc ncm_desc = {
	.bLength =		sizeof ncm_desc,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_NCM_TYPE,

	.bcdNcmVersion =	cpu_to_le16(0x0100),
	/* can process SetEthernetPacketFilter */
	.bmNetworkCapabilities = 0x3A,//NCAPS,//,
};

/* the default data interface has no endpoints ... */

static struct usb_interface_descriptor ncm_data_nop_intf = {
	.bLength =		sizeof ncm_data_nop_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	1,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	USB_CDC_NCM_PROTO_NTB,
	/* .iInterface = DYNAMIC */
};

/* ... but the "real" data interface has two bulk endpoints */

static struct usb_interface_descriptor ncm_data_intf = {
	.bLength =		sizeof ncm_data_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	1,
	.bAlternateSetting =	1,
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	USB_CDC_NCM_PROTO_NTB,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor fs_ncm_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(NCM_STATUS_BYTECOUNT),
	.bInterval =		1 << LOG2_STATUS_INTERVAL_MSEC,
};

static struct usb_endpoint_descriptor fs_ncm_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_ncm_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *ncm_fs_function[] = {
	(struct usb_descriptor_header *) &ncm_iad_desc,
	/* CDC NCM control descriptors */
	(struct usb_descriptor_header *) &ncm_control_intf,
	(struct usb_descriptor_header *) &ncm_header_desc,
	(struct usb_descriptor_header *) &ncm_union_desc,
	(struct usb_descriptor_header *) &ecm_desc,
	(struct usb_descriptor_header *) &ncm_desc,
	(struct usb_descriptor_header *) &fs_ncm_notify_desc,
	/* data interface, altsettings 0 and 1 */
	(struct usb_descriptor_header *) &ncm_data_nop_intf,
	(struct usb_descriptor_header *) &ncm_data_intf,
	(struct usb_descriptor_header *) &fs_ncm_in_desc,
	(struct usb_descriptor_header *) &fs_ncm_out_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_ncm_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(64),//NCM_STATUS_BYTECOUNT
	.bInterval =		2,//LOG2_STATUS_INTERVAL_MSEC + 4,
};
static struct usb_endpoint_descriptor hs_ncm_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_ncm_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_descriptor_header *ncm_hs_function[] = {
	(struct usb_descriptor_header *) &ncm_iad_desc,
	/* CDC NCM control descriptors */
	(struct usb_descriptor_header *) &ncm_control_intf,
	(struct usb_descriptor_header *) &ncm_header_desc,
	(struct usb_descriptor_header *) &ncm_union_desc,
	(struct usb_descriptor_header *) &ecm_desc,
	(struct usb_descriptor_header *) &ncm_desc,
	(struct usb_descriptor_header *) &hs_ncm_notify_desc,
	/* data interface, altsettings 0 and 1 */
	(struct usb_descriptor_header *) &ncm_data_nop_intf,
	(struct usb_descriptor_header *) &ncm_data_intf,
	(struct usb_descriptor_header *) &hs_ncm_in_desc,
	(struct usb_descriptor_header *) &hs_ncm_out_desc,
	NULL,
};

/* string descriptors: */

#define STRING_CTRL_IDX	0
#define STRING_MAC_IDX	1
#define STRING_DATA_IDX	2
#define STRING_IAD_IDX	3

static struct usb_string ncm_string_defs[] = {
	[STRING_CTRL_IDX].s = "CDC Network Control Model (NCM)",
	[STRING_MAC_IDX].s = NULL /* DYNAMIC */,
	[STRING_DATA_IDX].s = "CDC Network Data",
	[STRING_IAD_IDX].s = "CDC NCM",
};

static struct usb_gadget_strings ncm_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		ncm_string_defs,
};

static struct usb_gadget_strings *ncm_strings[] = {
	&ncm_string_table,
	NULL,
};

/*
 * Here are options for NCM Datagram Pointer table (NDP) parser.
 * There are 2 different formats: NDP16 and NDP32 in the spec (ch. 3),
 * in NDP16 offsets and sizes fields are 1 16bit word wide,
 * in NDP32 -- 2 16bit words wide. Also signatures are different.
 * To make the parser code the same, put the differences in the structure,
 * and switch pointers to the structures when the format is changed.
 */

struct ndp_parser_opts {
	u32		nth_sign;
	u32		ndp_sign;
	unsigned	nth_size;
	unsigned	ndp_size;
	unsigned	ndplen_align;
	/* sizes in u16 units */
	unsigned	dgram_item_len; /* index or length */
	unsigned	block_length;
	unsigned	fp_index;
	unsigned	reserved1;
	unsigned	reserved2;
	unsigned	next_fp_index;
};

#define INIT_NDP16_OPTS {					\
		.nth_sign = USB_CDC_NCM_NTH16_SIGN,		\
		.ndp_sign = USB_CDC_NCM_NDP16_NOCRC_SIGN,	\
		.nth_size = sizeof(struct usb_cdc_ncm_nth16),	\
		.ndp_size = sizeof(struct usb_cdc_ncm_ndp16),	\
		.ndplen_align = 4,				\
		.dgram_item_len = 1,				\
		.block_length = 1,				\
		.fp_index = 1,					\
		.reserved1 = 0,					\
		.reserved2 = 0,					\
		.next_fp_index = 1,				\
	}


#define INIT_NDP32_OPTS {					\
		.nth_sign = USB_CDC_NCM_NTH32_SIGN,		\
		.ndp_sign = USB_CDC_NCM_NDP32_NOCRC_SIGN,	\
		.nth_size = sizeof(struct usb_cdc_ncm_nth32),	\
		.ndp_size = sizeof(struct usb_cdc_ncm_ndp32),	\
		.ndplen_align = 8,				\
		.dgram_item_len = 2,				\
		.block_length = 2,				\
		.fp_index = 2,					\
		.reserved1 = 1,					\
		.reserved2 = 2,					\
		.next_fp_index = 2,				\
	}

static struct ndp_parser_opts ndp16_opts = INIT_NDP16_OPTS;
static struct ndp_parser_opts ndp32_opts = INIT_NDP32_OPTS;

static inline void put_ncm(__le16 **p, unsigned size, unsigned val)
{
	switch (size) {
	case 1:
		put_unaligned_le16((u16)val, *p);
		break;
	case 2:
		put_unaligned_le32((u32)val, *p);

		break;
	default:
		BUG();
	}

	*p += size;
}

static inline unsigned get_ncm(__le16 **p, unsigned size)
{
	unsigned tmp;

	switch (size) {
	case 1:
		tmp = get_unaligned_le16(*p);
		break;
	case 2:
		tmp = get_unaligned_le32(*p);
		break;
	default:
		BUG();
	}

	*p += size;
	return tmp;
}

/*-------------------------------------------------------------------------*/

static inline void ncm_reset_values(struct f_ncm *ncm)
{
	ncm->parser_opts = &ndp16_opts;
	ncm->is_crc = false;
	ncm->port.cdc_filter = DEFAULT_FILTER;

	/* doesn't make sense for ncm, fixed size used */
	ncm->port.header_len = 0;

	ncm->port.fixed_out_len = le32_to_cpu(ntb_parameters.dwNtbOutMaxSize);
	ncm->port.fixed_in_len = NTB_DEFAULT_IN_SIZE;
}

/*
 * Context: ncm->lock held
 */
static void ncm_do_notify(struct f_ncm *ncm)
{
	struct usb_request		*req = ncm->notify_req;
	struct usb_cdc_notification	*event;
	struct usb_composite_dev	*cdev = ncm->port.func.config->cdev;
	__le32				*data;
	int				status;

	/* notification already in flight? */
	if (!req)
		return;

	event = req->buf;
	switch (ncm->notify_state) {
	case NCM_NOTIFY_NONE:
		return;

	case NCM_NOTIFY_CONNECT:
		event->bNotificationType = USB_CDC_NOTIFY_NETWORK_CONNECTION;
		if (ncm->is_open)
			event->wValue = cpu_to_le16(1);
		else
			event->wValue = cpu_to_le16(0);
		event->wLength = 0;
		req->length = sizeof *event;

		DBG(cdev, "notify connect %s\n",
				ncm->is_open ? "true" : "false");
		ncm->notify_state = NCM_NOTIFY_NONE;
		break;

	case NCM_NOTIFY_SPEED:
		event->bNotificationType = USB_CDC_NOTIFY_SPEED_CHANGE;
		event->wValue = cpu_to_le16(0);
		event->wLength = cpu_to_le16(8);
		req->length = NCM_STATUS_BYTECOUNT;

		/* SPEED_CHANGE data is up/down speeds in bits/sec */
		data = (__le32 *)((uint8_t*)req->buf + sizeof *event);
		data[0] = cpu_to_le32(ncm_bitrate(cdev->gadget));
		data[1] = data[0];

		printf("notify speed %d\r\n", ncm_bitrate(cdev->gadget));
		ncm->notify_state = NCM_NOTIFY_CONNECT;
		break;
	}
	event->bmRequestType = 0xA1;
	event->wIndex = cpu_to_le16(ncm->ctrl_id);

	ncm->notify_req = NULL;
	/*
	 * In double buffering if there is a space in FIFO,
	 * completion callback can be called right after the call,
	 * so unlocking
	 */
	//spin_unlock(&ncm->lock);
	status = usb_ep_queue(ncm->notify, req, GFP_ATOMIC);
	//spin_lock(&ncm->lock);
	if (status < 0) {
		ncm->notify_req = req;
		DBG(cdev, "notify --> %d\n", status);
	}
}

/*
 * Context: ncm->lock held
 */
static void ncm_notify(struct f_ncm *ncm)
{
	/*
	 * NOTE on most versions of Linux, host side cdc-ethernet
	 * won't listen for notifications until its netdevice opens.
	 * The first notification then sits in the FIFO for a long
	 * time, and the second one is queued.
	 *
	 * If ncm_notify() is called before the second (CONNECT)
	 * notification is sent, then it will reset to send the SPEED
	 * notificaion again (and again, and again), but it's not a problem
	 */
	ncm->notify_state = NCM_NOTIFY_SPEED;
	ncm_do_notify(ncm);
}

static void ncm_notify_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_ncm			*ncm = req->context;
	//struct usb_composite_dev	*cdev = ncm->port.func.config->cdev;
	struct usb_cdc_notification	*event = req->buf;

	spin_lock(&ncm->lock);
	switch (req->status) {
	case 0:
		TRACE_INFO("Notification %02x sent\r\n",
		     event->bNotificationType);
		break;
	case -ECONNRESET:
	case -ESHUTDOWN:
		ncm->notify_state = NCM_NOTIFY_NONE;
		break;
	default:
		printf("event %02x --> %d\r\n",
			event->bNotificationType, req->status);
		break;
	}
	ncm->notify_req = req;
	ncm_do_notify(ncm);
	spin_unlock(&ncm->lock);
}

static void ncm_ep0out_complete(struct usb_ep *ep, struct usb_request *req)
{
	/* now for SET_NTB_INPUT_SIZE only */
	unsigned		in_size;
	struct usb_function	*f = req->context;
	struct f_ncm		*ncm = func_to_ncm(f);
	struct usb_composite_dev *cdev = ep->driver_data;

	req->context = NULL;
	cdev = cdev;
	if (req->status || req->actual != req->length) {
		DBG(cdev, "Bad control-OUT transfer\n");
		goto invalid;
	}

	in_size = get_unaligned_le32(req->buf);
	if (in_size < USB_CDC_NCM_NTB_MIN_IN_SIZE ||
	    in_size > le32_to_cpu(ntb_parameters.dwNtbInMaxSize)) {
		DBG(cdev, "Got wrong INPUT SIZE (%d) from host\n", in_size);
		goto invalid;
	}

	ncm->port.fixed_in_len = in_size;
	VDBG(cdev, "Set NTB INPUT SIZE %d\n", in_size);
	return;

invalid:
	usb_ep_set_halt(ep);
	return;
}

static int ncm_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_ncm		*ncm = func_to_ncm(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/*
	 * composite driver infrastructure handles everything except
	 * CDC class messages; interface activation uses set_alt().
	 */
	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_SET_ETHERNET_PACKET_FILTER:
		/*
		 * see 6.2.30: no data, wIndex = interface,
		 * wValue = packet filter bitmap
		 */
		if (w_length != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		DBG(cdev, "packet filter %02x\n", w_value);

		/*
		 * REVISIT locking of cdc_filter.  This assumes the UDC
		 * driver won't have a concurrent packet TX irq running on
		 * another CPU; or that if it does, this write is atomic...
		 */
		ncm->port.cdc_filter = w_value;
		value = 0;
		break;
	/*
	 * and optionally:
	 * case USB_CDC_SEND_ENCAPSULATED_COMMAND:
	 * case USB_CDC_GET_ENCAPSULATED_RESPONSE:
	 * case USB_CDC_SET_ETHERNET_MULTICAST_FILTERS:
	 * case USB_CDC_SET_ETHERNET_PM_PATTERN_FILTER:
	 * case USB_CDC_GET_ETHERNET_PM_PATTERN_FILTER:
	 * case USB_CDC_GET_ETHERNET_STATISTIC:
	 */

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_NTB_PARAMETERS:

		if (w_length == 0 || w_value != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		value = w_length > sizeof ntb_parameters ?
			sizeof ntb_parameters : w_length;
		memcpy(req->buf, &ntb_parameters, value);
		VDBG(cdev, "Host asked NTB parameters\n");
		break;

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_NTB_INPUT_SIZE:

		if (w_length < 4 || w_value != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		put_unaligned_le32(ncm->port.fixed_in_len, req->buf);
		value = 4;
		VDBG(cdev, "Host asked INPUT SIZE, sending %d\n",
		     ncm->fixed_in_len);
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_SET_NTB_INPUT_SIZE:
	{
		if (w_length != 4 || w_value != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		req->complete = ncm_ep0out_complete;
		req->length = w_length;
		req->context = f;

		value = req->length;
		break;
	}

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_NTB_FORMAT:
	{
		uint16_t format;

		if (w_length < 2 || w_value != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		format = (ncm->parser_opts == &ndp16_opts) ? 0x0000 : 0x0001;
		put_unaligned_le16(format, req->buf);
		value = 2;
		VDBG(cdev, "Host asked NTB FORMAT, sending %d\n", format);
		break;
	}

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_SET_NTB_FORMAT:
	{
		if (w_length != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		switch (w_value) {
		case 0x0000:
			ncm->parser_opts = &ndp16_opts;
			DBG(cdev, "NCM16 selected\n");
			break;
		case 0x0001:
			ncm->parser_opts = &ndp32_opts;
			DBG(cdev, "NCM32 selected\n");
			break;
		default:
			goto invalid;
		}
		value = 0;
		break;
	}
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_CRC_MODE:
	{
		uint16_t is_crc;

		if (w_length < 2 || w_value != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		is_crc = ncm->is_crc ? 0x0001 : 0x0000;
		put_unaligned_le16(is_crc, req->buf);
		value = 2;
		VDBG(cdev, "Host asked CRC MODE, sending %d\n", is_crc);
		break;
	}

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_SET_CRC_MODE:
	{
		int ndp_hdr_crc = 0;

		if (w_length != 0 || w_index != ncm->ctrl_id)
			goto invalid;
		switch (w_value) {
		case 0x0000:
			ncm->is_crc = false;
			ndp_hdr_crc = NCM_NDP_HDR_NOCRC;
			DBG(cdev, "non-CRC mode selected\n");
			break;
		case 0x0001:
			ncm->is_crc = true;
			ndp_hdr_crc = NCM_NDP_HDR_CRC;
			DBG(cdev, "CRC mode selected\n");
			break;
		default:
			goto invalid;
		}
		ncm->parser_opts->ndp_sign &= ~NCM_NDP_HDR_CRC_MASK;
		ncm->parser_opts->ndp_sign |= ndp_hdr_crc;
		value = 0;
		break;
	}

	/* and disabled in ncm descriptor: */
	/* case USB_CDC_GET_NET_ADDRESS: */
	/* case USB_CDC_SET_NET_ADDRESS: */
	 case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8) | USB_CDC_GET_MAX_DATAGRAM_SIZE: 
		if (w_length < 2 || w_value != 0)
			goto invalid;
		put_unaligned_le16(ETH_FRAME_LEN * 8, req->buf);
		value = 2;
	 	break;
	 case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8) | USB_CDC_SET_MAX_DATAGRAM_SIZE:
		req->length = w_length;
		req->context = f;

		value = req->length;xTimerStartFromISR( ncm->timer, 0 );
		break;
	default:
invalid:
		ERROR(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		DBG(cdev, "ncm req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "ncm req %02x.%02x response err %d\n",
					ctrl->bRequestType, ctrl->bRequest,
					value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}


static int ncm_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_ncm		*ncm = func_to_ncm(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret = -1;

	/* Control interface has only altsetting 0 */
	if (intf == ncm->ctrl_id) {
		if (alt != 0)
			goto fail;

		if (ncm->notify->driver_data) {
			DBG(cdev, "reset ncm control %d\n", intf);
			usb_ep_disable(ncm->notify);
		}
		TRACE_INFO("ncm_set_alt  notify\r\n");
		usb_ep_enable(ncm->notify, get_ep_desc(cdev->gadget, &fs_ncm_notify_desc, &hs_ncm_notify_desc));
		ncm->notify->driver_data = ncm;

	/* Data interface has two altsettings, 0 and 1 */
	} else if (intf == ncm->data_id) {
		if (alt > 1)
			goto fail;

		if (ncm->port.in_ep->driver_data) {
			ncm_reset_values(ncm);
		}

		if (alt == 1) {
			ncm->port.is_zlp_ok = true;
			ncm->port.cdc_filter = DEFAULT_FILTER;
			DBG(cdev, "activate ncm\n");
			printf("speed:%d\r\n", cdev->gadget->speed);
			if (cdev->gadget->speed == USB_SPEED_FULL) {
				ncm->port.in = get_ep_desc(cdev->gadget, &fs_ncm_in_desc, &hs_ncm_in_desc);
				ncm->port.out = get_ep_desc(cdev->gadget, &fs_ncm_out_desc, &hs_ncm_out_desc);
			}
			ret = gether_connect(&ncm->port);
			if (ret)
				return ret;
		}
		xTimerStopFromISR( ncm->timer, 0 );

		//spin_lock(&ncm->lock);
		//ncm_notify(ncm);
		//spin_unlock(&ncm->lock);
	} else
		goto fail;
	ncm->connect = true;
	return 0;
fail:
	return -EINVAL;
}

/*
 * Because the data interface supports multiple altsettings,
 * this NCM function *MUST* implement a get_alt() method.
 */
static int ncm_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_ncm		*ncm = func_to_ncm(f);

	if (intf == ncm->ctrl_id)
		return 0;
	return ncm->port.in_ep->driver_data ? 1 : 0;
}

NetworkBufferDescriptor_t * pxResizeNetworkBufferWithDescriptorNoPadding( NetworkBufferDescriptor_t * pxNetworkBuffer,
                                                                 size_t xNewSizeBytes, int offset )
{
    size_t xOriginalLength;
    uint8_t * pucBuffer;

    xOriginalLength = pxNetworkBuffer->xDataLength;


    pucBuffer = pvPortMalloc( xNewSizeBytes ); 

    if( pucBuffer == NULL )
    {
        /* In case the allocation fails, return NULL. */
        pxNetworkBuffer = NULL;
    }
    else
    {
        pxNetworkBuffer->xDataLength = xNewSizeBytes;

        ( void ) memcpy( pucBuffer + offset, pxNetworkBuffer->pucEthernetBuffer, xOriginalLength );
        vReleaseNetworkBuffer( pxNetworkBuffer->pucEthernetBuffer );
        pxNetworkBuffer->pucEthernetBuffer = pucBuffer;
    }

    return pxNetworkBuffer;
}

static NetworkBufferDescriptor_t *ncm_wrap_ntb(struct gether *port, NetworkBufferDescriptor_t*  pxBufferDescriptor)
{
	struct f_ncm	*ncm = func_to_ncm(&port->func);
	NetworkBufferDescriptor_t	*pxBufferDescriptor2;
	int		ncb_len = 0;
	__le16		*tmp;
	int		div = ntb_parameters.wNdpInDivisor;
	int		rem = ntb_parameters.wNdpInPayloadRemainder;
	int		pad = 0;
	int		ndp_align = ntb_parameters.wNdpInAlignment;
	int		ndp_pad = 0;
	unsigned	max_size = ncm->port.fixed_in_len;
	struct ndp_parser_opts *opts = ncm->parser_opts;
	unsigned	crc_len = ncm->is_crc ? sizeof(uint32_t) : 0;
	int         net_data_len = pxBufferDescriptor->xDataLength;

	ncb_len += opts->nth_size;
	ndp_pad = ALIGN(ncb_len, ndp_align) - ncb_len;
	ncb_len += ndp_pad;
	ncb_len += opts->ndp_size;
	ncb_len += opts->dgram_item_len << 2; /* Datagram entry */
	ncb_len += opts->dgram_item_len << 2; /* Zero datagram entry */
	pad = ALIGN(ncb_len, div) + rem - ncb_len;
	ncb_len += pad;

	if (ncb_len + pxBufferDescriptor->xDataLength + crc_len > max_size) {
		printf("invalid ncm len\r\n");
		vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
		return NULL;
	}

	//pxBufferDescriptor2 = pxGetNetworkBufferWithDescriptor(ncb_len + pxBufferDescriptor->xDataLength + crc_len, 0);
	pxBufferDescriptor2 = pxResizeNetworkBufferWithDescriptorNoPadding(pxBufferDescriptor, ncb_len + net_data_len + crc_len, ncb_len);
	if (!pxBufferDescriptor2) {
		printf("resize ncm buf failed\r\n");
		vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
		return NULL;
	}

	//memcpy(pxBufferDescriptor2->pucEthernetBuffer + ncb_len, pxBufferDescriptor->pucEthernetBuffer,  pxBufferDescriptor->xDataLength);
	//vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
	pxBufferDescriptor = pxBufferDescriptor2;

	tmp = (__le16 *)pxBufferDescriptor->pucEthernetBuffer;
	memset(tmp, 0, ncb_len);

	put_unaligned_le32(opts->nth_sign, tmp); /* dwSignature */
	tmp += 2;
	/* wHeaderLength */
	put_unaligned_le16(opts->nth_size, tmp++);
	tmp++; /* skip wSequence */
	put_ncm(&tmp, opts->block_length, ncb_len + net_data_len + crc_len); /* (d)wBlockLength */
	/* (d)wFpIndex */
	/* the first pointer is right after the NTH + align */
	put_ncm(&tmp, opts->fp_index, opts->nth_size + ndp_pad);

	tmp = tmp + ndp_pad;

	/* NDP */
	put_unaligned_le32(opts->ndp_sign, tmp); /* dwSignature */
	tmp += 2;
	/* wLength */
	put_unaligned_le16(ncb_len - opts->nth_size - pad, tmp++);

	tmp += opts->reserved1;
	tmp += opts->next_fp_index; /* skip reserved (d)wNextFpIndex */
	tmp += opts->reserved2;

	if (ncm->is_crc) {
		uint32_t crc;

		crc = xcrc32(pxBufferDescriptor->pucEthernetBuffer + ncb_len, net_data_len, ~0);
		put_unaligned_le32(crc, pxBufferDescriptor->pucEthernetBuffer + net_data_len + ncb_len);
	}

	/* (d)wDatagramIndex[0] */
	put_ncm(&tmp, opts->dgram_item_len, ncb_len);
	/* (d)wDatagramLength[0] */
	put_ncm(&tmp, opts->dgram_item_len, pxBufferDescriptor->xDataLength - ncb_len);
	/* (d)wDatagramIndex[1] and  (d)wDatagramLength[1] already zeroed */

	return pxBufferDescriptor;
}

static void *ncm_wrap_ext_ntb(struct gether *port, void*  bufferDescHandle)
{
	struct f_ncm	*ncm = func_to_ncm(&port->func);
	int		ncb_len = 0;
	__le16		*tmp;
	int		div = ntb_parameters.wNdpInDivisor;
	int		rem = ntb_parameters.wNdpInPayloadRemainder;
	int		pad = 0;
	int		ndp_align = ntb_parameters.wNdpInAlignment;
	int		ndp_pad = 0;
	unsigned	max_size = ncm->port.fixed_in_len;
	struct ndp_parser_opts *opts = ncm->parser_opts;
	unsigned	crc_len = ncm->is_crc ? sizeof(uint32_t) : 0;
	int         net_data_len = 0;
#if !USE_LWIP
	NetworkBufferDescriptor_t* pxBufferDescriptor = (NetworkBufferDescriptor_t*)bufferDescHandle;
	NetworkBufferDescriptor_t	*pxBufferDescriptor2;
#else
	struct pbuf *pxBufferDescriptor = (struct pbuf *)bufferDescHandle;
	struct pbuf	*pxBufferDescriptor2;
#endif
	ncb_len += opts->nth_size;
	ndp_pad = ALIGN(ncb_len, ndp_align) - ncb_len;
	ncb_len += ndp_pad;
	ncb_len += opts->ndp_size;
	ncb_len += opts->dgram_item_len << 2; /* Datagram entry */
	ncb_len += opts->dgram_item_len << 2; /* Zero datagram entry */
	pad = ALIGN(ncb_len, div) + rem - ncb_len;
	ncb_len += pad;

#if !USE_LWIP
	net_data_len = pxBufferDescriptor->xDataLength;
	if (ncb_len + pxBufferDescriptor->xDataLength + crc_len > max_size) {
		printf("invalid ncm len\r\n");
		vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
		return NULL;
	}

	//pxBufferDescriptor2 = pxGetNetworkBufferWithDescriptor(ncb_len + pxBufferDescriptor->xDataLength + crc_len, 0);
	pxBufferDescriptor2 = pxResizeNetworkBufferWithDescriptorNoPadding(pxBufferDescriptor, ncb_len + net_data_len + crc_len, ncb_len);
	if (!pxBufferDescriptor2) {
		printf("resize ncm buf failed\r\n");
		vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
		return NULL;
	}

	//memcpy(pxBufferDescriptor2->pucEthernetBuffer + ncb_len, pxBufferDescriptor->pucEthernetBuffer,  pxBufferDescriptor->xDataLength);
	//vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
	pxBufferDescriptor = pxBufferDescriptor2;

	tmp = (__le16 *)pxBufferDescriptor->pucEthernetBuffer;
#else
	int new_ncm_len;
	net_data_len = pxBufferDescriptor->len;
	if (ncb_len + net_data_len + crc_len > max_size) {
		printf("invalid ncm len\r\n");
		pbuf_free(pxBufferDescriptor);
		return NULL;
	}
	new_ncm_len = ncb_len + net_data_len + crc_len;
	pxBufferDescriptor2 = pbuf_alloc(PBUF_RAW, new_ncm_len, PBUF_RAM);
	if (!pxBufferDescriptor2) {
		printf("resize ncm buf failed\r\n");
		pbuf_free(pxBufferDescriptor);
		return NULL;
	}
	memcpy((void*)((char *)pxBufferDescriptor2->payload + ncb_len), pxBufferDescriptor->payload, pxBufferDescriptor->len);
	pbuf_free(pxBufferDescriptor);
	pxBufferDescriptor = pxBufferDescriptor2;

	tmp = (__le16 *)pxBufferDescriptor->payload;
#endif
	
	memset(tmp, 0, ncb_len);

	put_unaligned_le32(opts->nth_sign, tmp); /* dwSignature */
	tmp += 2;
	/* wHeaderLength */
	put_unaligned_le16(opts->nth_size, tmp++);
	tmp++; /* skip wSequence */
	put_ncm(&tmp, opts->block_length, ncb_len + net_data_len + crc_len); /* (d)wBlockLength */
	/* (d)wFpIndex */
	/* the first pointer is right after the NTH + align */
	put_ncm(&tmp, opts->fp_index, opts->nth_size + ndp_pad);

	tmp = tmp + ndp_pad;

	/* NDP */
	put_unaligned_le32(opts->ndp_sign, tmp); /* dwSignature */
	tmp += 2;
	/* wLength */
	put_unaligned_le16(ncb_len - opts->nth_size - pad, tmp++);

	tmp += opts->reserved1;
	tmp += opts->next_fp_index; /* skip reserved (d)wNextFpIndex */
	tmp += opts->reserved2;

#if !USE_LWIP
	if (ncm->is_crc) {
		uint32_t crc;

		crc = xcrc32(pxBufferDescriptor->pucEthernetBuffer + ncb_len, net_data_len, ~0);
		put_unaligned_le32(crc, pxBufferDescriptor->pucEthernetBuffer + net_data_len + ncb_len);
	}

	/* (d)wDatagramIndex[0] */
	put_ncm(&tmp, opts->dgram_item_len, ncb_len);
	/* (d)wDatagramLength[0] */
	put_ncm(&tmp, opts->dgram_item_len, pxBufferDescriptor->xDataLength - ncb_len);
	/* (d)wDatagramIndex[1] and  (d)wDatagramLength[1] already zeroed */
#else
	if (ncm->is_crc) {
		uint32_t crc;

		crc = xcrc32((unsigned char *)pxBufferDescriptor->payload + ncb_len, net_data_len, ~0);
		put_unaligned_le32(crc, (void*)((char *)pxBufferDescriptor->payload + net_data_len + ncb_len));
	}

	/* (d)wDatagramIndex[0] */
	put_ncm(&tmp, opts->dgram_item_len, ncb_len);
	/* (d)wDatagramLength[0] */
	put_ncm(&tmp, opts->dgram_item_len, pxBufferDescriptor->len - ncb_len);
	/* (d)wDatagramIndex[1] and  (d)wDatagramLength[1] already zeroed */
#endif
	return (void*)pxBufferDescriptor;
}


static int ncm_unwrap_ntb(struct gether *port, uint8_t* data_buf, int len, List_t *frames)
{
	struct f_ncm	*ncm = func_to_ncm(&port->func);
	__le16		*tmp =  (void *) data_buf;
	unsigned	index, index2;
	int ndp_index;
	unsigned	dg_len, dg_len2;
	unsigned	ndp_len;
#if !USE_LWIP
	NetworkBufferDescriptor_t*  pxBufferDescriptor;
#else
	struct pbuf *pxBufferDescriptor;
	struct pbuf *iter = NULL;
	struct pbuf *first_node = NULL;
#endif
	int		ret = -EINVAL;
	unsigned	max_size = le32_to_cpu(ntb_parameters.dwNtbOutMaxSize);
	struct ndp_parser_opts *opts = ncm->parser_opts;
	unsigned	crc_len = ncm->is_crc ? sizeof(uint32_t) : 0;
	int		dgram_counter;

	/* dwSignature */
	if (get_unaligned_le32(tmp) != opts->nth_sign) {
		INFO(port->func.config->cdev, "Wrong NTH SIGN, skblen %d\n", len);

		goto err;
	}
	tmp += 2;
	/* wHeaderLength */
	if (get_unaligned_le16(tmp++) != opts->nth_size) {
		INFO(port->func.config->cdev, "Wrong NTB headersize\n");
		goto err;
	}
	tmp++; /* skip wSequence */

	/* (d)wBlockLength */
	if (get_ncm(&tmp, opts->block_length) > max_size) {
		INFO(port->func.config->cdev, "OUT size exceeded\n");
		goto err;
	}

	ndp_index = get_ncm(&tmp, opts->fp_index);

	/* Run through all the NDP's in the NTB */

	do {
		/* NCM 3.2 */ 
		if (((ndp_index % 4) != 0) && (ndp_index < opts->nth_size)) { 
			INFO(port->func.config->cdev, "Bad index: %#X\n", 
			ndp_index); 
			goto err; 
		}
		/* walk through NDP */ 
		tmp = (void *)(data_buf + ndp_index);
		if (get_unaligned_le32(tmp) != opts->ndp_sign) {
			INFO(port->func.config->cdev, "Wrong NDP SIGN\n");
			goto err;
		}
		tmp += 2;

		ndp_len = get_unaligned_le16(tmp++);
		/*
		* NCM 3.3.1
		* entry is 2 items
		* item size is 16/32 bits, opts->dgram_item_len * 2 bytes
		* minimal: struct usb_cdc_ncm_ndpX + normal entry + zero entry
		* Each entry is a dgram index and a dgram length.
		*/
		if ((ndp_len < opts->ndp_size + 2 * 2 * (opts->dgram_item_len * 2))
				|| (ndp_len % opts->ndplen_align != 0)) { 
			INFO(port->func.config->cdev, "Bad NDP length: %#X\n", ndp_len);
			goto err; 
		}
		tmp += opts->reserved1;
		/* Check for another NDP (d)wNextNdpIndex */
		ndp_index = get_ncm(&tmp, opts->next_fp_index);
		tmp += opts->reserved2;

		ndp_len -= opts->ndp_size;
		index2 = get_ncm(&tmp, opts->dgram_item_len);
		dg_len2 = get_ncm(&tmp, opts->dgram_item_len);
		dgram_counter = 0;

		do {
			index = index2;
			dg_len = dg_len2;
			//printf("ncm payload len:%d\r\n", dg_len);
			if (dg_len < 14 + crc_len) { /* ethernet header + crc */
				INFO(port->func.config->cdev, "Bad dgram length: %x\n",
				     dg_len);
				goto err;
			}
			if (ncm->is_crc) {
				uint32_t crc, crc2;

				crc = get_unaligned_le32(data_buf +
							 index + dg_len - crc_len);
				crc2 = xcrc32(data_buf + index,
						 dg_len - crc_len, ~0);
				if (crc != crc2) {
					INFO(port->func.config->cdev, "Bad CRC\n");
					goto err;
				}
			}

			index2 = get_ncm(&tmp, opts->dgram_item_len);
			dg_len2 = get_ncm(&tmp, opts->dgram_item_len);
#if !USE_LWIP
			pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(dg_len - crc_len, 0);
			if (!pxBufferDescriptor) {
				goto err;
			}
			memcpy(pxBufferDescriptor->pucEthernetBuffer, data_buf + index, dg_len - crc_len);
			if (pxBufferDescriptor->xDataLength > (dg_len - crc_len)) {
				memset(pxBufferDescriptor->pucEthernetBuffer + dg_len - crc_len, 0, pxBufferDescriptor->xDataLength - (dg_len - crc_len));
			}
			listSET_LIST_ITEM_OWNER(&pxBufferDescriptor->xBufferListItem, pxBufferDescriptor);
			vListInsertEnd(frames, &pxBufferDescriptor->xBufferListItem);
#else
			pxBufferDescriptor = pbuf_alloc(PBUF_RAW, dg_len - crc_len, PBUF_RAM);
			if (!pxBufferDescriptor) {
				goto err;
			}
			
			memcpy(pxBufferDescriptor->payload, data_buf + index, dg_len - crc_len);
			if (pxBufferDescriptor->len > (dg_len - crc_len)) {
				memset((void*)((char *)pxBufferDescriptor->payload + dg_len - crc_len), 0, pxBufferDescriptor->len - (dg_len - crc_len));
			}

			if (first_node == NULL) {
				struct pbuf* header = (struct pbuf*)frames;
				first_node 	= pxBufferDescriptor;
				iter 	= pxBufferDescriptor;
				iter->next = NULL;
				header->next = first_node;
			} else {
				iter->next = pxBufferDescriptor;
				iter = pxBufferDescriptor;
				iter->next = NULL;
			}
#endif
			ndp_len -= ((opts->dgram_item_len << 1) << 1);

			dgram_counter++;

			if (index2 == 0 || dg_len2 == 0)
				break;
		} while (ndp_len > (opts->dgram_item_len  <<  1) << 1); /* zero entry */
	}while (ndp_index);


	VDBG(port->func.config->cdev,
	     "Parsed NTB with %d frames\n", dgram_counter);
	return 0;
err:
	return ret;
}

static void ncm_suspend(struct usb_function *f)
{
	struct f_ncm		*ncm = func_to_ncm(f);
	//struct usb_composite_dev *cdev = f->config->cdev;

	printf("%s:%d\n", __func__, __LINE__);

}
static void ncm_disable(struct usb_function *f)
{
	struct f_ncm		*ncm = func_to_ncm(f);
	//struct usb_composite_dev *cdev = f->config->cdev;

	DBG(cdev, "ncm deactivated\n");
	spin_lock(&ncm->lock);
	if (!ncm->connect) {
		spin_unlock(&ncm->lock);
		return;
	}
	TRACE_INFO("usb disconnect\r\n");

	if (ncm->notify->driver_data) {
		usb_ep_disable(ncm->notify);
		ncm->notify->driver_data = NULL;
		ncm->notify->desc = NULL;
	}
	gether_disconnect(&ncm->port);
	ncm->connect = false;
	spin_unlock(&ncm->lock);
}

/*static void ncm_disconnect(struct gether *port)
{
	struct f_ncm	*ncm = func_to_ncm(&port->func);
	spin_lock(&ncm->lock);
	if (ncm->notify->driver_data) {
		usb_ep_disable(ncm->notify);
		ncm->notify->driver_data = NULL;
		ncm->notify->desc = NULL;
	}
	spin_unlock(&ncm->lock);
}*/

static void ncm_open(struct gether *geth)
{
	struct f_ncm		*ncm = func_to_ncm(&geth->func);

	DBG(ncm->port.func.config->cdev, "%s\n", __func__);

	spin_lock(&ncm->lock);
	ncm->is_open = true;
	ncm_notify(ncm);
	spin_unlock(&ncm->lock);
}

static void ncm_close(struct gether *geth)
{
	struct f_ncm		*ncm = func_to_ncm(&geth->func);

	DBG(ncm->port.func.config->cdev, "%s\n", __func__);

	spin_lock(&ncm->lock);
	ncm->is_open = false;
	ncm_notify(ncm);
	spin_unlock(&ncm->lock);
}

/*-------------------------------------------------------------------------*/

/* ethernet function driver setup/binding */

static int ncm_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_ncm		*ncm = func_to_ncm(f);
	int			status;
	struct usb_ep		*ep;

	/* allocate instance-specific interface IDs */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	ncm->ctrl_id = status;
	ncm_iad_desc.bFirstInterface = status;

	ncm_control_intf.bInterfaceNumber = status;
	ncm_union_desc.bMasterInterface0 = status;

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	ncm->data_id = status;

	ncm_data_nop_intf.bInterfaceNumber = status;
	ncm_data_intf.bInterfaceNumber = status;
	ncm_union_desc.bSlaveInterface0 = status;

	status = -ENODEV;
	ep = usb_ep_autoconfig(cdev->gadget, &fs_ncm_in_desc);
	if (!ep)
		goto fail;
	ncm->port.in_ep = ep;
	ep->driver_data = cdev;	/* claim */

	ep = usb_ep_autoconfig(cdev->gadget, &fs_ncm_out_desc);
	if (!ep)
		goto fail;
	ncm->port.out_ep = ep;
	ep->driver_data = cdev;	/* claim */

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &fs_ncm_notify_desc);
	if (!ep)
		goto fail;
	ncm->notify = ep;
	ep->driver_data = cdev;	/* claim */
	/* allocate notification request and buffer */
	ncm->notify_req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!ncm->notify_req)
		goto fail;
	ncm->notify_req->buf = kmalloc(NCM_STATUS_BYTECOUNT, GFP_KERNEL);
	if (!ncm->notify_req->buf)
		goto fail;
	ncm->notify_req->context = ncm;
	ncm->notify_req->complete = ncm_notify_complete;
	
	status = -ENOMEM;

	/* copy descriptors, and track endpoint copies */
	f->descriptors = usb_copy_descriptors(ncm_fs_function);
	if (!f->descriptors)
		goto fail;

	/*
	 * support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_ncm_in_desc.bEndpointAddress =
				fs_ncm_in_desc.bEndpointAddress;
		hs_ncm_out_desc.bEndpointAddress =
				fs_ncm_out_desc.bEndpointAddress;
		hs_ncm_notify_desc.bEndpointAddress =
				fs_ncm_notify_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(ncm_hs_function);
		if (!f->hs_descriptors)
			goto fail;
	}

	DBG(cdev, "CDC Network: %s speed IN/%s OUT/%s NOTIFY/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			ncm->in_ep->name, ncm->out_ep->name,
			ncm->notify->name);
	return 0;

fail:
	if (f->descriptors)
		usb_free_descriptors(f->descriptors);

	if (ncm->notify_req) {
		kfree(ncm->notify_req->buf);
		usb_ep_free_request(ncm->notify, ncm->notify_req);
	}

	/* we might as well release our claims on endpoints */
	if (ncm->notify)
		ncm->notify->driver_data = NULL;
	if (ncm->port.out_ep->desc)
		ncm->port.out_ep->driver_data = NULL;
	if (ncm->port.in_ep->desc)
		ncm->port.in_ep->driver_data = NULL;

	printf("%s: can't bind, err %d\r\n", f->name, status);

	return status;
}

static void
ncm_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_ncm		*ncm = func_to_ncm(f);

	DBG(c->cdev, "ncm unbind\n");

	if (gadget_is_dualspeed(c->cdev->gadget))
		usb_free_descriptors(f->hs_descriptors);
	usb_free_descriptors(f->descriptors);

	kfree(ncm->notify_req->buf);
	usb_ep_free_request(ncm->notify, ncm->notify_req);

	ncm_string_defs[1].s = NULL;
	kfree(ncm);
}

static void prvNcmTimerCallback( TimerHandle_t xTimerHandle )
{
	struct f_ncm		*ncm = (struct f_ncm *)pvTimerGetTimerID(xTimerHandle);
	TRACE_INFO("prvNcmTimerCallback\r\n");
	ncm->is_open = true;
	ncm_notify(ncm);
}

/**
 * ncm_bind_config - add CDC Network link to a configuration
 * @c: the configuration to support the network link
 * @ethaddr: a buffer in which the ethernet address of the host side
 *	side of the link was recorded
 * Context: single threaded during gadget setup
 *
 * Returns zero on success, else negative errno.
 *
 * Caller must have called @gether_setup().  Caller is also responsible
 * for calling @gether_cleanup() before module unload.
 */
int ncm_bind_config(struct usb_configuration *c, u8 ethaddr[ETH_ALEN])
{
	struct f_ncm	*ncm;
	int		status;

	/* maybe allocate device-global string IDs */
	if (ncm_string_defs[0].id == 0) {

		/* control interface label */
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		ncm_string_defs[STRING_CTRL_IDX].id = status;
		ncm_control_intf.iInterface = status;

		/* data interface label */
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		ncm_string_defs[STRING_DATA_IDX].id = status;
		ncm_data_nop_intf.iInterface = status;
		ncm_data_intf.iInterface = status;

		/* MAC address */
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		ncm_string_defs[STRING_MAC_IDX].id = status;
		ecm_desc.iMACAddress = status;

		/* IAD */
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		ncm_string_defs[STRING_IAD_IDX].id = status;
		ncm_iad_desc.iFunction = status;
	}

	/* allocate and initialize one new instance */
	ncm = kzalloc(sizeof *ncm, GFP_KERNEL);
	if (!ncm)
		return -ENOMEM;

	/* export host's Ethernet address in CDC format */
	snprintf(ncm->ethaddr, sizeof ncm->ethaddr,
		"%02X%02X%02X%02X%02X%02X",
		ethaddr[0], ethaddr[1], ethaddr[2],
		ethaddr[3], ethaddr[4], ethaddr[5]);
	ncm_string_defs[STRING_MAC_IDX].s = ncm->ethaddr;

	spin_lock_init(&ncm->lock);
	ncm_reset_values(ncm);
	ncm->port.is_fixed = true;
	listSET_LIST_ITEM_OWNER(&(ncm->port.func.list), &(ncm->port.func));
	ncm->port.func.powner = (void*)ncm;

	ncm->port.func.name = "cdc_network";
	ncm->port.func.strings = ncm_strings;
	/* descriptors are per-instance copies */
	ncm->port.func.bind = ncm_bind;
	ncm->port.func.unbind = ncm_unbind;
	ncm->port.func.set_alt = ncm_set_alt;
	ncm->port.func.get_alt = ncm_get_alt;
	ncm->port.func.setup = ncm_setup;
	ncm->port.func.disable = ncm_disable;
	ncm->port.func.suspend = ncm_suspend;
	ncm->port.unwrap = ncm_unwrap_ntb;
	ncm->port.wrap = ncm_wrap_ntb;
	ncm->port.wrap_ext = ncm_wrap_ext_ntb;
	ncm->port.disconnect_cb = NULL;//ncm_disconnect;
	ncm->port.open = ncm_open;
	ncm->port.close = ncm_close;
	ncm->port.in = get_ep_desc(c->cdev->gadget, &hs_ncm_in_desc, &fs_ncm_in_desc);
	ncm->port.out = get_ep_desc(c->cdev->gadget, &hs_ncm_out_desc, &fs_ncm_out_desc);

	ncm->timer = xTimerCreate( "NcmTimer",
						pdMS_TO_TICKS(3000),		/* The period of the software timer in ticks. */
						pdFALSE,			/* xAutoReload is set to pdFALSE, so this is a one-shot timer. */
						(void*)ncm,				/* The timer's ID is not used. */
						prvNcmTimerCallback );

	status = usb_add_function(c, &ncm->port.func);
	if (status) {
		ncm_string_defs[1].s = NULL;
		kfree(ncm);
	}
	return status;
}

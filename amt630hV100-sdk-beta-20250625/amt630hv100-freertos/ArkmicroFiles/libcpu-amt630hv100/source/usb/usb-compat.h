#ifndef __USB_COMPAT_H__
#define __USB_COMPAT_H__

//#include <dm.h>
#include "usb.h"
#include "timer.h"


struct usb_bus {
	int busnum;			/* Bus number (in order of reg) */
	const char *bus_name;		/* stable id (PCI slot_name etc) */
	u8 uses_dma;			/* Does the host controller use DMA? */
	u8 uses_pio_for_control;	/*
					 * Does the host controller use PIO
					 * for control transfers?
					 */
	u8 otg_port;			/* 0, or number of OTG/HNP port */
	unsigned is_b_host:1;		/* true during some HNP roleswitches */
	unsigned b_hnp_enable:1;	/* OTG: did A-Host enable HNP? */
	unsigned no_stop_on_short:1;    /*
					 * Quirk: some controllers don't stop
					 * the ep queue on a short transfer
					 * with the URB_SHORT_NOT_OK flag set.
					 */
	unsigned no_sg_constraint:1;	/* no sg constraint */
	unsigned sg_tablesize;		/* 0 or largest number of sg list entries */

	int devnum_next;		/* Next open device number in
					 * round-robin allocation */
	
	int bandwidth_allocated;	/* on this bus: how much of the time
					 * reserved for periodic (intr/iso)
					 * requests is used, on average?
					 * Units: microseconds/frame.
					 * Limits: Full/low speed reserve 90%,
					 * while high speed reserves 80%.
					 */
	int bandwidth_int_reqs;		/* number of Interrupt requests */
	int bandwidth_isoc_reqs;	/* number of Isoc. requests */

	unsigned resuming_ports;	/* bit array: resuming root-hub ports */
};


struct usb_hcd {
	struct usb_bus		self;
	int has_tt;
	void *hcd_priv;
};

struct usb_host_endpoint {
	struct usb_endpoint_descriptor		desc;
#ifndef NO_GNU
	struct list_head urb_list;
#else
    List_t       urb_list;
#endif
	void *hcpriv;
};

/*
 * urb->transfer_flags:
 *
 * Note: URB_DIR_IN/OUT is automatically set in usb_submit_urb().
 */
#define URB_SHORT_NOT_OK	0x0001	/* report short reads as errors */
#define URB_ZERO_PACKET		0x0040	/* Finish bulk OUT with short packet */
#define URB_NO_INTERRUPT	0x0080	/* HINT: no non-error interrupt*/

struct urb;

typedef void (*usb_complete_t)(struct urb *);

struct urb {
	void *hcpriv;			/* private data for host controller */
#ifndef NO_GNU
	struct list_head urb_list;	/* list head for use by the urb's
					 * current owner */
#else
    ListItem_t           urb_list;
	void *queueHandle;
#endif
	struct usb_device *dev;		/* (in) pointer to associated device */
	struct usb_host_endpoint *ep;	/* (internal) pointer to endpoint */
	unsigned int pipe;		/* (in) pipe information */
	int status;			/* (return) non-ISO status */
	unsigned int transfer_flags;	/* (in) URB_SHORT_NOT_OK | ...*/
	void *transfer_buffer;		/* (in) associated data buffer */
	dma_addr_t transfer_dma;	/* (in) dma addr for transfer_buffer */
	u32 transfer_buffer_length;	/* (in) data buffer length */
	u32 actual_length;		/* (return) actual transfer length */
	unsigned char *setup_packet;	/* (in) setup packet (control only) */
	dma_addr_t setup_dma;
	int start_frame;		/* (modify) start frame (ISO) */
	void *context;
	usb_complete_t complete;	/* (in) completion routine */
	int interval;

	int error_count;		/* (return) number of ISO errors */
	int number_of_packets;		/* (in) number of ISO packets */
	struct usb_iso_packet_descriptor iso_frame_desc[0];
};

struct hc_driver {
	const char	*description;	/* "ehci-hcd" etc */
	const char	*product_desc;	/* product/vendor string */
	size_t		hcd_priv_size;	/* size of private data */

	/* irq handler */
	irqreturn_t	(*irq) (struct usb_hcd *hcd);

	int	flags;
#define	HCD_MEMORY	0x0001		/* HC regs use memory (else I/O) */
#define	HCD_LOCAL_MEM	0x0002		/* HC needs local memory */
#define	HCD_SHARED	0x0004		/* Two (or more) usb_hcds share HW */
#define	HCD_USB11	0x0010		/* USB 1.1 */
#define	HCD_USB2	0x0020		/* USB 2.0 */
#define	HCD_USB25	0x0030		/* Wireless USB 1.0 (USB 2.5)*/
#define	HCD_USB3	0x0040		/* USB 3.0 */
#define	HCD_USB31	0x0050		/* USB 3.1 */
#define	HCD_MASK	0x0070
#define	HCD_BH		0x0100		/* URB complete in BH context */

	/* called to init HCD and root hub */
	int	(*reset) (struct usb_hcd *hcd);
	int	(*start) (struct usb_hcd *hcd);

	/* NOTE:  these suspend/resume calls relate to the HC as
	 * a whole, not just the root hub; they're for PCI bus glue.
	 */
	/* called after suspending the hub, before entering D3 etc */
	int	(*pci_suspend)(struct usb_hcd *hcd, bool do_wakeup);

	/* called after entering D0 (etc), before resuming the hub */
	int	(*pci_resume)(struct usb_hcd *hcd, bool hibernated);

	/* cleanly make HCD stop writing memory and doing I/O */
	void	(*stop) (struct usb_hcd *hcd);

	/* shutdown HCD */
	void	(*shutdown) (struct usb_hcd *hcd);

	/* return current frame number */
	int	(*get_frame_number) (struct usb_hcd *hcd);

	/* manage i/o requests, device state */
	int	(*urb_enqueue)(struct usb_hcd *hcd,
				struct urb *urb, gfp_t mem_flags);
	int	(*urb_dequeue)(struct usb_hcd *hcd,
				struct urb *urb, int status);

	/*
	 * (optional) these hooks allow an HCD to override the default DMA
	 * mapping and unmapping routines.  In general, they shouldn't be
	 * necessary unless the host controller has special DMA requirements,
	 * such as alignment contraints.  If these are not specified, the
	 * general usb_hcd_(un)?map_urb_for_dma functions will be used instead
	 * (and it may be a good idea to call these functions in your HCD
	 * implementation)
	 */
	int	(*map_urb_for_dma)(struct usb_hcd *hcd, struct urb *urb,
				   gfp_t mem_flags);
	void    (*unmap_urb_for_dma)(struct usb_hcd *hcd, struct urb *urb);

	/* hw synch, freeing endpoint resources that urb_dequeue can't */
	void	(*endpoint_disable)(struct usb_hcd *hcd,
			struct usb_host_endpoint *ep);

	/* (optional) reset any endpoint state such as sequence number
	   and current window */
	void	(*endpoint_reset)(struct usb_hcd *hcd,
			struct usb_host_endpoint *ep);

	/* root hub support */
	int	(*hub_status_data) (struct usb_hcd *hcd, char *buf);
	int	(*hub_control) (struct usb_hcd *hcd,
				u16 typeReq, u16 wValue, u16 wIndex,
				char *buf, u16 wLength);
	int	(*bus_suspend)(struct usb_hcd *);
	int	(*bus_resume)(struct usb_hcd *);
	int	(*start_port_reset)(struct usb_hcd *, unsigned port_num);

		/* force handover of high-speed port to full-speed companion */
	void	(*relinquish_port)(struct usb_hcd *, int);
		/* has a port been handed over to a companion? */
	int	(*port_handed_over)(struct usb_hcd *, int);

		/* CLEAR_TT_BUFFER completion callback */
	void	(*clear_tt_buffer_complete)(struct usb_hcd *,
				struct usb_host_endpoint *);

	/* xHCI specific functions */
		/* Called by usb_alloc_dev to alloc HC device structures */
	int	(*alloc_dev)(struct usb_hcd *, struct usb_device *);
		/* Called by usb_disconnect to free HC device structures */
	void	(*free_dev)(struct usb_hcd *, struct usb_device *);
	/* Change a group of bulk endpoints to support multiple stream IDs */
	int	(*alloc_streams)(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		unsigned int num_streams, gfp_t mem_flags);
	/* Reverts a group of bulk endpoints back to not using stream IDs.
	 * Can fail if we run out of memory.
	 */
	int	(*free_streams)(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		gfp_t mem_flags);

	/* Bandwidth computation functions */
	/* Note that add_endpoint() can only be called once per endpoint before
	 * check_bandwidth() or reset_bandwidth() must be called.
	 * drop_endpoint() can only be called once per endpoint also.
	 * A call to xhci_drop_endpoint() followed by a call to
	 * xhci_add_endpoint() will add the endpoint to the schedule with
	 * possibly new parameters denoted by a different endpoint descriptor
	 * in usb_host_endpoint.  A call to xhci_add_endpoint() followed by a
	 * call to xhci_drop_endpoint() is not allowed.
	 */
		/* Allocate endpoint resources and add them to a new schedule */
	int	(*add_endpoint)(struct usb_hcd *, struct usb_device *,
				struct usb_host_endpoint *);
		/* Drop an endpoint from a new schedule */
	int	(*drop_endpoint)(struct usb_hcd *, struct usb_device *,
				 struct usb_host_endpoint *);
		/* Check that a new hardware configuration, set using
		 * endpoint_enable and endpoint_disable, does not exceed bus
		 * bandwidth.  This must be called before any set configuration
		 * or set interface requests are sent to the device.
		 */
	int	(*check_bandwidth)(struct usb_hcd *, struct usb_device *);
		/* Reset the device schedule to the last known good schedule,
		 * which was set from a previous successful call to
		 * check_bandwidth().  This reverts any add_endpoint() and
		 * drop_endpoint() calls since that last successful call.
		 * Used for when a check_bandwidth() call fails due to resource
		 * or bandwidth constraints.
		 */
	void	(*reset_bandwidth)(struct usb_hcd *, struct usb_device *);
		/* Returns the hardware-chosen device address */
	int	(*address_device)(struct usb_hcd *, struct usb_device *udev);
		/* prepares the hardware to send commands to the device */
	int	(*enable_device)(struct usb_hcd *, struct usb_device *udev);
		/* Notifies the HCD after a hub descriptor is fetched.
		 * Will block.
		 */
	int	(*update_hub_device)(struct usb_hcd *, struct usb_device *hdev,
			struct usb_tt *tt, gfp_t mem_flags);
	int	(*reset_device)(struct usb_hcd *, struct usb_device *);
		/* Notifies the HCD after a device is connected and its
		 * address is set
		 */
	int	(*update_device)(struct usb_hcd *, struct usb_device *);
	int	(*set_usb2_hw_lpm)(struct usb_hcd *, struct usb_device *, int);
	int	(*find_raw_port_number)(struct usb_hcd *, int);
	/* Call for power on/off the port if necessary */
	int	(*port_power)(struct usb_hcd *hcd, int portnum, bool enable);

};


#ifndef NO_GNU
#define usb_hcd_link_urb_to_ep(hcd, urb)	({		\
	int ret = 0;						\
	list_add_tail(&urb->urb_list, &urb->ep->urb_list);	\
	ret; })
#else
#define usb_hcd_link_urb_to_ep(hcd, urb)	do {		\
	list_add_tail(&urb->urb_list, &urb->ep->urb_list);	\
} while(0)	
#endif
#define usb_hcd_unlink_urb_from_ep(hcd, urb)	list_del_init(&urb->urb_list)
#define usb_hcd_check_unlink_urb(hdc, urb, status)	0

static inline void usb_hcd_giveback_urb(struct usb_hcd *hcd,
					struct urb *urb,
					int status)
{
	urb->status = status;
	if (urb->complete)
		urb->complete(urb);
}

static inline int usb_hcd_unmap_urb_for_dma(struct usb_hcd *hcd,
					struct urb *urb)
{
	/* TODO: add cache invalidation here */
	return 0;
}

static inline struct usb_bus *hcd_to_bus(struct usb_hcd *hcd)
{
	return &hcd->self;
}

static inline void usb_hcd_resume_root_hub(struct usb_hcd *hcd)
{
	(void)hcd;
	return;
}

/*
 * Generic bandwidth allocation constants/support
 */
#define FRAME_TIME_USECS	1000L
#define BitTime(bytecount) (7 * 8 * bytecount / 6) /* with integer truncation */
		/* Trying not to use worst-case bit-stuffing
		 * of (7/6 * 8 * bytecount) = 9.33 * bytecount */
		/* bytecount = data payload byte count */

#define NS_TO_US(ns)	DIV_ROUND_UP(ns, 1000L)
			/* convert nanoseconds to microseconds, rounding up */

/*
 * Full/low speed bandwidth allocation constants/support.
 */
#define BW_HOST_DELAY	1000L		/* nanoseconds */
#define BW_HUB_LS_SETUP	333L		/* nanoseconds */
			/* 4 full-speed bit times (est.) */

#define FRAME_TIME_BITS			12000L	/* frame = 1 millisecond */
#define FRAME_TIME_MAX_BITS_ALLOC	(90L * FRAME_TIME_BITS / 100L)
#define FRAME_TIME_MAX_USECS_ALLOC	(90L * FRAME_TIME_USECS / 100L)

/*
 * Ceiling [nano/micro]seconds (typical) for that many bytes at high speed
 * ISO is a bit less, no ACK ... from USB 2.0 spec, 5.11.3 (and needed
 * to preallocate bandwidth)
 */
#define USB2_HOST_DELAY	5	/* nsec, guess */
#define HS_NSECS(bytes) (((55 * 8 * 2083) \
	+ (2083UL * (3 + BitTime(bytes))))/1000 \
	+ USB2_HOST_DELAY)
#define HS_NSECS_ISO(bytes) (((38 * 8 * 2083) \
	+ (2083UL * (3 + BitTime(bytes))))/1000 \
	+ USB2_HOST_DELAY)
#define HS_USECS(bytes)		NS_TO_US(HS_NSECS(bytes))
#define HS_USECS_ISO(bytes)	NS_TO_US(HS_NSECS_ISO(bytes))


static inline long usb_calc_bus_time (int speed, int is_input, int isoc, int bytecount)
{
	unsigned long	tmp;

	switch (speed) {
	case USB_SPEED_LOW: 	/* INTR only */
		if (is_input) {
			tmp = (67667L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return 64060L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp;
		} else {
			tmp = (66700L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return 64107L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp;
		}
	case USB_SPEED_FULL:	/* ISOC or INTR */
		if (isoc) {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return ((is_input) ? 7268L : 6265L) + BW_HOST_DELAY + tmp;
		} else {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return 9107L + BW_HOST_DELAY + tmp;
		}
	case USB_SPEED_HIGH:	/* ISOC or INTR */
		/* FIXME adjust for input vs output */
		if (isoc)
			tmp = HS_NSECS_ISO (bytecount);
		else
			tmp = HS_NSECS (bytecount);
		return tmp;
	default:
		//pr_debug ("%s: bogus device speed!\n", usbcore_name);
		return -1;
	}
}


#ifdef CONFIG_DM_USB
static inline struct usb_device *usb_dev_get_parent(struct usb_device *udev)
{
	struct udevice *parent = udev->dev->parent;

	/*
	 * When called from usb-uclass.c: usb_scan_device() udev->dev points
	 * to the parent udevice, not the actual udevice belonging to the
	 * udev as the device is not instantiated yet.
	 *
	 * If dev is an usb-bus, then we are called from usb_scan_device() for
	 * an usb-device plugged directly into the root port, return NULL.
	 */
	if (device_get_uclass_id(udev->dev) == UCLASS_USB)
		return NULL;

	/*
	 * If these 2 are not the same we are being called from
	 * usb_scan_device() and udev itself is the parent.
	 */
	if (dev_get_parent_priv(udev->dev) != udev)
		return udev;

	/* We are being called normally, use the parent pointer */
	if (device_get_uclass_id(parent) == UCLASS_USB_HUB)
		return dev_get_parent_priv(parent);

	return NULL;
}
#else
static inline struct usb_device *usb_dev_get_parent(struct usb_device *dev)
{
	return dev->parent;
}
#endif

#if 0
static inline void usb_gadget_giveback_request(struct usb_ep *ep,
		struct usb_request *req)
{
	req->complete(ep, req);
}

static inline int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return 0;

	req->dma = dma_map_single(req->buf, req->length,
				  is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	return 0;
}

static inline void usb_gadget_unmap_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return;

	dma_unmap_single((void *)req->dma, req->length,
			 is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
}
#else
void usb_gadget_giveback_request(struct usb_ep *ep,
		struct usb_request *req);

int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in);

void usb_gadget_unmap_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in);
#endif

#endif /* __USB_COMPAT_H__ */

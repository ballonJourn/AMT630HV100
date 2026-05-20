/*
 * <linux/usb/gadget.h>
 *
 * We call the USB code inside a Linux-based peripheral device a "gadget"
 * driver, except for the hardware-specific bus glue.  One USB host can
 * master many USB gadgets, but the gadgets are only slaved to one host.
 *
 *
 * (C) Copyright 2002-2004 by David Brownell
 * All Rights Reserved.
 *
 * This software is licensed under the GNU GPL version 2.
 *
 * Ported to U-Boot by: Thomas Smits <ts.smits@gmail.com> and
 *                      Remy Bohmer <linux@bohmer.net>
 */

#ifndef __LINUX_USB_GADGET_H
#define __LINUX_USB_GADGET_H
/*
#include <errno.h>
#include <linux/compat.h>
#include <linux/list.h>*/
#include "usb_os_adapter.h"
#include <linux/errno.h>
#include <stdbool.h>

struct usb_ep;

/**
 * struct usb_request - describes one i/o request
 * @buf: Buffer used for data.  Always provide this; some controllers
 *	only use PIO, or don't use DMA for some endpoints.
 * @dma: DMA address corresponding to 'buf'.  If you don't set this
 *	field, and the usb controller needs one, it is responsible
 *	for mapping and unmapping the buffer.
 * @stream_id: The stream id, when USB3.0 bulk streams are being used
 * @length: Length of that data
 * @no_interrupt: If true, hints that no completion irq is needed.
 *	Helpful sometimes with deep request queues that are handled
 *	directly by DMA controllers.
 * @zero: If true, when writing data, makes the last packet be "short"
 *     by adding a zero length packet as needed;
 * @short_not_ok: When reading data, makes short packets be
 *     treated as errors (queue stops advancing till cleanup).
 * @complete: Function called when request completes, so this request and
 *	its buffer may be re-used.
 *	Reads terminate with a short packet, or when the buffer fills,
 *	whichever comes first.  When writes terminate, some data bytes
 *	will usually still be in flight (often in a hardware fifo).
 *	Errors (for reads or writes) stop the queue from advancing
 *	until the completion function returns, so that any transfers
 *	invalidated by the error may first be dequeued.
 * @context: For use by the completion callback
 * @list: For use by the gadget driver.
 * @status: Reports completion code, zero or a negative errno.
 *	Normally, faults block the transfer queue from advancing until
 *	the completion callback returns.
 *	Code "-ESHUTDOWN" indicates completion caused by device disconnect,
 *	or when the driver disabled the endpoint.
 * @actual: Reports bytes transferred to/from the buffer.  For reads (OUT
 *	transfers) this may be less than the requested length.  If the
 *	short_not_ok flag is set, short reads are treated as errors
 *	even when status otherwise indicates successful completion.
 *	Note that for writes (IN transfers) some data bytes may still
 *	reside in a device-side FIFO when the request is reported as
 *	complete.
 *
 * These are allocated/freed through the endpoint they're used with.  The
 * hardware's driver can add extra per-request data to the memory it returns,
 * which often avoids separate memory allocations (potential failures),
 * later when the request is queued.
 *
 * Request flags affect request handling, such as whether a zero length
 * packet is written (the "zero" flag), whether a short read should be
 * treated as an error (blocking request queue advance, the "short_not_ok"
 * flag), or hinting that an interrupt is not required (the "no_interrupt"
 * flag, for use with deep request queues).
 *
 * Bulk endpoints can use any size buffers, and can also be used for interrupt
 * transfers. interrupt-only endpoints can be much less functional.
 *
 * NOTE:  this is analagous to 'struct urb' on the host side, except that
 * it's thinner and promotes more pre-allocation.
 */

struct usb_request {
	void			*buf;
	unsigned		length;
	dma_addr_t		dma;

	unsigned		stream_id:16;
	unsigned		no_interrupt:1;
	unsigned		zero:1;
	unsigned		short_not_ok:1;

	void			(*complete)(struct usb_ep *ep,
					struct usb_request *req);
	void			*context;

	ListItem_t			list;
	void*				powner;

	int			status;
	unsigned		actual;
};

/*-------------------------------------------------------------------------*/

/* endpoint-specific parts of the api to the usb controller hardware.
 * unlike the urb model, (de)multiplexing layers are not required.
 * (so this api could slash overhead if used on the host side...)
 *
 * note that device side usb controllers commonly differ in how many
 * endpoints they support, as well as their capabilities.
 */
struct usb_ep_ops {
	int (*enable) (struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc);
	int (*disable) (struct usb_ep *ep);

	struct usb_request *(*alloc_request) (struct usb_ep *ep,
		gfp_t gfp_flags);
	void (*free_request) (struct usb_ep *ep, struct usb_request *req);

	int (*queue) (struct usb_ep *ep, struct usb_request *req,
		gfp_t gfp_flags);
	int (*dequeue) (struct usb_ep *ep, struct usb_request *req);

	int (*set_halt) (struct usb_ep *ep, int value);
	int (*set_wedge)(struct usb_ep *ep);
	int (*fifo_status) (struct usb_ep *ep);
	void (*fifo_flush) (struct usb_ep *ep);
};

/**
 * struct usb_ep - device side representation of USB endpoint
 * @name:identifier for the endpoint, such as "ep-a" or "ep9in-bulk"
 * @ops: Function pointers used to access hardware-specific operations.
 * @ep_list:the gadget's ep_list holds all of its endpoints
 * @maxpacket:The maximum packet size used on this endpoint.  The initial
 *	value can sometimes be reduced (hardware allowing), according to
 *      the endpoint descriptor used to configure the endpoint.
 * @maxpacket_limit:The maximum packet size value which can be handled by this
 *	endpoint. It's set once by UDC driver when endpoint is initialized, and
 *	should not be changed. Should not be confused with maxpacket.
 * @max_streams: The maximum number of streams supported
 * 	by this EP (0 - 16, actual number is 2^n)
 * @maxburst: the maximum number of bursts supported by this EP (for usb3)
 * @driver_data:for use by the gadget driver.  all other fields are
 *	read-only to gadget drivers.
 * @desc: endpoint descriptor.  This pointer is set before the endpoint is
 * 	enabled and remains valid until the endpoint is disabled.
 * @comp_desc: In case of SuperSpeed support, this is the endpoint companion
 * 	descriptor that is used to configure the endpoint
 *
 * the bus controller driver lists all the general purpose endpoints in
 * gadget->ep_list.  the control endpoint (gadget->ep0) is not in that list,
 * and is accessed only in response to a driver setup() callback.
 */
struct usb_ep {
	void			*driver_data;
	const char		*name;
	const struct usb_ep_ops	*ops;
	ListItem_t			ep_list;
	void*				powner;
	unsigned		maxpacket:16;
	unsigned		maxpacket_limit:16;
	unsigned		max_streams:16;
	unsigned		maxburst:5;
	const struct usb_endpoint_descriptor	*desc;
	const struct usb_ss_ep_comp_descriptor	*comp_desc;
};

/*-------------------------------------------------------------------------*/

/**
 * usb_ep_set_maxpacket_limit - set maximum packet size limit for endpoint
 * @ep:the endpoint being configured
 * @maxpacket_limit:value of maximum packet size limit
 *
 * This function shoud be used only in UDC drivers to initialize endpoint
 * (usually in probe function).
 */
static inline void usb_ep_set_maxpacket_limit(struct usb_ep *ep,
					      unsigned maxpacket_limit)
{
	ep->maxpacket_limit = maxpacket_limit;
	ep->maxpacket = maxpacket_limit;
}

/**
 * usb_ep_enable - configure endpoint, making it usable
 * @ep:the endpoint being configured.  may not be the endpoint named "ep0".
 *	drivers discover endpoints through the ep_list of a usb_gadget.
 * @desc:descriptor for desired behavior.  caller guarantees this pointer
 *	remains valid until the endpoint is disabled; the data byte order
 *	is little-endian (usb-standard).
 *
 * when configurations are set, or when interface settings change, the driver
 * will enable or disable the relevant endpoints.  while it is enabled, an
 * endpoint may be used for i/o until the driver receives a disconnect() from
 * the host or until the endpoint is disabled.
 *
 * the ep0 implementation (which calls this routine) must ensure that the
 * hardware capabilities of each endpoint match the descriptor provided
 * for it.  for example, an endpoint named "ep2in-bulk" would be usable
 * for interrupt transfers as well as bulk, but it likely couldn't be used
 * for iso transfers or for endpoint 14.  some endpoints are fully
 * configurable, with more generic names like "ep-a".  (remember that for
 * USB, "in" means "towards the USB master".)
 *
 * returns zero, or a negative error code.
 */
static inline int usb_ep_enable(struct usb_ep *ep,
				const struct usb_endpoint_descriptor *desc)
{
	return ep->ops->enable(ep, desc);
}

/**
 * usb_ep_disable - endpoint is no longer usable
 * @ep:the endpoint being unconfigured.  may not be the endpoint named "ep0".
 *
 * no other task may be using this endpoint when this is called.
 * any pending and uncompleted requests will complete with status
 * indicating disconnect (-ESHUTDOWN) before this call returns.
 * gadget drivers must call usb_ep_enable() again before queueing
 * requests to the endpoint.
 *
 * returns zero, or a negative error code.
 */
static inline int usb_ep_disable(struct usb_ep *ep)
{
	return ep->ops->disable(ep);
}

/**
 * usb_ep_alloc_request - allocate a request object to use with this endpoint
 * @ep:the endpoint to be used with with the request
 * @gfp_flags:GFP_* flags to use
 *
 * Request objects must be allocated with this call, since they normally
 * need controller-specific setup and may even need endpoint-specific
 * resources such as allocation of DMA descriptors.
 * Requests may be submitted with usb_ep_queue(), and receive a single
 * completion callback.  Free requests with usb_ep_free_request(), when
 * they are no longer needed.
 *
 * Returns the request, or null if one could not be allocated.
 */
static inline struct usb_request *usb_ep_alloc_request(struct usb_ep *ep,
						       gfp_t gfp_flags)
{
	return ep->ops->alloc_request(ep, gfp_flags);
}

/**
 * usb_ep_free_request - frees a request object
 * @ep:the endpoint associated with the request
 * @req:the request being freed
 *
 * Reverses the effect of usb_ep_alloc_request().
 * Caller guarantees the request is not queued, and that it will
 * no longer be requeued (or otherwise used).
 */
static inline void usb_ep_free_request(struct usb_ep *ep,
				       struct usb_request *req)
{
	ep->ops->free_request(ep, req);
}

/**
 * usb_ep_queue - queues (submits) an I/O request to an endpoint.
 * @ep:the endpoint associated with the request
 * @req:the request being submitted
 * @gfp_flags: GFP_* flags to use in case the lower level driver couldn't
 *	pre-allocate all necessary memory with the request.
 *
 * This tells the device controller to perform the specified request through
 * that endpoint (reading or writing a buffer).  When the request completes,
 * including being canceled by usb_ep_dequeue(), the request's completion
 * routine is called to return the request to the driver.  Any endpoint
 * (except control endpoints like ep0) may have more than one transfer
 * request queued; they complete in FIFO order.  Once a gadget driver
 * submits a request, that request may not be examined or modified until it
 * is given back to that driver through the completion callback.
 *
 * Each request is turned into one or more packets.  The controller driver
 * never merges adjacent requests into the same packet.  OUT transfers
 * will sometimes use data that's already buffered in the hardware.
 * Drivers can rely on the fact that the first byte of the request's buffer
 * always corresponds to the first byte of some USB packet, for both
 * IN and OUT transfers.
 *
 * Bulk endpoints can queue any amount of data; the transfer is packetized
 * automatically.  The last packet will be short if the request doesn't fill it
 * out completely.  Zero length packets (ZLPs) should be avoided in portable
 * protocols since not all usb hardware can successfully handle zero length
 * packets.  (ZLPs may be explicitly written, and may be implicitly written if
 * the request 'zero' flag is set.)  Bulk endpoints may also be used
 * for interrupt transfers; but the reverse is not true, and some endpoints
 * won't support every interrupt transfer.  (Such as 768 byte packets.)
 *
 * Interrupt-only endpoints are less functional than bulk endpoints, for
 * example by not supporting queueing or not handling buffers that are
 * larger than the endpoint's maxpacket size.  They may also treat data
 * toggle differently.
 *
 * Control endpoints ... after getting a setup() callback, the driver queues
 * one response (even if it would be zero length).  That enables the
 * status ack, after transfering data as specified in the response.  Setup
 * functions may return negative error codes to generate protocol stalls.
 * (Note that some USB device controllers disallow protocol stall responses
 * in some cases.)  When control responses are deferred (the response is
 * written after the setup callback returns), then usb_ep_set_halt() may be
 * used on ep0 to trigger protocol stalls.
 *
 * For periodic endpoints, like interrupt or isochronous ones, the usb host
 * arranges to poll once per interval, and the gadget driver usually will
 * have queued some data to transfer at that time.
 *
 * Returns zero, or a negative error code.  Endpoints that are not enabled
 * report errors; errors will also be
 * reported when the usb peripheral is disconnected.
 */
static inline int usb_ep_queue(struct usb_ep *ep,
			       struct usb_request *req, gfp_t gfp_flags)
{
	return ep->ops->queue(ep, req, gfp_flags);
}

/**
 * usb_ep_dequeue - dequeues (cancels, unlinks) an I/O request from an endpoint
 * @ep:the endpoint associated with the request
 * @req:the request being canceled
 *
 * if the request is still active on the endpoint, it is dequeued and its
 * completion routine is called (with status -ECONNRESET); else a negative
 * error code is returned.
 *
 * note that some hardware can't clear out write fifos (to unlink the request
 * at the head of the queue) except as part of disconnecting from usb.  such
 * restrictions prevent drivers from supporting configuration changes,
 * even to configuration zero (a "chapter 9" requirement).
 */
static inline int usb_ep_dequeue(struct usb_ep *ep, struct usb_request *req)
{
	return ep->ops->dequeue(ep, req);
}

/**
 * usb_ep_set_halt - sets the endpoint halt feature.
 * @ep: the non-isochronous endpoint being stalled
 *
 * Use this to stall an endpoint, perhaps as an error report.
 * Except for control endpoints,
 * the endpoint stays halted (will not stream any data) until the host
 * clears this feature; drivers may need to empty the endpoint's request
 * queue first, to make sure no inappropriate transfers happen.
 *
 * Note that while an endpoint CLEAR_FEATURE will be invisible to the
 * gadget driver, a SET_INTERFACE will not be.  To reset endpoints for the
 * current altsetting, see usb_ep_clear_halt().  When switching altsettings,
 * it's simplest to use usb_ep_enable() or usb_ep_disable() for the endpoints.
 *
 * Returns zero, or a negative error code.  On success, this call sets
 * underlying hardware state that blocks data transfers.
 * Attempts to halt IN endpoints will fail (returning -EAGAIN) if any
 * transfer requests are still queued, or if the controller hardware
 * (usually a FIFO) still holds bytes that the host hasn't collected.
 */
static inline int usb_ep_set_halt(struct usb_ep *ep)
{
	return ep->ops->set_halt(ep, 1);
}

/**
 * usb_ep_clear_halt - clears endpoint halt, and resets toggle
 * @ep:the bulk or interrupt endpoint being reset
 *
 * Use this when responding to the standard usb "set interface" request,
 * for endpoints that aren't reconfigured, after clearing any other state
 * in the endpoint's i/o queue.
 *
 * Returns zero, or a negative error code.  On success, this call clears
 * the underlying hardware state reflecting endpoint halt and data toggle.
 * Note that some hardware can't support this request (like pxa2xx_udc),
 * and accordingly can't correctly implement interface altsettings.
 */
static inline int usb_ep_clear_halt(struct usb_ep *ep)
{
	return ep->ops->set_halt(ep, 0);
}

/**
 * usb_ep_fifo_status - returns number of bytes in fifo, or error
 * @ep: the endpoint whose fifo status is being checked.
 *
 * FIFO endpoints may have "unclaimed data" in them in certain cases,
 * such as after aborted transfers.  Hosts may not have collected all
 * the IN data written by the gadget driver (and reported by a request
 * completion).  The gadget driver may not have collected all the data
 * written OUT to it by the host.  Drivers that need precise handling for
 * fault reporting or recovery may need to use this call.
 *
 * This returns the number of such bytes in the fifo, or a negative
 * errno if the endpoint doesn't use a FIFO or doesn't support such
 * precise handling.
 */
static inline int usb_ep_fifo_status(struct usb_ep *ep)
{
	if (ep->ops->fifo_status)
		return ep->ops->fifo_status(ep);
	else
		return -EOPNOTSUPP;
}

/**
 * usb_ep_fifo_flush - flushes contents of a fifo
 * @ep: the endpoint whose fifo is being flushed.
 *
 * This call may be used to flush the "unclaimed data" that may exist in
 * an endpoint fifo after abnormal transaction terminations.  The call
 * must never be used except when endpoint is not being used for any
 * protocol translation.
 */
static inline void usb_ep_fifo_flush(struct usb_ep *ep)
{
	if (ep->ops->fifo_flush)
		ep->ops->fifo_flush(ep);
}

/*-------------------------------------------------------------------------*/

/* utility to simplify managing config descriptors */

/* write vector of descriptors into buffer */
int usb_descriptor_fillbuf(void *, unsigned,
		const struct usb_descriptor_header **);
#endif	/* __LINUX_USB_GADGET_H */

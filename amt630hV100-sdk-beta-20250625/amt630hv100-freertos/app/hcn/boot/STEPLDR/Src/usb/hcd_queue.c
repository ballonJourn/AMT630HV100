/*
 * hcd_queue.c - DesignWare HS OTG Controller host queuing routines
 *
 * Copyright (C) 2004-2013 Synopsys, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file contains the functions to manage Queue Heads and Queue
 * Transfer Descriptors for Host mode
 */
#include "usb_os_adapter.h"
#include "trace.h"
#include <asm/dma-mapping.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "core.h"
#include "hcd.h"

/* Wait this long before releasing periodic reservation */
#define DWC2_UNRESERVE_DELAY (5)

#define swap(a, b) do {unsigned long tmp = (a); (a) = (b); (b) = tmp;} while(0)
unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r = a | b;

	if (!a || !b)
		return r;

	/* Isolate lsbit of r */
	r &= -r;

	while (!(b & r))
		b >>= 1;
	if (b == r)
		return r;

	for (;;) {
		while (!(a & r))
			a >>= 1;
		if (a == r)
			return r;
		if (a == b)
			return a;

		if (a < b)
			swap(a, b);
		a -= b;
		a >>= 1;
		if (a & r)
			a += b;
		a >>= 1;
	}
}

/**
 * dwc2_do_unreserve() - Actually release the periodic reservation
 *
 * This function actually releases the periodic bandwidth that was reserved
 * by the given qh.
 *
 * @hsotg: The HCD state structure for the DWC OTG controller
 * @qh:    QH for the periodic transfer.
 */
static void dwc2_do_unreserve(struct dwc2_hsotg *hsotg, struct dwc2_qh *qh)
{
	assert_spin_locked(&hsotg->lock);

	WARN_ON(!qh->unreserve_pending);

	/* No more unreserve pending--we're doing it */
	qh->unreserve_pending = false;
    if (!list_item_empty(&qh->qh_list_entry)) {
        WARN_ON(!list_item_empty(&qh->qh_list_entry));
        list_del_init(&qh->qh_list_entry);
    }
}

/**
 * dwc2_unreserve_timer_fn() - Timer function to release periodic reservation
 *
 * According to the kernel doc for usb_submit_urb() (specifically the part about
 * "Reserved Bandwidth Transfers"), we need to keep a reservation active as
 * long as a device driver keeps submitting.  Since we're using HCD_BH to give
 * back the URB we need to give the driver a little bit of time before we
 * release the reservation.  This worker is called after the appropriate
 * delay.
 *
 * @work: Pointer to a qh unreserve_work.
 */
/* static void dwc2_unreserve_timer_fn(unsigned long data)
{
} */


/**
 * dwc2_qh_init() - Initializes a QH structure
 *
 * @hsotg: The HCD state structure for the DWC OTG controller
 * @qh:    The QH to init
 * @urb:   Holds the information about the device/endpoint needed to initialize
 *         the QH
 * @mem_flags: Flags for allocating memory.
 */
static void dwc2_qh_init(struct dwc2_hsotg *hsotg, struct dwc2_qh *qh,
			 struct dwc2_hcd_urb *urb, gfp_t mem_flags)
{
	int dev_speed = dwc2_host_get_speed(hsotg, urb->priv);
	u8 ep_type = dwc2_hcd_get_pipe_type(&urb->pipe_info);
	bool ep_is_in = !!dwc2_hcd_is_pipe_in(&urb->pipe_info);
	u32 hprt = dwc2_readl(hsotg->regs + HPRT0);
	u32 prtspd = (hprt & HPRT0_SPD_MASK) >> HPRT0_SPD_SHIFT;
	bool do_split = (prtspd == HPRT0_SPD_HIGH_SPEED &&
			 dev_speed != USB_SPEED_HIGH);
	int maxp = dwc2_hcd_get_mps(&urb->pipe_info);
	int bytecount = dwc2_hb_mult(maxp) * dwc2_max_packet(maxp);
	char *speed, *type;

	/* Initialize QH */
	qh->hsotg = hsotg;
	qh->ep_type = ep_type;
	qh->ep_is_in = ep_is_in;

	qh->data_toggle = DWC2_HC_PID_DATA0;
	qh->maxp = maxp;
	INIT_LIST_HEAD(&qh->qtd_list);
	INIT_LIST_ITEM(&qh->qh_list_entry);
	qh->qh_list_entry.pvOwner = (void *)qh;
	qh->do_split = do_split;
	qh->dev_speed = dev_speed;

	switch (dev_speed) {
	case USB_SPEED_LOW:
		speed = "low";
		break;
	case USB_SPEED_FULL:
		speed = "full";
		break;
	case USB_SPEED_HIGH:
		speed = "high";
		break;
	default:
		speed = "?";
		break;
	}

	switch (qh->ep_type) {
	case USB_ENDPOINT_XFER_CONTROL:
		type = "control";
		break;
	case USB_ENDPOINT_XFER_BULK:
		type = "bulk";
		break;
	default:
		type = "?";
		break;
	}
	
	USB_UNUSED(bytecount);
	USB_UNUSED(speed);
	USB_UNUSED(type);
	dwc2_sch_dbg(hsotg, "QH=%p Init %s, %s speed, %d bytes:\n", qh, type,
		     speed, bytecount);
	dwc2_sch_dbg(hsotg, "QH=%p ...addr=%d, ep=%d, %s\n", qh,
		     dwc2_hcd_get_dev_addr(&urb->pipe_info),
		     dwc2_hcd_get_ep_num(&urb->pipe_info),
		     ep_is_in ? "IN" : "OUT");
}

/**
 * dwc2_hcd_qh_create() - Allocates and initializes a QH
 *
 * @hsotg:        The HCD state structure for the DWC OTG controller
 * @urb:          Holds the information about the device/endpoint needed
 *                to initialize the QH
 * @atomic_alloc: Flag to do atomic allocation if needed
 *
 * Return: Pointer to the newly allocated QH, or NULL on error
 */
struct dwc2_qh *dwc2_hcd_qh_create(struct dwc2_hsotg *hsotg,
				   struct dwc2_hcd_urb *urb,
					  gfp_t mem_flags)
{
	struct dwc2_qh *qh;

	if (!urb->priv)
		return NULL;

	/* Allocate memory */
	qh = (struct dwc2_qh *)kzalloc(sizeof(*qh), mem_flags);
	if (!qh)
		return NULL;

	dwc2_qh_init(hsotg, qh, urb, mem_flags);

	/* if (hsotg->params.dma_desc_enable &&
	    dwc2_hcd_qh_init_ddma(hsotg, qh, mem_flags) < 0) {
		dwc2_hcd_qh_free(hsotg, qh);
 		return NULL;
 	} */

	return qh;
}

/**
 * dwc2_hcd_qh_free() - Frees the QH
 *
 * @hsotg: HCD instance
 * @qh:    The QH to free
 *
 * QH should already be removed from the list. QTD list should already be empty
 * if called from URB Dequeue.
 *
 * Must NOT be called with interrupt disabled or spinlock held
 */
void dwc2_hcd_qh_free(struct dwc2_hsotg *hsotg, struct dwc2_qh *qh)
{
	/* Make sure any unreserve work is finished. */
	if (0) {
		unsigned long flags;

		spin_lock_irqsave(&hsotg->lock, flags);
		dwc2_do_unreserve(hsotg, qh);
		spin_unlock_irqrestore(&hsotg->lock, flags);
	}
	dwc2_host_put_tt_info(hsotg, qh->dwc_tt);

	kfree(qh);
}

/**
 * dwc2_hcd_qh_add() - Adds a QH to either the non periodic or periodic
 * schedule if it is not already in the schedule. If the QH is already in
 * the schedule, no action is taken.
 *
 * @hsotg: The HCD state structure for the DWC OTG controller
 * @qh:    The QH to add
 *
 * Return: 0 if successful, negative error code otherwise
 */
int dwc2_hcd_qh_add(struct dwc2_hsotg *hsotg, struct dwc2_qh *qh)
{
	u32 intr_mask;

	if (dbg_qh(qh))
		dev_vdbg(hsotg->dev, "%s()\n", __func__);

    if (!list_item_empty(&qh->qh_list_entry))
		/* QH already in a schedule */
		return 0;

	/* Add the new QH to the appropriate schedule */
	if (dwc2_qh_is_non_per(qh)) {
		/* Schedule right away */
		qh->start_active_frame = hsotg->frame_number;
		qh->next_active_frame = qh->start_active_frame;

		/* Always start in inactive schedule */
		list_add_tail(&qh->qh_list_entry,
			      &hsotg->non_periodic_sched_inactive);
		return 0;
	}

	intr_mask = dwc2_readl(hsotg->regs + GINTMSK);
	intr_mask |= GINTSTS_SOF;
	dwc2_writel(intr_mask, hsotg->regs + GINTMSK);

	return 0;
}

/**
 * dwc2_hcd_qh_unlink() - Removes a QH from either the non-periodic or periodic
 * schedule. Memory is not freed.
 *
 * @hsotg: The HCD state structure
 * @qh:    QH to remove from schedule
 */
void dwc2_hcd_qh_unlink(struct dwc2_hsotg *hsotg, struct dwc2_qh *qh)
{
	u32 intr_mask;

	dev_vdbg(hsotg->dev, "%s()\n", __func__);

    if (list_item_empty(&qh->qh_list_entry))
		/* QH is not in a schedule */
		return;

	if (dwc2_qh_is_non_per(qh)) {
		if (hsotg->non_periodic_qh_ptr == &qh->qh_list_entry) {
			hsotg->non_periodic_qh_ptr = listGET_NEXT(hsotg->non_periodic_qh_ptr);
		}
		list_del_init(&qh->qh_list_entry);
		return;
	}

	intr_mask = dwc2_readl(hsotg->regs + GINTMSK);
	intr_mask &= ~GINTSTS_SOF;
	dwc2_writel(intr_mask, hsotg->regs + GINTMSK);
}

/*
 * Deactivates a QH. For non-periodic QHs, removes the QH from the active
 * non-periodic schedule. The QH is added to the inactive non-periodic
 * schedule if any QTDs are still attached to the QH.
 *
 * For periodic QHs, the QH is removed from the periodic queued schedule. If
 * there are any QTDs still attached to the QH, the QH is added to either the
 * periodic inactive schedule or the periodic ready schedule and its next
 * scheduled frame is calculated. The QH is placed in the ready schedule if
 * the scheduled frame has been reached already. Otherwise it's placed in the
 * inactive schedule. If there are no QTDs attached to the QH, the QH is
 * completely removed from the periodic schedule.
 */
void dwc2_hcd_qh_deactivate(struct dwc2_hsotg *hsotg, struct dwc2_qh *qh,
			    int sched_next_periodic_split)
{
	u16 old_frame = qh->next_active_frame;
	
	dev_vdbg(hsotg->dev, "%s() ep_type:%d\n", __func__, qh->ep_type);
	
	USB_UNUSED(old_frame);

	if (dbg_qh(qh))
		dev_vdbg(hsotg->dev, "%s()\n", __func__);

	if (dwc2_qh_is_non_per(qh)) {
		dwc2_hcd_qh_unlink(hsotg, qh);
		if (!list_empty(&qh->qtd_list))
			/* Add back to inactive non-periodic schedule */
			dwc2_hcd_qh_add(hsotg, qh);
		return;
	}
}

/**
 * dwc2_hcd_qtd_init() - Initializes a QTD structure
 *
 * @qtd: The QTD to initialize
 * @urb: The associated URB
 */
void dwc2_hcd_qtd_init(struct dwc2_qtd *qtd, struct dwc2_hcd_urb *urb)
{
	qtd->urb = urb;
	if (dwc2_hcd_get_pipe_type(&urb->pipe_info) ==
			USB_ENDPOINT_XFER_CONTROL) {
		/*
		 * The only time the QTD data toggle is used is on the data
		 * phase of control transfers. This phase always starts with
		 * DATA1.
		 */
		qtd->data_toggle = DWC2_HC_PID_DATA1;
		qtd->control_phase = DWC2_CONTROL_SETUP;
	}

	/* Start split */
	qtd->complete_split = 0;
	qtd->isoc_split_pos = DWC2_HCSPLT_XACTPOS_ALL;
	qtd->in_process = 0;

	/* Store the qtd ptr in the urb to reference the QTD */
	urb->qtd = qtd;
}

/**
 * dwc2_hcd_qtd_add() - Adds a QTD to the QTD-list of a QH
 *			Caller must hold driver lock.
 *
 * @hsotg:        The DWC HCD structure
 * @qtd:          The QTD to add
 * @qh:           Queue head to add qtd to
 *
 * Return: 0 if successful, negative error code otherwise
 *
 * If the QH to which the QTD is added is not currently scheduled, it is placed
 * into the proper schedule based on its EP type.
 */
int dwc2_hcd_qtd_add(struct dwc2_hsotg *hsotg, struct dwc2_qtd *qtd,
		     struct dwc2_qh *qh)
{
	int retval;

	if (unlikely(!qh)) {
		dev_err(hsotg->dev, "%s: Invalid QH\n", __func__);
		retval = -EINVAL;
		goto fail;
	}

	retval = dwc2_hcd_qh_add(hsotg, qh);
	if (retval)
		goto fail;

	qtd->qh = qh;
	list_add_tail(&qtd->qtd_list_entry, &qh->qtd_list);

	return 0;
fail:
	return retval;
}

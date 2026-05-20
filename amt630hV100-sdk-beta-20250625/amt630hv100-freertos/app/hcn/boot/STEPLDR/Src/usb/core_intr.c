/*
 * core_intr.c - DesignWare HS OTG Controller common interrupt handling
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
 * This file contains the common interrupt handlers
 */

#include "usb_os_adapter.h"
#include "trace.h"
#include <asm/dma-mapping.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "core.h"
#include "hcd.h"

const char *dwc2_op_state_str(struct dwc2_hsotg *hsotg)
{
	switch (hsotg->op_state) {
	case OTG_STATE_A_HOST:
		return "a_host";
	case OTG_STATE_A_SUSPEND:
		return "a_suspend";
	case OTG_STATE_A_PERIPHERAL:
		return "a_peripheral";
	case OTG_STATE_B_PERIPHERAL:
		return "b_peripheral";
	case OTG_STATE_B_HOST:
		return "b_host";
	default:
		return "unknown";
	}
}

/**
 * dwc2_handle_mode_mismatch_intr() - Logs a mode mismatch warning message
 *
 * @hsotg: Programming view of DWC_otg controller
 */
static void dwc2_handle_mode_mismatch_intr(struct dwc2_hsotg *hsotg)
{
	/* Clear interrupt */
	dwc2_writel(GINTSTS_MODEMIS, hsotg->regs + GINTSTS);

	dev_warn(hsotg->dev, "Mode Mismatch Interrupt: currently in %s mode\n",
		 dwc2_is_host_mode(hsotg) ? "Host" : "Device");
}

/**
 * dwc2_handle_otg_intr() - Handles the OTG Interrupts. It reads the OTG
 * Interrupt Register (GOTGINT) to determine what interrupt has occurred.
 *
 * @hsotg: Programming view of DWC_otg controller
 */
static void dwc2_handle_otg_intr(struct dwc2_hsotg *hsotg)
{
	u32 gotgint;

	gotgint = dwc2_readl(hsotg->regs + GOTGINT);

	/* Clear GOTGINT */
	dwc2_writel(gotgint, hsotg->regs + GOTGINT);
}

/**
 * dwc2_handle_conn_id_status_change_intr() - Handles the Connector ID Status
 * Change Interrupt
 *
 * @hsotg: Programming view of DWC_otg controller
 *
 * Reads the OTG Interrupt Register (GOTCTL) to determine whether this is a
 * Device to Host Mode transition or a Host to Device Mode transition. This only
 * occurs when the cable is connected/removed from the PHY connector.
 */
static void dwc2_handle_conn_id_status_change_intr(struct dwc2_hsotg *hsotg)
{
	u32 gintmsk;

	/* Clear interrupt */
	dwc2_writel(GINTSTS_CONIDSTSCHNG, hsotg->regs + GINTSTS);

	/* Need to disable SOF interrupt immediately */
	gintmsk = dwc2_readl(hsotg->regs + GINTMSK);
	gintmsk &= ~GINTSTS_SOF;
	dwc2_writel(gintmsk, hsotg->regs + GINTMSK);

	dev_dbg(hsotg->dev, " ++Connector ID Status Change Interrupt++  (%s)\n",
		dwc2_is_host_mode(hsotg) ? "Host" : "Device");
}

/**
 * dwc2_handle_session_req_intr() - This interrupt indicates that a device is
 * initiating the Session Request Protocol to request the host to turn on bus
 * power so a new session can begin
 *
 * @hsotg: Programming view of DWC_otg controller
 *
 * This handler responds by turning on bus power. If the DWC_otg controller is
 * in low power mode, this handler brings the controller out of low power mode
 * before turning on bus power.
 */
static void dwc2_handle_session_req_intr(struct dwc2_hsotg *hsotg)
{
	/* Clear interrupt */
	dwc2_writel(GINTSTS_SESSREQINT, hsotg->regs + GINTSTS);

	dev_dbg(hsotg->dev, "Session request interrupt - lx_state=%d\n",
		hsotg->lx_state);
}

/*
 * This interrupt indicates that a device has been disconnected from the
 * root port
 */
static void dwc2_handle_disconnect_intr(struct dwc2_hsotg *hsotg)
{
	dwc2_writel(GINTSTS_DISCONNINT, hsotg->regs + GINTSTS);

	dev_dbg(hsotg->dev, "++Disconnect Detected Interrupt++ (%s) %s\n",
		dwc2_is_host_mode(hsotg) ? "Host" : "Device",
		dwc2_op_state_str(hsotg));

	if (hsotg->op_state == OTG_STATE_A_HOST)
		dwc2_hcd_disconnect(hsotg, false);
}

/*
 * This interrupt indicates that SUSPEND state has been detected on the USB.
 *
 * For HNP the USB Suspend interrupt signals the change from "a_peripheral"
 * to "a_host".
 *
 * When power management is enabled the core will be put in low power mode.
 */
static void dwc2_handle_usb_suspend_intr(struct dwc2_hsotg *hsotg)
{
	/* Clear interrupt */
	dwc2_writel(GINTSTS_USBSUSP, hsotg->regs + GINTSTS);

	dev_dbg(hsotg->dev, "USB SUSPEND\n");

	if (hsotg->op_state == OTG_STATE_A_PERIPHERAL) {
		dev_dbg(hsotg->dev, "a_peripheral->a_host\n");

		/* Change to L2 (suspend) state */
		hsotg->lx_state = DWC2_L2;
		/* Clear the a_peripheral flag, back to a_host */
		spin_unlock(&hsotg->lock);
		dwc2_hcd_start_isr(hsotg);
		spin_lock(&hsotg->lock);
		hsotg->op_state = OTG_STATE_A_HOST;
	}
}

#define GINTMSK_COMMON	(GINTSTS_WKUPINT | GINTSTS_SESSREQINT |		\
			 GINTSTS_CONIDSTSCHNG | GINTSTS_OTGINT |	\
			 GINTSTS_MODEMIS | GINTSTS_DISCONNINT |		\
			 GINTSTS_USBSUSP | GINTSTS_PRTINT)

/*
 * This function returns the Core Interrupt register
 */
static u32 dwc2_read_common_intr(struct dwc2_hsotg *hsotg)
{
	u32 gintsts;
	u32 gintmsk;
	u32 gahbcfg;
	u32 gintmsk_common = GINTMSK_COMMON;

	gintsts = dwc2_readl(hsotg->regs + GINTSTS);
	gintmsk = dwc2_readl(hsotg->regs + GINTMSK);
	gahbcfg = dwc2_readl(hsotg->regs + GAHBCFG);

	/* If any common interrupts set */
	if (gintsts & gintmsk_common)
		dev_dbg(hsotg->dev, "gintsts=%08x  gintmsk=%08x\n",
			gintsts, gintmsk);

	if (gahbcfg & GAHBCFG_GLBL_INTR_EN)
		return gintsts & gintmsk & gintmsk_common;
	else
		return 0;
}

/*
 * Common interrupt handler
 *
 * The common interrupts are those that occur in both Host and Device mode.
 * This handler handles the following interrupts:
 * - Mode Mismatch Interrupt
 * - OTG Interrupt
 * - Connector ID Status Change Interrupt
 * - Disconnect Interrupt
 * - Session Request Interrupt
 * - Resume / Remote Wakeup Detected Interrupt
 * - Suspend Interrupt
 */
irqreturn_t dwc2_handle_common_intr(int irq, void *dev)
{
	struct dwc2_hsotg *hsotg = dev;
	u32 gintsts;
	irqreturn_t retval = IRQ_NONE;

	if (!dwc2_is_controller_alive(hsotg)) {
		dev_warn(hsotg->dev, "Controller is dead\n");
		goto out;
	}

	gintsts = dwc2_read_common_intr(hsotg);
	if (gintsts & ~GINTSTS_PRTINT)
		retval = IRQ_HANDLED;

	if (gintsts & GINTSTS_MODEMIS)
		dwc2_handle_mode_mismatch_intr(hsotg);
	if (gintsts & GINTSTS_OTGINT)
		dwc2_handle_otg_intr(hsotg);
	if (gintsts & GINTSTS_CONIDSTSCHNG)
		dwc2_handle_conn_id_status_change_intr(hsotg);
	if (gintsts & GINTSTS_DISCONNINT)
		dwc2_handle_disconnect_intr(hsotg);
	if (gintsts & GINTSTS_SESSREQINT)
		dwc2_handle_session_req_intr(hsotg);

	if (gintsts & GINTSTS_USBSUSP)
		dwc2_handle_usb_suspend_intr(hsotg);

	if (gintsts & GINTSTS_PRTINT) {
		/*
		 * The port interrupt occurs while in device mode with HPRT0
		 * Port Enable/Disable
		 */
	}

out:
	return retval;
}

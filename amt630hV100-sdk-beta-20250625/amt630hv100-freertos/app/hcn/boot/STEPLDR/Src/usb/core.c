/*
 * core.c - DesignWare HS OTG Controller common routines
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
 * The Core code provides basic services for accessing and managing the
 * DWC_otg hardware. These services are used by both the Host Controller
 * Driver and the Peripheral Controller Driver.
 */
#include "usb_os_adapter.h"
#include "trace.h"
#include <asm/dma-mapping.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "core.h"
#include "hcd.h"

/**
 * dwc2_wait_for_mode() - Waits for the controller mode.
 * @hsotg:	Programming view of the DWC_otg controller.
 * @host_mode:	If true, waits for host mode, otherwise device mode.
 */
static void dwc2_wait_for_mode(struct dwc2_hsotg *hsotg,
			       bool host_mode)
{
	uint32_t tick = get_timer(0);
	unsigned int timeout = 110;//ms

	dev_vdbg(hsotg->dev, "Waiting for %s mode\n",
		 host_mode ? "host" : "device");

	while (1) {
		__s64 ms;

		if (dwc2_is_host_mode(hsotg) == host_mode) {
			dev_vdbg(hsotg->dev, "%s mode set\n",
				 host_mode ? "Host" : "Device");
			break;
		}

		ms = get_timer(tick);
		if (ms >= (__s64)timeout) {
			dev_warn(hsotg->dev, "%s: Couldn't set %s mode\n",
				 __func__, host_mode ? "host" : "device");
			break;
		}
		//vTaskDelay(2000 / portTICK_RATE_MS);
		mdelay(10);
	}
}

/*
 * Do core a soft reset of the core.  Be careful with this because it
 * resets all the internal state machines of the core.
 */
int dwc2_core_reset(struct dwc2_hsotg *hsotg, bool skip_wait)
{
	u32 greset;
	int count = 0;
	bool wait_for_host_mode = false;

	dev_vdbg(hsotg->dev, "%s()\n", __func__);

	/*
	 * If the current mode is host, either due to the force mode
	 * bit being set (which persists after core reset) or the
	 * connector id pin, a core soft reset will temporarily reset
	 * the mode to device. A delay from the IDDIG debounce filter
	 * will occur before going back to host mode.
	 *
	 * Determine whether we will go back into host mode after a
	 * reset and account for this delay after the reset.
	 */

	/* Core Soft Reset */
	greset = dwc2_readl(hsotg->regs + GRSTCTL);
	greset |= GRSTCTL_CSFTRST;
	dwc2_writel(greset, hsotg->regs + GRSTCTL);
	do {
		udelay(1);
		greset = dwc2_readl(hsotg->regs + GRSTCTL);
		if (++count > 50) {
			dev_warn(hsotg->dev,
				 "%s() HANG! Soft Reset GRSTCTL=%0x\n",
				 __func__, greset);
			return -EBUSY;
		}
	} while (greset & GRSTCTL_CSFTRST);

	/* Wait for AHB master IDLE state */
	count = 0;
	do {
		udelay(1);
		greset = dwc2_readl(hsotg->regs + GRSTCTL);
		if (++count > 50) {
			dev_warn(hsotg->dev,
				 "%s() HANG! AHB Idle GRSTCTL=%0x\n",
				 __func__, greset);
			return -EBUSY;
		}
	} while (!(greset & GRSTCTL_AHBIDLE));

	if (wait_for_host_mode && !skip_wait)
		dwc2_wait_for_mode(hsotg, true);

	return 0;
}

/*
 * Sets or clears force mode based on the dr_mode parameter.
 */
void dwc2_force_dr_mode(struct dwc2_hsotg *hsotg)
{
	switch (hsotg->dr_mode) {
	case USB_DR_MODE_HOST:
		/*
		 * NOTE: This is required for some rockchip soc based
		 * platforms on their host-only dwc2.
		 */
		msleep(50);

		break;
	default:
		dev_warn(hsotg->dev, "%s() Invalid dr_mode=%d\n",
			 __func__, hsotg->dr_mode);
		break;
	}
}

/*
 * Do core a soft reset of the core.  Be careful with this because it
 * resets all the internal state machines of the core.
 *
 * Additionally this will apply force mode as per the hsotg->dr_mode
 * parameter.
 */
int dwc2_core_reset_and_force_dr_mode(struct dwc2_hsotg *hsotg)
{
	int retval;

	retval = dwc2_core_reset(hsotg, false);
	if (retval)
		return retval;

	dwc2_force_dr_mode(hsotg);
	return 0;
}

/**
 * dwc2_flush_tx_fifo() - Flushes a Tx FIFO
 *
 * @hsotg: Programming view of DWC_otg controller
 * @num:   Tx FIFO to flush
 */
void dwc2_flush_tx_fifo(struct dwc2_hsotg *hsotg, const int num)
{
	u32 greset;
	int count = 0;

	dev_vdbg(hsotg->dev, "Flush Tx FIFO %d\n", num);

	greset = GRSTCTL_TXFFLSH;
	greset |= num << GRSTCTL_TXFNUM_SHIFT & GRSTCTL_TXFNUM_MASK;
	dwc2_writel(greset, hsotg->regs + GRSTCTL);

	do {
		greset = dwc2_readl(hsotg->regs + GRSTCTL);
		if (++count > 10000) {
			dev_warn(hsotg->dev,
				 "%s() HANG! GRSTCTL=%0x GNPTXSTS=0x%08x\n",
				 __func__, greset,
				 dwc2_readl(hsotg->regs + GNPTXSTS));
			break;
		}
		udelay(1);
	} while (greset & GRSTCTL_TXFFLSH);

	/* Wait for at least 3 PHY Clocks */
	udelay(1);
}

/**
 * dwc2_flush_rx_fifo() - Flushes the Rx FIFO
 *
 * @hsotg: Programming view of DWC_otg controller
 */
void dwc2_flush_rx_fifo(struct dwc2_hsotg *hsotg)
{
	u32 greset;
	int count = 0;

	dev_vdbg(hsotg->dev, "%s()\n", __func__);

	greset = GRSTCTL_RXFFLSH;
	dwc2_writel(greset, hsotg->regs + GRSTCTL);

	do {
		greset = dwc2_readl(hsotg->regs + GRSTCTL);
		if (++count > 10000) {
			dev_warn(hsotg->dev, "%s() HANG! GRSTCTL=%0x\n",
				 __func__, greset);
			break;
		}
		udelay(1);
	} while (greset & GRSTCTL_RXFFLSH);

	/* Wait for at least 3 PHY Clocks */
	udelay(1);
}

bool dwc2_is_controller_alive(struct dwc2_hsotg *hsotg)
{
	if (dwc2_readl(hsotg->regs + GSNPSID) == 0xffffffff)
		return false;
	else
		return true;
}

/**
 * dwc2_enable_global_interrupts() - Enables the controller's Global
 * Interrupt in the AHB Config register
 *
 * @hsotg: Programming view of DWC_otg controller
 */
void dwc2_enable_global_interrupts(struct dwc2_hsotg *hsotg)
{
	u32 ahbcfg = dwc2_readl(hsotg->regs + GAHBCFG);

	ahbcfg |= GAHBCFG_GLBL_INTR_EN;
	dwc2_writel(ahbcfg, hsotg->regs + GAHBCFG);
}

/**
 * dwc2_disable_global_interrupts() - Disables the controller's Global
 * Interrupt in the AHB Config register
 *
 * @hsotg: Programming view of DWC_otg controller
 */
void dwc2_disable_global_interrupts(struct dwc2_hsotg *hsotg)
{
	u32 ahbcfg = dwc2_readl(hsotg->regs + GAHBCFG);

	ahbcfg &= ~GAHBCFG_GLBL_INTR_EN;
	dwc2_writel(ahbcfg, hsotg->regs + GAHBCFG);
}

/* Returns the controller's GHWCFG2.OTG_MODE. */
unsigned int dwc2_op_mode(struct dwc2_hsotg *hsotg)
{
	u32 ghwcfg2 = dwc2_readl(hsotg->regs + GHWCFG2);

	return (ghwcfg2 & GHWCFG2_OP_MODE_MASK) >>
		GHWCFG2_OP_MODE_SHIFT;
}

/* Returns true if the controller is host-only. */
bool dwc2_hw_is_host(struct dwc2_hsotg *hsotg)
{
	unsigned int op_mode = dwc2_op_mode(hsotg);

	return (op_mode == GHWCFG2_OP_MODE_SRP_CAPABLE_HOST) ||
		(op_mode == GHWCFG2_OP_MODE_NO_SRP_CAPABLE_HOST);
}
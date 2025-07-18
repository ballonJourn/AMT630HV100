/*
 * platform.c - DesignWare HS OTG Controller platform driver
 *
 * Copyright (C) Matthijs Kooijman <matthijs@stdin.nl>
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
#include "usb_os_adapter.h"
#include "trace.h"
#include <asm/dma-mapping.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "core.h"
#include "hcd.h"
#include "chip.h"
#include "board.h"

int dwc2_lowlevel_hw_disable(struct dwc2_hsotg *hsotg)
{
	int ret = 0;

	if (ret == 0)
		hsotg->ll_hw_enabled = false;
	return ret;
}

int dwc2_lowlevel_hw_enable(struct dwc2_hsotg *hsotg)
{
	int ret = 0;

	if (ret == 0)
		hsotg->ll_hw_enabled = true;
	return ret;
}


static int dwc2_get_dr_mode(struct dwc2_hsotg *hsotg)
{
	if (get_usb_mode()) {
		hsotg->dr_mode = USB_DR_MODE_PERIPHERAL;//usb_get_dr_mode(hsotg->dev);
		if (hsotg->dr_mode == USB_DR_MODE_UNKNOWN)
			hsotg->dr_mode = USB_DR_MODE_OTG;
	} else {
        enum usb_dr_mode mode;
		hsotg->dr_mode = USB_DR_MODE_HOST;//usb_get_dr_mode(hsotg->dev);
		if (hsotg->dr_mode == USB_DR_MODE_UNKNOWN)
			hsotg->dr_mode = USB_DR_MODE_OTG;

		mode = hsotg->dr_mode;

		if (dwc2_hw_is_device(hsotg)) {
			if (IS_ENABLED(CONFIG_USB_DWC2_HOST)) {
				dev_err(hsotg->dev,
					"Controller does not support host mode.\n");
				return -EINVAL;
			}
			mode = USB_DR_MODE_PERIPHERAL;
		} else if (dwc2_hw_is_host(hsotg)) {
			if (IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL)) {
				dev_err(hsotg->dev,
					"Controller does not support device mode.\n");
				return -EINVAL;
			}
			mode = USB_DR_MODE_HOST;
		} else {
			if (IS_ENABLED(CONFIG_USB_DWC2_HOST))
				mode = USB_DR_MODE_HOST;
			else if (IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL))
				mode = USB_DR_MODE_PERIPHERAL;
		}

		if (mode != hsotg->dr_mode) {
			dev_warn(hsotg->dev,
				 "Configuration mismatch. dr_mode forced to %s\n",
				mode == USB_DR_MODE_HOST ? "host" : "device");

			hsotg->dr_mode = mode;
		}
	}
	return 0;
}

//static QueueHandle_t 	usb_irq_queue;
//static TaskHandle_t	usb_irq_task;
//static char first_irq = 1;
irqreturn_t dwc2_hsotg_irq(int irq, void *pw);
static void ark_usb_interrupt(void *param)
{
	struct dwc2_hsotg *hsotg = (struct dwc2_hsotg *)param;
    //printf("ark_usb_interrupt\r\n");
	dwc2_handle_common_intr(hsotg->irq, param);
	dwc2_hcd_irq(hsotg);
       dwc2_hsotg_irq(hsotg->irq, param);
	/*if (first_irq) {
		dwc2_hcd_irq(hsotg);
		first_irq = 0;
	} else {
		portDISABLE_INTERRUPTS();
		xQueueSendFromISR(usb_irq_queue, NULL, 0);
	}*/
}

#if 0
static void usb_irq_task_proc(void *pvParameters)
{
	struct dwc2_hsotg *hsotg = (struct dwc2_hsotg *)pvParameters;
	int ret = -1;

	while(1) {
		ret = xQueueReceive(usb_irq_queue, NULL, portMAX_DELAY);

		if (pdFALSE == ret) {
			continue;
		}
		//portENTER_CRITICAL();

		dwc2_hcd_irq(hsotg);
		portENABLE_INTERRUPTS();
		//portEXIT_CRITICAL();
	}
}
#endif

int dwc2_driver_init(struct dwc2_hsotg **dev, struct usb_hcd *hcd)
{
	struct dwc2_hsotg *hsotg;
	int retval = -1;
	u32 size = sizeof(struct device);

	hsotg = devm_kzalloc(NULL, sizeof(*hsotg), GFP_KERNEL);
	if (!hsotg)
		return -ENOMEM;

	hsotg->dev = (struct device *)kzalloc(size, (__GFP_ZERO | GFP_KERNEL));
	if (NULL == hsotg->dev)
		goto error;

	hsotg->phyif = GUSBCFG_PHYIF16;
	hsotg->regs = REGS_USB_BASE;

	spin_lock_init(&(hsotg->lock));

	hsotg->irq = USB_IRQn;
	retval = request_irq(hsotg->irq, 0, ark_usb_interrupt, (void*)hsotg);
	if (retval)
		goto error;

	retval = dwc2_get_dr_mode(hsotg);
	if (retval)
		goto error;

	/*
	 * Reset before dwc2_get_hwparams() then it could get power-on real
	 * reset value form registers.
	 */
	dwc2_core_reset_and_force_dr_mode(hsotg);

	/* Detect config values from hardware */
	retval = dwc2_get_hwparams(hsotg);
	if (retval)
		goto error;

	dwc2_force_dr_mode(hsotg);

	retval = dwc2_init_params(hsotg);
	if (retval)
		goto error;

	if (hsotg->dr_mode != USB_DR_MODE_HOST) {
		retval = dwc2_gadget_init(hsotg, hsotg->irq);
		if (retval)
			goto error;
		hsotg->gadget_enabled = 1;
	}

	if (hsotg->dr_mode != USB_DR_MODE_PERIPHERAL) {
		//usb_irq_queue = xQueueCreate(1, 0);
		//xTaskCreate(usb_irq_task_proc, "usb_irq_proc", configMINIMAL_STACK_SIZE * 5, (void*)dev, configMAX_PRIORITIES, &usb_irq_task);
		retval = dwc2_hcd_init(hsotg, hcd);
		if (retval) {
			if (hsotg->gadget_enabled)
				dwc2_hsotg_remove(hsotg);
			goto error;
		}
		hsotg->hcd_enabled = 1;
	}

	/* Gadget code manages lowlevel hw on its own */
	if (hsotg->dr_mode == USB_DR_MODE_PERIPHERAL)
	    dwc2_lowlevel_hw_disable(hsotg);

#if IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL) || \
	IS_ENABLED(CONFIG_USB_DWC2_DUAL_ROLE)
	/* Postponed adding a new gadget to the udc class driver list */
	if (hsotg->gadget_enabled) {
		retval = usb_add_gadget_udc(hsotg->dev, &hsotg->gadget);
		if (retval) {
			//hsotg->gadget.udc = NULL;
			dwc2_hsotg_remove(hsotg);
			goto error;
		}
	}
#endif

	if (dev) {
		*dev = hsotg;
	}

	return 0;

error:
	if (hsotg && hsotg->dev) {
		kfree(hsotg->dev);
	}
	if (hsotg) {
		kfree(hsotg);
	}
	return retval;
}

int dwc2_driver_uninit(struct dwc2_hsotg *hsotg)
{
	if (NULL == hsotg)
		return 0;
	if (hsotg->hcd_enabled)
		dwc2_hcd_remove(hsotg);
	if (hsotg->gadget_enabled)
		dwc2_hsotg_remove(hsotg);

	if (hsotg->dev)
		kfree(hsotg->dev);

	kfree(hsotg);
	return 0;
}

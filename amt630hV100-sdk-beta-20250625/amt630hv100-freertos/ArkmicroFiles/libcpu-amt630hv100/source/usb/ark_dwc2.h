/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * MUSB OTG driver u-boot specific functions
 *
 * Copyright © 2015 Hans de Goede <hdegoede@redhat.com>
 */
#ifndef __ARK_USB_DWC2_H__
#define __ARK_USB_DWC2_H__

int usb_dwc2_lowlevel_init();
int usb_dwc2_lowlevel_uninit();
void usb_sysctrl_init();
void reset_usb_phy();
void usb_disable_endpoint();
void usb_reset_endpoint();
void usb_dwc2_lowlevel_restart();
int usb_dwc2_reset(int inIrq, int isDev);

#endif

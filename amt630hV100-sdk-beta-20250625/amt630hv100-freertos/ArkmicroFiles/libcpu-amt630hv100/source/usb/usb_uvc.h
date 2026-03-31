#ifndef __USB_UVC_H
#define __USB_UVC_H

int usb_uvc_scan(void);
void usb_uvc_disconnect(void);
int usb_uvc_init(void);
int uvc_start(void);
int uvc_stop(void);
int uvc_set_disp_size(int width, int height);
int uvc_set_disp_pos(int x, int y);
#endif

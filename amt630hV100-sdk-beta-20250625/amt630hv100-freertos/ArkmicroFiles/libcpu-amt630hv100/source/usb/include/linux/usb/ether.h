#ifndef __ETHER_H
#define __ETHER_H
#include "FreeRTOS_IP.h"

//typedef struct NetworkBufferDescriptor_t *(*ncm_wrap)(struct gether *port, NetworkBufferDescriptor_t*  pxBufferDescriptor);
//typedef int (*ncm_unwrap)(struct gether *port, NetworkBufferDescriptor_t*  pxBufferDescriptor, List_t *frames);

struct gether {
	struct usb_function		func;
	/* endpoints handle full and/or high speeds */
	struct usb_ep			*in_ep;
	struct usb_ep			*out_ep;
	const struct usb_endpoint_descriptor *in, *out;

	bool                     connected;

	bool				is_zlp_ok;

	u16				cdc_filter;

	/* hooks for added framing, as needed for RNDIS and EEM. */
	u32				header_len;
	/* NCM requires fixed size bundles */
	bool				is_fixed;
	u32				fixed_out_len;
	u32				fixed_in_len;

	NetworkBufferDescriptor_t *(*wrap)(struct gether *port, NetworkBufferDescriptor_t*  pxBufferDescriptor);
	void *(*wrap_ext)(struct gether *port, void*  bufferDescHandle);
	int (*unwrap)(struct gether *port, uint8_t* data_buf, int len, List_t *frames);
	void (*disconnect_cb)(struct gether *port);

	void				(*open)(struct gether *);
	void				(*close)(struct gether *);

	void*                    ctx;
};

#ifndef ETH_FRAME_LEN
#define ETH_FRAME_LEN             1514
#endif
#ifndef ETH_ALEN
#define ETH_ALEN	6
#endif
#ifndef NET_IP_ALIGN
#define NET_IP_ALIGN	4
#endif

void gether_send(NetworkBufferDescriptor_t * const pxDescriptor);
void gether_send_ext(void * const pxDescriptor);
void gether_disconnect(struct gether *link);
int gether_connect(struct gether *link);
void gether_cleanup(void);
int gether_setup(struct usb_gadget *g, u8 ethaddr[ETH_ALEN]);

#endif

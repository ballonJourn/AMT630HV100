/*
*     ncm
*
*/
#include "usb_os_adapter.h"
#include <stdio.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/usb/ether.h>

#define STRING_MANUFACTURER 25
#define STRING_PRODUCT 2
#define STRING_USBDOWN 2
#define STRING_SERIAL  3
#define MAX_STRING_SERIAL	256
#define CONFIGURATION_NUMBER 1
#undef DEBUG
#define DRIVER_VERSION		"usb_ncm"
#define CONFIG_USB_GADGET_MANUFACTURER "arkmicro"

#if  (DEBUG)
#define debug(s, ...) printf("%s: " s "\n", "UsbNcm", ## __VA_ARGS__)
#else
#define debug(s, ...)
#endif

#define CONFIG_USB_GADGET_VENDOR_NUM 0x5001
#define CONFIG_USB_GADGET_PRODUCT_NUM  0x2003

static const char product[] = "USB ncm gadget";
static char g_ncm_serial[MAX_STRING_SERIAL];
static const char manufacturer[] = CONFIG_USB_GADGET_MANUFACTURER;
static uint8_t hostaddr[ETH_ALEN];
void g_ncm_set_serialnumber(char *s)
{
	memset(g_ncm_serial, 0, MAX_STRING_SERIAL);
	strncpy(g_ncm_serial, s, MAX_STRING_SERIAL - 1);
}

static struct usb_device_descriptor device_desc = {
	.bLength = sizeof device_desc,
	.bDescriptorType = USB_DT_DEVICE,

	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass = 0, /*0x02:CDC-modem , 0x00:CDC-serial*/
	.bDeviceProtocol =	0x00,

	.idVendor = cpu_to_le16(CONFIG_USB_GADGET_VENDOR_NUM),
	.idProduct = cpu_to_le16(CONFIG_USB_GADGET_PRODUCT_NUM),
	/* .iProduct = DYNAMIC */
	/* .iSerialNumber = DYNAMIC */
	.bNumConfigurations = 1,
};

/*
 * static strings, in UTF-8
 * IDs for those strings are assigned dynamically at g_ncm_bind()
 */
static struct usb_string g_ncm_string_defs[] = {
	{.s = manufacturer},
	{.s = product},
	{.s = g_ncm_serial},
};

static struct usb_gadget_strings g_ncm_string_tab = {
	.language = 0x0409, /* en-us */
	.strings = g_ncm_string_defs,
};

static struct usb_gadget_strings *g_ncm_composite_strings[] = {
	&g_ncm_string_tab,
	NULL,
};

static int usb_gadget_controller_number(struct usb_gadget *gadget)
{
    (void)gadget;
    return 0x02;
}
static int g_ncm_unbind(struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;

	usb_gadget_disconnect(gadget);

	return 0;
}

int ncm_bind_config(struct usb_configuration *c, u8 ethaddr[ETH_ALEN]);
static int g_ncm_do_config(struct usb_configuration *c)
{
	return ncm_bind_config(c, hostaddr);
}

static int g_ncm_config_register(struct usb_composite_dev *cdev)
{
	struct usb_configuration *config;
	const char *name = "usb_ncm";

	config = kmalloc(sizeof(*config), 0);
	if (!config)
		return -ENOMEM;

	memset(config, 0, sizeof(*config));

	config->label = name;
	config->bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER;
	config->bConfigurationValue = CONFIGURATION_NUMBER;
	config->iConfiguration = STRING_USBDOWN;
	config->bind = g_ncm_do_config;
       listSET_LIST_ITEM_OWNER(&config->list, config);

	return usb_add_config(cdev, config);
}

static int g_ncm_get_bcd_device_number(struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;
	int gcnum;

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum > 0)
		gcnum += 0x200;

	return gcnum;
}

static int g_ncm_bind(struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;
	int id, ret;
	int gcnum;

	ret = gether_setup(cdev->gadget, hostaddr);

	debug("%s: gadget: 0x%p cdev: 0x%p\n", __func__, gadget, cdev);

	id = usb_string_id(cdev);

	if (id < 0)
		return id;
	g_ncm_string_defs[0].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;

	g_ncm_string_defs[1].id = id;
	device_desc.iProduct = id;

	if (strlen(g_ncm_serial)) {
		id = usb_string_id(cdev);
		if (id < 0)
			return id;

		g_ncm_string_defs[2].id = id;
		device_desc.iSerialNumber = id;
	}

	ret = g_ncm_config_register(cdev);
	if (ret)
		goto error;

	gcnum = g_ncm_get_bcd_device_number(cdev);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(gcnum);
	else {
		debug("%s: controller '%s' not recognized\n",
			__func__, gadget->name);
		device_desc.bcdDevice = cpu_to_le16(0x9999);
	}

	debug("%s: calling usb_gadget_connect for "
			"controller '%s'\n", __func__, gadget->name);
	usb_gadget_connect(gadget);

	return 0;

 error:
	g_ncm_unbind(cdev);
	return -ENOMEM;
}

static struct usb_composite_driver g_ncm_driver = {
	.name = NULL,
	.dev = &device_desc,
	.strings = g_ncm_composite_strings,

	.bind = g_ncm_bind,
	.unbind = g_ncm_unbind,
};

int g_ncm_register(const char *name)
{
	int ret;

	debug("%s: g_ncm_driver.name = %s\n", __func__, name);
	g_ncm_driver.name = name;

	ret = usb_composite_register(&g_ncm_driver);
	if (ret) {
		printf("%s: failed!, error: %d\n", __func__, ret);
		return ret;
	}
	return 0;
}

void g_ncm_unregister(void)
{
	usb_composite_unregister(&g_ncm_driver);
}

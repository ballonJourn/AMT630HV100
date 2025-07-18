#include "usb_os_adapter.h"
#include "FreeRTOS_POSIX.h"
#include "pthread.h"
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "chip.h"
#include "dma-direction.h"
#include "dma-mapping.h"
/**
 * struct usb_udc - describes one usb device controller
 * @driver - the gadget driver pointer. For use by the class code
 * @dev - the child device to the actual controller
 * @gadget - the gadget. For use by the class code
 * @list - for use by the udc class driver
 *
 * This represents the internal data structure which is used by the UDC-class
 * to hold information about udc driver and gadget together.
 */
struct usb_udc {
	struct usb_gadget_driver	*driver;
	struct usb_gadget		*gadget;
	struct device			dev;
	ListItem_t		list;
};

static List_t udc_list;
static SemaphoreHandle_t udc_lock;
static int g_udc_core_init_flag = 0;
int usb_udc_core_init()
{
    if (g_udc_core_init_flag)
        return 0;
    g_udc_core_init_flag = 1;
    udc_lock = xSemaphoreCreateMutex();
    vListInitialise(&udc_list);
    return 0;
}

int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return 0;

	req->dma = dma_map_single(req->buf, req->length,
				  is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	return 0;
}

void usb_gadget_unmap_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return;

	dma_unmap_single((void *)req->dma, req->length,
			 is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
}

/* ------------------------------------------------------------------------- */

/**
 * usb_gadget_giveback_request - give the request back to the gadget layer
 * Context: in_interrupt()
 *
 * This is called by device controller drivers in order to return the
 * completed request back to the gadget layer.
 */
void usb_gadget_giveback_request(struct usb_ep *ep,
		struct usb_request *req)
{
	req->complete(ep, req);
}

/* ------------------------------------------------------------------------- */

void usb_gadget_set_state(struct usb_gadget *gadget,
		enum usb_device_state state)
{
	gadget->state = state;
}

/* ------------------------------------------------------------------------- */

/**
 * usb_gadget_udc_reset - notifies the udc core that bus reset occurs
 * @gadget: The gadget which bus reset occurs
 * @driver: The gadget driver we want to notify
 *
 * If the udc driver has bus reset handler, it needs to call this when the bus
 * reset occurs, it notifies the gadget driver that the bus reset occurs as
 * well as updates gadget state.
 */
void usb_gadget_udc_reset(struct usb_gadget *gadget,
		struct usb_gadget_driver *driver)
{
	driver->reset(gadget);
	usb_gadget_set_state(gadget, USB_STATE_DEFAULT);
}

/**
 * usb_gadget_udc_start - tells usb device controller to start up
 * @udc: The UDC to be started
 *
 * This call is issued by the UDC Class driver when it's about
 * to register a gadget driver to the device controller, before
 * calling gadget driver's bind() method.
 *
 * It allows the controller to be powered off until strictly
 * necessary to have it powered on.
 *
 * Returns zero on success, else negative errno.
 */
static inline int usb_gadget_udc_start(struct usb_udc *udc)
{
	return udc->gadget->ops->udc_start(udc->gadget, udc->driver);
}

/**
 * usb_gadget_udc_stop - tells usb device controller we don't need it anymore
 * @gadget: The device we want to stop activity
 * @driver: The driver to unbind from @gadget
 *
 * This call is issued by the UDC Class driver after calling
 * gadget driver's unbind() method.
 *
 * The details are implementation specific, but it can go as
 * far as powering off UDC completely and disable its data
 * line pullups.
 */
static inline void usb_gadget_udc_stop(struct usb_udc *udc)
{
	udc->gadget->ops->udc_stop(udc->gadget);
}
#if 0
/**
 * usb_udc_release - release the usb_udc struct
 * @dev: the dev member within usb_udc
 *
 * This is called by driver's core in order to free memory once the last
 * reference is released.
 */
static void usb_udc_release(struct device *dev)
{
	//struct usb_udc *udc;
       (void)dev;
	//udc = container_of(dev, struct usb_udc, dev);
	//kfree(udc);
}
#endif
/**
 * usb_add_gadget_udc_release - adds a new gadget to the udc class driver list
 * @parent: the parent device to this udc. Usually the controller driver's
 * device.
 * @gadget: the gadget to be added to the list.
 * @release: a gadget release function.
 *
 * Returns zero on success, negative errno otherwise.
 */
int usb_add_gadget_udc_release(struct device *parent, struct usb_gadget *gadget,
		void (*release)(struct device *dev))
{
	struct usb_udc		*udc;
	int			ret = -ENOMEM;

	udc = kzalloc(sizeof(*udc), GFP_KERNEL);
	if (!udc)
		goto err1;

	udc->gadget = gadget;

       INIT_LIST_ITEM(&udc->list);
	listSET_LIST_ITEM_OWNER(&udc->list, udc);

	xSemaphoreTake(udc_lock, portMAX_DELAY);
	list_add_tail(&udc->list, &udc_list);

	usb_gadget_set_state(gadget, USB_STATE_NOTATTACHED);

	xSemaphoreGive(udc_lock);

	return 0;

err1:
	return ret;
}

/**
 * usb_add_gadget_udc - adds a new gadget to the udc class driver list
 * @parent: the parent device to this udc. Usually the controller
 * driver's device.
 * @gadget: the gadget to be added to the list
 *
 * Returns zero on success, negative errno otherwise.
 */
int usb_add_gadget_udc(struct device *parent, struct usb_gadget *gadget)
{     
       usb_udc_core_init();
	return usb_add_gadget_udc_release(parent, gadget, NULL);
}

static void usb_gadget_remove_driver(struct usb_udc *udc)
{
	dev_dbg(&udc->dev, "unregistering UDC driver [%s]\n",
			udc->driver->function);

	usb_gadget_disconnect(udc->gadget);
	udc->driver->disconnect(udc->gadget);
	udc->driver->unbind(udc->gadget);
	usb_gadget_udc_stop(udc);

	udc->driver = NULL;
}

/**
 * usb_del_gadget_udc - deletes @udc from udc_list
 * @gadget: the gadget to be removed.
 *
 * This, will call usb_gadget_unregister_driver() if
 * the @udc is still busy.
 */
void usb_del_gadget_udc(struct usb_gadget *gadget)
{
	struct usb_udc		*udc = NULL;
       ListItem_t                   *pxListItem = NULL;

	xSemaphoreTake(udc_lock, portMAX_DELAY);
	list_for_each_entry(pxListItem, udc, &udc_list)
		if (udc->gadget == gadget)
			goto found;

	dev_err(gadget->dev.parent, "gadget not registered.\n");
	xSemaphoreGive(udc_lock);

	return;

found:
	dev_vdbg(gadget->dev.parent, "unregistering gadget\n");

	list_del(&udc->list);
	xSemaphoreGive(udc_lock);

	if (udc->driver)
		usb_gadget_remove_driver(udc);
}

/* ------------------------------------------------------------------------- */

static int udc_bind_to_driver(struct usb_udc *udc, struct usb_gadget_driver *driver)
{
	int ret;

	dev_dbg(&udc->dev, "registering UDC driver [%s]\n",
			driver->function);

	udc->driver = driver;

	ret = driver->bind(udc->gadget);
	if (ret)
		goto err1;
	ret = usb_gadget_udc_start(udc);
	if (ret) {
		driver->unbind(udc->gadget);
		goto err1;
	}
	usb_gadget_connect(udc->gadget);

	return 0;
err1:
	if (ret != -EISNAM)
		dev_err(&udc->dev, "failed to start %s: %d\n",
			udc->driver->function, ret);
	udc->driver = NULL;
	return ret;
}

int usb_gadget_probe_driver(struct usb_gadget_driver *driver)
{
	struct usb_udc		*udc = NULL;
	int			ret;
      ListItem_t                   *pxListItem = NULL;

	if (!driver || !driver->bind || !driver->setup)
		return -EINVAL;

	xSemaphoreTake(udc_lock, portMAX_DELAY);
	list_for_each_entry(pxListItem, udc, &udc_list) {
		/* For now we take the first one */
		if (!udc->driver)
			goto found;
	}

	printf("couldn't find an available UDC\n");
	xSemaphoreGive(udc_lock);
	return -ENODEV;
found:
	ret = udc_bind_to_driver(udc, driver);
	xSemaphoreGive(udc_lock);
	return ret;
}


int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
       usb_udc_core_init();
	return usb_gadget_probe_driver(driver);
}


int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct usb_udc		*udc = NULL;
	int			ret = -ENODEV;
       ListItem_t                   *pxListItem = NULL;

	if (!driver || !driver->unbind)
		return -EINVAL;
       
	xSemaphoreTake(udc_lock, portMAX_DELAY);
	list_for_each_entry(pxListItem, udc, &udc_list) 
		if (udc->driver == driver) {
			usb_gadget_remove_driver(udc);
			usb_gadget_set_state(udc->gadget,
					USB_STATE_NOTATTACHED);
			ret = 0;
			break;
		}

	xSemaphoreGive(udc_lock);
	return ret;
}


#include "FreeRTOS.h"
#include "chip.h"

#define GPIO_SWPORTA_DR							    0x00
#define GPIO_SWPORTA_DDR							0x04
#define GPIO_SWPORTA_CTL							0x08
#define GPIO_SWPORTA_INTEN							0x30
#define GPIO_SWPORTA_INTMASK						0x34
#define GPIO_SWPORTA_INTTYPE_LEVEL					0x38
#define GPIO_SWPORTA_INT_POLARITY					0x3c
#define GPIO_SWPORTA_INTSTATUS						0x40
#define GPIO_SWPORTA_RAW_INTSTATUS					0x44
#define GPIO_SWPORTA_DEBOUNCE						0x48
#define GPIO_SWPORTA_EOI						    0x4c
#define GPIO_SWPORTA_EXT_PORTA						0x50
#define GPIO_SWPORTA_EXT_PORTB						0x54
#define GPIO_SWPORTA_EXT_PORTC						0x58
#define GPIO_SWPORTA_EXT_PORTD						0x5c
#define GPIO_SWPORTA_LS_SYNC						0x60
#define GPIO_SWPORTA_ID_CODE						0x64
#define GPIO_SWPORTA_INT_BOTHEDGE					0x68
#define GPIO_SWPORTA_VER_ID_CODE					0x6C
#define GPIO_SWPORTA_CONFIG_REG2					0x70
#define GPIO_SWPORTA_CONFIG_REG1					0x74

#define GPIO_BANK			4
#define GPIO_NUM			128

typedef struct {
	ISRFunction_t handler;
	void *handler_param;
	int irq_type;
} GpioIrqDesc_t;
static GpioIrqDesc_t gpio_irq_descs[GPIO_NUM];

static __INLINE uint32_t gpio_get_regbase(int gpio)
{
	int gpiox = (gpio >> 5) & 0x3;

	return REGS_GPIO_BASE + 0x80 * gpiox;
}

/* static __INLINE int GPIO_BANK(unsigned gpio)
{
    return gpio >> 5;
} */

static __INLINE int GPIO_OFFSET(unsigned gpio)
{
	if (gpio == 96)
		return 2;
	else if (gpio == 97)
		return 0;
	else if (gpio == 98)
		return 3;
	else if (gpio == 99)
		return 1;
	else
    	return gpio & 0x1F;
}

static __INLINE void *GPIO_MODREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_DDR);
}

static __INLINE void *GPIO_WDATAREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_DR);
}

static __INLINE void *GPIO_RDATAREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_EXT_PORTA);
}

static __INLINE void *GPIO_INTENREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_INTEN);
}

static __INLINE void *GPIO_INTMASKREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_INTMASK);
}

static __INLINE void *GPIO_INTLVLREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_INTTYPE_LEVEL);
}

static __INLINE void *GPIO_INTPOLREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_INT_POLARITY);
}

void gpio_request(unsigned gpio)
{
	pinctrl_gpio_request(gpio);
}

void gpio_direction_output(unsigned gpio, int value)
{
	configASSERT(gpio < GPIO_NUM);

	gpio_request(gpio);
	writel(readl(GPIO_MODREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_MODREG(gpio));
	if (value)
		writel(readl(GPIO_WDATAREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));
	else
		writel(readl(GPIO_WDATAREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));	
}

void gpio_direction_input(unsigned gpio)
{
	configASSERT(gpio < GPIO_NUM);

	gpio_request(gpio);
	writel(readl(GPIO_MODREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_MODREG(gpio));
}

void gpio_set_value(unsigned gpio, int value)
{
	configASSERT(gpio < GPIO_NUM);

	if (value)
		writel(readl(GPIO_WDATAREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));
	else
		writel(readl(GPIO_WDATAREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));
}

int gpio_get_value(unsigned gpio)
{
	configASSERT(gpio < GPIO_NUM);

	return !!(readl(GPIO_RDATAREG(gpio)) & (1 << GPIO_OFFSET(gpio)));
}

static void gpio_toggle_trigger(unsigned gpio)
{
	u32 pol;

	pol = readl(GPIO_INTPOLREG(gpio));

	if (pol & (1 << GPIO_OFFSET(gpio)))
		pol &= ~(1 << GPIO_OFFSET(gpio));
	else
		pol |= (1 << GPIO_OFFSET(gpio));

	writel(pol, GPIO_INTPOLREG(gpio));
}

static void gpio_irq_enable(unsigned gpio)
{
	writel(readl(GPIO_INTENREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_INTENREG(gpio));
	writel(readl(GPIO_INTMASKREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_INTMASKREG(gpio));
}

static void gpio_irq_disable(unsigned gpio)
{
	writel(readl(GPIO_INTENREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_INTENREG(gpio));
	writel(readl(GPIO_INTMASKREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_INTMASKREG(gpio));
}

static int gpio_irq_set_irq_type(unsigned gpio, int type)
{
	unsigned long level, polarity;

	level = readl(GPIO_INTLVLREG(gpio));
	polarity = readl(GPIO_INTPOLREG(gpio));

	switch (type) {
	case IRQ_TYPE_EDGE_BOTH:
		level |= (1 << GPIO_OFFSET(gpio));
		gpio_toggle_trigger(gpio);
		break;
	case IRQ_TYPE_EDGE_RISING:
		level |= (1 << GPIO_OFFSET(gpio));
		polarity |= (1 << GPIO_OFFSET(gpio));
		break;
	case IRQ_TYPE_EDGE_FALLING:
		level |= (1 << GPIO_OFFSET(gpio));
		polarity &= ~(1 << GPIO_OFFSET(gpio));
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		level &= ~(1 << GPIO_OFFSET(gpio));
		polarity |= (1 << GPIO_OFFSET(gpio));
		break;
	case IRQ_TYPE_LEVEL_LOW:
		level &= ~(1 << GPIO_OFFSET(gpio));
		polarity &= ~(1 << GPIO_OFFSET(gpio));
		break;
	}

	writel(level, GPIO_INTLVLREG(gpio));
	if (type != IRQ_TYPE_EDGE_BOTH)
		writel(polarity, GPIO_INTPOLREG(gpio));

	return 0;
}

static void gpio_irq_handler(void *param)
{
	u32 status;
	int i, j;

	for (i = 0; i < GPIO_BANK; i++) {
		status = readl(REGS_GPIO_BASE + 0x80 * i + GPIO_SWPORTA_INTSTATUS);
		for (j = 0; j < 32; j++) {
			if (status & (1 << j)) {
				u32 gpio = i * 32 + j;

				/* clear interrupt */
				writel(1 << j, REGS_GPIO_BASE + 0x80 * i + GPIO_SWPORTA_EOI);
				if (gpio_irq_descs[gpio].handler != NULL) {
					gpio_irq_descs[gpio].handler(gpio_irq_descs[gpio].handler_param);
					if (gpio_irq_descs[gpio].irq_type == IRQ_TYPE_EDGE_BOTH)
						gpio_toggle_trigger(gpio);
				}
			}
		}
	}
}

int gpio_irq_request(unsigned gpio, int irq_type, ISRFunction_t irq_handler, void *param)
{
	configASSERT(gpio < GPIO_NUM);

	portENTER_CRITICAL();
	gpio_request(gpio);
	gpio_irq_descs[gpio].handler = irq_handler;
	gpio_irq_descs[gpio].handler_param = param;
	gpio_irq_descs[gpio].irq_type = irq_type;
	gpio_irq_set_irq_type(gpio, irq_type);
	request_irq(GPIOA_IRQn + ((gpio >> 5) & 0x3), 0, gpio_irq_handler, NULL);
	gpio_irq_enable(gpio);
	portEXIT_CRITICAL();

	return 0;
}

int gpio_irq_free(unsigned gpio)
{
	configASSERT(gpio < GPIO_NUM);
	
	portENTER_CRITICAL();
	gpio_irq_disable(gpio);
	gpio_irq_descs[gpio].handler = NULL;
	gpio_irq_descs[gpio].handler_param = NULL;
	portEXIT_CRITICAL();

	return 0;
}

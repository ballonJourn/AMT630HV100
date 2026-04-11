#include "amt630h.h"

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

#define GPIO_NUM			128

static unsigned int gpio_get_regbase(int gpio)
{
	int gpiox = (gpio >> 5) & 0x3;

	return GPIO_BASE + 0x80 * gpiox;
}

static int GPIO_OFFSET(unsigned gpio)
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

static void *GPIO_MODREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_DDR);
}

static void *GPIO_WDATAREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_DR);
}

static void *GPIO_RDATAREG(unsigned gpio)
{
    return (void*)(gpio_get_regbase(gpio) + GPIO_SWPORTA_EXT_PORTA);
}

void gpio_direction_output(unsigned gpio, int value)
{
	writel(readl(GPIO_MODREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_MODREG(gpio));
	if (value)
		writel(readl(GPIO_WDATAREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));
	else
		writel(readl(GPIO_WDATAREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));	
}

void gpio_direction_input(unsigned gpio)
{
	writel(readl(GPIO_MODREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_MODREG(gpio));
}

void gpio_set_value(unsigned gpio, int value)
{
	if (value)
		writel(readl(GPIO_WDATAREG(gpio)) | (1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));
	else
		writel(readl(GPIO_WDATAREG(gpio)) & ~(1 << GPIO_OFFSET(gpio)), GPIO_WDATAREG(gpio));
}

int gpio_get_value(unsigned gpio)
{
	return !!(readl(GPIO_RDATAREG(gpio)) & (1 << GPIO_OFFSET(gpio)));
}

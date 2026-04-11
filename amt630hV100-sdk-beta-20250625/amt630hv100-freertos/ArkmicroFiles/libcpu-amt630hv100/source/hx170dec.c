#include "FreeRTOS.h"
#include "chip.h"
#include "os_adapt.h"

#include "hx170dec.h"
#include "vdec.h"

#define REGS_VDEC_BASE	0x71200000

#define VDEC_MAX_CORES                 1 /* number of cores of the hardware IP */
#define VDEC_NUM_REGS_DEC             60 /* number of registers of the Decoder part */
#define VDEC_NUM_REGS_PP              41 /* number of registers of the Post Processor part */
#define VDEC_DEC_FIRST_REG             0 /* first register (0-based) index */
#define VDEC_DEC_LAST_REG             59 /* last register (0-based) index */
#define VDEC_PP_FIRST_REG             60
#define VDEC_PP_LAST_REG             100

struct vdec_device {
	unsigned int mmio_base;
	struct device *dev;
	int irq;
	int num_cores;
	unsigned long iobaseaddr;
	unsigned long iosize;
	QueueHandle_t dec_irq_done;
	SemaphoreHandle_t dec_sem;
	u32 regs[VDEC_NUM_REGS_DEC + VDEC_NUM_REGS_PP];
};
static struct vdec_device *vdec6731_global;

static inline void vdec_writel(const struct vdec_device *p, unsigned offset, u32 val)
{
	writel(val, p->mmio_base + offset);
}

static inline u32 vdec_readl(const struct vdec_device *p, unsigned offset)
{
	return readl(p->mmio_base + offset);
}

/**
 * Write a range of registers. First register is assumed to be
 * "Interrupt Register" and will be written last.
 */
static int vdec_regs_write(struct vdec_device *p, int begin, int end,
		const struct core_desc *core)
{
	int i;

	memcpy(&p->regs[begin], core->regs, (end - begin + 1) * 4);

	for (i = end; i >= begin; i--)
		vdec_writel(p, 4 * i, p->regs[i]);

	return 0;
}

/**
 * Read a range of registers [begin..end]
 */
static int vdec_regs_read(struct vdec_device *p, int begin, int end,
		const struct core_desc *core)
{
	int i;

	for (i = end; i >= begin; i--)
		p->regs[i] = vdec_readl(p, 4 * i);

	memcpy(core->regs, &p->regs[begin], (end - begin + 1) * 4);

	return 0;
}

long vdec_ioctl(unsigned int cmd, void *arg)
{
	int ret = 0;
	void *argp = arg;
	struct vdec_device *p = vdec6731_global;
	struct core_desc core;
	u32 reg;

	switch (cmd) {
		case HX170DEC_IOX_ASIC_ID:
			reg = vdec_readl(p, VDEC_IDR);
			memcpy(argp, &reg, sizeof(u32));
			break;

		case HX170DEC_IOC_MC_OFFSETS:
		case HX170DEC_IOCGHWOFFSET:
			memcpy(argp, &p->iobaseaddr, sizeof(p->iobaseaddr));
			break;
		case HX170DEC_IOCGHWIOSIZE: /* in bytes */
			memcpy(argp, &p->iosize, sizeof(p->iosize));
			break;
		case HX170DEC_IOC_MC_CORES:
			memcpy(argp, &p->num_cores, sizeof(p->num_cores));
			break;

		case HX170DEC_IOCS_DEC_PUSH_REG:
			memcpy(&core, (void *)arg, sizeof(struct core_desc));
			/* Skip VDEC_IDR (ID Register, ro) */
			core.regs++; // core.size -= 4;
			ret = vdec_regs_write(p, VDEC_DEC_FIRST_REG + 1, VDEC_DEC_LAST_REG, &core);
			break;
		case HX170DEC_IOCS_PP_PUSH_REG:
			break;

		case HX170DEC_IOCS_DEC_PULL_REG:
			memcpy(&core, (void *)arg, sizeof(struct core_desc));
			ret = vdec_regs_read(p, VDEC_DEC_FIRST_REG, VDEC_DEC_LAST_REG, &core);
			break;

		case HX170DEC_IOCS_PP_PULL_REG:
			break;

		case HX170DEC_IOCX_DEC_WAIT:
			memcpy(&core, (void *)arg, sizeof(struct core_desc));
			if (xQueueReceive(p->dec_irq_done, NULL, pdMS_TO_TICKS(1000)) != pdTRUE) {
				dev_err(p->dev, "wait_event_interruptible dec error %d\n", ret);
				ret = -ETIMEDOUT;
			} else {
				/* Update dec registers */
				ret = vdec_regs_read(p, VDEC_DEC_FIRST_REG, VDEC_DEC_LAST_REG, &core);
			}
			xQueueReset(p->dec_irq_done);
			break;
			
		case HX170DEC_IOCX_PP_WAIT:
			break;			

		case HX170DEC_IOCH_DEC_RESERVE:
			if (xSemaphoreTake(p->dec_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
				ret = 0; /* core id */
				dev_dbg(p->dev, "down dec_sem (core id %d)\n", ret);
			} else {
				dev_err(p->dev, "down_interruptible dec error\n");
				ret = -ETIMEDOUT;
			}
			break;
		case HX170DEC_IOCT_DEC_RELEASE:
			dev_dbg(p->dev, "up dec_sem\n");
			xSemaphoreGive(p->dec_sem);
			break;

		case HX170DEC_IOCQ_PP_RESERVE:

			break;
		case HX170DEC_IOCT_PP_RELEASE:
			break;

		default:
			dev_warn(p->dev, "unknown ioctl %x\n", cmd);
			ret = -EINVAL;
	}
	return ret;
}

/*
 * Platform driver related
 */

/* Should we use spin_lock_irqsave here? */
static void vdec_isr(void *dev_id)
{
	struct vdec_device *p = dev_id;
	u32 irq_status_dec;
	int handled = 0;

	/* interrupt status register read */
	irq_status_dec = vdec_readl(p, VDEC_DIR);
	//printf("irq_status=0x%x.\n", irq_status_dec);
	if (irq_status_dec & VDEC_DIR_ISET) {
		/* Clear IRQ */
		vdec_writel(p, VDEC_DIR, irq_status_dec & ~VDEC_DIR_ISET);

		xQueueSendFromISR(p->dec_irq_done, NULL, 0);
		handled++;
	}

	if (handled == 0) {
		dev_warn(p->dev, "Spurious IRQ (DIR=%08x)\n", irq_status_dec);
	}
}

#if 0
static void vdec_iram_mode_select(void)
{
	//0 MFC ram use as iram; 1 MFCram use MFC REF buffer
	u32 val = readl(REGS_SYSCTL_BASE + 0x78);
	val |= (1<<0);
	writel(val, REGS_SYSCTL_BASE + 0x78);
}
#endif

int vdec_init(void)
{
	struct vdec_device *p;
	int hwid;

	/* Allocate private data */
	p = pvPortMalloc(sizeof(struct vdec_device));
	if (!p) {
		dev_dbg(&pdev->dev, "out of memory\n");
		return -ENOMEM;
	}
	memset(p, 0, sizeof(struct vdec_device));

	p->mmio_base = REGS_VDEC_BASE;
	p->irq = JPG_IRQn;
	p->num_cores = VDEC_MAX_CORES;
	p->iosize = 0x200;
	p->iobaseaddr = REGS_VDEC_BASE;
	vdec6731_global = p;

	request_irq(p->irq, 0, vdec_isr, p);

	p->dec_irq_done = xQueueCreate(1, 0);
	p->dec_sem = xSemaphoreCreateCounting(VDEC_MAX_CORES, 1);

	dev_info(&pdev->dev, "VDEC controller at 0x%u, irq = %d\n",
			p->mmio_base, p->irq);

	/* Reset Asic (just in case..) */
	vdec_writel(p, VDEC_DIR, VDEC_DIR_ID | VDEC_DIR_ABORT);
	vdec_writel(p, VDEC_PPIR, VDEC_PPIR_ID);

	hwid = vdec_readl(p, VDEC_IDR);

	printf("Product ID: %#x (revision %d.%d.%d)\n", \
			(hwid & VDEC_IDR_PROD_ID) >> 16,
			(hwid & VDEC_IDR_MAJOR_VER) >> 12,
			(hwid & VDEC_IDR_MINOR_VER) >> 4,
			(hwid & VDEC_IDR_BUILD_VER));

	return 0;
}

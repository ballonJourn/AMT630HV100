#include "FreeRTOS.h"
#include "board.h"
#include "amt630h.h"
#include "pxp.h"
#include "timer.h"
#include "aic.h"
#include "uart.h"

#define PXP_CTRL               0x000
#define PXP_CTRL_SET           0x004
#define PXP_CTRL_CLR           0x008
#define PXP_CTRL_TOG           0x00C
#define PXP_STAT               0x010
#define PXP_STAT_SET           0x014
#define PXP_STAT_CLR           0x018
#define PXP_STAT_TOG           0x01c
#define PXP_OUTBUF             0x020
#define PXP_OUTBUF2            0x030
#define PXP_OUTSIZE            0x040
#define PXP_S0BUF              0x050
#define PXP_S0UBUF             0x060
#define PXP_S0VBUF             0x070
#define PXP_S0PARAM            0x080
#define PXP_S0BACKGROUND       0x090
#define PXP_S0CLIP             0x0A0
#define PXP_S0SCALE            0x0B0
#define PXP_S0OFFSET           0x0C0
#define PXP_CSCCOEFF0          0x0D0
#define PXP_CSCCOEFF1          0x0E0
#define PXP_CSCCOEFF2          0x0F0
#define PXP_NEXT               0x100
#define PXP_PAGETABLE          0x170
#define PXP_S0CKEYL            0x180
#define PXP_S0CKEYH            0x190
#define PXP_OLCKEYL            0x1A0
#define PXP_OLCKEYH            0x1B0
#define PXP_OL0BUF             0x200
#define PXP_OL0SIZE            0x210
#define PXP_OL0PARAM           0x220
#define PXP_OL1BUF             0x240
#define PXP_OL1SIZE            0x250
#define PXP_OL1PARAM           0x260
#define PXP_OL2BUF             0x280
#define PXP_OL2SIZE            0x290
#define PXP_OL2PARAM           0x2A0
#define PXP_OL3BUF             0x2C0
#define PXP_OL3SIZE            0x2D0
#define PXP_OL3PARAM           0x2E0
#define PXP_OL4BUF             0x300
#define PXP_OL4SIZE            0x310
#define PXP_OL4PARAM           0x320
#define PXP_OL5BUF             0x340
#define PXP_OL5SIZE            0x350
#define PXP_OL5PARAM           0x360
#define PXP_OL6BUF             0x380
#define PXP_OL6SIZE            0x390
#define PXP_OL6PARAM           0x3A0
#define PXP_OL7BUF             0x3C0
#define PXP_OL7SIZE            0x3D0
#define PXP_OL7PARAM           0x3E0
// offss for "NEXT" pointers
#define PXP_NEXTCTRL           0x000
#define PXP_NEXTRGBBUF         0x004
#define PXP_NEXTRGBBUF2        0x008
#define PXP_NEXTRGBSIZE        0x00C
#define PXP_NEXTS0BUF          0x010
#define PXP_NEXTS0UBUF         0x014
#define PXP_NEXTS0VBUF         0x018
#define PXP_NEXTS0PARAM        0x01C
#define PXP_NEXTS0BACKGROUND   0x020
#define PXP_NEXTS0CLIP         0x024
#define PXP_NEXTS0SCALE        0x028
#define PXP_NEXTS0OFFSET       0x02C
#define PXP_NEXTS0CKEYL        0x030
#define PXP_NEXTS0CKEYH        0x034
#define PXP_NEXTOLCKEYL        0x038
#define PXP_NEXTOLCKEYH        0x03C
#define PXP_NEXTOL0BUF         0x040
#define PXP_NEXTOL0SIZE        0x044
#define PXP_NEXTOL0PARAM       0x048
#define PXP_NEXTOL1BUF         0x050
#define PXP_NEXTOL1SIZE        0x054
#define PXP_NEXTOL1PARAM       0x058
#define PXP_NEXTOL2BUF         0x060
#define PXP_NEXTOL2SIZE        0x064
#define PXP_NEXTOL2PARAM       0x068
#define PXP_NEXTOL3BUF         0x070
#define PXP_NEXTOL3SIZE        0x074
#define PXP_NEXTOL3PARAM       0x078
#define PXP_NEXTOL4BUF         0x080
#define PXP_NEXTOL4SIZE        0x084
#define PXP_NEXTOL4PARAM       0x088
#define PXP_NEXTOL5BUF         0x090
#define PXP_NEXTOL5SIZE        0x094
#define PXP_NEXTOL5PARAM       0x098
#define PXP_NEXTOL6BUF         0x0A0
#define PXP_NEXTOL6SIZE        0x0A4
#define PXP_NEXTOL6PARAM       0x0A8
#define PXP_NEXTOL7BUF         0x0B0
#define PXP_NEXTOL7SIZE        0x0B4
#define PXP_NEXTOL7PARAM       0x0B8

static uint32_t pxpbase = REGS_PXP_BASE;

void waitforpxp_done(unsigned  int count)
{
    uint32_t status = 0;
    while(count--)
    {
      status = readl(pxpbase + PXP_STAT);
//      printf("+++++++++++status 0x%x++++++++++++++\n",status);
       if(status & 1)
           break;
    }
    if(count == 0)
       SendUartString("pxp timeout.\n");
}


/* this function can not used in isr */
int pxp_scaler_rotate(uint32_t s0buf, uint32_t s0ubuf, uint32_t s0vbuf,
					int s0format, uint32_t s0width, uint32_t s0height,
					uint32_t outbuf, uint32_t outbuf2, int outformat,
					uint32_t outwidth, uint32_t outheight, int outangle)
{
	uint32_t ctrl;
	int ret = 0;

	configASSERT(outangle >= PXP_ROTATE_0 && outangle <= PXP_ROTATE_270);

	if (outangle == PXP_ROTATE_90 || outangle == PXP_ROTATE_270) {
		uint32_t tmp = outheight;
		outheight = outwidth;
		outwidth = tmp;
	}

	writel(1UL << 31, pxpbase + PXP_CTRL);
    udelay(10);
	writel(0, pxpbase + PXP_CTRL);

	writel(outbuf, pxpbase + PXP_OUTBUF);
	writel(outbuf2, pxpbase + PXP_OUTBUF2);
	writel((0xffUL << 24) | (outwidth << 12) | outheight, pxpbase + PXP_OUTSIZE);
	writel(s0buf, pxpbase + PXP_S0BUF);
	writel(s0ubuf, pxpbase + PXP_S0UBUF);
	writel(s0vbuf, pxpbase + PXP_S0VBUF);
	writel((((s0width >> 3) & 0xff) << 8) | ((s0height >> 3) & 0xff), pxpbase + PXP_S0PARAM);
	writel(0, pxpbase + PXP_S0BACKGROUND);
	writel((((outwidth >> 3) & 0xff) << 8) | ((outheight >> 3) & 0xff), pxpbase + PXP_S0CLIP);
	writel(((s0height * 0x1000 / (outheight + 2)) << 16) | (s0width * 0x1000 / (outwidth + 2)), 
		pxpbase + PXP_S0SCALE);
	//YCbCr->RGB Coefficient Values bit31 ycbcr_mode
	writel((1 << 31) | 0x1f0 | (0x180 << 9) | (0x12a << 18), pxpbase + PXP_CSCCOEFF0);
	writel(0x204 | (0x198 <<16), pxpbase + PXP_CSCCOEFF1);
	writel(0x79c | (0x730 <<16), pxpbase + PXP_CSCCOEFF2);

	ctrl = (1 << 19) | (1 << 18) | ((s0format & 0xf) << 12) | (outangle << 8) |
#ifdef PXP_OUTPUT_MIRROR
		(1<< 10) |
#endif
		((outformat & 0xf) << 4) | 3;

	writel(ctrl, pxpbase + PXP_CTRL);

    waitforpxp_done(10000000);
        
	return ret;
}

int pxp_rotate(uint32_t s0buf, uint32_t s0ubuf, uint32_t s0vbuf,
					int s0format, uint32_t s0width, uint32_t s0height,
					uint32_t outbuf, uint32_t outbuf2, int outformat,
					int outangle)
{
	uint32_t ctrl;
	int ret = 0;

	configASSERT(outangle >= PXP_ROTATE_0 && outangle <= PXP_ROTATE_270);

	writel(1UL << 31, pxpbase + PXP_CTRL);
	udelay(10);
	writel(0, pxpbase + PXP_CTRL);

	writel(outbuf, pxpbase + PXP_OUTBUF);
	writel(outbuf2, pxpbase + PXP_OUTBUF2);
	writel((0xffUL << 24) | (s0width << 12) | s0height, pxpbase + PXP_OUTSIZE);
	writel(s0buf, pxpbase + PXP_S0BUF);
	writel(s0ubuf, pxpbase + PXP_S0UBUF);
	writel(s0vbuf, pxpbase + PXP_S0VBUF);
	writel((((s0width >> 3) & 0xff) << 8) | ((s0height >> 3) & 0xff), pxpbase + PXP_S0PARAM);
	writel(0, pxpbase + PXP_S0BACKGROUND);
	//YCbCr->RGB Coefficient Values bit31 ycbcr_mode
	writel((1 << 31) | 0x1f0 | (0x180 << 9) | (0x12a << 18), pxpbase + PXP_CSCCOEFF0);
	writel(0x204 | (0x198 <<16), pxpbase + PXP_CSCCOEFF1);
	writel(0x79c | (0x730 <<16), pxpbase + PXP_CSCCOEFF2);

	ctrl = ((s0format & 0xf) << 12) | (outangle << 8) | 
#ifdef PXP_OUTPUT_MIRROR
	       (1<< 10) |
#endif         
               ((outformat & 0xf) << 4) | 3;

	writel(ctrl, pxpbase + PXP_CTRL);

    waitforpxp_done(10000);
	return ret;
}

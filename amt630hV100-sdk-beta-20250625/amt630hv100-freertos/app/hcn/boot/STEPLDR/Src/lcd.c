#include "amt630h.h"
#include "board.h"
#include "lcd.h"
#include "gpio.h"
#include "sysctl.h"
#include "timer.h"

/* LCD timing */
#define LCD_PARAM0				0x000
#define LCD_PARAM1				0x004
#define LCD_PARAM2				0x008
#define LCD_PARAM3				0x00C
#define LCD_PARAM4				0x010
#define LCD_PARAM5				0x014
#define LCD_PARAM6				0x018
#define LCD_PARAM7				0x01C
#define LCD_PARAM8				0x020
#define LCD_PARAM9				0x024
#define LCD_PARAM10				0x028
#define LCD_PARAM11				0x02C
#define LCD_PARAM12				0x030
#define LCD_PARAM13				0x034
#define LCD_PARAM14				0x038
#define LCD_PARAM15				0x03C
#define LCD_PARAM16				0x040
#define LCD_PARAM17				0x044
#define LCD_PARAM18				0x048
#define LCD_PARAM19				0x04C
#define LCD_PARAM20				0x050
#define LCD_PARAM21				0x054
#define LCD_PARAM22				0x058

/* OSD */
#define LCD_OSD0_PARAM0			0x05C
#define LCD_OSD0_PARAM1			0x060
#define LCD_OSD0_PARAM2			0x064
#define LCD_OSD0_PARAM3			0x068
#define LCD_OSD0_PARAM4			0x06C
#define LCD_OSD0_PARAM5			0x070
#define LCD_OSD1_PARAM0			0x074
#define LCD_OSD1_PARAM1			0x078
#define LCD_OSD1_PARAM2			0x07C
#define LCD_OSD1_PARAM3			0x080
#define LCD_OSD1_PARAM4			0x084
#define LCD_OSD1_PARAM5			0x088
#define LCD_OSD01_PARAM			0x08C
#define LCD_OSD_COEF_SYNC		0x090
#define LCD_OSD0_PARAM2_RD		0x280
#define LCD_OSD0_PARAM3_RD		0x284
#define LCD_OSD0_PARAM4_RD		0x288
#define LCD_OSD1_PARAM2_RD		0x28C
#define LCD_OSD1_PARAM3_RD		0x290
#define LCD_OSD1_PARAM4_RD		0x294

/* Dithering */
#define LCD_DITHERING_CFG0		0x0BC
#define LCD_DITHERING_CFG1		0x0C0
#define LCD_DITHERING_CFG2		0x0C4
#define LCD_INTR_CLR			0x0C8
#define LCD_STATUS_REG			0x0CC
#define LCD_R_BIT_ORDER			0x130
#define LCD_G_BIT_ORDER			0x134
#define LCD_B_BIT_ORDER			0x138
#define LCD_GAMMA_REG			0x13c

/* CPU screen & SRGB screen */
#define LCD_SRGB_CFG			0x200
#define LCD_CPU_SCR_SOFT_REG	0x0E0
#define LCD_CPU_SCR_CTRL_REG	0x0E4
#define LCD_ADR_CLR_DATA_REG0	0x0E8
#define LCD_ADR_CLR_DATA_REG1	0x0EC
#define LCD_ADR_CLR_DATA_REG2	0x0F0
#define LCD_ADR_CLR_DATA_REG3	0x0F4
#define LCD_ADR_CLR_DATA_REG4	0x0F8
#define LCD_ADR_CLR_DATA_REG5	0x0FC
#define LCD_ADR_CLR_DATA_REG6	0x100
#define LCD_ADR_CLR_DATA_REG7	0x104
#define LCD_ADR_CLR_DATA_REG8	0x108
#define LCD_ADR_CLR_DATA_REG9	0x10C
#define LCD_ADR_CLR_DATA_REGA	0x110
#define LCD_ADR_CLR_DATA_REGB	0x114
#define LCD_ADR_CLR_DATA_REGC	0x118
#define LCD_ADR_CLR_DATA_REGD	0x11C
#define LCD_ADR_CLR_DATA_REGE	0x120
#define LCD_ADR_CLR_DATA_REGF	0x124
#define LCD_CPU_SCREEN_STATUS	0x128
#define LCD_OSD_CCM_REG0		0x208
#define LCD_OSD_CCM_REG1		0x20C
#define LCD_OSD_CCM_REG2		0x210
#define LCD_OSD_CCM_REG3		0x214
#define LCD_OSD_CCM_REG4		0x218
#define LCD_OSD_CCM_REG5		0x21C
#define LCD_CCM_EN_REG0			0x238
#define LCD_CCM_EN_REG1			0x23C
#define LCD_CCM_EN_REG2			0x240
#define LCD_CCM_EN_REG3			0x244
#define LCD_CCM_EN_REG4			0x248
#define LCD_CCM_EN_REG5			0x24C
#define LCD_CCM_EN_REG			0x250
#define LCD_ITU_EAV_CODE_CFG	0x254
#define LCD_ITU_SAV_CODE_CFG	0x258
#define LCD_HLOCK_CFG_RG		0x25C

static uint32_t fb_addr = 0;

struct ark_lcd_timing
{
	uint32_t vs_start;
	uint32_t hs_start;
	uint32_t hd;		//width
	uint32_t vd;		//height
	uint32_t vbp;
	uint32_t vfp;
	uint32_t vsw;
	uint32_t hbp;
	uint32_t hfp;
	uint32_t hsw;
};

static void ark_lcd_writel(uint32_t reg, uint32_t mask, uint8_t offset, uint32_t val)
{
	uint32_t tmp = readl(REGS_LCD_BASE+reg);
	tmp &= (~(mask << offset));
	tmp |= (val << offset);
	writel(tmp, REGS_LCD_BASE+reg);
}

int ark_lcd_set_osd_sync(LCD_OSD_LAYER osd)
{
	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	if(osd == LCD_OSD0)
		writel(readl(REGS_LCD_BASE+LCD_OSD_COEF_SYNC)|(1<<0), REGS_LCD_BASE+LCD_OSD_COEF_SYNC);
	else if(osd == LCD_OSD1)
		writel(readl(REGS_LCD_BASE+LCD_OSD_COEF_SYNC)|(1<<1), REGS_LCD_BASE+LCD_OSD_COEF_SYNC);
	else if(osd == LCD_OSD_NUMS)
		writel(readl(REGS_LCD_BASE+LCD_OSD_COEF_SYNC)|(3<<0), REGS_LCD_BASE+LCD_OSD_COEF_SYNC);
	return 0;
}

static int ark_lcd_set_osd_stride(LCD_OSD_LAYER osd, uint32_t width)
{
	uint32_t reg = LCD_OSD0_PARAM5 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	if(width > 0xFFF) {
		return -1;
	}
	ark_lcd_writel(reg, 0xFFF, 0, width);

	return 0;
}

int ark_lcd_get_osd_size(LCD_OSD_LAYER osd, uint32_t *width, uint32_t *height)
{
	uint32_t reg = LCD_OSD0_PARAM0 + osd*0x18;
	uint32_t val;

	if(osd>= LCD_OSD_NUMS) {
		return -1;
	}
	val = readl(REGS_LCD_BASE+reg);
	*width = (val >> 6) & 0xFFF;
	*height = (val >> 18) & 0xFFF;

	return 0;
}


int ark_lcd_set_osd_size(LCD_OSD_LAYER osd, uint32_t width, uint32_t height)
{
	uint32_t reg = LCD_OSD0_PARAM0 + osd*0x18;

	if(osd>= LCD_OSD_NUMS) {
		return -1;
	}
	width &= 0xFFF;
	height &= 0xFFF;
	ark_lcd_writel(reg, 0xFFFFFF, 6, (height<<12)|width);
	ark_lcd_set_osd_stride(osd, width);

	return 0;
}

int ark_lcd_set_osd_format(LCD_OSD_LAYER osd, LCD_OSD_FORMAT format)
{
	uint32_t reg = LCD_OSD0_PARAM0 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	if(format >= LCD_OSD_FORAMT_NUMS) {
		return -1;
	}
	ark_lcd_writel(reg, 0x7, 3, format);

	return 0;
}

int ark_lcd_osd_enable(LCD_OSD_LAYER osd, uint8_t enable)
{
	uint32_t reg = LCD_OSD0_PARAM0 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}
	ark_lcd_writel(reg, 0x1, 1, (enable?1:0));

	return 0;
}

int ark_lcd_osd_coeff_enable(LCD_OSD_LAYER osd, uint8_t enable)
{
	uint32_t reg = LCD_OSD0_PARAM0 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}
	ark_lcd_writel(reg, 0x1, 0, (enable?1:0));

	return 0;
}

int ark_lcd_osd_set_coeff(LCD_OSD_LAYER osd, uint32_t value)
{
	uint32_t reg = LCD_OSD0_PARAM1 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	ark_lcd_writel(reg, 0xFF, 24, value);

	return 0;
}

int ark_lcd_set_osd_possition(LCD_OSD_LAYER osd, uint32_t h, uint32_t v)
{
	uint32_t reg = LCD_OSD0_PARAM1 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	if((v > 0xFFF) || (h > 0xFFF)) {
		return -1;
	}
	ark_lcd_writel(reg, 0xFFFFFF, 0, ((v<<12)|h));

	return 0;
}

int ark_lcd_get_osd_yaddr(LCD_OSD_LAYER osd, uint32_t *yaddr)
{
	uint32_t reg = LCD_OSD0_PARAM2 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}
	*yaddr = readl(REGS_LCD_BASE+reg);

	return 0;
}

int ark_lcd_set_osd_yaddr(LCD_OSD_LAYER osd, uint32_t yaddr)
{
	uint32_t reg = LCD_OSD0_PARAM2 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}
	writel(yaddr, REGS_LCD_BASE+reg);

	return 0;
}

int ark_lcd_set_osd_uaddr(LCD_OSD_LAYER osd, uint32_t uaddr)
{
	uint32_t reg = LCD_OSD0_PARAM3 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}
	writel(uaddr, REGS_LCD_BASE+reg);

	return 0;
}

int ark_lcd_set_osd_vaddr(LCD_OSD_LAYER osd, uint32_t vaddr)
{
	uint32_t reg = LCD_OSD0_PARAM4 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}
	writel(vaddr, REGS_LCD_BASE+reg);

	return 0;
}

int ark_lcd_set_osd_mult_coef(LCD_OSD_LAYER osd, uint32_t value)
{
	uint32_t reg = LCD_OSD0_PARAM5 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	if(value > 0x7F) {
		return -1;
	}
	ark_lcd_writel(reg, 0x7F, 24, value);

	return 0;
}

int ark_lcd_set_osd_h_offset(LCD_OSD_LAYER osd, uint32_t offset)
{
	uint32_t reg = LCD_OSD0_PARAM5 + osd*0x18;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	if(offset > 0xFFF) {
		return -1;
	}
	ark_lcd_writel(reg, 0xFFF, 12, offset);

	return 0;
}

int ark_lcd_set_osd_yuv420_mode(LCD_OSD_LAYER osd, LCD_OSD_YUV420_MODE mode)
{
	uint32_t reg = LCD_OSD01_PARAM;

	if(osd >= LCD_OSD_NUMS) {
		return -1;
	}

	if(mode > LCD_OSD_Y_UV420) {
		return -1;
	}

	if (osd == LCD_OSD0)
		ark_lcd_writel(reg, 0x1, 12, mode);
	else
		ark_lcd_writel(reg, 0x1, 14, mode);

	return 0;
}


int ark_lcd_enable(uint8_t enable)
{
	ark_lcd_writel(LCD_PARAM0, 0x01, 0, (enable?1:0));

	return 0;
}

static int ark_lcd_timing_init(struct ark_lcd_timing *timing)
{
	uint32_t VS_START, HS_START, HD, VD;
	uint32_t VBP, VFP, VSW, TV;
	uint32_t HBP, HFP, HSW, TH;
	uint32_t lcd_param0, lcd_param1, lcd_param2, lcd_param3 ;
	uint32_t lcd_param4, lcd_param5, lcd_param6, lcd_param7 ;
	uint32_t lcd_param8, lcd_param9, lcd_param10, lcd_param11 ;
	uint32_t lcd_param12, lcd_param13, lcd_param14, lcd_param15 ;
	uint32_t lcd_param16, lcd_param17;
	uint8_t stop_lcd = 0 ;

	VS_START = timing->vs_start;
	HS_START = timing->hs_start;
	HD	= timing->hd;
	VD	= timing->vd;
	VBP = timing->vbp;
	VFP = timing->vfp;
	VSW = timing->vsw;
	TV	=(VS_START+VSW+VBP+VD+VFP);
	HBP = timing->hbp;
	HFP = timing->hfp;
	HSW = timing->hsw;
	TH	= (HS_START+HSW+HBP+HD+HFP);
#define      lcd_enable             0	// 1
#define      screen_width           HD
#define      rgb_pad_mode           LCD_WIRING_MODE
#define      direct_enable          0
#define      mem_lcd_enable         0
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      lcd_done_intr_enable   1
#define      dac_for_video          0
#define      dac_for_cvbs           0

#define      lcd_interlace_flag     0
#define      screen_type            1	//pRGB_i   0  pRGB_p   1   sRGB_p   2  ITU656   3
#define      VSW1_enable            0
#define      LcdVComp               0
#define      itu_pal_ntsc           0
#define      hsync_ivs              1
#define      vsync_ivs              1
#define      lcd_ac_ivs             0
#define      test_on_flag           0

#define      back_color             (0x00<<16)|(0x00<<8)|(0x00)
#define      DEN_h_rise             (HFP+HSW+HBP)
#define      DEN_h_fall             (HFP+HSW+HBP+HD)//(20+800*1)
#define      DEN0_v_rise            (VFP+VSW+VBP)
#define      DEN0_v_fall            (VFP+VSW+VBP+VD)
#define      DEN1_v_rise            (VFP+VSW+VBP)
#define      DEN1_v_fall            (VFP+VSW+VBP+VD)

// ÷°∆µ = LCD ±÷”/(CPL * LPS)
// √ø–– ±÷”÷‹∆⁄∏ˆ ˝
#define      CPL                    (TH)   // clock cycles per line  max 4096     ;  from register   timing2 16
#define      LPS                    (TV)   // Lines per screen value  ;  from register   timing1 0

#define      HSW_rise                (HFP)
#define      HSW_fall                (HFP+HSW)

#define      VSW0_v_rise             (VFP)   // Vertical sync width value    ;  from register   timing1 10
#define      VSW0_v_fall             (VFP+VSW)   // Vertical sync width value     ;  from register   timing1 10

#define      VSW0_h_rise             (HFP)
#define      VSW0_h_fall             (HFP+HSW)

#define      VSW1_v_rise             (VFP)          // Vertical sync width value ;  from register   timing1 10
#define      VSW1_v_fall             (VFP+VSW)      // Vertical sync width value  ;  from register   timing1 10

#define      VSW1_h_rise             (HFP)
#define      VSW1_h_fall             (HFP+HSW)

	writel(0, REGS_LCD_BASE+LCD_PARAM0);
	lcd_param0	=
		lcd_enable	 |
		(screen_width<<1)	|
		(rgb_pad_mode<<13)	|
		(direct_enable<<16) |
		(mem_lcd_enable<<17)|
		(range_coeff_y<<18) |
		(range_coeff_uv<<22)|
		(lcd_done_intr_enable<<26)|
		(dac_for_video<<27) |
		(dac_for_cvbs<<28)	|
		(1<<30) |
		(stop_lcd<<31);

	lcd_param1 =
		(lcd_interlace_flag<<0) |
		(screen_type<<1)	|
		(VSW1_enable<<13)	|
		(LcdVComp<<14)	|
		(itu_pal_ntsc<<15)	|
		(hsync_ivs<<17) |
		(vsync_ivs<<18) |
		(lcd_ac_ivs<<19)	|
		(1<<21) |
		(1<<23) |
		(test_on_flag<<31);

	lcd_param2	=
		(back_color<<0) |
		(1<<24) |
		(1<<25);

	lcd_param3	=
		(DEN_h_rise<<0) |
		(DEN_h_fall<<12)  ;

	lcd_param4	=
		(DEN0_v_rise<<0)	|
		(DEN0_v_fall<<12) ;

	lcd_param5	=
		(DEN1_v_rise<<0)	|
		(DEN1_v_fall<<12 ) ;

	lcd_param6	=
		(CPL<<0)	|			// clock cycles per line  max 4096	;  from register   timing2 16
		(LPS<<12) ;				// Lines per screen value	;  from register   timing1 0

	lcd_param7	=
		(HSW_rise<<0)|
		(HSW_fall<<12) ;

	lcd_param8	=
		(VSW0_v_rise<<0)|		// Vertical sync width value	;  from register   timing1 10
		(VSW0_v_fall<<12) ;		// Vertical sync width value	;  from register   timing1 10

	lcd_param9	=
		(VSW0_h_rise   <<0	)|
		(VSW0_h_fall   <<12 ) ;

	lcd_param10  =
		(VSW1_v_rise<<0)|
		(VSW1_v_fall<<12) ;

	lcd_param11  =
		(VSW1_h_rise<<0)|
		(VSW1_h_fall<<12) ;

	lcd_param12  = 66 | (129<<9) | (25<<18);
	lcd_param13  = 38 |  (74<<9) | (112<<18);
	lcd_param14  = 112 | (94<<9) | (18<<18);

	lcd_param15  = 256 | (0<<9) | (351<<18);
	lcd_param16  = 256 |  (86<<9) | (179<<18);
	lcd_param17  = 256 | (443<<10) | (0<<19);

	writel(lcd_param1, REGS_LCD_BASE+LCD_PARAM1);
	writel(lcd_param2, REGS_LCD_BASE+LCD_PARAM2);
	writel(lcd_param3, REGS_LCD_BASE+LCD_PARAM3);
	writel(lcd_param4, REGS_LCD_BASE+LCD_PARAM4);
	writel(lcd_param5, REGS_LCD_BASE+LCD_PARAM5);
	writel(lcd_param6, REGS_LCD_BASE+LCD_PARAM6);
	writel(lcd_param7, REGS_LCD_BASE+LCD_PARAM7);
	writel(lcd_param8, REGS_LCD_BASE+LCD_PARAM8);
	writel(lcd_param9, REGS_LCD_BASE+LCD_PARAM9);
	writel(lcd_param10, REGS_LCD_BASE+LCD_PARAM10);
	writel(lcd_param11, REGS_LCD_BASE+LCD_PARAM11);
	writel(lcd_param12, REGS_LCD_BASE+LCD_PARAM12);
	writel(lcd_param13, REGS_LCD_BASE+LCD_PARAM13);
	writel(lcd_param14, REGS_LCD_BASE+LCD_PARAM14);
	writel(lcd_param15, REGS_LCD_BASE+LCD_PARAM15);
	writel(lcd_param16, REGS_LCD_BASE+LCD_PARAM16);
	writel(lcd_param17, REGS_LCD_BASE+LCD_PARAM17);
	writel(lcd_param0, REGS_LCD_BASE+LCD_PARAM0);

#if LCD_WIRING_BIT_ORDER == LCD_WIRING_BIT_ORDER_LSB
	writel((0<<21|1<<18|2<<15|3<<12|4<<9|5<<6|6<<3|7<<0), REGS_LCD_BASE+LCD_R_BIT_ORDER);
	writel((0<<21|1<<18|2<<15|3<<12|4<<9|5<<6|6<<3|7<<0), REGS_LCD_BASE+LCD_G_BIT_ORDER);
	writel((0<<21|1<<18|2<<15|3<<12|4<<9|5<<6|6<<3|7<<0), REGS_LCD_BASE+LCD_B_BIT_ORDER);
#elif LCD_WIRING_BIT_ORDER == LCD_WIRING_BIT_ORDER_MSB
	writel((7<<21|6<<18|5<<15|4<<12|3<<9|2<<6|1<<3|0<<0), REGS_LCD_BASE+LCD_R_BIT_ORDER);
	writel((7<<21|6<<18|5<<15|4<<12|3<<9|2<<6|1<<3|0<<0), REGS_LCD_BASE+LCD_G_BIT_ORDER);
	writel((7<<21|6<<18|5<<15|4<<12|3<<9|2<<6|1<<3|0<<0), REGS_LCD_BASE+LCD_B_BIT_ORDER);
#endif

	return 0;
}

static int ark_lcd_hw_init()
{
	uint32_t HD, VD;

	HD = LCD_WIDTH;
	VD = LCD_HEIGHT;

#if LCD_H_FLIP
	uint8_t osd_0_hflip = 1;
	uint8_t osd_1_hflip = 1;
	uint8_t osd_0_uvfirst = 1;
#else
	uint8_t osd_0_hflip = 0;
	uint8_t osd_1_hflip = 0;
	uint8_t osd_0_uvfirst = 0;
#endif
#if LCD_V_FLIP
	uint8_t osd_0_vflip = 1;
	uint8_t osd_1_vflip = 1;
#else
	uint8_t osd_0_vflip = 0;
	uint8_t osd_1_vflip = 0;
#endif
	uint8_t osd_0_yuv420mode = 1;
	//uint8_t osd_0_yuv422mode = 0;
	uint32_t osd_01_param =
		//osd_0_yuv422mode<<20|
		osd_0_hflip<<16|
		osd_0_vflip<<17|
		osd_1_hflip<<18|
		osd_1_vflip<<19|
		osd_0_yuv420mode<<12|
		osd_0_uvfirst<<13;
	writel(osd_01_param, REGS_LCD_BASE+LCD_OSD01_PARAM);
	//osd0 init
	ark_lcd_set_osd_size(LCD_OSD0, HD, VD);
	ark_lcd_set_osd_format(LCD_OSD0, LCD_OSD_FORAMT_YUV420);
	ark_lcd_set_osd_possition(LCD_OSD0, 0, 0);
	ark_lcd_set_osd_yaddr(LCD_OSD0, fb_addr);
	ark_lcd_set_osd_uaddr(LCD_OSD0, fb_addr + HD * VD);
	ark_lcd_set_osd_vaddr(LCD_OSD0, fb_addr + HD * VD);
	ark_lcd_set_osd_h_offset(LCD_OSD0, 0);
	ark_lcd_set_osd_mult_coef(LCD_OSD0, 64);
	ark_lcd_osd_enable(LCD_OSD0, 0);
	ark_lcd_set_osd_sync(LCD_OSD0);

	//osd1 init
	ark_lcd_set_osd_size(LCD_OSD1, HD, VD);
#if LCD_BPP == 32
	ark_lcd_set_osd_format(LCD_OSD1, LCD_OSD_FORAMT_ARGB888);
	ark_lcd_osd_coeff_enable(LCD_OSD1, 0);
#elif LCD_BPP == 16
	ark_lcd_set_osd_format(LCD_OSD1, LCD_OSD_FORAMT_RGB565);
	ark_lcd_osd_coeff_enable(LCD_OSD1, 1);
	ark_lcd_osd_set_coeff(LCD_OSD1, 255);
#endif
	ark_lcd_set_osd_possition(LCD_OSD1, 0, 0);
	ark_lcd_set_osd_h_offset(LCD_OSD1, 0);
	ark_lcd_set_osd_mult_coef(LCD_OSD1, 64);
	ark_lcd_set_osd_yaddr(LCD_OSD1, fb_addr);
	ark_lcd_osd_enable(LCD_OSD1, 0);
	ark_lcd_set_osd_sync(LCD_OSD1);

	ark_lcd_enable(1);

	return 0;
}

#define LCD_SRC_FREQ	396000000//498000000
static int ark_lcd_clk_init(uint32_t freq)
{
	int div;

	if (freq > 16000000) {
		vSysctlConfigure(SYS_VOU_CLK_CFG, 0, 3, 0);
		div = DIV_ROUND_UP(LCD_SRC_FREQ, freq);
	} else if (freq >= 14000000) { //14-16MHÈÖçÁΩÆ‰∏∫16MHZ
		vSysctlConfigure(SYS_VOU_CLK_CFG, 0, 3, 0);
		div = DIV_ROUND_UP(LCD_SRC_FREQ, 16000000);
	} else {
		vSysctlConfigure(SYS_VOU_CLK_CFG, 0, 3, 2);
		div = DIV_ROUND_UP(24000000, freq);
	}

	vSysctlConfigure(SYS_VOU_CLK_CFG, 3, 0x1f, div - 1);

	return 0;
}


#if LCD_INTERFACE_TYPE == LCD_INTERFACE_LVDS
static void lvds_screen_reset(void)
{
	gpio_direction_output(LVDS_SCREEN_RST_GPIO, 0);
	udelay(100);
	gpio_direction_output(LVDS_SCREEN_RST_GPIO, 1);
	mdelay(20);
}
#endif

int lcd_init(void)
{
	struct ark_lcd_timing timing;
    int ret = 0;

	/* lcd pad select */
#if LCD_INTERFACE_TYPE == LCD_INTERFACE_TTL
	vSysctlConfigure(SYS_PAD_CTRL04, 0, 0xffffffff, 0x55555555);
	vSysctlConfigure(SYS_PAD_CTRL05, 0, 0xffffff, 0x555555);
#elif LCD_INTERFACE_TYPE == LCD_INTERFACE_LVDS
	vSysctlConfigure(SYS_PAD_CTRL04, 0, 0xfffff, 0xaaaaa);
#endif

#if LCD_INTERFACE_TYPE == LCD_INTERFACE_MIPI
	mipi_init();
#endif

	/* lcd timing init */
	timing.vs_start = 0;
	timing.hs_start = 0;
	timing.hd = LCD_WIDTH;
	timing.vd = LCD_HEIGHT;
	timing.vbp = LCD_TIMING_VBP;
	timing.vfp = LCD_TIMING_VFP;
	timing.vsw = LCD_TIMING_VSW;
	timing.hbp = LCD_TIMING_HBP;
	timing.hfp = LCD_TIMING_HFP;
	timing.hsw = LCD_TIMING_HSW;
	ark_lcd_clk_init(LCD_CLK_FREQ);
	ark_lcd_timing_init(&timing);
	ark_lcd_hw_init();

#if LCD_INTERFACE_TYPE == LCD_INTERFACE_LVDS
	mdelay(2);
	uint32_t lvds_cfg = 0xe0ec;
#if LVDS_PANEL_FORMAT == LVDS_PANEL_FORMAT_TI
	lvds_cfg |= (1 << 16);
#endif
#if LVDS_PANEL_DATA == LVDS_PANEL_DATA_8BIT
	lvds_cfg &= ~(1 << 15);
	lvds_cfg |= (1 << 4) | 1;
#endif
	writel(lvds_cfg, REGS_SYSCTL_BASE + SYS_ANA3_CFG);
	lvds_screen_reset();
#endif

    return ret;
}

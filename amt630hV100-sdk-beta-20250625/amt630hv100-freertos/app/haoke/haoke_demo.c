/*********************
 *      INCLUDES
 *********************/
#ifndef LV_DRV_NO_CONF
#include "lv_drv_conf.h"
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#if defined(LV_EX_CONF_PATH)
#define __LV_TO_STR_AUX(x) #x
#define __LV_TO_STR(x) __LV_TO_STR_AUX(x)
#include __LV_TO_STR(LV_EX_CONF_PATH)
#undef __LV_TO_STR_AUX
#undef __LV_TO_STR
#else
#include "lv_ex_conf.h"
#endif

#include "vg_driver.h"

/*********************
 *      DEFINES
 *********************/
#define IMAGES_PATH	"/images/"

#define MODEL_RADIO_TEXT "收音机"

LV_IMG_DECLARE(haoke_demo_img_0);
LV_IMG_DECLARE(haoke_demo_img_1);
LV_IMG_DECLARE(haoke_demo_img_2);
LV_IMG_DECLARE(haoke_demo_img_3);
LV_IMG_DECLARE(haoke_demo_img_4);
LV_IMG_DECLARE(haoke_demo_img_5);
LV_IMG_DECLARE(haoke_demo_img_6);
LV_IMG_DECLARE(haoke_demo_img_7);
LV_IMG_DECLARE(haoke_demo_img_8);
LV_IMG_DECLARE(haoke_demo_img_9);
LV_IMG_DECLARE(haoke_demo_img_n0);
LV_IMG_DECLARE(haoke_demo_img_n1);
LV_IMG_DECLARE(haoke_demo_img_n2);
LV_IMG_DECLARE(haoke_demo_img_n3); 
LV_IMG_DECLARE(haoke_demo_img_n4);
LV_IMG_DECLARE(haoke_demo_img_n5);
LV_IMG_DECLARE(haoke_demo_img_n6);
LV_IMG_DECLARE(haoke_demo_img_n7);
LV_IMG_DECLARE(haoke_demo_img_n8);
LV_IMG_DECLARE(haoke_demo_img_n9);
LV_IMG_DECLARE(haoke_demo_img_n10);
LV_IMG_DECLARE(haoke_demo_img_n11);
LV_IMG_DECLARE(haoke_demo_img_n12);
LV_IMG_DECLARE(haoke_demo_img_n13);
LV_IMG_DECLARE(haoke_demo_img_n14);
LV_IMG_DECLARE(haoke_demo_img_n15);
LV_IMG_DECLARE(haoke_demo_img_n16);
LV_IMG_DECLARE(haoke_demo_img_n17);
LV_IMG_DECLARE(haoke_demo_img_n18);
LV_IMG_DECLARE(haoke_demo_img_n19);
LV_IMG_DECLARE(haoke_demo_img_n20);
LV_IMG_DECLARE(haoke_demo_img_n21);
LV_IMG_DECLARE(haoke_demo_img_n22);
LV_IMG_DECLARE(haoke_demo_img_n23);
LV_IMG_DECLARE(haoke_demo_img_n24);
LV_IMG_DECLARE(haoke_demo_img_n25);
LV_IMG_DECLARE(haoke_demo_img_n26);
LV_IMG_DECLARE(haoke_demo_img_n27);
LV_IMG_DECLARE(haoke_demo_img_n28);
LV_IMG_DECLARE(haoke_demo_img_n29);
LV_IMG_DECLARE(haoke_demo_img_n30);
LV_IMG_DECLARE(haoke_demo_img_n31);
LV_IMG_DECLARE(haoke_demo_img_n32);
LV_IMG_DECLARE(haoke_demo_img_n33);
LV_IMG_DECLARE(haoke_demo_img_n34);
LV_IMG_DECLARE(haoke_demo_img_n35);
LV_IMG_DECLARE(haoke_demo_img_n36);
LV_IMG_DECLARE(haoke_demo_img_n37);
LV_IMG_DECLARE(haoke_demo_img_n38);
LV_IMG_DECLARE(haoke_demo_img_n39);
LV_IMG_DECLARE(haoke_demo_img_n40);
LV_IMG_DECLARE(haoke_demo_img_n41);
LV_IMG_DECLARE(haoke_demo_img_n42);
LV_IMG_DECLARE(haoke_demo_img_n43);
LV_IMG_DECLARE(haoke_demo_img_n44);
LV_IMG_DECLARE(haoke_demo_img_n45);
LV_IMG_DECLARE(haoke_demo_img_n46);
LV_IMG_DECLARE(haoke_demo_img_n47);
LV_IMG_DECLARE(haoke_demo_img_n48);
LV_IMG_DECLARE(haoke_demo_img_n49);
LV_IMG_DECLARE(haoke_demo_img_n50);
LV_IMG_DECLARE(haoke_demo_img_n51);
LV_IMG_DECLARE(haoke_demo_img_n52);
LV_IMG_DECLARE(haoke_demo_img_n53);
LV_IMG_DECLARE(haoke_demo_img_n54);
LV_IMG_DECLARE(haoke_demo_img_n55);
LV_IMG_DECLARE(haoke_demo_img_n56);
LV_IMG_DECLARE(haoke_demo_img_n57);
LV_IMG_DECLARE(haoke_demo_img_n58);
LV_IMG_DECLARE(haoke_demo_img_n59);
LV_IMG_DECLARE(haoke_demo_img_n60);
LV_IMG_DECLARE(haoke_demo_img_n61);
LV_IMG_DECLARE(haoke_demo_img_n62);
LV_IMG_DECLARE(haoke_demo_img_n63);
LV_IMG_DECLARE(haoke_demo_img_n64);
LV_IMG_DECLARE(haoke_demo_img_n65);
LV_IMG_DECLARE(haoke_demo_img_n66);
LV_IMG_DECLARE(haoke_demo_img_n67);
LV_IMG_DECLARE(haoke_demo_img_n68);
LV_IMG_DECLARE(haoke_demo_img_n69);
LV_IMG_DECLARE(haoke_demo_img_n70);
LV_IMG_DECLARE(haoke_demo_img_n71);
LV_IMG_DECLARE(haoke_demo_img_n72);
LV_IMG_DECLARE(haoke_demo_img_n73);
LV_IMG_DECLARE(haoke_demo_img_n74);
LV_IMG_DECLARE(haoke_demo_img_n75);
LV_IMG_DECLARE(haoke_demo_img_n76);
LV_IMG_DECLARE(haoke_demo_img_n77);
LV_IMG_DECLARE(haoke_demo_img_n78);
LV_IMG_DECLARE(haoke_demo_img_n79);
LV_IMG_DECLARE(haoke_demo_img_n80);


typedef struct {
	lv_obj_t *obj;
	const void **src_img;
	int	src_type;
	uint32_t num;
	uint32_t period;
	uint32_t last_tick;
	uint32_t index;
} xd_img_animate_t;

typedef struct {
	int x;
	int y;
	int speed;
	const void *src_img;
} needle_info_t;

static char *radio_freq_text[] = {"频道1", "频道2", "98.1", "106.1", "108.0", "87.5"};

static const void *bat_src_img[] = {
	"bat10.png", "bat9.png", "bat8.png", "bat7.png", "bat6.png",
	"bat5.png", "bat4.png", "bat3.png", "bat2.png", "bat1.png",
	"bat0.png"
};

static const void *gear_src_img[] = {
	"gear_n.png", "gear_p.png", "gear_r.png", "gear_d1.png", 
	"gear_d2.png", "gear_d3.png", "gear_d4.png"
};

static const void *num_src_img[] = {
	&haoke_demo_img_0, &haoke_demo_img_1, &haoke_demo_img_2, &haoke_demo_img_3, &haoke_demo_img_4,
	&haoke_demo_img_5, &haoke_demo_img_6, &haoke_demo_img_7, &haoke_demo_img_8, &haoke_demo_img_9
};

static needle_info_t needles[] = {
	{210, 369, 0, &haoke_demo_img_n0}, {207, 365, 1, &haoke_demo_img_n1}, {204, 361, 2, &haoke_demo_img_n2}, {200, 357, 3, &haoke_demo_img_n3}, 
	{198, 352, 4, &haoke_demo_img_n4}, {195, 348, 5, &haoke_demo_img_n5}, {193, 343, 6, &haoke_demo_img_n6}, {190, 339, 7, &haoke_demo_img_n7},
	{188, 334, 8, &haoke_demo_img_n8}, {185, 329, 9, &haoke_demo_img_n9}, {184, 324, 10, &haoke_demo_img_n10}, {182, 320, 11, &haoke_demo_img_n11},
	{181, 315, 12, &haoke_demo_img_n12}, {180, 310, 13, &haoke_demo_img_n13}, {179, 305, 14, &haoke_demo_img_n14}, {179, 300, 15, &haoke_demo_img_n15},
	{178, 295, 16, &haoke_demo_img_n16}, {178, 290, 17, &haoke_demo_img_n17}, {178, 285, 18, &haoke_demo_img_n18}, {179, 279, 19, &haoke_demo_img_n19},
	{179, 274, 20, &haoke_demo_img_n20}, {180, 268, 21, &haoke_demo_img_n21}, {181, 263, 22, &haoke_demo_img_n22}, {182, 257, 23, &haoke_demo_img_n23},
	{184, 251, 24, &haoke_demo_img_n24}, {185, 245, 25, &haoke_demo_img_n25}, {187, 239, 26, &haoke_demo_img_n26}, {189, 233, 27, &haoke_demo_img_n27},
	{191, 227, 28, &haoke_demo_img_n28}, {194, 221, 29, &haoke_demo_img_n29}, {197, 215, 30, &haoke_demo_img_n30}, {199, 210, 31, &haoke_demo_img_n31},
	{203, 204, 32, &haoke_demo_img_n32}, {206, 198, 33, &haoke_demo_img_n33}, {209, 193, 34, &haoke_demo_img_n34}, {213, 188, 35, &haoke_demo_img_n35},
	{217, 183, 36, &haoke_demo_img_n36}, {221, 177, 37, &haoke_demo_img_n37}, {225, 173, 38, &haoke_demo_img_n38}, {229, 168, 39, &haoke_demo_img_n39},
	{233, 163, 40, &haoke_demo_img_n40}, {238, 159, 41, &haoke_demo_img_n41}, {243, 154, 42, &haoke_demo_img_n42}, {248, 150, 43, &haoke_demo_img_n43},
	{253, 146, 44, &haoke_demo_img_n44}, {258, 142, 45, &haoke_demo_img_n45}, {263, 139, 46, &haoke_demo_img_n46}, {268, 135, 47, &haoke_demo_img_n47}, 
	{274, 132, 48, &haoke_demo_img_n48}, {279, 129, 49, &haoke_demo_img_n49}, {285, 126, 50, &haoke_demo_img_n50}, {291, 123, 51, &haoke_demo_img_n51},
	{297, 121, 52, &haoke_demo_img_n52}, {302, 119, 53, &haoke_demo_img_n53}, {308, 117, 54, &haoke_demo_img_n54}, {314, 115, 55, &haoke_demo_img_n55},
	{320, 113, 56, &haoke_demo_img_n56}, {326, 112, 57, &haoke_demo_img_n57}, {333, 110, 58, &haoke_demo_img_n58}, {339, 109, 59, &haoke_demo_img_n59},
	{345, 108, 60, &haoke_demo_img_n60}, {351, 108, 61, &haoke_demo_img_n61}, {357, 107, 62, &haoke_demo_img_n62}, {362, 107, 63, &haoke_demo_img_n63},
	{368, 107, 64, &haoke_demo_img_n64}, {373, 108, 65, &haoke_demo_img_n65}, {379, 108, 66, &haoke_demo_img_n66}, {384, 109, 67, &haoke_demo_img_n67},
	{389, 110, 68, &haoke_demo_img_n68}, {393, 111, 69, &haoke_demo_img_n69}, {398, 112, 70, &haoke_demo_img_n70}, {403, 114, 71, &haoke_demo_img_n71},
	{407, 116, 72, &haoke_demo_img_n72}, {412, 117, 73, &haoke_demo_img_n73}, {416, 120, 74, &haoke_demo_img_n74}, {420, 122, 75, &haoke_demo_img_n75},
	{425, 125, 76, &haoke_demo_img_n76}, {429, 127, 77, &haoke_demo_img_n77}, {433, 130, 78, &haoke_demo_img_n78}, {437, 133, 79, &haoke_demo_img_n79},
	{441, 137, 80, &haoke_demo_img_n80},
};

static int needles_num;
static int needle_dir = 0;
static int needle_inx = 0;

static lv_obj_t *bg_img;
static lv_obj_t *batlevel_img;
static lv_obj_t *needle_img;
static lv_obj_t *snt_img;
static lv_obj_t *snu_img;
static lv_obj_t *gear_img;
static lv_obj_t *left_img;
static lv_obj_t *right_img;
static lv_obj_t *model_bg_img;
#define RADIO_BTN_NUM	6
static lv_obj_t *radio_btns[RADIO_BTN_NUM];
static lv_obj_t *radio_freq_label;
static lv_obj_t *pressed_radio_btn;

static lv_ll_t xd_img_animate_ll;

/*********************
 *      FUNCTIONS
 *********************/

static void haoke_demo_img_refresh(lv_obj_t *img, const void **src_img, int src_type, uint32_t num, uint32_t period)
{
	xd_img_animate_t *imgani = _lv_ll_ins_tail(&xd_img_animate_ll);

	if (!imgani)
		return;

	imgani->obj = img;
	imgani->src_img = src_img;
	imgani->src_type = src_type;
	imgani->num = num;
	imgani->period = period;
	imgani->last_tick = lv_tick_get();
	imgani->index = 0;
}

static void haoke_demo_timer_task(lv_task_t * task)
{
	static uint32_t radio_btn_tick;
	static uint32_t radio_btn_inx = 0;
	static uint32_t df_tick;
	uint32_t tick = lv_tick_get();

	xd_img_animate_t *imgani;
	imgani = _lv_ll_get_head(&xd_img_animate_ll);
    while(imgani) {
		if (tick - imgani->last_tick > imgani->period) {
			if (imgani->src_type == LV_IMG_SRC_FILE) {
				char imgsrc[32] = IMAGES_PATH;
				strcat(imgsrc, imgani->src_img[imgani->index]);
				lv_img_set_src(imgani->obj, imgsrc);
			} else if (imgani->src_type == LV_IMG_SRC_VARIABLE) {
				lv_img_set_src(imgani->obj, imgani->src_img[imgani->index]);
			}
			
			imgani->index = (imgani->index + 1)	% imgani->num;
			imgani->last_tick = tick;
		}
        imgani = _lv_ll_get_next(&xd_img_animate_ll, imgani);
	}

	if (needle_img) {
		if (!needle_dir) {
			needle_inx = needle_inx + 1;
			if (needle_inx == needles_num - 1)
				needle_dir = 1;
		} else {
			needle_inx = needle_inx - 1;
			if (needle_inx == 0)
				needle_dir = 0;
		}
		lv_obj_set_pos(needle_img, needles[needle_inx].x, needles[needle_inx].y);
		lv_img_set_src(needle_img, needles[needle_inx].src_img);
		lv_img_set_src(snt_img, num_src_img[needles[needle_inx].speed / 10]);
		lv_img_set_src(snu_img, num_src_img[needles[needle_inx].speed % 10]);
	}

	if (tick - radio_btn_tick > 2333) {
		radio_btn_inx = (radio_btn_inx + 1) % RADIO_BTN_NUM;
		lv_imgbtn_set_state(pressed_radio_btn, LV_BTN_STATE_RELEASED);
		lv_imgbtn_set_state(radio_btns[radio_btn_inx], LV_BTN_STATE_PRESSED);
		pressed_radio_btn = radio_btns[radio_btn_inx];
		char radio_freq[16];
		strncpy(radio_freq, radio_freq_text[radio_btn_inx], 16);
		strcat(radio_freq, "MHz");
		lv_label_set_text(radio_freq_label, radio_freq);
		radio_btn_tick = tick;
	}

	if (tick - df_tick > 1000) {
		lv_obj_set_hidden(left_img, !lv_obj_get_hidden(left_img));
		lv_obj_set_hidden(right_img, !lv_obj_get_hidden(right_img));
		df_tick = tick;
	}	
}

static lv_obj_t * haoke_demo_create_img(lv_obj_t *par, lv_coord_t x, 
												lv_coord_t y, const void * src_img)
{
	lv_obj_t *img = lv_img_create(par, NULL);

	if (img) {
		lv_obj_set_pos(img, x, y);
		lv_img_set_src(img, src_img);
	}

	return img;
}

static lv_obj_t * haoke_demo_create_label(lv_obj_t *par, lv_coord_t x, lv_coord_t y, 
												 const char * text, lv_color_t color, const lv_font_t *font)
{
	lv_obj_t *label = lv_label_create(par, NULL);

	if (label) {
		lv_obj_set_pos(label, x, y);
		lv_label_set_text(label, text);
		lv_obj_set_style_local_text_color(label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color);
		lv_obj_set_style_local_text_font(label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, font);
	}

	return label;
}		

static lv_obj_t * haoke_demo_create_imgbtn(lv_obj_t *par, const void *nomal_img, const void *press_img)
{
	lv_obj_t *imgbtn = lv_imgbtn_create(par, NULL);

	if (imgbtn) {
		lv_imgbtn_set_src(imgbtn, LV_BTN_STATE_RELEASED, nomal_img);
		lv_imgbtn_set_src(imgbtn, LV_BTN_STATE_PRESSED, press_img);
	}

	return imgbtn;
}

static void haoke_demo_imgbtn_set_property(lv_obj_t *imgbtn, lv_coord_t x, lv_coord_t y, const char *text)
{
	if (imgbtn) {
		lv_obj_set_pos(imgbtn, x, y);
		if (text) {
			lv_obj_t *label = lv_label_create(imgbtn, NULL);
			if (!label) return;
			lv_label_set_text(label, text);
			lv_obj_set_style_local_text_color(label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
			lv_obj_set_style_local_text_font(label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_THEME_DEFAULT_FONT_SUBTITLE);
		}
	}
}

static void haoke_demo_radio_model_open(void)
{
	haoke_demo_create_img(bg_img, 643, 259, "radio_logo.png");
	haoke_demo_create_label(bg_img, 850, 195, "AF", LV_COLOR_WHITE, LV_THEME_DEFAULT_FONT_SMALL);
	haoke_demo_create_label(bg_img, 901, 195, "ST", LV_COLOR_WHITE, LV_THEME_DEFAULT_FONT_SMALL);
	haoke_demo_create_label(bg_img, 950, 195, "TA", LV_COLOR_WHITE, LV_THEME_DEFAULT_FONT_SMALL);
	for (int i = 0; i < RADIO_BTN_NUM; i++) 
		radio_btns[i] = haoke_demo_create_imgbtn(bg_img, "amsn.png", "amsp.png");
	haoke_demo_imgbtn_set_property(radio_btns[0], 642, 471, radio_freq_text[0]);
	haoke_demo_imgbtn_set_property(radio_btns[1], 762, 471, radio_freq_text[1]);
	haoke_demo_imgbtn_set_property(radio_btns[2], 882, 471, radio_freq_text[2]);
	haoke_demo_imgbtn_set_property(radio_btns[3], 642, 541, radio_freq_text[3]);
	haoke_demo_imgbtn_set_property(radio_btns[4], 762, 541, radio_freq_text[4]);
	haoke_demo_imgbtn_set_property(radio_btns[5], 882, 541, radio_freq_text[5]);
	lv_imgbtn_set_state(radio_btns[0], LV_BTN_STATE_PRESSED);
	pressed_radio_btn = radio_btns[0];
	char radio_freq[16];
	strncpy(radio_freq, radio_freq_text[0], 16);
	strcat(radio_freq, "MHz");
	radio_freq_label = haoke_demo_create_label(bg_img, 834, 278, radio_freq, LV_COLOR_WHITE, LV_THEME_DEFAULT_FONT_TITLE);
}

#ifdef REVERSE_UI
static lv_obj_t * carback_scr = NULL;
static lv_obj_t * back_scr = NULL;
#endif

#ifdef REVERSE_TRACK
#define REVERSE_TRACK_NUM	113
static int cur_reverse_track = 0;
void set_reverse_track_index(int index)
{
	cur_reverse_track = index;
}

int get_reverse_track_index(void)
{
	return cur_reverse_track;
}
#endif

static void carkback_task(lv_task_t * task)
{
	if (get_carback_status()) {
#ifdef REVERSE_UI
		if (back_scr == NULL) {
			back_scr = lv_scr_act();
			lv_scr_load(carback_scr);
		}
#endif
#ifdef REVERSE_TRACK
		static int index = 0;
		static int dir = 1;

		set_reverse_track_index(index);
		if (dir) {
			if (++index == REVERSE_TRACK_NUM - 1)
				dir = 0;
		} else if (!dir) {
			if (--index == 0)
				dir = 1;
		}
#endif
	} else {
#ifdef REVERSE_UI
		if (back_scr) {
			lv_scr_load(back_scr);
			back_scr = NULL;
		}
#endif
	}
}

#ifdef REVERSE_UI
int is_carback_scr_active(void)
{
	return back_scr != NULL;
}
#endif

void haoke_demo(void)
{
	/* set transparent background color */
#if LCD_BPP == 32
	lv_color_t transp_color;
	transp_color.full = 0;
	lv_disp_set_bg_color(NULL, transp_color);
	lv_disp_set_bg_opa(NULL, LV_OPA_COVER);
#endif

	_lv_ll_init(&xd_img_animate_ll, sizeof(xd_img_animate_t));
	needles_num = sizeof(needles) / sizeof(needles[0]);

	lv_obj_t * scr = lv_obj_create(NULL, NULL);
	lv_obj_set_style_local_bg_color(scr, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_scr_load(scr);

	bg_img = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_src(bg_img, "bg.png");

	haoke_demo_create_img(bg_img, 321, 319, "ready.png");
	haoke_demo_create_img(bg_img, 502, 143, "battery.png");
	batlevel_img = haoke_demo_create_img(bg_img, 540, 178, "bat10.png");
	haoke_demo_img_refresh(batlevel_img, bat_src_img, LV_IMG_SRC_VARIABLE, 11, 5000);
	needle_img = haoke_demo_create_img(bg_img, 210, 369, &haoke_demo_img_n0);
	snt_img = haoke_demo_create_img(bg_img, 263, 220, &haoke_demo_img_0);
	snu_img = haoke_demo_create_img(bg_img, 366, 220, &haoke_demo_img_0);

	gear_img = haoke_demo_create_img(bg_img, 9, 80, "gear_n.png");
	haoke_demo_img_refresh(gear_img, gear_src_img, LV_IMG_SRC_VARIABLE, 7, 3333);
	haoke_demo_create_img(bg_img, 8, 168, "vol.png");
	haoke_demo_create_img(bg_img, 13, 277, "usb.png");
	haoke_demo_create_img(bg_img, 21, 365, "bt.png");

	left_img = haoke_demo_create_img(lv_scr_act(), 6, 0, "left.png");
	right_img = haoke_demo_create_img(lv_scr_act(), 952, 0, "right.png");

	haoke_demo_create_img(bg_img, 639, 0, "dhd.png");
	haoke_demo_create_img(bg_img, 725, 0, "fhd.png");
	haoke_demo_create_img(bg_img, 796, 0, "ffl.png");
	haoke_demo_create_img(bg_img, 874, 0, "oml.png");
	haoke_demo_create_img(bg_img, 627, 79, "cruise.png");
	haoke_demo_create_img(bg_img, 694, 79, "wecu.png");
	haoke_demo_create_img(bg_img, 762, 79, "wcon.png");
	haoke_demo_create_img(bg_img, 827, 79, "wbrake.png");
	haoke_demo_create_img(bg_img, 895, 79, "wp.png");
	haoke_demo_create_img(bg_img, 961, 79, "wturn.png");

	model_bg_img = haoke_demo_create_img(bg_img, 14, 545, "model_bg.png");
	haoke_demo_create_img(model_bg_img, 23, 0, "model_radio.png");
	haoke_demo_create_label(model_bg_img, 70, 10, MODEL_RADIO_TEXT, LV_COLOR_WHITE,
		LV_THEME_DEFAULT_FONT_NORMAL);
	haoke_demo_radio_model_open();

	lv_task_create(haoke_demo_timer_task, 50, LV_TASK_PRIO_HIGH, NULL);
#ifdef REVERSE_UI
	carback_scr = lv_obj_create(NULL, NULL);
	lv_obj_set_style_local_bg_opa(carback_scr, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,
			LV_OPA_TRANSP);
	haoke_demo_create_img(carback_scr, 732, 30, "cruise.png");
	haoke_demo_create_img(carback_scr, 732, 100, "wecu.png");
	haoke_demo_create_img(carback_scr, 732, 170, "wcon.png");
	haoke_demo_create_img(carback_scr, 732, 240, "wbrake.png");
	haoke_demo_create_img(carback_scr, 732, 310, "wp.png");
	haoke_demo_create_img(carback_scr, 732, 380, "wturn.png");
#endif
	carkback_task(NULL);
	lv_task_create(carkback_task, 10, LV_TASK_PRIO_HIGHEST, NULL);
}

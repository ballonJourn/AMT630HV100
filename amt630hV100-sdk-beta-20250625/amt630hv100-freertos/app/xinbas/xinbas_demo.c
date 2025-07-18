
#if !defined(VG_ONLY) && !defined(AWTK)

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

typedef struct {
	lv_obj_t *obj;
	const void **src_img;
	int	src_type;
	uint32_t num;
	uint32_t period;
	uint32_t last_tick;
	uint32_t index;
} xd_img_animate_t;

static const void *wt_img_name[] = {
	"wt1.png", "wt2.png", "wt3.png", "wt4.png", "wt5.png", 
	"wt6.png", "wt7.png", "wt8.png", "wt9.png",
};

static const void *wtnt_img_name[] = {
	"wtn0.png", "wtn1.png", "wtn2.png", "wtn3.png", "wtn4.png", 
	"wtn5.png", "wtn6.png", "wtn7.png", "wtn8.png", "wtn9.png"
};

static const void *rs_src_img[] = {
	"rs1.png", "rs2.png", "rs3.png", "rs4.png", "rs5.png",
	"rs6.png", "rs7.png", "rs8.png", "rs9.png", "rs10.png",
	"rs11.png"
};

static const void *fc_src_img[] = {
	"fc9.png", "fc8.png", "fc7.png", "fc6.png", "fc5.png",
	"fc4.png", "fc3.png", "fc2.png", "fc1.png"
};

static const void *fcnt_src_img[] = {
	"wtn9.png", "wtn8.png", "wtn7.png", "wtn6.png", "wtn5.png", 
	"wtn4.png", "wtn3.png", "wtn2.png", "wtn1.png", "wtn0.png"
};

static lv_obj_t *bg_img;
static lv_obj_t *wt_img;
static lv_obj_t *wtnt_img;
static lv_obj_t *rs_img;
static lv_obj_t *fc_img;
static lv_obj_t *fcnt_img;
static lv_obj_t *left_img;
static lv_obj_t *right_img;

#if defined(VG_DRIVER) && !defined(LVGL_VG_GPU)
static lv_obj_t *dashboard;
static int dashboard_speed = 0;
#endif


static lv_ll_t xd_img_animate_ll;

/*********************
 *      FUNCTIONS
 *********************/

static void xinbas_demo_img_refresh(lv_obj_t *img, const void **src_img, int src_type, uint32_t num, uint32_t period)
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

static void xinbas_demo_timer_task(lv_task_t * task)
{
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

	if (tick - df_tick > 1000) {
		lv_obj_set_hidden(left_img, !lv_obj_get_hidden(left_img));
		lv_obj_set_hidden(right_img, !lv_obj_get_hidden(right_img));
		df_tick = tick;
	}

#if defined(VG_DRIVER) && !defined(LVGL_VG_GPU)
	static int dir = 1;

	lv_openvg_set_param(dashboard, &dashboard_speed);
	if (dir && ++dashboard_speed == 360)
		dir = 0;
	else if (!dir && --dashboard_speed == 0)
		dir = 1;
#endif
}

static lv_obj_t * xinbas_demo_create_img(lv_obj_t *par, lv_coord_t x, 
												lv_coord_t y, const void * src_img)
{
	lv_obj_t *img = lv_img_create(par, NULL);

	if (img) {
		lv_obj_set_pos(img, x, y);
		lv_img_set_src(img, src_img);
	}

	return img;
}

void xinbas_demo(void)
{
	_lv_ll_init(&xd_img_animate_ll, sizeof(xd_img_animate_t));

	lv_obj_t * scr = lv_obj_create(NULL, NULL);
    lv_scr_load(scr);

	bg_img = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_src(bg_img, "xb_bg.png");
	
#if defined(VG_DRIVER) && !defined(LVGL_VG_GPU)
	dashboard = lv_openvg_create(lv_scr_act(), NULL);
	lv_obj_set_pos(dashboard, xm_vg_get_offset_x(), xm_vg_get_offset_y());
	lv_obj_set_size(dashboard, xm_vg_get_width(), xm_vg_get_height());
	lv_openvg_set_param(dashboard, &dashboard_speed);
#endif

	wt_img = xinbas_demo_create_img(bg_img, 50, 123, "wt1.png");
	xinbas_demo_img_refresh(wt_img, wt_img_name, LV_IMG_SRC_VARIABLE, 9, 2000);
	xinbas_demo_create_img(bg_img, 130, 156, "wti.png");
	wtnt_img = xinbas_demo_create_img(bg_img, 118, 193, "wtn1.png");
	xinbas_demo_img_refresh(wtnt_img, wtnt_img_name, LV_IMG_SRC_VARIABLE, 9, 2000);
	xinbas_demo_create_img(bg_img, 135, 193, "wtn0.png");
	xinbas_demo_create_img(bg_img, 155, 194, "wtt.png");
	rs_img = xinbas_demo_create_img(bg_img, 43, 262, "rs1.png");
	xinbas_demo_img_refresh(rs_img, rs_src_img, LV_IMG_SRC_VARIABLE, 11, 500);
	xinbas_demo_create_img(bg_img, 121, 365, "rss.png");
	fc_img = xinbas_demo_create_img(bg_img, 908, 125, "fc9.png");
	xinbas_demo_img_refresh(fc_img, fc_src_img, LV_IMG_SRC_VARIABLE, 9, 5000);
	xinbas_demo_create_img(bg_img, 842, 252, "fci.png");
	fcnt_img = xinbas_demo_create_img(bg_img, 824, 291, "wtn9.png");
	xinbas_demo_img_refresh(fcnt_img, fcnt_src_img, LV_IMG_SRC_VARIABLE, 9, 5000);
	xinbas_demo_create_img(bg_img, 841, 291, "wtn0.png");
	xinbas_demo_create_img(bg_img, 858, 291, "fcp.png");
	//xinbas_demo_create_img(bg_img, 482, 359, "kmh.png");
	//xinbas_demo_create_img(bg_img, 486, 476, "gear6.png");
	left_img = xinbas_demo_create_img(bg_img, 68, 11, "xb_left.png");
	xinbas_demo_create_img(bg_img, 216, 21, "xb_dhd.png");
	xinbas_demo_create_img(bg_img, 374, 22, "xb_oml.png");
	xinbas_demo_create_img(bg_img, 495, 15, "brl.png");
	xinbas_demo_create_img(bg_img, 616, 22, "xb_ffl.png");
	xinbas_demo_create_img(bg_img, 768, 21, "xb_fhd.png");
	right_img = xinbas_demo_create_img(bg_img, 890, 11, "xb_right.png");
	xinbas_demo_create_img(bg_img, 33, 506, "tripa.png");
	xinbas_demo_create_img(bg_img, 764, 506,"tripb.png");
	xinbas_demo_create_img(bg_img, 28, 554, "time.png");
	xinbas_demo_create_img(bg_img, 265, 554, "enw.png");
	xinbas_demo_create_img(bg_img, 345, 554, "eow.png");
	xinbas_demo_create_img(bg_img, 442, 554, "abs.png");
	xinbas_demo_create_img(bg_img, 518, 556, "fcw.png");
	xinbas_demo_create_img(bg_img, 574, 554, "wtw.png");
	xinbas_demo_create_img(bg_img, 647, 554, "warn.png");
	xinbas_demo_create_img(bg_img, 769, 557, "odo.png");

	lv_task_create(xinbas_demo_timer_task, 20, LV_TASK_PRIO_HIGHEST, NULL);
}

#endif

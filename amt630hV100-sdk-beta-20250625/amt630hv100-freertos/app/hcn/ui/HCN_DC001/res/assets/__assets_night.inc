#include "awtk.h"
#include "base/assets_manager.h"
#ifndef WITH_FS_RES
extern TK_CONST_DATA_ALIGN(const unsigned char ui_home_page[]);
extern TK_CONST_DATA_ALIGN(const unsigned char ui_link_page[]);
extern TK_CONST_DATA_ALIGN(const unsigned char style_default[]);
#ifdef WITH_STB_IMAGE
extern TK_CONST_DATA_ALIGN(const unsigned char image_bg_center[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_bg_cricle[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_bg_halo_bule[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_bg_halo_green[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_bg_halo_red[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_level_100_[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_level_10_[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_level_25_[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_level_50_[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_level_75_[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_pointer[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_progress_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_box_power_progress_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dashboard_progress_e[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dashboard_progress_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dashboard_progress_s[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_bluetooth_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_bluetooth_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_instrument_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_instrument_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_music_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_music_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_navi_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_navi_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_settings_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_dock_settings_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_E[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_gear_d_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_gear_d_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_gear_n_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_gear_n_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_gear_r_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_gear_r_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_icon_answer[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_icon_incoming_call[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_icon_media_cover_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_icon_riding_distance[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_icon_riding_time[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_interconnection_bg[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_left_bg_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_left_bg_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_N[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_pointer_bule[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_pointer_green[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_pointer_red[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_0[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_1[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_2[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_3[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_4[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_5[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_6[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_7[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_8[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_9[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_bg[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_level_bg[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_level_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_level_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_pointer[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_power_progress[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_progress_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_progress_p[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_S[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_0[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_1[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_2[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_3[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_4[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_5[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_6[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_7[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_8[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_speed_9[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_abs_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_auto_high_beams_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_bluetooth_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_brake_failure_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_charge_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_ecu_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_gps_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_high_beam_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_high_temperature_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_left[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_left_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_lock_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_low_beam_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_low_temperature_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_motor_failure_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_ready[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_ready_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_right[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_right_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_signal_0_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_signal_1_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_signal_2_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_signal_3_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_system_failure_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_tcs_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_top_tire_pressure_n[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_unit_km_h[]);
extern TK_CONST_DATA_ALIGN(const unsigned char image_wallpaper[]);
#else
#endif /*WITH_STB_IMAGE*/
#ifdef WITH_VGCANVAS
#endif /*WITH_VGCANVAS*/
#ifdef WITH_TRUETYPE_FONT
extern TK_CONST_DATA_ALIGN(const unsigned char font_default[]);
#else /*WITH_TRUETYPE_FONT*/
#endif /*WITH_TRUETYPE_FONT*/
#endif /*WITH_FS_RES*/

ret_t assets_init_night(void) {
  assets_manager_t* am = assets_manager();

#ifdef WITH_FS_RES
  assets_manager_preload(am, ASSET_TYPE_STYLE, "default");
#else
  assets_manager_add(am, ui_home_page);
  assets_manager_add(am, ui_link_page);
  assets_manager_add(am, style_default);
  assets_manager_add(am, image_bg_center);
  assets_manager_add(am, image_bg_cricle);
  assets_manager_add(am, image_bg_halo_bule);
  assets_manager_add(am, image_bg_halo_green);
  assets_manager_add(am, image_bg_halo_red);
  assets_manager_add(am, image_box_power_level_100_);
  assets_manager_add(am, image_box_power_level_10_);
  assets_manager_add(am, image_box_power_level_25_);
  assets_manager_add(am, image_box_power_level_50_);
  assets_manager_add(am, image_box_power_level_75_);
  assets_manager_add(am, image_box_power_pointer);
  assets_manager_add(am, image_box_power_progress_n);
  assets_manager_add(am, image_box_power_progress_p);
  assets_manager_add(am, image_dashboard_progress_e);
  assets_manager_add(am, image_dashboard_progress_n);
  assets_manager_add(am, image_dashboard_progress_s);
  assets_manager_add(am, image_dock_bluetooth_n);
  assets_manager_add(am, image_dock_bluetooth_p);
  assets_manager_add(am, image_dock_instrument_n);
  assets_manager_add(am, image_dock_instrument_p);
  assets_manager_add(am, image_dock_music_n);
  assets_manager_add(am, image_dock_music_p);
  assets_manager_add(am, image_dock_navi_n);
  assets_manager_add(am, image_dock_navi_p);
  assets_manager_add(am, image_dock_settings_n);
  assets_manager_add(am, image_dock_settings_p);
  assets_manager_add(am, image_E);
  assets_manager_add(am, image_gear_d_n);
  assets_manager_add(am, image_gear_d_p);
  assets_manager_add(am, image_gear_n_n);
  assets_manager_add(am, image_gear_n_p);
  assets_manager_add(am, image_gear_r_n);
  assets_manager_add(am, image_gear_r_p);
  assets_manager_add(am, image_icon_answer);
  assets_manager_add(am, image_icon_incoming_call);
  assets_manager_add(am, image_icon_media_cover_n);
  assets_manager_add(am, image_icon_riding_distance);
  assets_manager_add(am, image_icon_riding_time);
  assets_manager_add(am, image_interconnection_bg);
  assets_manager_add(am, image_left_bg_n);
  assets_manager_add(am, image_left_bg_p);
  assets_manager_add(am, image_N);
  assets_manager_add(am, image_pointer_bule);
  assets_manager_add(am, image_pointer_green);
  assets_manager_add(am, image_pointer_red);
  assets_manager_add(am, image_power_0);
  assets_manager_add(am, image_power_1);
  assets_manager_add(am, image_power_2);
  assets_manager_add(am, image_power_3);
  assets_manager_add(am, image_power_4);
  assets_manager_add(am, image_power_5);
  assets_manager_add(am, image_power_6);
  assets_manager_add(am, image_power_7);
  assets_manager_add(am, image_power_8);
  assets_manager_add(am, image_power_9);
  assets_manager_add(am, image_power_bg);
  assets_manager_add(am, image_power_level_bg);
  assets_manager_add(am, image_power_level_n);
  assets_manager_add(am, image_power_level_p);
  assets_manager_add(am, image_power_pointer);
  assets_manager_add(am, image_power_progress);
  assets_manager_add(am, image_progress_n);
  assets_manager_add(am, image_progress_p);
  assets_manager_add(am, image_S);
  assets_manager_add(am, image_speed_0);
  assets_manager_add(am, image_speed_1);
  assets_manager_add(am, image_speed_2);
  assets_manager_add(am, image_speed_3);
  assets_manager_add(am, image_speed_4);
  assets_manager_add(am, image_speed_5);
  assets_manager_add(am, image_speed_6);
  assets_manager_add(am, image_speed_7);
  assets_manager_add(am, image_speed_8);
  assets_manager_add(am, image_speed_9);
  assets_manager_add(am, image_top_abs_n);
  assets_manager_add(am, image_top_auto_high_beams_n);
  assets_manager_add(am, image_top_bluetooth_n);
  assets_manager_add(am, image_top_brake_failure_n);
  assets_manager_add(am, image_top_charge_n);
  assets_manager_add(am, image_top_ecu_n);
  assets_manager_add(am, image_top_gps_n);
  assets_manager_add(am, image_top_high_beam_n);
  assets_manager_add(am, image_top_high_temperature_n);
  assets_manager_add(am, image_top_left);
  assets_manager_add(am, image_top_left_n);
  assets_manager_add(am, image_top_lock_n);
  assets_manager_add(am, image_top_low_beam_n);
  assets_manager_add(am, image_top_low_temperature_n);
  assets_manager_add(am, image_top_motor_failure_n);
  assets_manager_add(am, image_top_ready);
  assets_manager_add(am, image_top_ready_n);
  assets_manager_add(am, image_top_right);
  assets_manager_add(am, image_top_right_n);
  assets_manager_add(am, image_top_signal_0_n);
  assets_manager_add(am, image_top_signal_1_n);
  assets_manager_add(am, image_top_signal_2_n);
  assets_manager_add(am, image_top_signal_3_n);
  assets_manager_add(am, image_top_system_failure_n);
  assets_manager_add(am, image_top_tcs_n);
  assets_manager_add(am, image_top_tire_pressure_n);
  assets_manager_add(am, image_unit_km_h);
  assets_manager_add(am, image_wallpaper);
#ifdef WITH_VGCANVAS
#endif /*WITH_VGCANVAS*/
#ifdef WITH_TRUETYPE_FONT
  assets_manager_add(am, font_default);
#else /*WITH_TRUETYPE_FONT*/
#endif /*WITH_TRUETYPE_FONT*/
#endif

  tk_init_assets();
  return RET_OK;
}

#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "data/data_hub.h"
#include <lvgl.h>

static lv_obj_t *s_splash   = nullptr;
static lv_obj_t *s_wells    = nullptr;
static lv_obj_t *s_well     = nullptr;
static lv_obj_t *s_history  = nullptr;
static lv_obj_t *s_settings = nullptr;
static lv_obj_t *s_help     = nullptr;

void ui_init() {
	theme_init();

	s_wells    = screen_wells_create();
	s_well     = screen_well_create();
	s_history  = screen_history_create();
	s_settings = screen_settings_create();
	s_help     = screen_help_create();
	s_splash   = screen_splash_create();

	lv_scr_load(s_splash);
}

void ui_show_wells() {
	lv_scr_load_anim(s_wells, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, false);
}

void ui_show_well(uint8_t idx) {
	DataHub::instance().setSelectedWell(idx);
	screen_well_update();
	lv_scr_load_anim(s_well, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_show_history() {
	screen_history_update();
	lv_scr_load_anim(s_history, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_show_settings() {
	lv_scr_load_anim(s_settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_show_help() {
	lv_scr_load_anim(s_help, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_tick() {
	lv_obj_t *act = lv_scr_act();
	if (act == s_wells)         screen_wells_update();
	else if (act == s_well)     screen_well_update();
	else if (act == s_history)  screen_history_update();
	else if (act == s_settings) screen_settings_update();
}

#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "data/data_hub.h"
#include <lvgl.h>

static lv_obj_t *s_splash   = nullptr;
static lv_obj_t *s_wells    = nullptr;
static lv_obj_t *s_well     = nullptr;
static lv_obj_t *s_actions  = nullptr;
static lv_obj_t *s_history  = nullptr;
static lv_obj_t *s_settings = nullptr;
static lv_obj_t *s_pin      = nullptr;
static lv_obj_t *s_config   = nullptr;
static lv_obj_t *s_help     = nullptr;

/* Las pantallas se crean la primera vez que se navegan, no todas al arrancar:
   asi el pool de LVGL no se agota en ui_init() (antes petaba en screen_help). */

void ui_init() {
	/* el tema ya se fijo en main con theme_init(mode) antes de llamar aqui */
	s_wells  = screen_wells_create();
	s_splash = screen_splash_create();

	lv_scr_load(s_splash);
}

void ui_show_wells() {
	if (!s_wells) s_wells = screen_wells_create();
	screen_wells_update();
	lv_scr_load_anim(s_wells, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, false);
}

void ui_show_well(uint8_t idx) {
	if (!s_well) s_well = screen_well_create();
	DataHub::instance().setSelectedWell(idx);
	screen_well_update();
	lv_scr_load_anim(s_well, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_show_actions() {
	if (!s_actions) s_actions = screen_actions_create();
	screen_actions_update();
	lv_scr_load_anim(s_actions, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_show_history() {
	if (!s_history) s_history = screen_history_create();
	screen_history_update();
	lv_scr_load_anim(s_history, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_show_settings() {
	if (!s_settings) s_settings = screen_settings_create();
	lv_scr_load_anim(s_settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_show_pin(void (*on_ok)()) {
	if (!s_pin) s_pin = screen_pin_create();
	screen_pin_prepare(on_ok);
	lv_scr_load_anim(s_pin, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

void ui_show_config() {
	if (!s_config) s_config = screen_config_create();
	screen_config_enter();
	lv_scr_load_anim(s_config, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

void ui_show_help() {
	if (!s_help) s_help = screen_help_create();
	lv_scr_load_anim(s_help, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

void ui_tick() {
	lv_obj_t *act = lv_scr_act();
	if (act == s_wells)         screen_wells_update();
	else if (act == s_well)     screen_well_update();
	else if (act == s_actions)  screen_actions_update();
	else if (act == s_history)  screen_history_update();
	else if (act == s_settings) screen_settings_update();
}

void ui_rebuild_all() {
	lv_obj_t *old = lv_scr_act();

	enum { W_WELLS, W_WELL, W_ACTIONS, W_HISTORY, W_SETTINGS,
	       W_PIN, W_CONFIG, W_HELP, W_SPLASH } which = W_WELLS;
	if      (old == s_well)     which = W_WELL;
	else if (old == s_actions)  which = W_ACTIONS;
	else if (old == s_history)  which = W_HISTORY;
	else if (old == s_settings) which = W_SETTINGS;
	else if (old == s_pin)      which = W_PIN;
	else if (old == s_config)   which = W_CONFIG;
	else if (old == s_help)     which = W_HELP;
	else if (old == s_splash)   which = W_SPLASH;

	/* invalida punteros y borra todo lo creado salvo la pantalla activa */
	lv_obj_t *all[] = { s_wells, s_well, s_actions, s_history, s_settings,
	                    s_pin, s_config, s_help, s_splash };
	s_wells = s_well = s_actions = s_history = s_settings = nullptr;
	s_pin = s_config = s_help = s_splash = nullptr;
	for (lv_obj_t *o : all)
		if (o && o != old) lv_obj_del(o);

	/* recrea la que estaba activa y cargala SIN animacion (para poder borrar
	   'old' de inmediato: una animacion seguiria referenciandolo) */
	lv_obj_t *ns = nullptr;
	switch (which) {
		case W_WELL:     s_well     = ns = screen_well_create();    screen_well_update();     break;
		case W_ACTIONS:  s_actions  = ns = screen_actions_create(); screen_actions_update();  break;
		case W_HISTORY:  s_history  = ns = screen_history_create(); screen_history_update();  break;
		case W_SETTINGS: s_settings = ns = screen_settings_create();                          break;
		case W_CONFIG:   s_config   = ns = screen_config_create();  screen_config_enter();    break;
		case W_HELP:     s_help     = ns = screen_help_create();                              break;
		case W_SPLASH:   s_splash   = ns = screen_splash_create();                            break;
		case W_PIN:      /* no se reabre el PIN: volver a Ajustes */
		default:         s_wells    = ns = screen_wells_create();   screen_wells_update();    break;
	}

	lv_scr_load(ns);
	lv_obj_del(old);
}

#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include "hal/touch.h"
#include <lvgl.h>

static lv_obj_t *lbl_counters;
static lv_obj_t *lbl_active;

static void on_back(lv_event_t *e) { (void)e; ui_show_wells(); }
static void on_help(lv_event_t *e) { (void)e; ui_show_help(); }

static void on_recal(lv_event_t *e) {
	(void)e;
	touch_force_calibrate();            /* dibuja directamente con TFT_eSPI */
	lv_obj_invalidate(lv_scr_act());    /* forzar redibujado completo de LVGL */
}

static lv_obj_t *row(lv_obj_t *parent, const char *k, const char *v, lv_coord_t y) {
	lv_obj_t *lk = lv_label_create(parent);
	lv_label_set_text(lk, k);
	lv_obj_set_style_text_font(lk, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(lk, COL_MUTED, 0);
	lv_obj_align(lk, LV_ALIGN_TOP_LEFT, 12, y);

	lv_obj_t *lv = lv_label_create(parent);
	lv_label_set_text(lv, v);
	lv_obj_set_style_text_font(lv, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(lv, COL_TEXT, 0);
	lv_obj_align(lv, LV_ALIGN_TOP_LEFT, 170, y);
	return lv;
}

lv_obj_t *screen_settings_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *title = lv_label_create(scr);
	lv_label_set_text(title, "Ajustes / Diagnostico");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(title, COL_TEXT, 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 8);

	char buf[40];
	snprintf(buf, sizeof(buf), "%s:%d", PLC_HOST, PLC_PORT);
	row(scr, "PLC (Modbus TCP)", buf, 34);
	row(scr, "WiFi de planta", WIFI_SSID[0] ? WIFI_SSID : "sin configurar", 54);
	row(scr, "Contrato de mapa", "v1 (Mapa B)", 74);
	row(scr, "MQTT", FEATURE_MQTT ? "habilitado" : "deshabilitado", 94);

	lbl_active   = row(scr, "Fuente activa", "--", 114);
	lbl_counters = row(scr, "Tramas OK / ERR", "0 / 0", 134);

	lv_obj_t *brecal = lv_btn_create(scr);
	lv_obj_set_size(brecal, 180, 40);
	lv_obj_align(brecal, LV_ALIGN_TOP_LEFT, 12, 160);
	lv_obj_set_style_bg_color(brecal, COL_ORANGE, 0);
	lv_obj_add_event_cb(brecal, on_recal, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *rl = lv_label_create(brecal);
	lv_label_set_text(rl, "Recalibrar pantalla");
	lv_obj_center(rl);

	lv_obj_t *bhelp = lv_btn_create(scr);
	lv_obj_set_size(bhelp, 110, 40);
	lv_obj_align(bhelp, LV_ALIGN_TOP_LEFT, 200, 160);
	lv_obj_set_style_bg_color(bhelp, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bhelp, on_help, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *hl = lv_label_create(bhelp);
	lv_label_set_text(hl, LV_SYMBOL_LIST " Ayuda");
	lv_obj_center(hl);

	lv_obj_t *ver = lv_label_create(scr);
	lv_label_set_text_fmt(ver, "%s  v%s", APP_NAME, APP_VERSION);
	lv_obj_add_style(ver, &st_title, 0);
	lv_obj_align(ver, LV_ALIGN_BOTTOM_LEFT, 10, -10);

	lv_obj_t *back = lv_btn_create(scr);
	lv_obj_set_size(back, 90, 40);
	lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -8, -6);
	lv_obj_set_style_bg_color(back, COL_TEAL_D, 0);
	lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *bl = lv_label_create(back);
	lv_label_set_text(bl, LV_SYMBOL_LEFT " Volver");
	lv_obj_center(bl);

	return scr;
}

void screen_settings_update() {
	DataHub &hub = DataHub::instance();
	lv_label_set_text(lbl_active, hub.activeSourceName());
	lv_label_set_text_fmt(lbl_counters, "%lu / %lu",
	                      (unsigned long)hub.txOk(), (unsigned long)hub.txErr());
}

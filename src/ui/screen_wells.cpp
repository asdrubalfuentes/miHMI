#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdint.h>

/* Barra superior con margen para que la info no se solape */
#define WELLS_TOP_H   46
#define WELLS_CARD_H  ((SCREEN_H - WELLS_TOP_H - 8 * (NUM_WELLS + 1)) / NUM_WELLS)

static lv_obj_t *lbl_global;
static lv_obj_t *card[NUM_WELLS];
static lv_obj_t *lbl_name[NUM_WELLS];
static lv_obj_t *lbl_state[NUM_WELLS];
static lv_obj_t *lbl_metrics[NUM_WELLS];
static lv_obj_t *lbl_flags[NUM_WELLS];

static void on_card(lv_event_t *e) {
	uint8_t i = (uint8_t)(intptr_t)lv_event_get_user_data(e);
	ui_show_well(i);
}
static void on_help(lv_event_t *e) { (void)e; ui_show_help(); }
static void on_settings(lv_event_t *e) { (void)e; ui_show_settings(); }

lv_obj_t *screen_wells_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	/* --- barra superior --- */
	lv_obj_t *top = lv_obj_create(scr);
	lv_obj_set_pos(top, 0, 0);
	lv_obj_set_size(top, SCREEN_W, WELLS_TOP_H);
	lv_obj_set_style_bg_color(top, COL_CARD, 0);
	lv_obj_set_style_radius(top, 0, 0);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *title = lv_label_create(top);
	lv_label_set_text(title, "ESTACIONES");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(title, COL_TEXT, 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 6);

	lbl_global = lv_label_create(top);
	lv_label_set_text(lbl_global, "--");
	lv_obj_set_style_text_font(lbl_global, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lbl_global, COL_MUTED, 0);
	lv_obj_align(lbl_global, LV_ALIGN_BOTTOM_LEFT, 10, -5);

	lv_obj_t *bset = lv_btn_create(top);
	lv_obj_set_size(bset, 34, 26);
	lv_obj_align(bset, LV_ALIGN_RIGHT_MID, -48, 0);
	lv_obj_set_style_bg_color(bset, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bset, on_settings, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bset));
	lv_label_set_text(lv_obj_get_child(bset, 0), LV_SYMBOL_SETTINGS);

	lv_obj_t *bhelp = lv_btn_create(top);
	lv_obj_set_size(bhelp, 34, 26);
	lv_obj_align(bhelp, LV_ALIGN_RIGHT_MID, -8, 0);
	lv_obj_set_style_bg_color(bhelp, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bhelp, on_help, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bhelp));
	lv_label_set_text(lv_obj_get_child(bhelp, 0), "?");

	/* --- lista de estaciones --- */
	lv_obj_t *list = lv_obj_create(scr);
	lv_obj_set_pos(list, 0, WELLS_TOP_H);
	lv_obj_set_size(list, SCREEN_W, SCREEN_H - WELLS_TOP_H);
	lv_obj_set_style_bg_color(list, COL_BG, 0);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_pad_all(list, 8, 0);
	lv_obj_set_style_pad_row(list, 8, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

	for (uint8_t i = 0; i < NUM_WELLS; i++) {
		lv_obj_t *c = lv_obj_create(list);
		lv_obj_set_size(c, LV_PCT(100), WELLS_CARD_H);
		lv_obj_add_style(c, &st_card, 0);
		lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_event_cb(c, on_card, LV_EVENT_CLICKED, (void *)(intptr_t)i);
		card[i] = c;

		lv_obj_t *n = lv_label_create(c);
		lv_label_set_text(n, "Estacion");
		lv_obj_set_style_text_font(n, &lv_font_montserrat_20, 0);
		lv_obj_set_style_text_color(n, COL_TEXT, 0);
		lv_obj_align(n, LV_ALIGN_TOP_LEFT, 0, 0);
		lbl_name[i] = n;

		lv_obj_t *s = lv_label_create(c);
		lv_label_set_text(s, "--");
		lv_obj_set_style_text_font(s, &lv_font_montserrat_14, 0);
		lv_obj_align(s, LV_ALIGN_TOP_RIGHT, 0, 2);
		lbl_state[i] = s;

		lv_obj_t *m = lv_label_create(c);
		lv_label_set_text(m, "");
		lv_obj_set_style_text_font(m, &lv_font_montserrat_14, 0);
		lv_obj_set_style_text_color(m, COL_MUTED, 0);
		lv_obj_align(m, LV_ALIGN_LEFT_MID, 0, 6);
		lbl_metrics[i] = m;

		lv_obj_t *f = lv_label_create(c);
		lv_label_set_text(f, "");
		lv_obj_set_style_text_font(f, &lv_font_montserrat_12, 0);
		lv_obj_align(f, LV_ALIGN_BOTTOM_LEFT, 0, 0);
		lbl_flags[i] = f;

		lv_obj_t *chev = lv_label_create(c);
		lv_label_set_text(chev, LV_SYMBOL_RIGHT);
		lv_obj_set_style_text_color(chev, COL_MUTED, 0);
		lv_obj_align(chev, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
	}

	return scr;
}

void screen_wells_update() {
	const PlantData &p = DataHub::instance().data();

	lv_label_set_text_fmt(lbl_global, "%s   latido %u  %s",
	                      p.origin ? "LOGO! real" : "PLC-SIM",
	                      p.heartbeat,
	                      p.alarmOr ? "ALARMA activa" : "sin alarmas");
	lv_obj_set_style_text_color(lbl_global, p.alarmOr ? COL_WARN : COL_MUTED, 0);

	for (uint8_t i = 0; i < NUM_WELLS; i++) {
		const WellData &d = p.well[i];
		lv_label_set_text(lbl_name[i], d.name);

		const char *txt; lv_color_t col;
		if (!d.linkOk)        { txt = "SIN ENLACE"; col = COL_STOP; }
		else if (d.inAlarm()) { txt = "ALARMA";     col = COL_WARN; }
		else                  { txt = "OK";         col = COL_RUN;  }
		lv_label_set_text(lbl_state[i], txt);
		lv_obj_set_style_text_color(lbl_state[i], col, 0);

		char m[40];   /* LVGL no formatea %f */
		snprintf(m, sizeof(m), "Nivel %.2f m   Caudal %.1f m3/h",   /* unidades fijas */
		         d.levelEng, d.flowEng);
		lv_label_set_text(lbl_metrics[i], m);

		lv_label_set_text_fmt(lbl_flags[i], "%s  %s  %s  RSSI %d",
		                      d.presostato ? "presion" : "sin presion",
		                      d.sirenOn ? "SIRENA" : (d.sirenAuto ? "sirena:auto" : "sirena:man"),
		                      d.tamper ? "TAPA" : "tapa ok",
		                      d.rssi);
		lv_obj_set_style_text_color(lbl_flags[i], d.sirenOn ? COL_STOP : COL_MUTED, 0);
	}
}

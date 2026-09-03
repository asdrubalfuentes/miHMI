/**
 * screen_scale.cpp  -  Pagina de RANGOS DE ESCALA por estacion.
 *
 * Edita el bloque hb+20..31 del MAPA B (cero/span crudo <-> ingenieria, unidad y
 * filtro, por variable) y lo aplica al PLC (LOGO! 9 / PLC-SIM) escribiendo esos
 * registros y pulsando el coil cb+8. La escala VIVE en el PLC; esto es un editor.
 */
#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>

static const char *UNITS_LEVEL = "%\nm\ncm\nmca";
static const char *UNITS_FLOW  = "L/s\nm3/h\nL/min\nGPM";

static uint8_t      st_  = 0;      /* estacion */
static uint8_t      var_ = 0;      /* 0 = nivel, 1 = caudal */
static StationScale edit_;
static bool         loaded_ = false;

static lv_obj_t *lbl_title, *lbl_status;
static lv_obj_t *ta_rmin, *ta_rmax, *ta_emin, *ta_emax, *ta_filt, *dd_unit;
static lv_obj_t *kb;
static lv_obj_t *btn_lvl, *btn_flw;

static DataSource *plc() { return DataHub::instance().primary(); }
static ScaleVar   &curVar() { return var_ ? edit_.flow : edit_.level; }

/* ---- widgets <-> edit_ ---- */
static void set_ta_int(lv_obj_t *ta, long v)   { char b[16]; snprintf(b, sizeof(b), "%ld", v); lv_textarea_set_text(ta, b); }
static void set_ta_x100(lv_obj_t *ta, int v)   { char b[16]; snprintf(b, sizeof(b), "%.2f", v / 100.0); lv_textarea_set_text(ta, b); }
static long get_ta_int(lv_obj_t *ta)           { return atol(lv_textarea_get_text(ta)); }
static int  get_ta_x100(lv_obj_t *ta)          { return (int)lroundf(atof(lv_textarea_get_text(ta)) * 100.0f); }

static void refresh_fields() {
	ScaleVar &v = curVar();
	set_ta_int(ta_rmin, v.rawMin);
	set_ta_int(ta_rmax, v.rawMax);
	set_ta_x100(ta_emin, v.engMin);
	set_ta_x100(ta_emax, v.engMax);
	set_ta_int(ta_filt, v.filter);
	lv_dropdown_set_options(dd_unit, var_ ? UNITS_FLOW : UNITS_LEVEL);
	lv_dropdown_set_selected(dd_unit, v.unit < 4 ? v.unit : 0);

	lv_obj_set_style_bg_color(btn_lvl, var_ ? COL_CARD : COL_TEAL, 0);
	lv_obj_set_style_bg_color(btn_flw, var_ ? COL_TEAL : COL_CARD, 0);
}

static void commit_fields() {
	ScaleVar &v = curVar();
	v.rawMin = (uint16_t)get_ta_int(ta_rmin);
	v.rawMax = (uint16_t)get_ta_int(ta_rmax);
	v.engMin = (int16_t)get_ta_x100(ta_emin);
	v.engMax = (int16_t)get_ta_x100(ta_emax);
	v.filter = (uint16_t)get_ta_int(ta_filt);
	v.unit   = lv_dropdown_get_selected(dd_unit);
}

/* ---- eventos ---- */
static void on_back(lv_event_t *e) { (void)e; ui_show_settings(); }

static void on_var(lv_event_t *e) {
	commit_fields();
	var_ = (uint8_t)(intptr_t)lv_event_get_user_data(e);
	refresh_fields();
}

static void on_station(lv_event_t *e) {
	int d = (int)(intptr_t)lv_event_get_user_data(e);
	st_ = (uint8_t)((st_ + NUM_WELLS + d) % NUM_WELLS);
	loaded_ = false;
	if (plc()) plc()->requestScale(st_);
}

static void on_read(lv_event_t *e) {
	(void)e;
	loaded_ = false;
	if (plc()) plc()->requestScale(st_);
}

static void on_apply(lv_event_t *e) {
	(void)e;
	commit_fields();
	bool ok = plc() && plc()->applyScale(st_, edit_);
	static const char *btns[] = {"OK", ""};
	lv_obj_t *mb = lv_msgbox_create(nullptr, "Rangos",
		ok ? "Enviado al PLC. Verifica el sello."
		   : "No hay enlace con el PLC.", btns, false);
	lv_obj_center(mb);
	if (plc()) plc()->requestScale(st_);
}

static void on_kb_event(lv_event_t *e) {
	lv_event_code_t c = lv_event_get_code(e);
	if (c == LV_EVENT_READY || c == LV_EVENT_CANCEL) {
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		commit_fields();
	}
}

static void on_ta_focus(lv_event_t *e) {
	lv_obj_t *ta = lv_event_get_target(e);
	lv_keyboard_set_textarea(kb, ta);
	lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(kb);
}
static void on_ta_defocus(lv_event_t *e) {
	(void)e;
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	commit_fields();
}

/* ---- helpers de layout ---- */
static lv_obj_t *field_row(lv_obj_t *parent, const char *lab, bool numeric) {
	lv_obj_t *row = lv_obj_create(parent);
	lv_obj_set_size(row, LV_PCT(100), 30);
	lv_obj_set_style_bg_opa(row, 0, 0);
	lv_obj_set_style_border_width(row, 0, 0);
	lv_obj_set_style_pad_all(row, 0, 0);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *l = lv_label_create(row);
	lv_label_set_text(l, lab);
	lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(l, COL_MUTED, 0);
	lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);

	lv_obj_t *ta = lv_textarea_create(row);
	lv_obj_set_size(ta, 120, 30);
	lv_obj_align(ta, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_textarea_set_one_line(ta, true);
	if (numeric) lv_textarea_set_accepted_chars(ta, "0123456789.-");
	lv_obj_add_event_cb(ta, on_ta_focus, LV_EVENT_FOCUSED, nullptr);
	lv_obj_add_event_cb(ta, on_ta_defocus, LV_EVENT_DEFOCUSED, nullptr);
	return ta;
}

/* ---- create ---- */
lv_obj_t *screen_scale_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	/* barra superior */
	lv_obj_t *top = lv_obj_create(scr);
	lv_obj_set_pos(top, 0, 0);
	lv_obj_set_size(top, SCREEN_W, 30);
	lv_obj_set_style_bg_color(top, COL_CARD, 0);
	lv_obj_set_style_radius(top, 0, 0);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *bb = lv_btn_create(top);
	lv_obj_set_size(bb, 34, 24);
	lv_obj_align(bb, LV_ALIGN_LEFT_MID, 6, 0);
	lv_obj_set_style_bg_color(bb, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bb, on_back, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bb));
	lv_label_set_text(lv_obj_get_child(bb, 0), LV_SYMBOL_LEFT);

	lbl_title = lv_label_create(top);
	lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
	lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *bp = lv_btn_create(top);
	lv_obj_set_size(bp, 26, 24);
	lv_obj_align(bp, LV_ALIGN_RIGHT_MID, -40, 0);
	lv_obj_set_style_bg_color(bp, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bp, on_station, LV_EVENT_CLICKED, (void *)(intptr_t)-1);
	lv_obj_center(lv_label_create(bp));
	lv_label_set_text(lv_obj_get_child(bp, 0), LV_SYMBOL_LEFT);

	lv_obj_t *bn = lv_btn_create(top);
	lv_obj_set_size(bn, 26, 24);
	lv_obj_align(bn, LV_ALIGN_RIGHT_MID, -8, 0);
	lv_obj_set_style_bg_color(bn, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bn, on_station, LV_EVENT_CLICKED, (void *)(intptr_t)1);
	lv_obj_center(lv_label_create(bn));
	lv_label_set_text(lv_obj_get_child(bn, 0), LV_SYMBOL_RIGHT);

	/* toggle de variable + estado */
	btn_lvl = lv_btn_create(scr);
	lv_obj_set_size(btn_lvl, 90, 28);
	lv_obj_set_pos(btn_lvl, 8, 36);
	lv_obj_add_event_cb(btn_lvl, on_var, LV_EVENT_CLICKED, (void *)(intptr_t)0);
	lv_obj_center(lv_label_create(btn_lvl));
	lv_label_set_text(lv_obj_get_child(btn_lvl, 0), "Nivel");

	btn_flw = lv_btn_create(scr);
	lv_obj_set_size(btn_flw, 90, 28);
	lv_obj_set_pos(btn_flw, 104, 36);
	lv_obj_add_event_cb(btn_flw, on_var, LV_EVENT_CLICKED, (void *)(intptr_t)1);
	lv_obj_center(lv_label_create(btn_flw));
	lv_label_set_text(lv_obj_get_child(btn_flw, 0), "Caudal");

	lbl_status = lv_label_create(scr);
	lv_label_set_text(lbl_status, "leyendo...");
	lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lbl_status, COL_MUTED, 0);
	lv_obj_set_pos(lbl_status, 200, 42);

	/* formulario (scrollable) */
	lv_obj_t *form = lv_obj_create(scr);
	lv_obj_set_pos(form, 8, 70);
	lv_obj_set_size(form, SCREEN_W - 16, 118);
	lv_obj_set_style_bg_opa(form, 0, 0);
	lv_obj_set_style_border_width(form, 0, 0);
	lv_obj_set_style_pad_all(form, 0, 0);
	lv_obj_set_style_pad_row(form, 4, 0);
	lv_obj_set_flex_flow(form, LV_FLEX_FLOW_COLUMN);

	ta_rmin = field_row(form, "cero  (raw_min)", true);
	ta_rmax = field_row(form, "span  (raw_max)", true);
	ta_emin = field_row(form, "eng_min", true);
	ta_emax = field_row(form, "eng_max", true);
	ta_filt = field_row(form, "filtro 0..100", true);

	lv_obj_t *urow = lv_obj_create(form);
	lv_obj_set_size(urow, LV_PCT(100), 30);
	lv_obj_set_style_bg_opa(urow, 0, 0);
	lv_obj_set_style_border_width(urow, 0, 0);
	lv_obj_set_style_pad_all(urow, 0, 0);
	lv_obj_clear_flag(urow, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *ul = lv_label_create(urow);
	lv_label_set_text(ul, "unidad");
	lv_obj_set_style_text_font(ul, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(ul, COL_MUTED, 0);
	lv_obj_align(ul, LV_ALIGN_LEFT_MID, 0, 0);
	dd_unit = lv_dropdown_create(urow);
	lv_obj_set_size(dd_unit, 120, 30);
	lv_obj_align(dd_unit, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_dropdown_set_options(dd_unit, UNITS_LEVEL);

	/* botones LEER / APLICAR */
	lv_obj_t *br = lv_btn_create(scr);
	lv_obj_set_size(br, 100, 40);
	lv_obj_set_pos(br, 8, SCREEN_H - 44);
	lv_obj_set_style_bg_color(br, COL_TEAL_D, 0);
	lv_obj_add_event_cb(br, on_read, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(br));
	lv_label_set_text(lv_obj_get_child(br, 0), "LEER");

	lv_obj_t *ba = lv_btn_create(scr);
	lv_obj_set_size(ba, 190, 40);
	lv_obj_set_pos(ba, 116, SCREEN_H - 44);
	lv_obj_set_style_bg_color(ba, COL_ORANGE, 0);
	lv_obj_add_event_cb(ba, on_apply, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(ba));
	lv_label_set_text(lv_obj_get_child(ba, 0), "APLICAR AL PLC");

	/* teclado numerico (oculto) */
	kb = lv_keyboard_create(scr);
	lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_event_cb(kb, on_kb_event, LV_EVENT_ALL, nullptr);

	return scr;
}

/* ---- update (cada tick mientras esta visible) ---- */
void screen_scale_update() {
	lv_label_set_text_fmt(lbl_title, "RANGOS  Est %u", (unsigned)(st_ + 1));

	if (!plc()) {
		lv_label_set_text(lbl_status, "sin fuente PLC");
		return;
	}
	if (!loaded_) {
		if (plc()->scaleValid(st_)) {
			edit_ = plc()->getScale(st_);
			loaded_ = true;
			refresh_fields();
		} else {
			lv_label_set_text(lbl_status, "leyendo...");
			return;
		}
	}
	lv_label_set_text_fmt(lbl_status, "sello %u", edit_.stamp);
}

void screen_scale_enter() {
	loaded_ = false;
	if (plc()) plc()->requestScale(st_);
}

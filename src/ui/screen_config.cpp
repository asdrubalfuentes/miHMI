/**
 * screen_config.cpp  -  Configuracion de administrador (tras PIN).
 *
 * Edita hmicfg: WiFi de planta, destino Modbus del PLC (host/puerto/unit/sondeo),
 * nombres de estacion y PIN de administrador. GUARDAR escribe en microSD (si hay)
 * y en NVS. Los cambios de red se aplican al reiniciar.
 */
#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/hmi_config.h"
#include "net/ota_hmi.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static lv_obj_t *lbl_store;
static lv_obj_t *ta_ssid, *ta_pass, *ta_host, *ta_port, *ta_unit, *ta_poll;
static lv_obj_t *ta_wip, *ta_wgw, *ta_wmask, *ta_wdns1, *ta_wdns2;
static lv_obj_t *ta_name[NUM_WELLS];
static lv_obj_t *ta_pin;
static lv_obj_t *kb;

static lv_obj_t *btn_theme[3];      /* Auto / Claro / Oscuro */
static uint8_t   theme_sel = 0;

static void theme_hl() {
	for (uint8_t i = 0; i < 3; i++)
		lv_obj_set_style_bg_color(btn_theme[i], i == theme_sel ? COL_ORANGE : COL_TEAL_D, 0);
}
static void apply_theme_async(void *arg) {
	theme_set_mode((ThemeMode)(intptr_t)arg);   /* reconstruye la UI: fuera del evento */
}
static void on_theme_btn(lv_event_t *e) {
	theme_sel = (uint8_t)(intptr_t)lv_event_get_user_data(e);
	theme_hl();
	lv_async_call(apply_theme_async, (void *)(intptr_t)theme_sel);   /* vista previa en caliente */
}

static void cpystr(char *d, const char *s, size_t n) {
	strncpy(d, s ? s : "", n - 1);
	d[n - 1] = 0;
}

/* ---- teclado compartido ---- */
static void on_kb_event(lv_event_t *e) {
	lv_event_code_t c = lv_event_get_code(e);
	if (c == LV_EVENT_READY || c == LV_EVENT_CANCEL)
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}
static void on_ta_focus(lv_event_t *e) {
	lv_obj_t *ta = lv_event_get_target(e);
	bool num = lv_obj_has_flag(ta, LV_OBJ_FLAG_USER_1);
	lv_keyboard_set_mode(kb, num ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
	lv_keyboard_set_textarea(kb, ta);
	lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(kb);
}
static void on_ta_defocus(lv_event_t *e) {
	(void)e;
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

/* ---- guardar ---- */
static void mb_saved_cb(lv_event_t *e) {
	lv_obj_t *mb = lv_event_get_current_target(e);
	if (lv_msgbox_get_active_btn(mb) == 0) { ESP.restart(); return; }
	lv_msgbox_close(mb);
	ui_show_settings();
}

static void on_save(lv_event_t *e) {
	(void)e;
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

	HmiConfig c = hmicfg::editable();
	cpystr(c.wifiSsid, lv_textarea_get_text(ta_ssid), sizeof(c.wifiSsid));
	cpystr(c.wifiPass, lv_textarea_get_text(ta_pass), sizeof(c.wifiPass));
	cpystr(c.wifiIp,   lv_textarea_get_text(ta_wip),   sizeof(c.wifiIp));
	cpystr(c.wifiGw,   lv_textarea_get_text(ta_wgw),   sizeof(c.wifiGw));
	cpystr(c.wifiMask, lv_textarea_get_text(ta_wmask), sizeof(c.wifiMask));
	cpystr(c.wifiDns1, lv_textarea_get_text(ta_wdns1), sizeof(c.wifiDns1));
	cpystr(c.wifiDns2, lv_textarea_get_text(ta_wdns2), sizeof(c.wifiDns2));
	cpystr(c.plcHost,  lv_textarea_get_text(ta_host), sizeof(c.plcHost));

	long port = atol(lv_textarea_get_text(ta_port));
	long unit = atol(lv_textarea_get_text(ta_unit));
	long poll = atol(lv_textarea_get_text(ta_poll));
	c.plcPort = (port > 0 && port < 65536) ? (uint16_t)port : c.plcPort;
	c.plcUnit = (unit >= 0 && unit < 256) ? (uint8_t)unit : c.plcUnit;
	c.pollMs  = (poll >= 100 && poll <= 60000) ? (uint16_t)poll : c.pollMs;

	for (uint8_t s = 0; s < NUM_WELLS; s++)
		cpystr(c.stationName[s], lv_textarea_get_text(ta_name[s]), WELL_NAME_LEN);

	const char *pin = lv_textarea_get_text(ta_pin);
	if (pin && *pin) c.pinHash = hmicfg::hashPin(pin);

	c.theme = theme_sel;

	hmicfg::save(c);

	static const char *btns[] = {"Reiniciar", "Luego", ""};
	lv_obj_t *mb = lv_msgbox_create(nullptr, "Guardado",
		hmicfg::sdMounted() ? "Guardado en microSD + NVS.\nLos cambios de red se aplican al reiniciar."
		                    : "Sin microSD: guardado solo en NVS.\nLos cambios de red se aplican al reiniciar.",
		btns, false);
	lv_obj_add_event_cb(mb, mb_saved_cb, LV_EVENT_VALUE_CHANGED, nullptr);
	lv_obj_center(mb);
}

static void on_back(lv_event_t *e) { (void)e; ui_show_settings(); }

/* ---- OTA ---- */
static void mb_ota_cb(lv_event_t *e) {
	lv_obj_t *mb = lv_event_get_current_target(e);
	if (lv_msgbox_get_active_btn(mb) == 0) otaHmi::request();
	lv_msgbox_close(mb);
}
static void on_ota(lv_event_t *e) {
	(void)e;
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	static const char *btns[] = {"Buscar", "Cancelar", ""};
	lv_obj_t *mb = lv_msgbox_create(nullptr, "Actualizar firmware",
		"Se buscara una version nueva por WiFi. Si la hay, el HMI se actualiza y reinicia solo.\nNo cortes la alimentacion.",
		btns, false);
	lv_obj_add_event_cb(mb, mb_ota_cb, LV_EVENT_VALUE_CHANGED, nullptr);
	lv_obj_center(mb);
}

/* ---- helper de fila ---- */
static lv_obj_t *field(lv_obj_t *parent, const char *lab, bool numeric, bool password) {
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
	lv_obj_set_size(ta, 175, 30);
	lv_obj_align(ta, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_textarea_set_one_line(ta, true);
	if (numeric) {
		lv_textarea_set_accepted_chars(ta, "0123456789.");
		lv_obj_add_flag(ta, LV_OBJ_FLAG_USER_1);
	}
	if (password) lv_textarea_set_password_mode(ta, true);
	lv_obj_add_event_cb(ta, on_ta_focus, LV_EVENT_FOCUSED, nullptr);
	lv_obj_add_event_cb(ta, on_ta_defocus, LV_EVENT_DEFOCUSED, nullptr);
	return ta;
}

lv_obj_t *screen_config_create() {
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

	lv_obj_t *title = lv_label_create(top);
	lv_label_set_text(title, "Configuracion");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(title, COL_TEXT, 0);
	lv_obj_align(title, LV_ALIGN_LEFT_MID, 48, 0);

	lbl_store = lv_label_create(scr);
	lv_label_set_text(lbl_store, "almacenamiento: --");
	lv_obj_set_style_text_font(lbl_store, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lbl_store, COL_MUTED, 0);
	lv_obj_set_pos(lbl_store, 10, 34);

	/* formulario scrollable */
	lv_obj_t *form = lv_obj_create(scr);
	lv_obj_set_pos(form, 8, 52);
	lv_obj_set_size(form, SCREEN_W - 16, SCREEN_H - 52 - 48);
	lv_obj_set_style_bg_opa(form, 0, 0);
	lv_obj_set_style_border_width(form, 0, 0);
	lv_obj_set_style_pad_all(form, 0, 0);
	lv_obj_set_style_pad_row(form, 4, 0);
	lv_obj_set_flex_flow(form, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(form, LV_DIR_VER);

	ta_ssid  = field(form, "WiFi SSID", false, false);
	ta_pass  = field(form, "WiFi clave", false, false);
	ta_wip   = field(form, "IP fija (vacio=DHCP)", true, false);
	ta_wgw   = field(form, "Gateway", true, false);
	ta_wmask = field(form, "Mascara", true, false);
	ta_wdns1 = field(form, "DNS 1", true, false);
	ta_wdns2 = field(form, "DNS 2", true, false);
	ta_host = field(form, "PLC host", false, false);
	ta_port = field(form, "PLC puerto", true, false);
	ta_unit = field(form, "PLC unit id", true, false);
	ta_poll = field(form, "Sondeo (ms)", true, false);
	for (uint8_t s = 0; s < NUM_WELLS; s++) {
		char lab[16]; snprintf(lab, sizeof(lab), "Estacion %u", (unsigned)(s + 1));
		ta_name[s] = field(form, lab, false, false);
	}
	ta_pin = field(form, "PIN nuevo (vacio=igual)", true, true);

	/* ---- Tema de colores ---- */
	lv_obj_t *trow = lv_obj_create(form);
	lv_obj_set_size(trow, LV_PCT(100), 30);
	lv_obj_set_style_bg_opa(trow, 0, 0);
	lv_obj_set_style_border_width(trow, 0, 0);
	lv_obj_set_style_pad_all(trow, 0, 0);
	lv_obj_clear_flag(trow, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *tl = lv_label_create(trow);
	lv_label_set_text(tl, "Tema");
	lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(tl, COL_MUTED, 0);
	lv_obj_align(tl, LV_ALIGN_LEFT_MID, 0, 0);

	static const char *tn[3] = { "Auto", "Claro", "Oscuro" };
	for (uint8_t i = 0; i < 3; i++) {
		btn_theme[i] = lv_btn_create(trow);
		lv_obj_set_size(btn_theme[i], 58, 28);
		lv_obj_align(btn_theme[i], LV_ALIGN_RIGHT_MID, -(2 - i) * 60, 0);
		lv_obj_add_event_cb(btn_theme[i], on_theme_btn, LV_EVENT_CLICKED, (void *)(intptr_t)i);
		lv_obj_center(lv_label_create(btn_theme[i]));
		lv_label_set_text(lv_obj_get_child(btn_theme[i], 0), tn[i]);
		lv_obj_set_style_text_font(btn_theme[i], &lv_font_montserrat_12, 0);
	}

	/* ---- OTA: buscar actualizacion ---- */
	lv_obj_t *bota = lv_btn_create(form);
	lv_obj_set_size(bota, LV_PCT(100), 32);
	lv_obj_set_style_bg_color(bota, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bota, on_ota, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bota));
	lv_label_set_text(lv_obj_get_child(bota, 0), "Buscar actualizacion  (v" APP_VERSION ")");

	/* GUARDAR */
	lv_obj_t *bs = lv_btn_create(scr);
	lv_obj_set_size(bs, SCREEN_W - 16, 40);
	lv_obj_set_pos(bs, 8, SCREEN_H - 44);
	lv_obj_set_style_bg_color(bs, COL_ORANGE, 0);
	lv_obj_add_event_cb(bs, on_save, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bs));
	lv_label_set_text(lv_obj_get_child(bs, 0), "GUARDAR");

	/* teclado compartido (oculto) */
	kb = lv_keyboard_create(scr);
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_event_cb(kb, on_kb_event, LV_EVENT_ALL, nullptr);

	return scr;
}

void screen_config_enter() {
	const HmiConfig &c = hmicfg::get();
	lv_textarea_set_text(ta_ssid, c.wifiSsid);
	lv_textarea_set_text(ta_pass, c.wifiPass);
	lv_textarea_set_text(ta_wip,   c.wifiIp);
	lv_textarea_set_text(ta_wgw,   c.wifiGw);
	lv_textarea_set_text(ta_wmask, c.wifiMask);
	lv_textarea_set_text(ta_wdns1, c.wifiDns1);
	lv_textarea_set_text(ta_wdns2, c.wifiDns2);
	lv_textarea_set_text(ta_host, c.plcHost);
	char b[12];
	snprintf(b, sizeof(b), "%u", c.plcPort); lv_textarea_set_text(ta_port, b);
	snprintf(b, sizeof(b), "%u", c.plcUnit); lv_textarea_set_text(ta_unit, b);
	snprintf(b, sizeof(b), "%u", c.pollMs);  lv_textarea_set_text(ta_poll, b);
	for (uint8_t s = 0; s < NUM_WELLS; s++)
		lv_textarea_set_text(ta_name[s], c.stationName[s]);
	lv_textarea_set_text(ta_pin, "");

	theme_sel = (uint8_t)theme_mode();
	theme_hl();

	lv_label_set_text_fmt(lbl_store, "almacenamiento: %s%s",
	                      hmicfg::source(),
	                      hmicfg::sdMounted() ? "  (microSD OK)" : "  (sin microSD)");
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

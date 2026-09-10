/**
 * screen_pin.cpp  -  Puerta de PIN de administrador.
 *
 * Se muestra antes de las pantallas restringidas (Configuracion). Al acertar
 * ejecuta el callback que le paso ui_show_pin(); a los 3 fallos vuelve a Ajustes.
 * El PIN nunca se guarda en claro: hmicfg compara contra un hash.
 */
#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/hmi_config.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *lbl_title;
static lv_obj_t *lbl_dots;
static char      pin_[9];
static uint8_t   fails_;
static void (*on_ok_)() = nullptr;

static void refresh_dots() {
	char b[10];
	size_t n = strlen(pin_);
	if (n > 8) n = 8;
	for (size_t i = 0; i < n; i++) b[i] = '*';
	b[n] = 0;
	lv_label_set_text(lbl_dots, n ? b : "----");
}

static const char *KEYMAP[] = {
	"1", "2", "3", "\n",
	"4", "5", "6", "\n",
	"7", "8", "9", "\n",
	LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};

static void on_key(lv_event_t *e) {
	lv_obj_t *m = lv_event_get_target(e);
	const char *txt = lv_btnmatrix_get_btn_text(m, lv_btnmatrix_get_selected_btn(m));
	if (!txt) return;

	if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
		size_t n = strlen(pin_);
		if (n) pin_[n - 1] = 0;
	} else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
		if (hmicfg::checkPin(pin_)) {
			pin_[0] = 0; fails_ = 0;
			refresh_dots();
			if (on_ok_) on_ok_();
			return;
		}
		fails_++;
		pin_[0] = 0;
		lv_label_set_text(lbl_title, "PIN incorrecto");
		lv_obj_set_style_text_color(lbl_title, COL_STOP, 0);
		if (fails_ >= 3) { fails_ = 0; ui_show_settings(); return; }
	} else if (txt[0] >= '0' && txt[0] <= '9') {
		size_t n = strlen(pin_);
		if (n < 8) { pin_[n] = txt[0]; pin_[n + 1] = 0; }
	}
	refresh_dots();
}

static void on_cancel(lv_event_t *e) {
	(void)e;
	pin_[0] = 0; fails_ = 0;
	ui_show_settings();
}

lv_obj_t *screen_pin_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *top = lv_obj_create(scr);
	lv_obj_set_pos(top, 0, 0);
	lv_obj_set_size(top, SCREEN_W, 30);
	lv_obj_set_style_bg_color(top, COL_CARD, 0);
	lv_obj_set_style_radius(top, 0, 0);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *bx = lv_btn_create(top);
	lv_obj_set_size(bx, 34, 24);
	lv_obj_align(bx, LV_ALIGN_LEFT_MID, 6, 0);
	lv_obj_set_style_bg_color(bx, COL_TEAL_D, 0);
	lv_obj_add_event_cb(bx, on_cancel, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bx));
	lv_label_set_text(lv_obj_get_child(bx, 0), LV_SYMBOL_CLOSE);

	lbl_title = lv_label_create(top);
	lv_label_set_text(lbl_title, "PIN de administrador");
	lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
	lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 48, 0);

	lbl_dots = lv_label_create(scr);
	lv_label_set_text(lbl_dots, "----");
	lv_obj_set_style_text_font(lbl_dots, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_dots, COL_TEXT, 0);
	lv_obj_align(lbl_dots, LV_ALIGN_TOP_MID, 0, 44);

	lv_obj_t *m = lv_btnmatrix_create(scr);
	lv_btnmatrix_set_map(m, KEYMAP);
	lv_obj_set_size(m, SCREEN_W - 24, SCREEN_H - 92);
	lv_obj_align(m, LV_ALIGN_BOTTOM_MID, 0, -8);
	lv_obj_set_style_text_font(m, &lv_font_montserrat_20, 0);
	lv_obj_add_event_cb(m, on_key, LV_EVENT_VALUE_CHANGED, nullptr);

	return scr;
}

void screen_pin_prepare(void (*on_ok)()) {
	on_ok_ = on_ok;
	pin_[0] = 0;
	fails_ = 0;
	lv_label_set_text(lbl_title, "PIN de administrador");
	lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
	refresh_dots();
}

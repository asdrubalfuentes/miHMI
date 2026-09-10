/**
 * screen_actions.cpp  -  Acciones de mando de una estacion (superficie de comandos
 * del MAPA B: silenciar, reconocer alarmas, modo de sirena, reset de acumulados).
 *
 * Se abre desde la pantalla de detalle de estacion. Todas las acciones con efecto
 * piden confirmacion salvo el cambio de modo de sirena.
 */
#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include <lvgl.h>
#include <stdio.h>

static lv_obj_t *lbl_title;
static lv_obj_t *lbl_ack_sub;      /* estado de "pendientes" bajo RECONOCER */
static lv_obj_t *btn_mode;         /* SIRENA: AUTO / MANUAL */

static uint8_t sel()          { return DataHub::instance().selectedWell(); }
static const WellData &cur()  { return DataHub::instance().data().well[sel()]; }

/* --- confirmacion generica: msgbox Si/No que SIEMPRE se cierra --- */
static CmdType s_pending;

static void mb_confirm_cb(lv_event_t *e) {
	lv_obj_t *mb = lv_event_get_current_target(e);
	if (lv_msgbox_get_active_btn(mb) == 0)
		DataHub::instance().enqueue(s_pending, sel());
	lv_msgbox_close(mb);
}

static void ask(CmdType cmd, const char *msg) {
	s_pending = cmd;
	static const char *btns[] = {"Si", "No", ""};
	lv_obj_t *mb = lv_msgbox_create(nullptr, "Confirmar", msg, btns, false);
	lv_obj_center(mb);
	lv_obj_add_event_cb(mb, mb_confirm_cb, LV_EVENT_VALUE_CHANGED, nullptr);
}

/* --- eventos --- */
static void on_back(lv_event_t *e)        { (void)e; ui_show_well(sel()); }
static void on_silence(lv_event_t *e)     { (void)e; ask(CmdType::Silence,    "Silenciar la sirena?"); }
static void on_ack(lv_event_t *e)         { (void)e; ask(CmdType::AckAlarms,  "Reconocer y limpiar las alarmas pendientes?"); }
static void on_reset_day(lv_event_t *e)   { (void)e; ask(CmdType::ResetDay,   "Poner a cero el acumulado del DIA?"); }
static void on_reset_month(lv_event_t *e) { (void)e; ask(CmdType::ResetMonth, "Poner a cero el acumulado del MES?"); }

static void on_mode(lv_event_t *e) {
	(void)e;
	DataHub::instance().enqueue(cur().sirenAuto ? CmdType::SirenManual
	                                            : CmdType::SirenAuto, sel());
}

/* --- helper: boton ancho con etiqueta centrada; devuelve el boton --- */
static lv_obj_t *act_btn(lv_obj_t *parent, const char *txt, lv_color_t col,
                         lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_event_cb_t cb) {
	lv_obj_t *b = lv_btn_create(parent);
	lv_obj_set_pos(b, x, y);
	lv_obj_set_size(b, w, 40);
	lv_obj_set_style_bg_color(b, col, 0);
	lv_obj_set_style_radius(b, 6, 0);
	lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *l = lv_label_create(b);
	lv_label_set_text(l, txt);
	lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
	lv_obj_center(l);
	return b;
}

static void set_btn_text(lv_obj_t *b, const char *txt) {
	lv_label_set_text(lv_obj_get_child(b, 0), txt);
}

/* --- create --- */
lv_obj_t *screen_actions_create() {
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

	lv_obj_t *back = lv_btn_create(top);
	lv_obj_set_size(back, 34, 24);
	lv_obj_align(back, LV_ALIGN_LEFT_MID, 6, 0);
	lv_obj_set_style_bg_color(back, COL_TEAL_D, 0);
	lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(back));
	lv_label_set_text(lv_obj_get_child(back, 0), LV_SYMBOL_LEFT);

	lbl_title = lv_label_create(top);
	lv_label_set_text(lbl_title, "Acciones");
	lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
	lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 48, 0);

	const lv_coord_t W = SCREEN_W - 24;
	act_btn(scr, "SILENCIAR SIRENA",  COL_WARN,   12, 40,  W, on_silence);
	act_btn(scr, "RECONOCER ALARMAS", COL_ORANGE, 12, 88,  W, on_ack);

	lbl_ack_sub = lv_label_create(scr);
	lv_label_set_text(lbl_ack_sub, "sin pendientes");
	lv_obj_set_style_text_font(lbl_ack_sub, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lbl_ack_sub, COL_MUTED, 0);
	lv_obj_set_pos(lbl_ack_sub, 16, 130);

	btn_mode = act_btn(scr, "SIRENA: AUTO", COL_TEAL_D, 12, 150, W, on_mode);

	act_btn(scr, "RESET DIA", COL_TEAL, 12, 198, (SCREEN_W - 32) / 2, on_reset_day);
	act_btn(scr, "RESET MES", COL_TEAL, SCREEN_W / 2 + 4, 198, (SCREEN_W - 32) / 2, on_reset_month);

	return scr;
}

/* --- update --- */
void screen_actions_update() {
	const WellData &d = cur();
	lv_label_set_text_fmt(lbl_title, "Acciones  -  %s", d.name);
	set_btn_text(btn_mode, d.sirenAuto ? "SIRENA: AUTO" : "SIRENA: MANUAL");

	if (d.hasLatched()) {
		char b[56];
		snprintf(b, sizeof(b), "Pendiente: %s%s",
		         (d.alarmsLatched & MAPB_ALM_LEVEL_LOLO) ? "marcha en seco " : "",
		         (d.alarmsLatched & MAPB_ALM_TAMPER)     ? "tapa abierta"    : "");
		lv_label_set_text(lbl_ack_sub, b);
		lv_obj_set_style_text_color(lbl_ack_sub, COL_WARN, 0);
	} else {
		lv_label_set_text(lbl_ack_sub, "sin pendientes");
		lv_obj_set_style_text_color(lbl_ack_sub, COL_MUTED, 0);
	}
}

#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include "data/hmi_config.h"
#include "hal/net_clock.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

/* ---- widgets refrescados por screen_well_update() ---- */
static lv_obj_t *lbl_name;
static lv_obj_t *lbl_ticker;                 /* cintillo con tiras de mensajes */
static lv_obj_t *lbl_status;                 /* OK / ALARMA / SIN ENLACE */
static lv_obj_t *lbl_stnbtn;                 /* etiqueta del boton "Estacion N" */
static lv_obj_t *arc_level, *lbl_level_pct, *lbl_level_m;
static lv_obj_t *lbl_flow;
static lv_obj_t *lbl_today;
static lv_obj_t *lbl_alarms;

static uint8_t sel() { return DataHub::instance().selectedWell(); }
static const WellData &cur() { return DataHub::instance().data().well[sel()]; }

/* ------------------------- eventos ------------------------- */
static void on_actions(lv_event_t *e) { (void)e; ui_show_actions(); }

static void on_prev(lv_event_t *e) {
	(void)e;
	DataHub::instance().setSelectedWell((sel() + NUM_WELLS - 1) % NUM_WELLS);
	screen_well_update();
}
static void on_next(lv_event_t *e) {
	(void)e;
	DataHub::instance().setSelectedWell((sel() + 1) % NUM_WELLS);
	screen_well_update();
}
static void on_list(lv_event_t *e)     { (void)e; ui_show_wells();    }
static void on_hist(lv_event_t *e)     { (void)e; ui_show_history();  }
static void on_settings(lv_event_t *e) { (void)e; ui_show_settings(); }

/* ------------------------- helpers ------------------------- */
static lv_obj_t *icon_btn(lv_obj_t *parent, const char *sym, lv_coord_t w, lv_event_cb_t cb) {
	lv_obj_t *b = lv_btn_create(parent);
	lv_obj_set_size(b, w, 24);
	lv_obj_set_style_bg_color(b, COL_TEAL_D, 0);
	lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *l = lv_label_create(b);
	lv_label_set_text(l, sym);
	lv_obj_center(l);
	return b;
}

static lv_obj_t *action_btn(lv_obj_t *parent, const char *txt, lv_color_t col,
                            lv_coord_t x, lv_coord_t w, lv_event_cb_t cb) {
	lv_obj_t *b = lv_btn_create(parent);
	lv_obj_set_pos(b, x, SCREEN_H - 44);
	lv_obj_set_size(b, w, 40);
	lv_obj_set_style_bg_color(b, col, 0);
	lv_obj_set_style_radius(b, 6, 0);
	lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *l = lv_label_create(b);
	lv_label_set_text(l, txt);
	lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
	lv_obj_center(l);
	return l;
}

/* ------------------------- create ------------------------- */
lv_obj_t *screen_well_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	/* --- barra superior: volver | ‹ nombre › | ajustes --- */
	lv_obj_t *top = lv_obj_create(scr);
	lv_obj_set_pos(top, 0, 0);
	lv_obj_set_size(top, SCREEN_W, 30);
	lv_obj_set_style_bg_color(top, COL_CARD, 0);
	lv_obj_set_style_radius(top, 0, 0);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *blist = icon_btn(top, LV_SYMBOL_LEFT, 34, on_list);
	lv_obj_align(blist, LV_ALIGN_LEFT_MID, 6, 0);

	lv_obj_t *bprev = icon_btn(top, LV_SYMBOL_LEFT, 26, on_prev);
	lv_obj_align(bprev, LV_ALIGN_CENTER, -66, 0);

	lbl_name = lv_label_create(top);
	lv_label_set_text(lbl_name, "Estacion");
	lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_name, COL_TEXT, 0);
	lv_obj_align(lbl_name, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *bnext = icon_btn(top, LV_SYMBOL_RIGHT, 26, on_next);
	lv_obj_align(bnext, LV_ALIGN_CENTER, 66, 0);

	lv_obj_t *bset = icon_btn(top, LV_SYMBOL_SETTINGS, 34, on_settings);
	lv_obj_align(bset, LV_ALIGN_RIGHT_MID, -6, 0);

	/* --- cintillo: tira de mensajes con scroll circular --- */
	lv_obj_t *tk = lv_obj_create(scr);
	lv_obj_set_pos(tk, 0, 30);
	lv_obj_set_size(tk, SCREEN_W, 20);
	lv_obj_set_style_bg_color(tk, COL_SUNKEN, 0);
	lv_obj_set_style_radius(tk, 0, 0);
	lv_obj_set_style_border_width(tk, 0, 0);
	lv_obj_set_style_pad_all(tk, 0, 0);
	lv_obj_clear_flag(tk, LV_OBJ_FLAG_SCROLLABLE);

	lbl_ticker = lv_label_create(tk);
	lv_label_set_long_mode(lbl_ticker, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(lbl_ticker, SCREEN_W - 12);
	lv_label_set_text(lbl_ticker, APP_NAME);
	lv_obj_set_style_text_font(lbl_ticker, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lbl_ticker, COL_MUTED, 0);
	lv_obj_align(lbl_ticker, LV_ALIGN_LEFT_MID, 6, 0);

	/* --- nivel (arco) --- */
	arc_level = lv_arc_create(scr);
	lv_obj_set_size(arc_level, 124, 124);
	lv_obj_set_pos(arc_level, 8, 52);
	lv_arc_set_rotation(arc_level, 135);
	lv_arc_set_bg_angles(arc_level, 0, 270);
	lv_arc_set_range(arc_level, 0, 100);
	lv_arc_set_value(arc_level, 0);
	lv_obj_remove_style(arc_level, nullptr, LV_PART_KNOB);
	lv_obj_clear_flag(arc_level, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_arc_color(arc_level, COL_CARD, LV_PART_MAIN);
	lv_obj_set_style_arc_color(arc_level, COL_TEAL, LV_PART_INDICATOR);
	lv_obj_set_style_arc_width(arc_level, 12, LV_PART_MAIN);
	lv_obj_set_style_arc_width(arc_level, 12, LV_PART_INDICATOR);

	lbl_level_pct = lv_label_create(scr);
	lv_label_set_text(lbl_level_pct, "--%");
	lv_obj_set_style_text_font(lbl_level_pct, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_level_pct, COL_TEXT, 0);
	lv_obj_align_to(lbl_level_pct, arc_level, LV_ALIGN_CENTER, 0, -6);

	lbl_level_m = lv_label_create(scr);
	lv_label_set_text(lbl_level_m, "-- m");
	lv_obj_set_style_text_font(lbl_level_m, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_level_m, COL_MUTED, 0);
	lv_obj_align_to(lbl_level_m, arc_level, LV_ALIGN_CENTER, 0, 22);

	lv_obj_t *lvl_title = lv_label_create(scr);
	lv_label_set_text(lvl_title, "NIVEL");
	lv_obj_add_style(lvl_title, &st_title, 0);
	lv_obj_align_to(lvl_title, arc_level, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

	/* --- caudal + acumulado del dia --- */
	lv_obj_t *flow_title = lv_label_create(scr);
	lv_label_set_text(flow_title, "CAUDAL");
	lv_obj_add_style(flow_title, &st_title, 0);
	lv_obj_set_pos(flow_title, 156, 54);

	lbl_flow = lv_label_create(scr);
	lv_label_set_text(lbl_flow, "-- L/s");
	lv_obj_set_style_text_font(lbl_flow, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_flow, COL_TEXT, 0);
	lv_obj_set_pos(lbl_flow, 156, 68);

	lv_obj_t *today_title = lv_label_create(scr);
	lv_label_set_text(today_title, "ACUMULADO HOY");
	lv_obj_add_style(today_title, &st_title, 0);
	lv_obj_set_pos(today_title, 156, 114);

	lbl_today = lv_label_create(scr);
	lv_label_set_text(lbl_today, "-- m3");
	lv_obj_set_style_text_font(lbl_today, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_today, COL_TEAL, 0);
	lv_obj_set_pos(lbl_today, 156, 126);

	/* estado de la estacion: bajo ACUMULADO (antes en el cintillo, se solapaba) */
	lbl_status = lv_label_create(scr);
	lv_label_set_text(lbl_status, "--");
	lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_16, 0);
	lv_obj_set_pos(lbl_status, 156, 160);

	/* alarma activa (linea que hace scroll) */
	lbl_alarms = lv_label_create(scr);
	lv_label_set_long_mode(lbl_alarms, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(lbl_alarms, 156);
	lv_label_set_text(lbl_alarms, "");
	lv_obj_set_style_text_font(lbl_alarms, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lbl_alarms, COL_WARN, 0);
	lv_obj_set_pos(lbl_alarms, 156, 182);

	/* --- botonera inferior: Estacion N | ACCIONES | historico --- */
	lbl_stnbtn = action_btn(scr, "Estacion", COL_TEAL_D, 8, 100, on_list);
	action_btn(scr, "ACCIONES", COL_TEAL_D, 112, 110, on_actions);
	action_btn(scr, LV_SYMBOL_LIST, COL_TEAL_D, 228, 84, on_hist);

	return scr;
}

/* ------------------------- update ------------------------- */
static const char *comm_str() {
	DataHub &hub = DataHub::instance();
	DataSource *pri = hub.primary();
	bool on_plc = pri && strcmp(hub.activeSourceName(), pri->name()) == 0;
	SrcHealth h = hub.primaryHealth();
	if (on_plc && h == SrcHealth::Ok)     return "PLC conectado";
	if (on_plc && h == SrcHealth::Stale)  return "PLC intermitente";
	return "PLC sin conexion (sim)";
}

static const char *first_alarm(uint16_t bits) {
	struct { uint16_t m; const char *n; } T[] = {
		{MAPB_ALM_LEVEL_HI, "Nivel alto"}, {MAPB_ALM_LEVEL_LOLO, "Marcha en seco"},
		{MAPB_ALM_LEVEL_LO, "Nivel bajo"}, {MAPB_ALM_NO_FLOW, "Sin caudal"},
		{MAPB_ALM_PRESS_FAIL, "Falla presostato"}, {MAPB_ALM_VOLT_LOSS, "Sin voltaje"},
		{MAPB_ALM_TAMPER, "Tapa abierta"}, {MAPB_ALM_LORA_LOSS, "Sin enlace LoRa"},
		{MAPB_ALM_STALE, "Dato obsoleto"}, {MAPB_ALM_SCALE_BAD, "Escala invalida"},
		{MAPB_ALM_OVERRANGE, "Sobre-rango"},
	};
	for (auto &e : T) if (bits & e.m) return e.n;
	return "";
}

void screen_well_update() {
	DataHub &hub = DataHub::instance();
	const WellData &d = hub.data().well[hub.selectedWell()];

	unsigned idx1 = (unsigned)(hub.selectedWell() + 1);
	lv_label_set_text_fmt(lbl_name, "%u/%u", idx1, (unsigned)NUM_WELLS);
	lv_label_set_text(lbl_stnbtn, d.name);

	/* cintillo: Mi HMI | Estacion | version | hora | estado de comunicacion */
	char hm[8]; netclock::hm(hm, sizeof(hm));
	char tk[160];
	snprintf(tk, sizeof(tk), "%s   |   %s   |   v%s   |   %s   |   %s",
	         APP_NAME, d.name, APP_VERSION, hm, comm_str());
	lv_label_set_text(lbl_ticker, tk);

	/* LVGL no formatea %f: usar snprintf de libc y set_text */
	char b[28];

	/* --- nivel: % grande (arco) + metros de profundidad debajo ---
	   Cambio de rumbo 2026-09: el nivel llega SIEMPRE ya escalado en metros
	 * (lo calibra el nodo remoto en su propio portal, no el HMI) -- ya no hay
	 * pagina de rangos ni bloque de escala que consultar aqui. La barra 0-100%
	 * es solo una referencia visual contra la profundidad de fondo de escala
	 * configurada (Ajustes, "profundidad a fondo de escala"). */
	float depthM = d.levelEng;   /* ya viene en metros, MB_LEVEL_SCALE ya aplicado */
	float hi = (float)hmicfg::get().levelMaxM;
	float pct = (hi > 0) ? (depthM / hi) * 100.0f : 0.0f;
	if (pct < 0) pct = 0; else if (pct > 100) pct = 100;

	lv_arc_set_value(arc_level, (int)lroundf(pct));
	snprintf(b, sizeof(b), "%d %%", (int)lroundf(pct));
	lv_label_set_text(lbl_level_pct, b);
	snprintf(b, sizeof(b), "%.2f m", depthM);
	lv_label_set_text(lbl_level_m, b);

	snprintf(b, sizeof(b), "%.1f m3/h", d.flowEng);   /* unidad fija, ver nodeIO ch[1].unit */
	lv_label_set_text(lbl_flow, b);
	snprintf(b, sizeof(b), "%.1f m3", d.totalDayM3);
	lv_label_set_text(lbl_today, b);

	if (!d.linkOk) {
		lv_label_set_text(lbl_status, "SIN ENLACE");
		lv_obj_set_style_text_color(lbl_status, COL_STOP, 0);
	} else if (d.inAlarm()) {
		lv_label_set_text(lbl_status, d.sirenOn ? "ALARMA*" : "ALARMA");
		lv_obj_set_style_text_color(lbl_status, COL_WARN, 0);
	} else if (d.hasLatched()) {
		lv_label_set_text(lbl_status, "PENDIENTE ACK");
		lv_obj_set_style_text_color(lbl_status, COL_WARN, 0);
	} else {
		lv_label_set_text(lbl_status, "OK");
		lv_obj_set_style_text_color(lbl_status, COL_RUN, 0);
	}

	lv_label_set_text(lbl_alarms,
	                  d.inAlarm()     ? first_alarm(d.alarms)
	                  : d.hasLatched() ? first_alarm(d.alarmsLatched)
	                                   : "");
}

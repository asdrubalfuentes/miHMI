#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

/* ---- widgets refrescados por screen_well_update() ---- */
static lv_obj_t *lbl_name;
static lv_obj_t *lbl_mb, *lbl_lora, *lbl_net;
static lv_obj_t *lbl_pump;
static lv_obj_t *arc_level, *lbl_level_pct, *lbl_level_m;
static lv_obj_t *lbl_flow, *lbl_flow_m3h;
static lv_obj_t *lbl_today;

static uint8_t sel() { return DataHub::instance().selectedWell(); }

/* ------------------------- eventos ------------------------- */
static void mb_start_cb(lv_event_t *e) {
	lv_obj_t *mb = lv_event_get_current_target(e);
	if (lv_msgbox_get_active_btn(mb) == 0)
		DataHub::instance().enqueue(CmdType::PumpStart, sel());
	lv_msgbox_close(mb);
}

static void mb_stop_cb(lv_event_t *e) {
	lv_obj_t *mb = lv_event_get_current_target(e);
	if (lv_msgbox_get_active_btn(mb) == 0)
		DataHub::instance().enqueue(CmdType::PumpStop, sel());
	lv_msgbox_close(mb);
}

static void confirm(const char *msg, lv_event_cb_t cb) {
	static const char *btns[] = {"Si", "No", ""};
	lv_obj_t *mb = lv_msgbox_create(nullptr, "Confirmar", msg, btns, false);
	lv_obj_center(mb);
	lv_obj_add_event_cb(mb, cb, LV_EVENT_VALUE_CHANGED, nullptr);
}

static void on_start(lv_event_t *e) {
	(void)e;
	static char m[64];
	snprintf(m, sizeof(m), "Encender la bomba de %s?", DataHub::instance().data().well[sel()].name);
	confirm(m, mb_start_cb);
}

static void on_stop(lv_event_t *e) {
	(void)e;
	static char m[64];
	snprintf(m, sizeof(m), "Detener la bomba de %s?", DataHub::instance().data().well[sel()].name);
	confirm(m, mb_stop_cb);
}

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
static lv_obj_t *make_stat(lv_obj_t *parent, const char *txt, lv_coord_t x) {
	lv_obj_t *l = lv_label_create(parent);
	lv_label_set_text(l, txt);
	lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(l, COL_MUTED, 0);
	lv_obj_align(l, LV_ALIGN_TOP_LEFT, x, 8);
	return l;
}

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
	return b;
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
	lv_label_set_text(lbl_name, "Pozo");
	lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_name, COL_TEXT, 0);
	lv_obj_align(lbl_name, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *bnext = icon_btn(top, LV_SYMBOL_RIGHT, 26, on_next);
	lv_obj_align(bnext, LV_ALIGN_CENTER, 66, 0);

	lv_obj_t *bset = icon_btn(top, LV_SYMBOL_SETTINGS, 34, on_settings);
	lv_obj_align(bset, LV_ALIGN_RIGHT_MID, -6, 0);

	/* estado de fuentes de datos (fila fina bajo la barra) */
	lbl_mb   = make_stat(scr, "MB",   8);
	lbl_lora = make_stat(scr, "LoRa", 44);
	lbl_net  = make_stat(scr, "NET",  96);
	lbl_pump = lv_label_create(scr);
	lv_label_set_text(lbl_pump, "PARO");
	lv_obj_set_style_text_font(lbl_pump, &lv_font_montserrat_14, 0);
	lv_obj_align(lbl_pump, LV_ALIGN_TOP_RIGHT, -10, 6);

	/* --- nivel (arco) --- */
	arc_level = lv_arc_create(scr);
	lv_obj_set_size(arc_level, 132, 132);
	lv_obj_set_pos(arc_level, 8, 40);
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
	lv_obj_set_style_text_font(lbl_level_m, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(lbl_level_m, COL_MUTED, 0);
	lv_obj_align_to(lbl_level_m, arc_level, LV_ALIGN_CENTER, 0, 20);

	lv_obj_t *lvl_title = lv_label_create(scr);
	lv_label_set_text(lvl_title, "NIVEL DE POZO");
	lv_obj_add_style(lvl_title, &st_title, 0);
	lv_obj_align(lvl_title, LV_ALIGN_TOP_LEFT, 24, 174);

	/* --- caudal + acumulado del dia --- */
	lv_obj_t *flow_title = lv_label_create(scr);
	lv_label_set_text(flow_title, "CAUDAL");
	lv_obj_add_style(flow_title, &st_title, 0);
	lv_obj_set_pos(flow_title, 156, 42);

	lbl_flow = lv_label_create(scr);
	lv_label_set_text(lbl_flow, "-- L/s");
	lv_obj_set_style_text_font(lbl_flow, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_flow, COL_TEXT, 0);
	lv_obj_set_pos(lbl_flow, 156, 58);

	lbl_flow_m3h = lv_label_create(scr);
	lv_label_set_text(lbl_flow_m3h, "-- m3/h");
	lv_obj_set_style_text_font(lbl_flow_m3h, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(lbl_flow_m3h, COL_MUTED, 0);
	lv_obj_set_pos(lbl_flow_m3h, 156, 94);

	lv_obj_t *today_title = lv_label_create(scr);
	lv_label_set_text(today_title, "ACUMULADO HOY");
	lv_obj_add_style(today_title, &st_title, 0);
	lv_obj_set_pos(today_title, 156, 124);

	lbl_today = lv_label_create(scr);
	lv_label_set_text(lbl_today, "-- m3");
	lv_obj_set_style_text_font(lbl_today, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_today, COL_TEAL, 0);
	lv_obj_set_pos(lbl_today, 156, 140);

	/* --- botonera inferior --- */
	action_btn(scr, "ENCENDER", COL_RUN,  8,   96, on_start);
	action_btn(scr, "DETENER",  COL_STOP, 112, 96, on_stop);
	action_btn(scr, LV_SYMBOL_LIST " Histor.", COL_TEAL_D, 216, 96, on_hist);

	return scr;
}

/* ------------------------- update ------------------------- */
static void set_health(lv_obj_t *l, SrcHealth h) {
	lv_obj_set_style_text_color(l, ui_health_color((int)h), 0);
}

void screen_well_update() {
	DataHub &hub = DataHub::instance();
	const WellData &d = hub.data().well[hub.selectedWell()];

	lv_label_set_text_fmt(lbl_name, "%s   (%u/%u)",
	                      d.name, (unsigned)(hub.selectedWell() + 1), (unsigned)NUM_WELLS);

	lv_arc_set_value(arc_level, (int)lroundf(d.levelPct));
	lv_label_set_text_fmt(lbl_level_pct, "%d%%", (int)lroundf(d.levelPct));
	lv_label_set_text_fmt(lbl_level_m, "%.1f m", d.levelM);
	lv_label_set_text_fmt(lbl_flow, "%.1f L/s", d.flowLps);
	lv_label_set_text_fmt(lbl_flow_m3h, "%.0f m3/h", d.flowM3h);
	lv_label_set_text_fmt(lbl_today, "%.0f m3", d.totalDayM3);

	if (d.pumpFault) {
		lv_label_set_text(lbl_pump, "FALLO");
		lv_obj_set_style_text_color(lbl_pump, COL_WARN, 0);
	} else if (d.pumpRun) {
		lv_label_set_text(lbl_pump, "MARCHA");
		lv_obj_set_style_text_color(lbl_pump, COL_RUN, 0);
	} else {
		lv_label_set_text(lbl_pump, "PARO");
		lv_obj_set_style_text_color(lbl_pump, COL_STOP, 0);
	}

	set_health(lbl_mb, hub.primaryHealth());
	set_health(lbl_lora, hub.backupHealth());
	lv_obj_set_style_text_color(lbl_net, hub.mqttUp() ? COL_RUN : COL_MUTED, 0);
}

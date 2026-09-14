#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include <lvgl.h>
#include <stdio.h>

static lv_obj_t *chart;
static lv_chart_series_t *ser;
static lv_obj_t *lbl_title;
static lv_obj_t *lbl_sub;
static lv_obj_t *lbl_month;
static lv_obj_t *btn_toggle;
static bool      showMonths = false;   /* false = dias (HIST_DAYS), true = meses (HIST_MONTHS) */

static void on_back(lv_event_t *e) {
	(void)e;
	ui_show_well(DataHub::instance().selectedWell());
}

static void on_toggle(lv_event_t *e) {
	(void)e;
	showMonths = !showMonths;
	lv_chart_set_point_count(chart, showMonths ? HIST_MONTHS : HIST_DAYS);
	lv_label_set_text(lv_obj_get_child(btn_toggle, 0), showMonths ? "Ver dias" : "Ver meses");
	screen_history_update();
}

lv_obj_t *screen_history_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	lbl_title = lv_label_create(scr);
	lv_label_set_text(lbl_title, "Historico de caudal");
	lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
	lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 10, 8);

	lbl_sub = lv_label_create(scr);
	lv_label_set_text_fmt(lbl_sub, "Ultimos %d dias  (m3/dia)", HIST_DAYS);
	lv_obj_add_style(lbl_sub, &st_title, 0);
	lv_obj_align(lbl_sub, LV_ALIGN_TOP_LEFT, 10, 30);

	btn_toggle = lv_btn_create(scr);
	lv_obj_set_size(btn_toggle, 90, 24);
	lv_obj_align(btn_toggle, LV_ALIGN_TOP_RIGHT, -10, 26);
	lv_obj_set_style_bg_color(btn_toggle, COL_TEAL, 0);
	lv_obj_add_event_cb(btn_toggle, on_toggle, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(btn_toggle));
	lv_label_set_text(lv_obj_get_child(btn_toggle, 0), "Ver meses");

	chart = lv_chart_create(scr);
	lv_obj_set_size(chart, 296, 118);
	lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 46);
	lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
	lv_chart_set_point_count(chart, HIST_DAYS);
	lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);
	lv_chart_set_div_line_count(chart, 4, 0);
	lv_obj_set_style_bg_color(chart, COL_CARD, 0);
	lv_obj_set_style_border_width(chart, 0, 0);
	lv_obj_set_style_pad_all(chart, 4, 0);
	ser = lv_chart_add_series(chart, COL_TEAL, LV_CHART_AXIS_PRIMARY_Y);

	lv_obj_t *mtitle = lv_label_create(scr);
	lv_label_set_text(mtitle, "ACUMULADO DEL MES");
	lv_obj_add_style(mtitle, &st_title, 0);
	lv_obj_align(mtitle, LV_ALIGN_TOP_LEFT, 10, 174);

	lbl_month = lv_label_create(scr);
	lv_label_set_text(lbl_month, "-- m3");
	lv_obj_set_style_text_font(lbl_month, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_month, COL_TEAL, 0);
	lv_obj_align(lbl_month, LV_ALIGN_TOP_LEFT, 10, 188);

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

void screen_history_update() {
	DataHub &hub = DataHub::instance();
	const WellData &d = hub.data().well[hub.selectedWell()];

	lv_label_set_text_fmt(lbl_title, "Historico  -  %s", d.name);

	const float *arr = showMonths ? d.histMonthM3 : d.histDayM3;
	int          n   = showMonths ? HIST_MONTHS : HIST_DAYS;

	if (showMonths) lv_label_set_text_fmt(lbl_sub, "Ultimos %d meses  (m3/mes)", HIST_MONTHS);
	else            lv_label_set_text_fmt(lbl_sub, "Ultimos %d dias  (m3/dia)", HIST_DAYS);

	float mx = 1.0f;
	for (int i = 0; i < n; i++)
		if (arr[i] > mx) mx = arr[i];
	lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, (lv_coord_t)(mx * 1.15f));

	for (int i = 0; i < n; i++)
		lv_chart_set_value_by_id(chart, ser, i, (lv_coord_t)arr[i]);
	lv_chart_refresh(chart);

	char b[24];
	snprintf(b, sizeof(b), "%.1f m3", d.totalMonthM3);
	lv_label_set_text(lbl_month, b);
}

#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include "data/hmi_config.h"
#include "hal/touch.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

static lv_obj_t *lbl_plc;       /* estado del enlace con el PLC (grande) */
static lv_obj_t *lbl_ip;        /* IP del HMI */
static lv_obj_t *lbl_wifi;      /* SSID + RSSI */
static lv_obj_t *lbl_origin;
static lv_obj_t *lbl_counters;

static void on_back(lv_event_t *e) { (void)e; ui_show_wells(); }
static void on_config(lv_event_t *e) { (void)e; ui_show_pin(ui_show_config); }

static void on_recal(lv_event_t *e) {
	(void)e;
	touch_force_calibrate();
	lv_obj_invalidate(lv_scr_act());
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
	lv_obj_align(lv, LV_ALIGN_TOP_LEFT, 150, y);
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
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 6);

	/* Volver: esquina superior derecha (antes abajo, se solapaba con Escala/Recalibr.) */
	lv_obj_t *back = lv_btn_create(scr);
	lv_obj_set_size(back, 78, 26);
	lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -6, 3);
	lv_obj_set_style_bg_color(back, COL_TEAL_D, 0);
	lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *bl = lv_label_create(back);
	lv_label_set_text(bl, LV_SYMBOL_LEFT " Volver");
	lv_obj_set_style_text_font(bl, &lv_font_montserrat_12, 0);
	lv_obj_center(bl);

	/* estado del enlace con el PLC: la respuesta a "estoy conectado?" */
	lv_obj_t *k = lv_label_create(scr);
	lv_label_set_text(k, "Enlace PLC");
	lv_obj_set_style_text_font(k, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(k, COL_MUTED, 0);
	lv_obj_align(k, LV_ALIGN_TOP_LEFT, 12, 30);
	lbl_plc = lv_label_create(scr);
	lv_label_set_text(lbl_plc, "--");
	lv_obj_set_style_text_font(lbl_plc, &lv_font_montserrat_16, 0);
	lv_obj_align(lbl_plc, LV_ALIGN_TOP_LEFT, 110, 28);

	const HmiConfig &c = hmicfg::get();
	char buf[56];
	snprintf(buf, sizeof(buf), "%s:%u  u%u", c.plcHost, c.plcPort, c.plcUnit);
	row(scr, "PLC destino", buf, 52);
	lbl_ip     = row(scr, "IP del HMI", "--", 72);
	lbl_wifi   = row(scr, "WiFi", c.wifiSsid[0] ? c.wifiSsid : "sin configurar", 92);
	lbl_origin = row(scr, "Origen / latido", "--", 112);
	lbl_counters = row(scr, "Tramas OK / ERR", "0 / 0", 132);
	snprintf(buf, sizeof(buf), "%s%s", hmicfg::source(),
	         hmicfg::sdMounted() ? " (microSD OK)" : " (sin microSD)");
	row(scr, "Config", buf, 152);

	lv_obj_t *bcfg = lv_btn_create(scr);
	lv_obj_set_size(bcfg, 120, 40);
	lv_obj_align(bcfg, LV_ALIGN_TOP_LEFT, 12, 172);
	lv_obj_set_style_bg_color(bcfg, COL_ORANGE, 0);
	lv_obj_add_event_cb(bcfg, on_config, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bcfg));
	lv_label_set_text(lv_obj_get_child(bcfg, 0), "Configuracion");

	/* El boton "Escala" se quito: la calibracion ahora vive en el portal del
	 * nodo remoto (cambio de rumbo 2026-09), no en el HMI. */
	lv_obj_t *brecal = lv_btn_create(scr);
	lv_obj_set_size(brecal, 84, 40);
	lv_obj_align(brecal, LV_ALIGN_TOP_LEFT, 138, 172);
	lv_obj_set_style_bg_color(brecal, COL_TEAL_D, 0);
	lv_obj_add_event_cb(brecal, on_recal, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(brecal));
	lv_label_set_text(lv_obj_get_child(brecal, 0), "Recalibr.");

	lv_obj_t *ver = lv_label_create(scr);
	lv_label_set_text_fmt(ver, "%s  v%s", APP_NAME, APP_VERSION);
	lv_obj_add_style(ver, &st_title, 0);
	lv_obj_align(ver, LV_ALIGN_BOTTOM_LEFT, 10, -10);

	return scr;
}

void screen_settings_update() {
	DataHub &hub = DataHub::instance();
	const PlantData &p = hub.data();
	DataSource *pri = hub.primary();

	bool on_plc = pri && strcmp(hub.activeSourceName(), pri->name()) == 0;
	SrcHealth h = hub.primaryHealth();

	const char *txt; lv_color_t col;
	if (on_plc && h == SrcHealth::Ok)        { txt = "CONECTADO";           col = COL_RUN;  }
	else if (on_plc && h == SrcHealth::Stale){ txt = "INTERMITENTE";        col = COL_WARN; }
	else                                     { txt = "SIN CONEXION (sim)";  col = COL_STOP; }
	lv_label_set_text(lbl_plc, txt);
	lv_obj_set_style_text_color(lbl_plc, col, 0);

	const char *ssid = hmicfg::get().wifiSsid;
	lv_label_set_text_fmt(lbl_ip, "%s", pri ? pri->localIp().c_str() : "-");
	lv_label_set_text_fmt(lbl_wifi, "%s  %d dBm",
	                      ssid[0] ? ssid : "--", pri ? pri->linkRssi() : 0);
	lv_label_set_text_fmt(lbl_origin, "%s  ~ %u",
	                      p.origin ? "LOGO! real" : "PLC-SIM", p.heartbeat);
	lv_label_set_text_fmt(lbl_counters, "%lu / %lu",
	                      (unsigned long)hub.txOk(), (unsigned long)hub.txErr());
}

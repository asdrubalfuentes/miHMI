#include "display.h"
#include <lvgl.h>
#include "config.h"

static TFT_eSPI tft = TFT_eSPI();

/* Buffer parcial de LVGL (1/6 de pantalla aprox). */
static lv_color_t s_buf[SCREEN_W * DRAW_BUF_LINES];
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;

static uint8_t s_bl_pct  = 80;
static bool    s_invert  = true;    /* esta CYD (TPM408-2.8) necesita inversion; ajustable con 'inv' */

TFT_eSPI &display_tft() { return tft; }

void display_set_invert(bool on) {
	s_invert = on;
	tft.invertDisplay(on);
}
bool display_invert() { return s_invert; }

/* --- callback de volcado a pantalla --- */
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);

	tft.startWrite();
	tft.setAddrWindow(area->x1, area->y1, w, h);
	tft.pushColors((uint16_t *)&color_p->full, w * h, true);
	tft.endWrite();

	lv_disp_flush_ready(drv);
}

void display_backlight_pct(uint8_t pct) {
	s_bl_pct = constrain(pct, 0, 100);
	/* analogWrite: portable entre core 2.x y 3.x del ESP32 (8 bits) */
	analogWrite(PIN_TFT_BL, map(s_bl_pct, 0, 100, 0, 255));
}

void display_hw_init() {
	tft.begin();
	tft.setRotation(TFT_ROTATION);
	display_set_invert(s_invert);          /* corrige la inversion del panel */
	tft.fillScreen(TFT_BLACK);
	pinMode(PIN_TFT_BL, OUTPUT);
	display_backlight_pct(s_bl_pct);
}

void display_lvgl_init() {
	lv_disp_draw_buf_init(&s_draw_buf, s_buf, nullptr, SCREEN_W * DRAW_BUF_LINES);

	lv_disp_drv_init(&s_disp_drv);
	s_disp_drv.hor_res  = SCREEN_W;
	s_disp_drv.ver_res  = SCREEN_H;
	s_disp_drv.flush_cb = disp_flush;
	s_disp_drv.draw_buf = &s_draw_buf;
	lv_disp_drv_register(&s_disp_drv);
}

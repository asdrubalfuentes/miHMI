#include "display.h"
#include <lvgl.h>
#include "config.h"

static TFT_eSPI tft = TFT_eSPI();

/* Buffer parcial de LVGL (1/6 de pantalla aprox). */
static lv_color_t s_buf[SCREEN_W * DRAW_BUF_LINES];
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;

static uint8_t s_bl_pct = 80;

TFT_eSPI &display_tft() { return tft; }

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

void display_backlight_auto() {
	int raw = analogRead(PIN_LDR);          /* 0..4095, mas luz -> menor valor en la CYD */
	int lux = map(raw, 0, 4095, 100, 0);
	lux = constrain(lux, 0, 100);
	/* suavizado simple + piso de 15 % para que nunca quede negra */
	uint8_t objetivo = constrain(15 + (lux * 85) / 100, 15, 100);
	int delta = (int)objetivo - (int)s_bl_pct;
	if (abs(delta) >= 3) display_backlight_pct(s_bl_pct + (delta > 0 ? 3 : -3));
}

void display_hw_init() {
	tft.begin();
	tft.setRotation(TFT_ROTATION);
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

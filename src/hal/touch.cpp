#include "touch.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include <lvgl.h>
#include "config.h"
#include "display.h"

/* Bus SPI dedicado para el tactil (independiente del de la pantalla) */
static SPIClass s_touchSPI(VSPI);
static XPT2046_Touchscreen s_ts(PIN_TOUCH_CS, PIN_TOUCH_IRQ);

/* Objetivos en pixeles de la rutina de calibracion */
static const int16_t TGT_TL_X = 20;
static const int16_t TGT_TL_Y = 20;
static const int16_t TGT_BR_X = SCREEN_W - 20;
static const int16_t TGT_BR_Y = SCREEN_H - 20;

/* Calibracion en RAM: valores crudos leidos en cada objetivo */
struct TouchCal {
	int16_t rawTLx = 300,  rawTLy = 300;
	int16_t rawBRx = 3800, rawBRy = 3800;
	bool    valid  = false;
};
static TouchCal s_cal;

static Preferences s_prefs;

/* ------------------------------------------------------------------ */
static void cal_load() {
	s_prefs.begin("tcal", true);
	if (s_prefs.getBool("ok", false)) {
		s_cal.rawTLx = s_prefs.getShort("tlx", s_cal.rawTLx);
		s_cal.rawTLy = s_prefs.getShort("tly", s_cal.rawTLy);
		s_cal.rawBRx = s_prefs.getShort("brx", s_cal.rawBRx);
		s_cal.rawBRy = s_prefs.getShort("bry", s_cal.rawBRy);
		s_cal.valid  = true;
	}
	s_prefs.end();
}

static void cal_save() {
	s_prefs.begin("tcal", false);
	s_prefs.putShort("tlx", s_cal.rawTLx);
	s_prefs.putShort("tly", s_cal.rawTLy);
	s_prefs.putShort("brx", s_cal.rawBRx);
	s_prefs.putShort("bry", s_cal.rawBRy);
	s_prefs.putBool("ok", true);
	s_prefs.end();
	s_cal.valid = true;
}

/* Dibuja una cruz y espera un toque estable; devuelve el promedio crudo. */
static void read_target(int16_t px, int16_t py, int16_t &rawx, int16_t &rawy) {
	TFT_eSPI &tft = display_tft();

	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextDatum(MC_DATUM);
	tft.drawString("Calibracion de pantalla", SCREEN_W / 2, SCREEN_H / 2 - 20, 2);
	tft.drawString("Toque el centro de la cruz", SCREEN_W / 2, SCREEN_H / 2 + 6, 2);

	tft.drawLine(px - 10, py, px + 10, py, TFT_RED);
	tft.drawLine(px, py - 10, px, py + 10, TFT_RED);
	tft.drawCircle(px, py, 6, TFT_YELLOW);

	/* esperar a que suelten un toque previo */
	while (s_ts.touched()) delay(10);
	delay(150);

	/* esperar toque */
	while (!s_ts.touched()) delay(10);

	/* promediar mientras se mantiene pulsado */
	long sx = 0, sy = 0;
	int n = 0;
	while (s_ts.touched() && n < 200) {
		TS_Point p = s_ts.getPoint();
		sx += p.x;
		sy += p.y;
		n++;
		delay(5);
	}
	rawx = (n > 0) ? (int16_t)(sx / n) : 2048;
	rawy = (n > 0) ? (int16_t)(sy / n) : 2048;

	tft.fillCircle(px, py, 4, TFT_GREEN);
	delay(300);
}

static void run_calibration() {
	TFT_eSPI &tft = display_tft();
	tft.fillScreen(TFT_BLACK);

	read_target(TGT_TL_X, TGT_TL_Y, s_cal.rawTLx, s_cal.rawTLy);
	read_target(TGT_BR_X, TGT_BR_Y, s_cal.rawBRx, s_cal.rawBRy);
	cal_save();

	tft.fillScreen(TFT_BLACK);
	tft.setTextDatum(MC_DATUM);
	tft.setTextColor(TFT_GREEN, TFT_BLACK);
	tft.drawString("Calibracion guardada", SCREEN_W / 2, SCREEN_H / 2, 2);
	delay(600);
}

/* ------------------------------------------------------------------ */
void touch_hw_init() {
	s_touchSPI.begin(PIN_TOUCH_CLK, PIN_TOUCH_MISO, PIN_TOUCH_MOSI, PIN_TOUCH_CS);
	s_ts.begin(s_touchSPI);
	s_ts.setRotation(TOUCH_ROTATION);
	cal_load();
}

void touch_calibrate_if_needed() {
	if (!s_cal.valid) run_calibration();
}

void touch_force_calibrate() {
	run_calibration();
}

/* --- callback de lectura para LVGL --- */
static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
	(void)drv;
	if (!s_ts.touched()) {
		data->state = LV_INDEV_STATE_RELEASED;
		return;
	}
	TS_Point p = s_ts.getPoint();

	int32_t x = map(p.x, s_cal.rawTLx, s_cal.rawBRx, TGT_TL_X, TGT_BR_X);
	int32_t y = map(p.y, s_cal.rawTLy, s_cal.rawBRy, TGT_TL_Y, TGT_BR_Y);

	data->point.x = (lv_coord_t)constrain(x, 0, SCREEN_W - 1);
	data->point.y = (lv_coord_t)constrain(y, 0, SCREEN_H - 1);
	data->state   = LV_INDEV_STATE_PRESSED;
}

void touch_lvgl_init() {
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type    = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = touch_read;
	lv_indev_drv_register(&indev_drv);
}

#include "ota_hmi.h"
#include "net/ota_update.h"
#include "config.h"
#include "hal/display.h"
#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>

// Integracion del OTA en el HMI. Ver ota_hmi.h.

namespace {

volatile bool  s_request   = false;
uint32_t       s_lastCheck = 0;
const uint32_t CHECK_EVERY_MS = 6UL * 3600UL * 1000UL;

void draw(ota::Phase ph, int pct, const char *d) {
	TFT_eSPI &t = display_tft();
	t.fillScreen(TFT_BLACK);
	t.setTextColor(TFT_WHITE, TFT_BLACK);
	t.setTextDatum(MC_DATUM);
	t.drawString("Actualizacion de firmware", SCREEN_W / 2, 34, 4);

	const char *m = "";
	switch (ph) {
		case ota::Phase::Check:    m = "Buscando version..."; break;
		case ota::Phase::UpToDate: m = "Ya esta al dia";      break;
		case ota::Phase::Download: m = "Descargando...";      break;
		case ota::Phase::Verify:   m = "Verificando...";      break;
		case ota::Phase::Flash:    m = "Escribiendo...";      break;
		case ota::Phase::Done:     m = "Listo. Reiniciando";  break;
		case ota::Phase::Error:    m = "Error";               break;
	}
	t.drawString(m, SCREEN_W / 2, 84, 4);

	if (ph == ota::Phase::Download) {
		int w = SCREEN_W - 60;
		int fill = (w - 4) * (pct < 0 ? 0 : pct > 100 ? 100 : pct) / 100;
		t.drawRect(30, 126, w, 26, TFT_WHITE);
		t.fillRect(32, 128, fill, 22, TFT_GREEN);
		char b[8];
		snprintf(b, sizeof(b), "%d%%", pct);
		t.drawString(b, SCREEN_W / 2, 172, 4);
	}
	if (d && *d) t.drawString(d, SCREEN_W / 2, 206, 2);
}

void run_interactive() {
	ota::Config oc;
	oc.owner = OTA_GH_OWNER;
	oc.repo  = OTA_GH_REPO;
	oc.currentVersion = APP_VERSION;

	ota::Result r = ota::run(oc, draw);   // si actualiza, reinicia dentro
	if (r.ok && !r.hasUpdate)      { draw(ota::Phase::UpToDate, 0, "v" APP_VERSION); delay(1800); }
	else if (!r.ok)               { draw(ota::Phase::Error, 0, r.error);            delay(2600); }

	/* LVGL cree que su pantalla sigue dibujada: forzar un repintado completo. */
	lv_obj_invalidate(lv_scr_act());
}

}  // namespace

namespace otaHmi {

void request() { s_request = true; }

void service() {
	if (WiFi.status() != WL_CONNECTED) return;

	if (s_request) {
		s_request   = false;
		s_lastCheck = millis();
		run_interactive();
		return;
	}

	/* el chequeo periodico espera a que el arranque se considere estable, para no
	 * pelear con el contador de reinicios de panic_screen. */
	if (millis() < BOOT_STABLE_MS + 5000UL) return;
	if (s_lastCheck != 0 && millis() - s_lastCheck < CHECK_EVERY_MS) return;
	s_lastCheck = millis();

	ota::Config oc;
	oc.owner = OTA_GH_OWNER;
	oc.repo  = OTA_GH_REPO;
	oc.currentVersion = APP_VERSION;
	ota::Result r = ota::check(oc, nullptr);    // silencioso
	if (r.ok && r.hasUpdate) run_interactive(); // hay version nueva -> aplica con pantalla
}

}  // namespace otaHmi

#include "panic_screen.h"
#include "display.h"
#include "config.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_system.h>
#include <Preferences.h>

// Aviso de fallo del HMI + modo basico tras reinicios en cadena.  Ver panic_screen.h.
//
// El motivo del reset se pide a esp_reset_reason() DESPUES de reiniciar: dibujar
// durante el panic real no es fiable (interrupciones cortadas, SPI a medias), asi
// que el patron es panic -> reboot -> este modulo lo detecta y lo explica.
//
// El contador de reinicios anormales va en NVS (flash): sobrevive a panic,
// watchdog, brownout y corte de alimentacion.  RTC_DATA_ATTR NO sirve: el
// arranque pone a cero .rtc.bss en cada reset.  La etapa de arranque es solo
// diagnostico y va en RTC_NOINIT_ATTR (mejor esfuerzo, sin desgaste de flash).

#define STAGE_MAGIC  0xB0075747u

RTC_NOINIT_ATTR static uint32_t s_stageMagic;
RTC_NOINIT_ATTR static char     s_stage[24];

static bool s_safeMode = false;

void panic_screen_stage(const char *name) {
	s_stageMagic = STAGE_MAGIC;
	strncpy(s_stage, name ? name : "?", sizeof(s_stage) - 1);
	s_stage[sizeof(s_stage) - 1] = 0;
}

bool panic_safe_mode() { return s_safeMode; }

/* --- contador de reinicios anormales en NVS --- */
static uint32_t fails_get() {
	Preferences p;
	p.begin("boot", true);
	uint32_t n = p.getUInt("fails", 0);
	p.end();
	return n;
}
static void fails_set(uint32_t n) {
	Preferences p;
	p.begin("boot", false);
	p.putUInt("fails", n);
	p.end();
}

void panic_boot_ok() {
	if (fails_get() != 0) {
		fails_set(0);
		Serial.println("[boot] arranque estable: contador de reinicios a 0");
	}
}

/* --- pintado comun: banda superior, titulo y hasta 3 lineas --- */
static void draw(uint16_t bg, const char *kicker, const char *title,
                 const char *l1, const char *l2, const char *l3) {
	TFT_eSPI &tft = display_tft();

	display_backlight_pct(100);
	tft.fillScreen(bg);
	tft.setTextColor(TFT_WHITE, bg);

	tft.setTextDatum(TL_DATUM);
	tft.drawString(kicker, 12, 12, 2);
	tft.drawFastHLine(0, 34, SCREEN_W, TFT_WHITE);

	tft.setTextDatum(MC_DATUM);
	uint8_t tf = (title && strlen(title) <= 18) ? 4 : 2;
	tft.drawString(title, SCREEN_W / 2, 62, tf);

	tft.setTextDatum(TL_DATUM);
	int16_t y = 100;
	if (l1) { tft.drawString(l1, 12, y, 2); y += 24; }
	if (l2) { tft.drawString(l2, 12, y, 2); y += 24; }
	if (l3) { tft.drawString(l3, 12, y, 2); y += 24; }

	tft.setTextDatum(BC_DATUM);
	tft.setTextColor(TFT_YELLOW, bg);
	tft.drawString(APP_NAME "  v" APP_VERSION, SCREEN_W / 2, SCREEN_H - 8, 2);
}

void panic_screen_fatal(const char *title, const char *l1,
                        const char *l2, const char *l3) {
	Serial.printf("[FATAL] %s | %s | %s | %s\n",
	              title ? title : "", l1 ? l1 : "", l2 ? l2 : "", l3 ? l3 : "");
	draw(TFT_RED, "FALLO FATAL - EQUIPO DETENIDO", title, l1, l2, l3);
	s_stageMagic = 0;                  /* no es un crash: no re-avisar al reiniciar */
	for (;;) delay(1000);
}

bool panic_screen_check_reset() {
	esp_reset_reason_t r = esp_reset_reason();

	if (r == ESP_RST_POWERON) {        /* encendido en frio: empezar limpio */
		fails_set(0);
		return false;
	}

	const char *why;
	const char *hint;
	switch (r) {
		case ESP_RST_PANIC:
			why = "Excepcion de software";
			hint = "Vea el backtrace en el monitor serie.";
			break;
		case ESP_RST_TASK_WDT:
			why = "Bloqueo (watchdog de tarea)";
			hint = "Un modulo no respondio > 5 s.";
			break;
		case ESP_RST_INT_WDT:
		case ESP_RST_WDT:
			why = "Watchdog";
			hint = "Vea el backtrace en el monitor serie.";
			break;
		case ESP_RST_BROWNOUT:
			why = "Alimentacion insuficiente";
			hint = "Cable USB corto o fuente 5V de 1A.";
			break;
		default:
			return false;              /* reset por software, deep-sleep, etc. */
	}

	uint32_t fails = fails_get() + 1;
	fails_set(fails);

	char stage[40];
	if (s_stageMagic == STAGE_MAGIC && s_stage[0])
		snprintf(stage, sizeof(stage), "Se reinicio durante: %s", s_stage);
	else
		snprintf(stage, sizeof(stage), "Se reinicio durante: (desconocido)");

	Serial.printf("[RESET] motivo=%d (%s) | %s | fallos seguidos=%u/%u\n",
	              (int)r, why, stage, (unsigned)fails, BOOT_MAX_FAILS);

	if (fails >= BOOT_MAX_FAILS) {
		s_safeMode = true;
		Serial.println("[RESET] limite alcanzado -> MODO BASICO (defaults)");
		draw(TFT_NAVY, "MODO BASICO", "Config. por defecto",
		     why, stage, "microSD y ajustes guardados: OMITIDOS");
		delay(7000);
		return true;
	}

	char line[40];
	snprintf(line, sizeof(line), "Reinicios seguidos: %u de %u",
	         (unsigned)fails, BOOT_MAX_FAILS);
	draw(TFT_RED, "EL HMI SE REINICIO SOLO", why, stage, hint, line);
	delay(7000);
	return true;
}

#include "light_ctrl.h"
#include "display.h"
#include "config.h"

// Ver light_ctrl.h.
//
// Pipeline:  ADC (GPIO34) -> mediana anti-glitch -> luz% (2 puntos de calib.) ->
//            EMA -> escena (con histeresis) -> backlight (con slew).
// Trabajar en "luz %" hace el ajuste independiente de la polaridad del LDR:
// rawBright y rawDark se miden tal cual y da igual cual sea mayor.

namespace {

LightCfg     s_cfg;
int          s_raw   = 0;      /* ADC desglicheado 0..4095            */
int          s_luz   = 0;      /* 0..100 % de luz (segun calibracion) */
float        s_ema   = 0.0f;   /* EMA de s_luz                        */
uint8_t      s_bl    = 80;
light::Scene s_scene = light::SC_NIGHT;

int  s_hist[5] = { 0, 0, 0, 0, 0 };
uint8_t s_hi   = 0;
bool s_histFull = false;

int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

int median5(int a, int b, int c, int d, int e) {
	int x[5] = { a, b, c, d, e };
	for (int i = 1; i < 5; i++) {
		int k = x[i], j = i - 1;
		while (j >= 0 && x[j] > k) { x[j + 1] = x[j]; j--; }
		x[j + 1] = k;
	}
	return x[2];
}

/* El LDR de la CYD (GPIO34, solo-entrada, sin pull) mete caidas sueltas.
   Mediana de 5 lecturas seguidas -> quita el pico intra-tick. */
int read_ldr() {
	return median5(analogRead(PIN_LDR), analogRead(PIN_LDR), analogRead(PIN_LDR),
	               analogRead(PIN_LDR), analogRead(PIN_LDR));
}

/* raw -> % de luz, interpolando entre los 2 puntos de calibracion.
   Da igual si rawBright < rawDark o al reves. */
int luz_pct(int raw) {
	int b = s_cfg.rawBright, d = s_cfg.rawDark;
	if (b == d) return 50;
	long p = 100L * (raw - d) / (b - d);
	return (int)clampi((int)p, 0, 100);
}

const char *name_of(light::Scene s) {
	switch (s) {
		case light::SC_CLOSED: return "CERRADO";
		case light::SC_NIGHT:  return "NOCHE";
		case light::SC_DAY:    return "DIA";
		case light::SC_BRIGHT: return "DIA_FUERTE";
	}
	return "?";
}

light::Scene classify(int luz) {
	int h = s_cfg.hystPct;

	/* CERRADO <-> ABIERTO con histeresis */
	if (s_scene == light::SC_CLOSED) {
		if (luz < (int)s_cfg.pctClosed + h) return light::SC_CLOSED;
	} else {
		if (luz <= (int)s_cfg.pctClosed - h) return light::SC_CLOSED;
	}

	if (luz >= (int)s_cfg.pctDay)   return light::SC_BRIGHT;
	if (luz >= (int)s_cfg.pctTheme) return light::SC_DAY;
	return light::SC_NIGHT;
}

uint8_t target_bl(int luz) {
	const LightCfg &c = s_cfg;
	if (!c.enabled) return c.blManual;

	switch (s_scene) {
		case light::SC_CLOSED: return c.blClosed;
		case light::SC_BRIGHT: return c.blMax;
		default: {
			int lo = c.pctClosed, hi = c.pctDay;
			if (hi <= lo) hi = lo + 1;
			int p = clampi((luz - lo) * 100 / (hi - lo), 0, 100);
			return (uint8_t)(c.blMin + (int)(c.blMax - c.blMin) * p / 100);
		}
	}
}

}  // namespace

namespace light {

void begin() {
	s_cfg   = hmicfg::get().light;
	s_raw   = read_ldr();
	for (int i = 0; i < 5; i++) s_hist[i] = s_raw;
	s_histFull = true;
	s_luz   = luz_pct(s_raw);
	s_ema   = s_luz;
	s_scene = classify(s_luz);
	s_bl    = target_bl(s_luz);
	display_backlight_pct(s_bl);
}

void tick() {
	s_raw = read_ldr();
	s_hist[s_hi] = s_raw;
	s_hi = (uint8_t)((s_hi + 1) % 5);
	if (s_hi == 0) s_histFull = true;

	int base = s_histFull
	         ? median5(s_hist[0], s_hist[1], s_hist[2], s_hist[3], s_hist[4])
	         : s_raw;

	s_luz = luz_pct(base);
	s_ema += (s_luz - s_ema) * 0.30f;
	int v = (int)(s_ema + 0.5f);

	Scene prev = s_scene;
	s_scene    = classify(v);
	uint8_t t  = target_bl(v);

	if (s_scene != prev)
		Serial.printf("[light] escena  %s -> %s   raw=%d luz=%d%% ema=%d%%  backlight->%u%%\n",
		              name_of(prev), name_of(s_scene), s_raw, s_luz, v, t);

	int d = (int)t - (int)s_bl;                /* slew: como mucho 4 %/tick */
	d = clampi(d, -4, 4);
	if (d != 0) {
		s_bl = (uint8_t)clampi((int)s_bl + d, 0, 100);
		display_backlight_pct(s_bl);
	}
}

int   raw()   { return s_raw; }
int   luz()   { return s_luz; }
int   ema()   { return (int)(s_ema + 0.5f); }
Scene scene() { return s_scene; }
const char *sceneName() { return name_of(s_scene); }
uint8_t backlight() { return s_bl; }

/* Respaldo para THEME_AUTO: "de dia" = luz ambiente alta y sostenida.
   Con histeresis amplia (30..55 %) para no parpadear entre claro y oscuro. */
bool ambientBright() {
	static bool bright = false;
	int e = ema();
	if (!bright && e >= 55) bright = true;
	else if (bright && e < 30) bright = false;
	return bright;
}

int  calSample() { return s_raw; }

const LightCfg &cfg() { return s_cfg; }
void            setCfg(const LightCfg &lc) { s_cfg = lc; }

}  // namespace light

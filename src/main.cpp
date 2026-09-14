/**
 * miHMI  -  HMI de captacion de pozos profundos
 * Placa: Cheap Yellow Display  ESP32-2432S028R  (ILI9341 320x240 + XPT2046)
 * GUI:   LVGL 8.4
 *
 * Fase 1: esqueleto + interfaz grafica (splash -> pantalla inicial) con datos
 *         simulados (MockSource).  La comunicacion de campo (Modbus RTU / RS485
 *         y pasarela LoRa) se anade en fases posteriores sobre la misma capa
 *         de abstraccion (DataSource / DataHub).
 */
#include <Arduino.h>
#include <lvgl.h>

#include "config.h"
#include "hal/display.h"
#include "hal/touch.h"
#include "hal/panic_screen.h"
#include "hal/light_ctrl.h"
#include "hal/net_clock.h"
#include "net/ota_hmi.h"
#include "ui/ui.h"
#include "ui/theme.h"
#include "data/data_hub.h"
#include "data/hist_log.h"
#include "data/hmi_config.h"
#include "data/mock_source.h"
#include "data/modbus_tcp_source.h"

static MockSource      g_mock;   /* respaldo / desarrollo sin PLC */
static ModbusTcpSource g_plc;    /* primaria: Modbus TCP -> Mapa B (LOGO! 9 / PLC-SIM) */

#if LV_USE_LOG
static void lv_log_cb(const char *buf) {
	Serial.print("[LVGL] ");
	Serial.println(buf);
}
#endif

void setup() {
	Serial.begin(115200);
	delay(200);
	Serial.println();
	Serial.println(APP_NAME "  v" APP_VERSION);

	/* LED RGB (activo a nivel bajo en la CYD): apagar */
	pinMode(PIN_LED_R, OUTPUT); digitalWrite(PIN_LED_R, HIGH);
	pinMode(PIN_LED_G, OUTPUT); digitalWrite(PIN_LED_G, HIGH);
	pinMode(PIN_LED_B, OUTPUT); digitalWrite(PIN_LED_B, HIGH);

	/* Pantalla + tactil ANTES de LVGL (la calibracion dibuja con TFT_eSPI) */
	panic_screen_stage("pantalla");
	display_hw_init();

	/* Si el arranque anterior murio por panic / watchdog / brownout, avisar
	   a pantalla completa antes de seguir. */
	panic_screen_check_reset();

	panic_screen_stage("tactil");
	touch_hw_init();
	touch_calibrate_if_needed();

	/* Configuracion persistente: microSD -> NVS -> defaults (config.h/secrets.h).
	   Va DESPUES de la pantalla/tactil: si la microSD falla o cuelga el bus SPI,
	   la interfaz igual arranca y el fallo se ve, en vez de quedar a oscuras. */
	panic_screen_stage("config/SD");
	hmicfg::begin(panic_safe_mode());
	histlog::begin();            /* historico de dia/mes desde microSD (/hist.json) */
	display_set_invert(hmicfg::get().dispInvert);   /* inversion de color del panel */

	/* Adaptacion de pantalla al ambiente (LDR).  Parametros por consola serie. */
	panic_screen_stage("luz");
	light::begin();

	/* LVGL */
	panic_screen_stage("LVGL");
	lv_init();
#if LV_USE_LOG
	lv_log_register_print_cb(lv_log_cb);
#endif
	display_lvgl_init();
	touch_lvgl_init();

	/* Datos: primaria Modbus TCP (Mapa B); respaldo simulado con failover */
	panic_screen_stage("datos");
	DataHub::instance().setPrimary(&g_plc);
	DataHub::instance().setBackup(&g_mock);
	DataHub::instance().begin();

	netclock::begin();          /* SNTP: sincroniza cuando la WiFi este arriba */

	/* Interfaz */
	panic_screen_stage("UI");
	theme_init((ThemeMode)hmicfg::get().theme);   /* paleta antes de crear pantallas */
	ui_init();

	Serial.println("HMI listo.");
	panic_screen_stage("run");
}

/* ===== Consola serie ====================================================
   Una linea + Enter.

   BRILLO (LDR, independiente del tema):
     ldr                         streamea LDR + luz% + escena + brillo ('q' para)
     q                           detiene el stream
     light                       muestra parametros y estado
     light cal bright|dark       fija rawBright/rawDark = raw actual
     light set <param> <n>       enabled  bright dark (raw)  closed mid day hyst (%)
                                 blmin blmax blclosed blmanual (%)
     light default | light save

   TEMA (colores, independiente del brillo):
     theme                       muestra el tema actual
     theme auto|claro|oscuro     fija y guarda el modo (auto = por luz ambiente)
*/
static bool     s_ldrStream = false;
static uint32_t s_ldrNext   = 0;
static int      s_ldrMin    = 4095;
static int      s_ldrMax    = 0;

static int cli_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void cli_now() {
	Serial.printf("[light] estado: raw=%d  luz=%d%%  ema=%d%%  escena=%s  backlight=%u%%\n",
	              light::raw(), light::luz(), light::ema(), light::sceneName(),
	              light::backlight());
}

static const char *theme_mode_name(ThemeMode m) {
	return m == THEME_LIGHT ? "claro" : m == THEME_DARK ? "oscuro" : "auto";
}

static void cli_print_light() {
	const LightCfg &c = light::cfg();
	Serial.println(F("--- light : adaptacion de pantalla (2 puntos de calib. + umbrales en % de luz) ---"));
	Serial.printf("  enabled   %-5u  1=brillo auto por LDR, 0=fijo (blmanual)\n", c.enabled);
	Serial.printf("  bright    %-5u  raw con LUZ plena  -> luz 100%%   ('light cal bright')\n", c.rawBright);
	Serial.printf("  dark      %-5u  raw a OSCURAS      -> luz 0%%     ('light cal dark')\n", c.rawDark);
	Serial.printf("  closed    %-5u  luz%% <= -> CERRADO (backlight = blclosed)\n", c.pctClosed);
	Serial.printf("  mid       %-5u  luz%% >= -> escena DIA (por debajo, NOCHE)\n", c.pctTheme);
	Serial.printf("  day       %-5u  luz%% >= -> DIA_FUERTE (backlight = blmax)\n", c.pctDay);
	Serial.printf("  hyst      %-5u  histeresis del umbral CERRADO (%% de luz)\n", c.hystPct);
	Serial.printf("  blmin     %-5u  %% backlight minimo estando abierto\n", c.blMin);
	Serial.printf("  blmax     %-5u  %% backlight maximo\n", c.blMax);
	Serial.printf("  blclosed  %-5u  %% backlight con el tablero cerrado\n", c.blClosed);
	Serial.printf("  blmanual  %-5u  %% backlight fijo cuando enabled=0\n", c.blManual);
	Serial.printf("  >> ahora: raw=%d  luz=%d%%  ema=%d%%  escena=%s  backlight=%u%%\n",
	              light::raw(), light::luz(), light::ema(), light::sceneName(),
	              light::backlight());
}

static void cli_exec(char *line) {
	char *tok[4] = { nullptr, nullptr, nullptr, nullptr };
	int   nt = 0;
	for (char *p = strtok(line, " \t"); p && nt < 4; p = strtok(nullptr, " \t")) tok[nt++] = p;
	if (nt == 0) return;

	if (!strcmp(tok[0], "ldr")) {
		s_ldrStream = true; s_ldrNext = millis(); s_ldrMin = 4095; s_ldrMax = 0;
		Serial.println("[ldr] stream ON  ->  'q' para parar");
		return;
	}
	if (!strcmp(tok[0], "q")) { s_ldrStream = false; Serial.println("[ldr] stream OFF"); return; }

	if (!strcmp(tok[0], "time")) {
		if (nt >= 3 && !strcmp(tok[1], "server")) { hmicfg::saveTime(tok[2], nullptr); netclock::begin(); }
		else if (nt >= 3 && !strcmp(tok[1], "tz")) { hmicfg::saveTime(nullptr, tok[2]); netclock::begin(); }
		char st[40]; netclock::status(st, sizeof(st));
		Serial.printf("[time] %s   servidor=%s   tz=%s\n",
		              st, hmicfg::get().ntpServer, hmicfg::get().tz);
		return;
	}

	if (!strcmp(tok[0], "depth")) {
		if (nt >= 2) hmicfg::saveLevelMaxM((uint8_t)cli_clamp(atoi(tok[1]), 1, 255));
		Serial.printf("[depth] profundidad a fondo de escala = %u m (fallback sin escala del PLC)\n",
		              hmicfg::get().levelMaxM);
		return;
	}

	if (!strcmp(tok[0], "inv")) {
		bool on = (nt >= 2) ? (atoi(tok[1]) != 0) : !display_invert();
		display_set_invert(on);
		hmicfg::saveDispInvert(on ? 1 : 0);
		Serial.printf("[disp] inversion de color = %s (guardada)\n", on ? "ON" : "OFF");
		return;
	}

	if (!strcmp(tok[0], "theme")) {
		if (nt >= 2) {
			ThemeMode m;
			if      (!strcmp(tok[1], "auto"))   m = THEME_AUTO;
			else if (!strcmp(tok[1], "claro"))  m = THEME_LIGHT;
			else if (!strcmp(tok[1], "oscuro")) m = THEME_DARK;
			else { Serial.println("uso: theme <auto|claro|oscuro>"); return; }
			theme_set_mode(m);
			hmicfg::saveTheme((uint8_t)m);
		}
		Serial.printf("[theme] modo=%s  ahora=%s\n",
		              theme_mode_name(theme_mode()), theme_is_dark() ? "oscuro" : "claro");
		return;
	}

	if (!strcmp(tok[0], "light")) {
		if (nt == 1) { cli_print_light(); return; }
		LightCfg c = light::cfg();

		if (!strcmp(tok[1], "save"))    { hmicfg::saveLight(c); return; }
		if (!strcmp(tok[1], "cal")) {
			if (nt < 3) { Serial.println("uso: light cal <bright|dark>"); return; }
			int r = light::calSample();
			if      (!strcmp(tok[2], "bright")) c.rawBright = (uint16_t)r;
			else if (!strcmp(tok[2], "dark"))   c.rawDark   = (uint16_t)r;
			else { Serial.println("uso: light cal <bright|dark>"); return; }
			light::setCfg(c);
			light::tick();
			Serial.printf("[light] cal %s = raw %d  (bright=%u dark=%u).  'light save' para persistir.\n",
			              tok[2], r, c.rawBright, c.rawDark);
			cli_now();
			return;
		}
		if (!strcmp(tok[1], "default")) {
			c = hmicfg::lightDefaults();
			light::setCfg(c);
			light::tick();
			Serial.println("[light] valores de fabrica aplicados (usa 'light save' para fijarlos)");
			cli_print_light();
			return;
		}
		if (!strcmp(tok[1], "set")) {
			if (nt < 4) { Serial.println("uso: light set <param> <n>"); return; }
			const char *k = tok[2];
			int v = atoi(tok[3]);
			if      (!strcmp(k, "enabled"))  c.enabled   = v ? 1 : 0;
			else if (!strcmp(k, "bright"))   c.rawBright = (uint16_t)cli_clamp(v, 0, 4095);
			else if (!strcmp(k, "dark"))     c.rawDark   = (uint16_t)cli_clamp(v, 0, 4095);
			else if (!strcmp(k, "closed"))   c.pctClosed = (uint8_t) cli_clamp(v, 0, 100);
			else if (!strcmp(k, "day"))      c.pctDay    = (uint8_t) cli_clamp(v, 0, 100);
			else if (!strcmp(k, "mid"))      c.pctTheme  = (uint8_t) cli_clamp(v, 0, 100);
			else if (!strcmp(k, "hyst"))     c.hystPct   = (uint8_t) cli_clamp(v, 0, 50);
			else if (!strcmp(k, "blmin"))    c.blMin     = (uint8_t) cli_clamp(v, 0, 100);
			else if (!strcmp(k, "blmax"))    c.blMax     = (uint8_t) cli_clamp(v, 0, 100);
			else if (!strcmp(k, "blclosed")) c.blClosed  = (uint8_t) cli_clamp(v, 0, 100);
			else if (!strcmp(k, "blmanual")) c.blManual  = (uint8_t) cli_clamp(v, 0, 100);
			else { Serial.printf("param desconocido: '%s'\n", k); return; }
			light::setCfg(c);
			light::tick();                 /* re-evalua ya: dispara aviso si cambia de escena */
			Serial.printf("[light] %s aplicado (%d).  'light save' para persistir.\n", k, v);
			cli_now();
			return;
		}
		Serial.println("uso: light | light cal <bright|dark> | light set <param> <n> | light default | light save");
		return;
	}

	Serial.println("comandos: ldr | q | light | theme");
}

static void serial_console() {
	static char   line[48];
	static uint8_t n = 0;

	while (Serial.available()) {
		char c = (char)Serial.read();
		if (c == '\r') continue;
		if (c != '\n' && n < sizeof(line) - 1) { line[n++] = c; continue; }
		line[n] = 0;
		n = 0;
		if (line[0]) cli_exec(line);
	}

	if (s_ldrStream && (int32_t)(millis() - s_ldrNext) >= 0) {
		s_ldrNext += 300;
		int r = light::raw();
		if (r < s_ldrMin) s_ldrMin = r;
		if (r > s_ldrMax) s_ldrMax = r;
		Serial.printf("[ldr] raw=%4d luz=%3d%% ema=%3d%%  %-10s  bl=%3u%%  rawmin=%4d rawmax=%4d\n",
		              r, light::luz(), light::ema(), light::sceneName(), light::backlight(),
		              s_ldrMin, s_ldrMax);
	}
}

void loop() {
	static uint32_t t_app = 0;
	static bool     boot_ok = false;

	serial_console();
	lv_timer_handler();
	g_plc.service();          /* bombea la pila Modbus TCP en cada iteracion */

	uint32_t now = millis();
	if (!boot_ok && now >= BOOT_STABLE_MS) {   /* llevamos un rato sin caer */
		boot_ok = true;
		panic_boot_ok();                       /* pone a cero el contador de reinicios */
	}
	if (now - t_app >= APP_TICK_MS) {
		t_app = now;
		DataHub::instance().tick();
		ui_tick();
		light::tick();                              /* LDR -> backlight */
		theme_eval_auto(light::ambientBright());    /* THEME_AUTO -> claro/oscuro por luz */
		otaHmi::service();                          /* OTA: peticion manual o chequeo cada 6 h */
	}

	delay(LVGL_TASK_MS);
}

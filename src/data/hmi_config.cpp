#include "hmi_config.h"
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Almacen de configuracion: microSD (JSON) con respaldo en NVS y valores por
// defecto de config.h / secrets.h. Ver hmi_config.h.

namespace {

HmiConfig   g_cfg;
bool        g_sd  = false;
const char *g_src = "defaults";

SPIClass    g_sdSPI(SD_SPI_HOST);
Preferences g_nvs;

void cpstr(char *dst, const char *src, size_t n) {
	if (!src) { dst[0] = 0; return; }
	strncpy(dst, src, n - 1);
	dst[n - 1] = 0;
}

uint32_t fnv1a(const char *s) {
	uint32_t h = 2166136261u;
	for (; s && *s; ++s) { h ^= (uint8_t)*s; h *= 16777619u; }
	return h;
}

bool scaleVarEq(const ScaleVar &a, const ScaleVar &b) {
	return a.rawMin == b.rawMin && a.rawMax == b.rawMax && a.engMin == b.engMin
	    && a.engMax == b.engMax && a.unit   == b.unit   && a.filter == b.filter;
}

void apply_defaults() {
	cpstr(g_cfg.wifiSsid, WIFI_SSID, sizeof(g_cfg.wifiSsid));
	cpstr(g_cfg.wifiPass, WIFI_PASS, sizeof(g_cfg.wifiPass));
	cpstr(g_cfg.plcHost,  PLC_HOST,  sizeof(g_cfg.plcHost));
	g_cfg.plcPort = PLC_PORT;
	g_cfg.plcUnit = PLC_UNIT;
	g_cfg.pollMs  = MB_POLL_MS;
	g_cfg.pinHash = fnv1a(HMI_ADMIN_PIN_DEFAULT);
	g_cfg.theme   = 0;
	g_cfg.dispInvert = 1;
	g_cfg.levelMaxM = 80;
	cpstr(g_cfg.ntpServer, "ntp.shoa.cl", sizeof(g_cfg.ntpServer));
	cpstr(g_cfg.tz, "<-04>4<-03>,M9.1.6/24,M4.1.6/24", sizeof(g_cfg.tz));  /* Chile continental */
	for (uint8_t s = 0; s < NUM_WELLS; s++)
		snprintf(g_cfg.stationName[s], WELL_NAME_LEN, "Estacion %u", (unsigned)(s + 1));

	g_cfg.light = hmicfg::lightDefaults();
}

// ---- NVS ----------------------------------------------------------------
void nvs_load() {
	g_nvs.begin("hmicfg", true);
	if (g_nvs.getBool("set", false)) {
		String ssid = g_nvs.getString("ssid", g_cfg.wifiSsid);
		String pass = g_nvs.getString("pass", g_cfg.wifiPass);
		String host = g_nvs.getString("host", g_cfg.plcHost);
		cpstr(g_cfg.wifiSsid, ssid.c_str(), sizeof(g_cfg.wifiSsid));
		cpstr(g_cfg.wifiPass, pass.c_str(), sizeof(g_cfg.wifiPass));
		cpstr(g_cfg.plcHost,  host.c_str(), sizeof(g_cfg.plcHost));
		{ String s = g_nvs.getString("wip",   g_cfg.wifiIp);   cpstr(g_cfg.wifiIp,   s.c_str(), sizeof(g_cfg.wifiIp)); }
		{ String s = g_nvs.getString("wgw",   g_cfg.wifiGw);   cpstr(g_cfg.wifiGw,   s.c_str(), sizeof(g_cfg.wifiGw)); }
		{ String s = g_nvs.getString("wmask", g_cfg.wifiMask); cpstr(g_cfg.wifiMask, s.c_str(), sizeof(g_cfg.wifiMask)); }
		{ String s = g_nvs.getString("wdns1", g_cfg.wifiDns1); cpstr(g_cfg.wifiDns1, s.c_str(), sizeof(g_cfg.wifiDns1)); }
		{ String s = g_nvs.getString("wdns2", g_cfg.wifiDns2); cpstr(g_cfg.wifiDns2, s.c_str(), sizeof(g_cfg.wifiDns2)); }
		g_cfg.plcPort = g_nvs.getUShort("port", g_cfg.plcPort);
		g_cfg.plcUnit = g_nvs.getUChar("unit", g_cfg.plcUnit);
		g_cfg.pollMs  = g_nvs.getUShort("poll", g_cfg.pollMs);
		g_cfg.pinHash = g_nvs.getULong("pin", g_cfg.pinHash);
		g_cfg.theme   = g_nvs.getUChar("theme", g_cfg.theme);
		g_cfg.dispInvert = g_nvs.getUChar("dinv", g_cfg.dispInvert);
		g_cfg.levelMaxM  = g_nvs.getUChar("lvlmax", g_cfg.levelMaxM);
		{ String s = g_nvs.getString("ntp", g_cfg.ntpServer); cpstr(g_cfg.ntpServer, s.c_str(), sizeof(g_cfg.ntpServer)); }
		{ String s = g_nvs.getString("tz",  g_cfg.tz);        cpstr(g_cfg.tz, s.c_str(), sizeof(g_cfg.tz)); }
		if (g_nvs.getBytesLength("light") == sizeof(g_cfg.light))    /* si no, se queda el default */
			g_nvs.getBytes("light", &g_cfg.light, sizeof(g_cfg.light));
		if (g_nvs.getBytesLength("scalec") == sizeof(g_cfg.scaleCache))
			g_nvs.getBytes("scalec", &g_cfg.scaleCache, sizeof(g_cfg.scaleCache));
		for (uint8_t s = 0; s < NUM_WELLS; s++) {
			char k[8]; snprintf(k, sizeof(k), "n%u", s);
			String n = g_nvs.getString(k, g_cfg.stationName[s]);
			cpstr(g_cfg.stationName[s], n.c_str(), WELL_NAME_LEN);
		}
		g_src = "NVS";
	}
	g_nvs.end();
}

void nvs_save() {
	g_nvs.begin("hmicfg", false);
	g_nvs.putString("ssid", g_cfg.wifiSsid);
	g_nvs.putString("pass", g_cfg.wifiPass);
	g_nvs.putString("host", g_cfg.plcHost);
	g_nvs.putString("wip",   g_cfg.wifiIp);
	g_nvs.putString("wgw",   g_cfg.wifiGw);
	g_nvs.putString("wmask", g_cfg.wifiMask);
	g_nvs.putString("wdns1", g_cfg.wifiDns1);
	g_nvs.putString("wdns2", g_cfg.wifiDns2);
	g_nvs.putUShort("port", g_cfg.plcPort);
	g_nvs.putUChar ("unit", g_cfg.plcUnit);
	g_nvs.putUShort("poll", g_cfg.pollMs);
	g_nvs.putULong ("pin",  g_cfg.pinHash);
	g_nvs.putUChar ("theme", g_cfg.theme);
	g_nvs.putUChar ("dinv",  g_cfg.dispInvert);
	g_nvs.putUChar ("lvlmax", g_cfg.levelMaxM);
	g_nvs.putString("ntp",   g_cfg.ntpServer);
	g_nvs.putString("tz",    g_cfg.tz);
	g_nvs.putBytes ("light", &g_cfg.light, sizeof(g_cfg.light));
	g_nvs.putBytes ("scalec", &g_cfg.scaleCache, sizeof(g_cfg.scaleCache));
	for (uint8_t s = 0; s < NUM_WELLS; s++) {
		char k[8]; snprintf(k, sizeof(k), "n%u", s);
		g_nvs.putString(k, g_cfg.stationName[s]);
	}
	g_nvs.putBool("set", true);
	g_nvs.end();
}

// ---- microSD (JSON) ---------------------------------------------------
bool sd_load() {
	File f = SD.open(HMI_CFG_PATH, FILE_READ);
	if (!f) return false;
	JsonDocument doc;
	DeserializationError e = deserializeJson(doc, f);
	f.close();
	if (e) {
		Serial.printf("[cfg] %s ilegible: %s\n", HMI_CFG_PATH, e.c_str());
		return false;
	}
	const char *js;
	if ((js = doc["wifi"]["ssid"] | (const char *)nullptr)) cpstr(g_cfg.wifiSsid, js, sizeof(g_cfg.wifiSsid));
	if ((js = doc["wifi"]["pass"] | (const char *)nullptr)) cpstr(g_cfg.wifiPass, js, sizeof(g_cfg.wifiPass));
	if ((js = doc["wifi"]["ip"]   | (const char *)nullptr)) cpstr(g_cfg.wifiIp,   js, sizeof(g_cfg.wifiIp));
	if ((js = doc["wifi"]["gw"]   | (const char *)nullptr)) cpstr(g_cfg.wifiGw,   js, sizeof(g_cfg.wifiGw));
	if ((js = doc["wifi"]["mask"] | (const char *)nullptr)) cpstr(g_cfg.wifiMask, js, sizeof(g_cfg.wifiMask));
	if ((js = doc["wifi"]["dns1"] | (const char *)nullptr)) cpstr(g_cfg.wifiDns1, js, sizeof(g_cfg.wifiDns1));
	if ((js = doc["wifi"]["dns2"] | (const char *)nullptr)) cpstr(g_cfg.wifiDns2, js, sizeof(g_cfg.wifiDns2));
	if ((js = doc["plc"]["host"]  | (const char *)nullptr)) cpstr(g_cfg.plcHost,  js, sizeof(g_cfg.plcHost));
	g_cfg.plcPort = doc["plc"]["port"]    | g_cfg.plcPort;
	g_cfg.plcUnit = doc["plc"]["unit"]    | g_cfg.plcUnit;
	g_cfg.pollMs  = doc["plc"]["poll_ms"] | g_cfg.pollMs;
	g_cfg.theme      = doc["ui"]["theme"]       | g_cfg.theme;
	g_cfg.dispInvert = doc["ui"]["disp_invert"] | g_cfg.dispInvert;
	g_cfg.levelMaxM  = doc["ui"]["level_max_m"] | g_cfg.levelMaxM;
	if ((js = doc["time"]["server"] | (const char *)nullptr)) cpstr(g_cfg.ntpServer, js, sizeof(g_cfg.ntpServer));
	if ((js = doc["time"]["tz"]     | (const char *)nullptr)) cpstr(g_cfg.tz, js, sizeof(g_cfg.tz));

	JsonObjectConst lc = doc["light"];
	g_cfg.light.enabled   = lc["enabled"]    | g_cfg.light.enabled;
	g_cfg.light.rawBright = lc["raw_bright"] | g_cfg.light.rawBright;
	g_cfg.light.rawDark   = lc["raw_dark"]   | g_cfg.light.rawDark;
	g_cfg.light.pctClosed = lc["pct_closed"] | g_cfg.light.pctClosed;
	g_cfg.light.pctDay    = lc["pct_day"]    | g_cfg.light.pctDay;
	g_cfg.light.pctTheme  = lc["pct_theme"]  | g_cfg.light.pctTheme;
	g_cfg.light.hystPct   = lc["hyst_pct"]   | g_cfg.light.hystPct;
	g_cfg.light.blClosed  = lc["bl_closed"]  | g_cfg.light.blClosed;
	g_cfg.light.blMin     = lc["bl_min"]     | g_cfg.light.blMin;
	g_cfg.light.blMax     = lc["bl_max"]     | g_cfg.light.blMax;
	g_cfg.light.blManual  = lc["bl_manual"]  | g_cfg.light.blManual;
	const char *pin = doc["admin"]["pin"].is<const char *>()
	                      ? doc["admin"]["pin"].as<const char *>() : nullptr;  // PIN en claro (opcional)
	uint32_t ph = doc["admin"]["pin_hash"] | 0u;                              // o su hash
	if (pin && *pin) g_cfg.pinHash = fnv1a(pin);
	else if (ph)     g_cfg.pinHash = ph;
	JsonArray names = doc["stations"].as<JsonArray>();
	if (!names.isNull()) {
		uint8_t s = 0;
		for (JsonVariant v : names) {
			if (s >= NUM_WELLS) break;
			const char *nm = v.is<const char *>() ? v.as<const char *>() : nullptr;
			if (nm && *nm) cpstr(g_cfg.stationName[s], nm, WELL_NAME_LEN);
			s++;
		}
	}
	JsonArray sc = doc["scale"].as<JsonArray>();
	if (!sc.isNull()) {
		uint8_t s = 0;
		for (JsonVariant v : sc) {
			if (s >= NUM_WELLS) break;
			ScaleCache &c = g_cfg.scaleCache[s];
			c.known = v["known"] | c.known;
			JsonObjectConst lv = v["level"], fl = v["flow"];
			c.level.rawMin = lv["raw_min"] | c.level.rawMin;
			c.level.rawMax = lv["raw_max"] | c.level.rawMax;
			c.level.engMin = lv["eng_min"] | c.level.engMin;
			c.level.engMax = lv["eng_max"] | c.level.engMax;
			c.level.unit   = lv["unit"]    | c.level.unit;
			c.level.filter = lv["filter"]  | c.level.filter;
			c.flow.rawMin  = fl["raw_min"] | c.flow.rawMin;
			c.flow.rawMax  = fl["raw_max"] | c.flow.rawMax;
			c.flow.engMin  = fl["eng_min"] | c.flow.engMin;
			c.flow.engMax  = fl["eng_max"] | c.flow.engMax;
			c.flow.unit    = fl["unit"]    | c.flow.unit;
			c.flow.filter  = fl["filter"]  | c.flow.filter;
			s++;
		}
	}
	return true;
}

bool sd_save() {
	JsonDocument doc;
	doc["version"]        = 1;
	doc["wifi"]["ssid"]   = g_cfg.wifiSsid;
	doc["wifi"]["pass"]   = g_cfg.wifiPass;
	doc["wifi"]["ip"]     = g_cfg.wifiIp;
	doc["wifi"]["gw"]     = g_cfg.wifiGw;
	doc["wifi"]["mask"]   = g_cfg.wifiMask;
	doc["wifi"]["dns1"]   = g_cfg.wifiDns1;
	doc["wifi"]["dns2"]   = g_cfg.wifiDns2;
	doc["plc"]["host"]    = g_cfg.plcHost;
	doc["plc"]["port"]    = g_cfg.plcPort;
	doc["plc"]["unit"]    = g_cfg.plcUnit;
	doc["plc"]["poll_ms"] = g_cfg.pollMs;
	doc["ui"]["theme"]       = g_cfg.theme;
	doc["ui"]["disp_invert"] = g_cfg.dispInvert;
	doc["ui"]["level_max_m"] = g_cfg.levelMaxM;
	doc["time"]["server"]    = g_cfg.ntpServer;
	doc["time"]["tz"]        = g_cfg.tz;
	doc["admin"]["pin_hash"] = g_cfg.pinHash;   // nunca se escribe el PIN en claro

	JsonObject lc = doc["light"].to<JsonObject>();
	lc["enabled"]    = g_cfg.light.enabled;
	lc["raw_bright"] = g_cfg.light.rawBright;
	lc["raw_dark"]   = g_cfg.light.rawDark;
	lc["pct_closed"] = g_cfg.light.pctClosed;
	lc["pct_day"]    = g_cfg.light.pctDay;
	lc["pct_theme"]  = g_cfg.light.pctTheme;
	lc["hyst_pct"]   = g_cfg.light.hystPct;
	lc["bl_closed"]  = g_cfg.light.blClosed;
	lc["bl_min"]     = g_cfg.light.blMin;
	lc["bl_max"]     = g_cfg.light.blMax;
	lc["bl_manual"]  = g_cfg.light.blManual;
	JsonArray names = doc["stations"].to<JsonArray>();
	for (uint8_t s = 0; s < NUM_WELLS; s++) names.add(g_cfg.stationName[s]);

	JsonArray sc = doc["scale"].to<JsonArray>();
	for (uint8_t s = 0; s < NUM_WELLS; s++) {
		const ScaleCache &c = g_cfg.scaleCache[s];
		JsonObject o = sc.add<JsonObject>();
		o["known"] = c.known;
		JsonObject lv = o["level"].to<JsonObject>();
		lv["raw_min"] = c.level.rawMin; lv["raw_max"] = c.level.rawMax;
		lv["eng_min"] = c.level.engMin; lv["eng_max"] = c.level.engMax;
		lv["unit"]    = c.level.unit;   lv["filter"]  = c.level.filter;
		JsonObject fl = o["flow"].to<JsonObject>();
		fl["raw_min"] = c.flow.rawMin; fl["raw_max"] = c.flow.rawMax;
		fl["eng_min"] = c.flow.engMin; fl["eng_max"] = c.flow.engMax;
		fl["unit"]    = c.flow.unit;   fl["filter"]  = c.flow.filter;
	}

	SD.remove(HMI_CFG_PATH);
	File f = SD.open(HMI_CFG_PATH, FILE_WRITE);
	if (!f) return false;
	bool ok = serializeJsonPretty(doc, f) > 0;
	f.close();
	return ok;
}

}  // namespace

namespace hmicfg {

bool begin(bool safeMode) {
	apply_defaults();

	if (safeMode) {                       /* MODO BASICO: ni microSD ni NVS */
		g_sd  = false;
		g_src = "defaults (modo basico)";
		Serial.println("[cfg] MODO BASICO: microSD y NVS omitidos, solo defaults");
		return true;
	}

	pinMode(PIN_SD_CS, OUTPUT);
	digitalWrite(PIN_SD_CS, HIGH);
	g_sdSPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

	/* 20 MHz -> 4 MHz -> 1 MHz: tarjetas/cableado lentos de la CYD */
	const uint32_t freqs[] = { 20000000, 4000000, 1000000 };
	for (uint8_t i = 0; i < 3 && !g_sd; i++) {
		g_sd = SD.begin(PIN_SD_CS, g_sdSPI, freqs[i]);
		if (!g_sd) { SD.end(); delay(20); }
		else Serial.printf("[cfg] microSD montada a %lu Hz\n", freqs[i]);
	}

	if (g_sd) {
		const char *t;
		switch (SD.cardType()) {
			case CARD_MMC:  t = "MMC";  break;
			case CARD_SD:   t = "SDSC"; break;
			case CARD_SDHC: t = "SDHC"; break;
			default:        t = "?";    break;
		}
		Serial.printf("[cfg] microSD tipo=%s  %lu MB\n",
		              t, (uint32_t)(SD.cardSize() / (1024ULL * 1024ULL)));
	} else {
		Serial.println("[cfg] microSD NO detectada (revise formato FAT32 / tarjeta <=32GB / contactos)");
	}

	bool fromSd = false;
	if (g_sd && SD.exists(HMI_CFG_PATH)) {
		fromSd = sd_load();
		if (fromSd) { g_src = "microSD"; nvs_save(); }   // espeja a NVS
	}
	if (!fromSd) {
		nvs_load();                                       // deja g_src = "NVS" si habia
		if (g_sd && !SD.exists(HMI_CFG_PATH)) sd_save();  // siembra el archivo
	}
	Serial.printf("[cfg] origen=%s  ssid='%s'  plc=%s:%u u%u  poll=%u  pin=%s\n",
	              g_src, g_cfg.wifiSsid, g_cfg.plcHost, g_cfg.plcPort, g_cfg.plcUnit,
	              g_cfg.pollMs, g_cfg.pinHash ? "si" : "no");
	return true;
}

const HmiConfig &get()      { return g_cfg; }
HmiConfig        editable() { return g_cfg; }
bool             sdMounted(){ return g_sd; }
const char      *source()   { return g_src; }

bool save(const HmiConfig &c) {
	g_cfg = c;
	nvs_save();
	bool sdok = g_sd && sd_save();
	Serial.printf("[cfg] guardado  NVS=ok  microSD=%s\n",
	              g_sd ? (sdok ? "ok" : "FALLO") : "no montada");
	return true;
}

bool saveLight(const LightCfg &lc) {
	g_cfg.light = lc;
	nvs_save();
	bool sdok = g_sd && sd_save();
	Serial.printf("[cfg] light guardado  NVS=ok  microSD=%s\n",
	              g_sd ? (sdok ? "ok" : "FALLO") : "no montada");
	return true;
}

bool saveTheme(uint8_t mode) {
	g_cfg.theme = mode;
	nvs_save();
	bool sdok = g_sd && sd_save();
	Serial.printf("[cfg] tema guardado (%u)  NVS=ok  microSD=%s\n",
	              mode, g_sd ? (sdok ? "ok" : "FALLO") : "no montada");
	return true;
}

bool saveDispInvert(uint8_t on) {
	g_cfg.dispInvert = on ? 1 : 0;
	nvs_save();
	bool sdok = g_sd && sd_save();
	Serial.printf("[cfg] inversion de panel guardada (%u)  NVS=ok  microSD=%s\n",
	              g_cfg.dispInvert, g_sd ? (sdok ? "ok" : "FALLO") : "no montada");
	return true;
}

bool saveLevelMaxM(uint8_t m) {
	g_cfg.levelMaxM = m ? m : 1;
	nvs_save();
	bool sdok = g_sd && sd_save();
	Serial.printf("[cfg] profundidad de escala guardada (%u m)  NVS=ok  microSD=%s\n",
	              g_cfg.levelMaxM, g_sd ? (sdok ? "ok" : "FALLO") : "no montada");
	return true;
}

bool saveScaleCache(uint8_t s, const StationScale &sc) {
	if (s >= NUM_WELLS) return false;
	ScaleCache &c = g_cfg.scaleCache[s];
	bool changed = !c.known || !scaleVarEq(c.level, sc.level) || !scaleVarEq(c.flow, sc.flow);
	if (!changed) return true;
	c.level = sc.level; c.flow = sc.flow; c.known = true;
	nvs_save();
	bool sdok = g_sd && sd_save();
	Serial.printf("[cfg] escala est.%u cacheada (nivel raw %u..%u)  NVS=ok  microSD=%s\n",
	              (unsigned)s, sc.level.rawMin, sc.level.rawMax,
	              g_sd ? (sdok ? "ok" : "FALLO") : "no montada");
	return true;
}

bool saveTime(const char *server, const char *tz) {
	if (server && *server) cpstr(g_cfg.ntpServer, server, sizeof(g_cfg.ntpServer));
	if (tz && *tz)         cpstr(g_cfg.tz, tz, sizeof(g_cfg.tz));
	nvs_save();
	bool sdok = g_sd && sd_save();
	Serial.printf("[cfg] NTP guardado: %s / %s  NVS=ok  microSD=%s\n",
	              g_cfg.ntpServer, g_cfg.tz, g_sd ? (sdok ? "ok" : "FALLO") : "no montada");
	return true;
}

LightCfg lightDefaults() {
	LightCfg d;
	d.enabled   = 1;
	d.rawBright = 0;       /* CYD observada: luz plena -> raw ~0 */
	d.rawDark   = 300;     /*                oscuras   -> raw alto (ruidoso 255..900) */
	d.pctClosed = 12;      /* <=12% de luz -> nadie mira */
	d.pctDay    = 80;      /* >=80% -> brillo al tope */
	d.pctTheme  = 55;      /* >=55% -> tema claro */
	d.hystPct   = 6;
	d.blClosed  = 0;       /* tablero cerrado -> pantalla apagada */
	d.blMin     = 20;
	d.blMax     = 100;
	d.blManual  = 80;
	return d;
}

uint32_t hashPin(const char *pin) { return fnv1a(pin); }

bool checkPin(const char *pin) {
	if (g_cfg.pinHash == 0) return true;          // sin PIN configurado
	return fnv1a(pin) == g_cfg.pinHash;
}

}  // namespace hmicfg

#include "hist_log.h"
#include "hmi_config.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <time.h>
#include <string.h>

#define HIST_PATH "/hist.json"

namespace {

float hDay[NUM_WELLS][HIST_DAYS]     = {};
float hMonth[NUM_WELLS][HIST_MONTHS] = {};

/* Ultimo total visto (para saber, al cruzar el dia/mes, cual fue el valor de
 * CIERRE -- no el que ya llego reseteado a ~0 del dia/mes nuevo). */
float prevDay[NUM_WELLS]   = {};
float prevMonth[NUM_WELLS] = {};

int  lastMday = -1, lastMon = -1;

void shiftIn(float *arr, int n, float v) {
	for (int i = 0; i < n - 1; i++) arr[i] = arr[i + 1];
	arr[n - 1] = v;
}

void save() {
	if (!hmicfg::sdMounted()) return;
	JsonDocument doc;
	for (uint8_t w = 0; w < NUM_WELLS; w++) {
		JsonArray d = doc["day"][w].to<JsonArray>();
		for (int i = 0; i < HIST_DAYS; i++) d.add(hDay[w][i]);
		JsonArray m = doc["month"][w].to<JsonArray>();
		for (int i = 0; i < HIST_MONTHS; i++) m.add(hMonth[w][i]);
	}
	SD.remove(HIST_PATH);
	File f = SD.open(HIST_PATH, FILE_WRITE);
	if (!f) return;
	serializeJson(doc, f);
	f.close();
}

void load() {
	if (!hmicfg::sdMounted() || !SD.exists(HIST_PATH)) return;
	File f = SD.open(HIST_PATH, FILE_READ);
	if (!f) return;
	JsonDocument doc;
	DeserializationError e = deserializeJson(doc, f);
	f.close();
	if (e) { Serial.printf("[hist] %s ilegible: %s\n", HIST_PATH, e.c_str()); return; }

	for (uint8_t w = 0; w < NUM_WELLS; w++) {
		JsonArray d = doc["day"][w].as<JsonArray>();
		int i = 0; for (JsonVariant v : d)   { if (i >= HIST_DAYS)   break; hDay[w][i++]   = v.as<float>(); }
		JsonArray m = doc["month"][w].as<JsonArray>();
		i = 0;     for (JsonVariant v : m)   { if (i >= HIST_MONTHS) break; hMonth[w][i++] = v.as<float>(); }
	}
}

}  // namespace

namespace histlog {

void begin() { load(); }

void tick(PlantData &data) {
	time_t now = time(nullptr);
	if (now > 1600000000) {          // solo con hora real sincronizada (netclock)
		struct tm lt;
		localtime_r(&now, &lt);

		if (lastMday < 0) {
			lastMday = lt.tm_mday;
			lastMon  = lt.tm_mon;
		} else {
			if (lt.tm_mday != lastMday) {
				for (uint8_t w = 0; w < NUM_WELLS; w++) shiftIn(hDay[w], HIST_DAYS, prevDay[w]);
				lastMday = lt.tm_mday;
				save();
				Serial.println("[hist] cierre de dia apilado");
			}
			if (lt.tm_mon != lastMon) {
				for (uint8_t w = 0; w < NUM_WELLS; w++) shiftIn(hMonth[w], HIST_MONTHS, prevMonth[w]);
				lastMon = lt.tm_mon;
				save();
				Serial.println("[hist] cierre de mes apilado");
			}
		}
	}

	for (uint8_t w = 0; w < NUM_WELLS; w++) {
		prevDay[w]   = data.well[w].totalDayM3;
		prevMonth[w] = data.well[w].totalMonthM3;
		memcpy(data.well[w].histDayM3,   hDay[w],   sizeof(hDay[w]));
		memcpy(data.well[w].histMonthM3, hMonth[w], sizeof(hMonth[w]));
	}
}

}  // namespace histlog

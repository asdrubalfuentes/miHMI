#include "net_clock.h"
#include "data/hmi_config.h"
#include <Arduino.h>
#include <time.h>

// SNTP del ESP32.  configTzTime() deja la config y el cliente SNTP sincroniza
// solo cuando hay red; reintenta en segundo plano.

namespace netclock {

void begin() {
	const HmiConfig &c = hmicfg::get();
	const char *srv = c.ntpServer[0] ? c.ntpServer : "pool.ntp.org";
	const char *tz  = c.tz[0]        ? c.tz        : "UTC0";
	configTzTime(tz, srv, "pool.ntp.org", "time.google.com");
	Serial.printf("[ntp] servidor=%s  tz=%s\n", srv, tz);
}

bool valid() {
	return time(nullptr) > 1735689600;   /* > 2025-01-01 => ya sincronizado */
}

bool hm(char *out, size_t n) {
	if (!valid()) {
		if (n) { strncpy(out, "--:--", n); out[n - 1] = 0; }
		return false;
	}
	time_t t = time(nullptr);
	struct tm ti;
	localtime_r(&t, &ti);
	snprintf(out, n, "%02d:%02d", ti.tm_hour, ti.tm_min);
	return true;
}

void status(char *out, size_t n) {
	if (!valid()) { snprintf(out, n, "sin sincronizar"); return; }
	time_t t = time(nullptr);
	struct tm ti;
	localtime_r(&t, &ti);
	strftime(out, n, "%Y-%m-%d %H:%M:%S", &ti);
}

}  // namespace netclock

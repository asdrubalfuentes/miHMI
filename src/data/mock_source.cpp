#include "mock_source.h"
#include "hmi_config.h"
#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Fuente simulada: emula el MAPA B para desarrollar la UI sin PLC.

bool MockSource::begin() {
	last_ = lastOk_ = millis();

	for (uint8_t w = 0; w < NUM_WELLS; w++) {
		WellData &d = latest.well[w];
		snprintf(d.name, WELL_NAME_LEN, "%s", hmicfg::get().stationName[w]);

		for (int i = 0; i < HIST_DAYS; i++) {
			d.histDayM3[i] = 240.0f + i * 20.0f + w * 60.0f + (random(0, 120) - 60);
			if (d.histDayM3[i] < 0) d.histDayM3[i] = 0;
		}
		d.totalMonthM3 = 5200.0f + w * 900.0f + random(0, 1200);
		d.totalDayM3   = d.histDayM3[HIST_DAYS - 1];
		d.levelEng   = 50.0f + w * 8.0f;
		d.levelUnit  = 0;   /* % */
		d.flowUnit   = 0;   /* L/s */
		d.linkOk     = true;
		d.voltLocal  = true;
		d.sirenAuto  = true;
	}
	latest.count       = NUM_WELLS;
	latest.origin      = 0;      // SIM
	latest.contractVer = 1;
	return true;
}

void MockSource::poll() {
	uint32_t now = millis();
	float dt = (now - last_) / 1000.0f;
	if (dt < 0) dt = 0;
	last_ = now;
	lastOk_ = now;
	latest.heartbeat = (uint16_t)(now / 1000UL);

	float t = now / 1000.0f;
	uint16_t alarmOr = 0;

	for (uint8_t w = 0; w < NUM_WELLS; w++) {
		WellData &d = latest.well[w];
		float ph = w * 2.1f;

		bool bombeando = d.presostato;
		float drift = bombeando ? -6.0f : 0.0f;
		d.levelEng = constrain(55.0f + 18.0f * sinf(t / (22.0f + w * 4) + ph) + drift
		                       + (random(-100, 100) / 100.0f), 0.0f, 100.0f);
		d.levelRaw = (uint16_t)(800 + d.levelEng * 32.0f);

		float target = bombeando ? (38.0f + w * 5.0f) : 0.0f;
		float k = constrain(dt * 0.6f, 0.0f, 1.0f);
		d.flowEng += (target - d.flowEng) * k;
		if (bombeando) d.flowEng += random(-40, 40) / 100.0f;
		if (d.flowEng < 0.05f) d.flowEng = 0.0f;
		d.flowRaw = (uint16_t)(800 + d.flowEng * 64.0f);

		float dV = d.flowEng * dt / 1000.0f;   /* L/s -> m3 */
		d.totalDayM3   += dV;
		d.totalMonthM3 += dV;
		d.histDayM3[HIST_DAYS - 1] = d.totalDayM3;

		d.presostato = bombeando;
		d.linkOk     = true;

		// arbol de alarmas minimo
		uint16_t a = 0;
		if (d.levelEng >= 90.0f) a |= MAPB_ALM_LEVEL_HI;
		if (d.levelEng <= 10.0f) a |= MAPB_ALM_LEVEL_LO;
		if (d.levelEng <= 5.0f)  a |= MAPB_ALM_LEVEL_LOLO;
		if (d.tamper)            a |= MAPB_ALM_TAMPER;
		if (!d.voltLocal)        a |= MAPB_ALM_VOLT_LOSS;
		d.alarms = a;
		d.alarmsLatched |= a & (MAPB_ALM_LEVEL_LOLO | MAPB_ALM_TAMPER);  /* mismo mask que el PLC */
		alarmOr |= a;

		// sirena
		if (d.sirenAuto) d.sirenOn = (a != 0);
		d.rssi = -60 - w * 10;
		d.ageS = 0;
	}
	latest.alarmOr = alarmOr;
}

bool MockSource::sendCommand(const Command &cmd) {
	if (cmd.well >= NUM_WELLS) return false;
	WellData &d = latest.well[cmd.well];
	switch (cmd.type) {
		case CmdType::SirenOn:     if (!d.sirenAuto) d.sirenOn = true;  break;
		case CmdType::SirenOff:    if (!d.sirenAuto) d.sirenOn = false; break;
		case CmdType::SirenAuto:   d.sirenAuto = true;  break;
		case CmdType::SirenManual: d.sirenAuto = false; break;
		case CmdType::Silence:     d.sirenOn = false;   break;
		case CmdType::ResetDay:    d.totalDayM3 = 0.0f;   break;
		case CmdType::ResetMonth:  d.totalMonthM3 = 0.0f; break;
		case CmdType::AckAlarms:   d.alarmsLatched &= d.alarms; break;  /* deja solo las activas */
	}
	lastOk_ = millis();
	return true;
}

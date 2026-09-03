#include "mock_source.h"
#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

bool MockSource::begin() {
	last_ = lastOk_ = millis();

	for (uint8_t w = 0; w < NUM_WELLS; w++) {
		WellData &d = latest.well[w];
		snprintf(d.name, WELL_NAME_LEN, "Pozo %u", (unsigned)(w + 1));

		for (int i = 0; i < HIST_DAYS; i++) {
			d.histDayM3[i] = 240.0f + i * 20.0f + w * 60.0f + (random(0, 120) - 60);
			if (d.histDayM3[i] < 0) d.histDayM3[i] = 0;
		}
		d.totalMonthM3 = 5200.0f + w * 900.0f + random(0, 1200);
		d.totalDayM3   = d.histDayM3[HIST_DAYS - 1];
		d.mode         = PumpMode::Manual;
		d.levelPct     = 50.0f + w * 8.0f;
		d.pumpRun      = (w == 0);   /* uno arranca en marcha para ver contraste */
	}
	latest.count = NUM_WELLS;
	return true;
}

void MockSource::poll() {
	uint32_t now = millis();
	float dt = (now - last_) / 1000.0f;
	if (dt < 0) dt = 0;
	last_ = now;
	lastOk_ = now;

	float t = now / 1000.0f;

	for (uint8_t w = 0; w < NUM_WELLS; w++) {
		WellData &d = latest.well[w];
		float ph = w * 2.1f;   /* desfase por pozo */

		float drift = d.pumpRun ? -6.0f : 0.0f;
		d.levelPct = 55.0f + 18.0f * sinf(t / (22.0f + w * 4) + ph) + drift + (random(-100, 100) / 100.0f);
		d.levelPct = constrain(d.levelPct, 0.0f, 100.0f);
		d.levelM   = 3.0f + (d.levelPct / 100.0f) * (20.0f + w * 4);

		float target = d.pumpRun ? (38.0f + w * 5.0f) : 0.0f;
		float k = constrain(dt * 0.6f, 0.0f, 1.0f);
		d.flowLps += (target - d.flowLps) * k;
		if (d.pumpRun) d.flowLps += random(-40, 40) / 100.0f;
		if (d.flowLps < 0.05f) d.flowLps = 0.0f;
		d.flowM3h = d.flowLps * 3.6f;

		float dV = d.flowLps * dt / 1000.0f;
		d.totalDayM3   += dV;
		d.totalMonthM3 += dV;
		d.histDayM3[HIST_DAYS - 1] = d.totalDayM3;

		d.pumpFault = false;
	}
}

bool MockSource::sendCommand(const Command &cmd) {
	if (cmd.well >= NUM_WELLS) return false;
	WellData &d = latest.well[cmd.well];
	switch (cmd.type) {
		case CmdType::PumpStart:     d.pumpRun = true;            break;
		case CmdType::PumpStop:      d.pumpRun = false;           break;
		case CmdType::SetModeAuto:   d.mode = PumpMode::Auto;     break;
		case CmdType::SetModeManual: d.mode = PumpMode::Manual;   break;
	}
	lastOk_ = millis();
	return true;
}

#include "modbus_tcp_source.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>

// MAPA B del contrato de orquestacion. Solo lectura de datos + comandos de la
// superficie congelada (sirena / silenciar / reset). Ver map_b.h.

bool ModbusTcpSource::wifiUp() const {
	return WIFI_SSID[0] != 0 && WiFi.status() == WL_CONNECTED;
}

bool ModbusTcpSource::begin() {
	for (uint8_t s = 0; s < NUM_WELLS; s++)
		snprintf(latest.well[s].name, WELL_NAME_LEN, "Estacion %u", (unsigned)(s + 1));
	latest.count = NUM_WELLS;

	ipOk_ = plcIp_.fromString(PLC_HOST);

	if (WIFI_SSID[0]) {
		WiFi.persistent(false);
		WiFi.mode(WIFI_STA);
		WiFi.setSleep(false);
		WiFi.setAutoReconnect(true);
		WiFi.begin(WIFI_SSID, WIFI_PASS);
		wifiStarted_ = true;
	}

	mb_.client();
	mb_.autoConnect(true);
	return true;
}

void ModbusTcpSource::service() {
	mb_.task();
}

void ModbusTcpSource::poll() {
	mb_.task();

	if (!wifiUp()) return;

	if (!ipOk_) {                          // PLC_HOST era un nombre, no una IP
		ipOk_ = WiFi.hostByName(PLC_HOST, plcIp_);
		if (!ipOk_) return;
	}

	if (!mb_.isConnected(plcIp_)) {
		mb_.connect(plcIp_, PLC_PORT);
		return;                            // reintenta en el proximo poll
	}

	uint32_t now = millis();
	if (now - lastPoll_ < MB_POLL_MS) return;
	lastPoll_ = now;
	kickReads();
}

void ModbusTcpSource::kickReads() {
	for (uint8_t s = 0; s < NUM_WELLS; s++) {
		mb_.readHreg(plcIp_, s * MAPB_HR_STRIDE, hr_[s], 14,
			[this, s](Modbus::ResultCode ev, uint16_t, void *) -> bool {
				if (ev == Modbus::EX_SUCCESS) applyStation(s);
				return true;
			}, PLC_UNIT);
	}
	mb_.readIreg(plcIp_, MAPB_IR_G_MARK, ir_, 10,
		[this](Modbus::ResultCode ev, uint16_t, void *) -> bool {
			if (ev == Modbus::EX_SUCCESS) applyGlobal();
			return true;
		}, PLC_UNIT);
}

void ModbusTcpSource::applyStation(uint8_t s) {
	if (s >= NUM_WELLS) return;
	const uint16_t *r = hr_[s];
	WellData &d = latest.well[s];

	d.levelPct = r[MAPB_HR_LEVEL] / MB_LEVEL_SCALE;   // TODO Fase 5: usar unidad (+28)
	d.levelM   = d.levelPct;
	d.flowLps  = r[MAPB_HR_FLOW] / MB_FLOW_SCALE;
	d.flowM3h  = d.flowLps * 3.6f;

	d.totalDayM3   = mapb_u32(r[MAPB_HR_DAY_W0], r[MAPB_HR_DAY_W1]) / MB_ACCUM_SCALE;
	d.totalMonthM3 = mapb_u32(r[MAPB_HR_MON_W0], r[MAPB_HR_MON_W1]) / MB_ACCUM_SCALE;

	uint16_t st = r[MAPB_HR_STATUS];
	d.pumpRun   = (st & MAPB_ST_PRESOSTATO) != 0;      // proxy: presostato (Fase 5 revisa el modelo)
	d.pumpFault = (st & MAPB_ST_IN_ALARM) != 0;
	d.mode      = PumpMode::Manual;

	lastOk_ = millis();
}

void ModbusTcpSource::applyGlobal() {
	origin_    = ir_[MAPB_IR_G_ORIGIN    - MAPB_IR_G_MARK];
	heartbeat_ = ir_[MAPB_IR_G_HEARTBEAT - MAPB_IR_G_MARK];
	lastOk_    = millis();
}

bool ModbusTcpSource::isHealthy() const {
	return wifiUp() && (millis() - lastOk_ < 6000);
}

bool ModbusTcpSource::sendCommand(const Command &cmd) {
	if (cmd.well >= NUM_WELLS || !wifiUp() || !ipOk_) return false;
	uint16_t base = cmd.well * MAPB_CO_STRIDE;

	// El unico actuador del Mapa B es la sirena; hasta la Fase 5 (que redefine
	// CmdType a Silenciar/SirenaAuto/ResetDia/ResetMes) mapeamos lo que hay.
	switch (cmd.type) {
		case CmdType::PumpStart:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_MANUAL, true, nullptr, PLC_UNIT);
			break;
		case CmdType::PumpStop:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_MANUAL, false, nullptr, PLC_UNIT);
			break;
		case CmdType::SetModeAuto:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_AUTO, true, nullptr, PLC_UNIT);
			break;
		case CmdType::SetModeManual:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_AUTO, false, nullptr, PLC_UNIT);
			break;
	}
	return true;
}

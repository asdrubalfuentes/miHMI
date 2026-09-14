#include "modbus_tcp_source.h"
#include "config.h"
#include "hmi_config.h"
#include <Arduino.h>
#include <WiFi.h>

// MAPA B del contrato de orquestacion. Lectura de datos + comandos de la
// superficie congelada (sirena / silenciar / reset) + bloque de escala. Ver map_b.h.
// El destino (WiFi de planta, IP/puerto/unit del PLC, periodo de sondeo) sale de
// hmicfg (microSD / NVS / defaults), no de los #define.

static void cpy(char *d, const char *s, size_t n) {
	strncpy(d, s ? s : "", n - 1);
	d[n - 1] = 0;
}

bool ModbusTcpSource::wifiUp() const {
	return ssid_[0] != 0 && WiFi.status() == WL_CONNECTED;
}

String ModbusTcpSource::localIp() const {
	return wifiUp() ? WiFi.localIP().toString() : String("sin WiFi");
}

int ModbusTcpSource::linkRssi() const {
	return wifiUp() ? (int)WiFi.RSSI() : 0;
}

bool ModbusTcpSource::begin() {
	const HmiConfig &c = hmicfg::get();
	cpy(ssid_, c.wifiSsid, sizeof(ssid_));
	cpy(pass_, c.wifiPass, sizeof(pass_));
	cpy(host_, c.plcHost,  sizeof(host_));
	port_   = c.plcPort ? c.plcPort : PLC_PORT;
	unit_   = c.plcUnit;
	pollMs_ = c.pollMs ? c.pollMs : MB_POLL_MS;

	for (uint8_t s = 0; s < NUM_WELLS; s++)
		snprintf(latest.well[s].name, WELL_NAME_LEN, "%s", c.stationName[s]);
	latest.count = NUM_WELLS;

	ipOk_ = plcIp_.fromString(host_);

	if (ssid_[0]) {
		WiFi.persistent(false);
		WiFi.mode(WIFI_STA);
		WiFi.setSleep(false);
		WiFi.setAutoReconnect(true);

		// IP fija (c.wifiIp no vacio): sin dns1/dns2, WiFi.config() los deja en
		// 0.0.0.0 y el HMI queda sin NINGUN DNS (rompe "Buscar actualizacion" --
		// mismo bug ya corregido en nodeIO_master v1.5.3). dns1 = wifiDns1 si se
		// configuro, si no el propio gateway (la mayoria hace de proxy DNS);
		// dns2 = wifiDns2 si se configuro, si no 8.8.8.8 de respaldo.
		if (c.wifiIp[0]) {
			IPAddress ip, gw, mask, dns1, dns2;
			ip.fromString(c.wifiIp);
			gw.fromString(c.wifiGw[0] ? c.wifiGw : c.wifiIp);
			if (!mask.fromString(c.wifiMask[0] ? c.wifiMask : "255.255.255.0"))
				mask = IPAddress(255, 255, 255, 0);
			if (!(c.wifiDns1[0] && dns1.fromString(c.wifiDns1))) dns1 = gw;
			if (!(c.wifiDns2[0] && dns2.fromString(c.wifiDns2))) dns2 = IPAddress(8, 8, 8, 8);
			WiFi.config(ip, gw, mask, dns1, dns2);
		}

		WiFi.begin(ssid_, pass_);
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

	if (!ipOk_) {                          // host era un nombre, no una IP
		ipOk_ = WiFi.hostByName(host_, plcIp_);
		if (!ipOk_) return;
	}
	uint32_t now = millis();
	if (!mb_.isConnected(plcIp_)) {
		if (now - lastConnTry_ < 3000) return;   // connect() bloquea: no martillearlo
		lastConnTry_ = now;
		mb_.connect(plcIp_, port_);
		return;                                  // lee en el proximo poll
	}

	if (now - lastPoll_ < pollMs_) return;
	lastPoll_ = now;
	kickReads();
}

void ModbusTcpSource::kickReads() {
	for (uint8_t s = 0; s < NUM_WELLS; s++) {
		mb_.readHreg(plcIp_, s * MAPB_HR_STRIDE, hr_[s], MAPB_HR_BLOCK_LEN,
			[this, s](Modbus::ResultCode ev, uint16_t, void *) -> bool {
				if (ev == Modbus::EX_SUCCESS) applyStation(s);
				return true;
			}, unit_);
	}
	mb_.readHreg(plcIp_, MAPB_G_MARK, ir_, 10,   // v2: bloque global en Holding Registers
		[this](Modbus::ResultCode ev, uint16_t, void *) -> bool {
			if (ev == Modbus::EX_SUCCESS) applyGlobal();
			return true;
		}, unit_);
}

void ModbusTcpSource::applyStation(uint8_t s) {
	if (s >= NUM_WELLS) return;
	const uint16_t *r = hr_[s];
	WellData &d = latest.well[s];

	d.levelEng  = r[MAPB_HR_LEVEL] / MB_LEVEL_SCALE;
	d.flowEng   = r[MAPB_HR_FLOW]  / MB_FLOW_SCALE;
	d.levelRaw  = r[MAPB_HR_LEVEL_RAW];
	d.flowRaw   = r[MAPB_HR_FLOW_RAW];
	d.levelUnit = (uint8_t)r[MAPB_HR_UNIT_LEVEL];
	d.flowUnit  = (uint8_t)r[MAPB_HR_UNIT_FLOW];

	/* el bloque completo trae hb+20..31: mantener scale_[s] siempre fresco */
	StationScale &sc = scale_[s];
	sc.level.rawMin = r[MAPB_HR_LVL_RAWMIN]; sc.level.rawMax = r[MAPB_HR_LVL_RAWMAX];
	sc.level.engMin = (int16_t)r[MAPB_HR_LVL_ENGMIN]; sc.level.engMax = (int16_t)r[MAPB_HR_LVL_ENGMAX];
	sc.level.unit = r[MAPB_HR_UNIT_LEVEL]; sc.level.filter = r[MAPB_HR_FILTER];
	sc.flow.rawMin = r[MAPB_HR_FLW_RAWMIN]; sc.flow.rawMax = r[MAPB_HR_FLW_RAWMAX];
	sc.flow.engMin = (int16_t)r[MAPB_HR_FLW_ENGMIN]; sc.flow.engMax = (int16_t)r[MAPB_HR_FLW_ENGMAX];
	sc.flow.unit = r[MAPB_HR_UNIT_FLOW]; sc.flow.filter = r[MAPB_HR_FILTER];
	sc.stamp = r[MAPB_HR_CFG_STAMP];
	sc.valid = true;

	/* Restauracion automatica: si el PLC quedo sin calibrar (rawMax de nivel
	 * en 0 -- nunca pasa con un valor real aplicado) y hay algo cacheado por
	 * el HMI, se lo volvemos a mandar. Un solo intento por perdida; se rearma
	 * solo cuando vuelve a verse un valor real (o sea, tras una restauracion
	 * exitosa, o si alguien lo configura de nuevo desde la pantalla). */
	const HmiConfig &hc = hmicfg::get();
	if (sc.level.rawMax == 0) {
		if (hc.scaleCache[s].known && !scaleRestored_[s]) {
			scaleRestored_[s] = true;
			StationScale restore;
			restore.level = hc.scaleCache[s].level;
			restore.flow  = hc.scaleCache[s].flow;
			Serial.printf("[plc] estacion %u sin calibrar, restaurando desde cache\n", (unsigned)s);
			applyScale(s, restore);
		}
	} else {
		scaleRestored_[s] = false;
		hmicfg::saveScaleCache(s, sc);
	}

	d.totalDayM3   = mapb_u32(r[MAPB_HR_DAY_W0], r[MAPB_HR_DAY_W1]) / MB_ACCUM_SCALE;
	d.totalMonthM3 = mapb_u32(r[MAPB_HR_MON_W0], r[MAPB_HR_MON_W1]) / MB_ACCUM_SCALE;

	uint16_t st = r[MAPB_HR_STATUS];
	d.presostato = (st & MAPB_ST_PRESOSTATO) != 0;
	d.voltLocal  = (st & MAPB_ST_VOLT_LOCAL) != 0;
	d.tamper     = (st & MAPB_ST_TAMPER)     != 0;
	d.sirenOn    = (st & MAPB_ST_SIREN_ON)   != 0;
	d.sirenAuto  = (st & MAPB_ST_SIREN_AUTO) != 0;
	d.linkOk     = (st & MAPB_ST_LINK_OK)    != 0;

	d.alarms        = r[MAPB_HR_ALARMS];
	d.alarmsLatched = r[MAPB_HR_ALARMS_LATCHED];
	d.rssi   = (int16_t)r[MAPB_HR_RSSI];
	d.ageS   = r[MAPB_HR_AGE];

	lastOk_ = millis();
}

void ModbusTcpSource::applyGlobal() {
	latest.origin      = ir_[MAPB_G_ORIGIN    - MAPB_G_MARK];
	latest.heartbeat   = ir_[MAPB_G_HEARTBEAT - MAPB_G_MARK];
	latest.contractVer = ir_[MAPB_G_CONTRACT  - MAPB_G_MARK];
	latest.alarmOr     = ir_[MAPB_G_ALARM_OR  - MAPB_G_MARK];
	lastOk_ = millis();
}

bool ModbusTcpSource::isHealthy() const {
	return wifiUp() && (millis() - lastOk_ < 6000);
}

bool ModbusTcpSource::sendCommand(const Command &cmd) {
	if (cmd.well >= NUM_WELLS || !wifiUp() || !ipOk_) return false;
	uint16_t base = cmd.well * MAPB_CO_STRIDE;

	switch (cmd.type) {
		case CmdType::SirenOn:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_MANUAL, true, nullptr, unit_);  break;
		case CmdType::SirenOff:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_MANUAL, false, nullptr, unit_); break;
		case CmdType::SirenAuto:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_AUTO, true, nullptr, unit_);    break;
		case CmdType::SirenManual:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SIREN_AUTO, false, nullptr, unit_);   break;
		case CmdType::Silence:
			mb_.writeCoil(plcIp_, base + MAPB_CO_SILENCE, true, nullptr, unit_);       break;
		case CmdType::ResetDay:
			mb_.writeCoil(plcIp_, base + MAPB_CO_ARM_RESET, true, nullptr, unit_);
			mb_.writeCoil(plcIp_, base + MAPB_CO_RESET_DAY, true, nullptr, unit_);     break;
		case CmdType::ResetMonth:
			mb_.writeCoil(plcIp_, base + MAPB_CO_ARM_RESET, true, nullptr, unit_);
			mb_.writeCoil(plcIp_, base + MAPB_CO_RESET_MONTH, true, nullptr, unit_);   break;
		case CmdType::AckAlarms:
			mb_.writeCoil(plcIp_, base + MAPB_CO_ACK_ALARMS, true, nullptr, unit_);    break;
	}
	return true;
}

// --- bloque de escala (hb+20..31) ---------------------------------------
void ModbusTcpSource::requestScale(uint8_t s) {
	if (s >= NUM_WELLS || !wifiUp() || !ipOk_) return;
	if (!mb_.isConnected(plcIp_)) mb_.connect(plcIp_, port_);  // autoConnect completa la lectura
	mb_.readHreg(plcIp_, s * MAPB_HR_STRIDE + MAPB_HR_SCALE_BASE, scaleBuf_[s], 12,
		[this, s](Modbus::ResultCode ev, uint16_t, void *) -> bool {
			if (ev != Modbus::EX_SUCCESS) return true;
			const uint16_t *v = scaleBuf_[s];
			StationScale &sc = scale_[s];
			sc.level.rawMin = v[0]; sc.level.rawMax = v[1];
			sc.level.engMin = (int16_t)v[2]; sc.level.engMax = (int16_t)v[3];
			sc.level.unit = v[8]; sc.level.filter = v[10];
			sc.flow.rawMin = v[4]; sc.flow.rawMax = v[5];
			sc.flow.engMin = (int16_t)v[6]; sc.flow.engMax = (int16_t)v[7];
			sc.flow.unit = v[9]; sc.flow.filter = v[10];
			sc.stamp = v[11];
			sc.valid = true;
			return true;
		}, unit_);
}

bool ModbusTcpSource::scaleValid(uint8_t s) const {
	return s < NUM_WELLS && scale_[s].valid;
}

StationScale ModbusTcpSource::getScale(uint8_t s) const {
	return (s < NUM_WELLS) ? scale_[s] : StationScale{};
}

bool ModbusTcpSource::applyScale(uint8_t s, const StationScale &sc) {
	if (s >= NUM_WELLS || !wifiUp() || !ipOk_) return false;
	if (!mb_.isConnected(plcIp_)) mb_.connect(plcIp_, port_);

	wbuf_[0]  = sc.level.rawMin;  wbuf_[1]  = sc.level.rawMax;
	wbuf_[2]  = (uint16_t)sc.level.engMin; wbuf_[3] = (uint16_t)sc.level.engMax;
	wbuf_[4]  = sc.flow.rawMin;   wbuf_[5]  = sc.flow.rawMax;
	wbuf_[6]  = (uint16_t)sc.flow.engMin;  wbuf_[7] = (uint16_t)sc.flow.engMax;
	wbuf_[8]  = sc.level.unit;    wbuf_[9]  = sc.flow.unit;
	wbuf_[10] = sc.level.filter;  wbuf_[11] = sc.stamp;   // el PLC reescribe el sello

	uint16_t hbase = s * MAPB_HR_STRIDE + MAPB_HR_SCALE_BASE;
	uint16_t cbase = s * MAPB_CO_STRIDE;
	mb_.writeHreg(plcIp_, hbase, wbuf_, 12,
		[this, s, cbase](Modbus::ResultCode ev, uint16_t, void *) -> bool {
			if (ev == Modbus::EX_SUCCESS)
				mb_.writeCoil(plcIp_, cbase + MAPB_CO_APPLY_SCALE, true, nullptr, unit_);
			return true;
		}, unit_);
	return true;
}

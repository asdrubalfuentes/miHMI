/**
 * modbus_tcp_source.h  -  Fuente de datos primaria (Fase 2).
 *
 * Cliente Modbus TCP por WiFi que lee el MAPA B del contrato
 * (../../../ORCHESTRATION/REGISTER_MAP.md) del LOGO! 9 real o del PLC-SIM.
 * Asincrono / no bloqueante (libreria emelianov/modbus-esp8266).
 *
 * Ademas de poll(), llamar a service() en cada loop() para bombear la pila TCP.
 */
#pragma once
#include "data_source.h"
#include "map_b.h"
#include <IPAddress.h>
#include <ModbusIP_ESP8266.h>

class ModbusTcpSource : public DataSource {
public:
	bool begin() override;
	void poll() override;
	bool sendCommand(const Command &cmd) override;
	bool isHealthy() const override;
	uint32_t lastOkMs() const override { return lastOk_; }
	const char *name() const override { return "PLC-TCP"; }

	/* Bombea la pila Modbus TCP; llamar frecuentemente desde loop(). */
	void service();

	/* Estado del enlace para diagnostico en la UI. */
	bool     wifiUp() const;
	uint16_t plcOrigin() const { return origin_; }   /* 0 = SIM, 1 = LOGO! real */
	uint16_t heartbeat() const { return heartbeat_; }

private:
	void kickReads();
	void applyStation(uint8_t s);
	void applyGlobal();

	ModbusIP  mb_;
	IPAddress plcIp_;
	bool      ipOk_        = false;
	bool      wifiStarted_ = false;
	uint32_t  lastPoll_    = 0;
	uint32_t  lastOk_      = 0;

	uint16_t  hr_[NUM_WELLS][MAPB_HR_BLOCK_LEN] = {};
	uint16_t  ir_[10] = {};
	uint16_t  origin_    = 0;
	uint16_t  heartbeat_ = 0;
};

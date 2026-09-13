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
	String localIp() const override;
	int    linkRssi() const override;

	/* Bombea la pila Modbus TCP; llamar frecuentemente desde loop(). */
	void service();

	/* Estado del enlace para diagnostico en la UI. */
	bool     wifiUp() const;

	/* --- Pagina de rangos de escala (bloque hb+20..31 del MAPA B) --- */
	void         requestScale(uint8_t s) override;         // lectura bajo demanda
	bool         scaleValid(uint8_t s) const override;
	StationScale getScale(uint8_t s) const override;
	bool         applyScale(uint8_t s, const StationScale &sc) override;  // escribe + pulsa cb+8

private:
	void kickReads();
	void applyStation(uint8_t s);
	void applyGlobal();

	ModbusIP  mb_;
	IPAddress plcIp_;
	bool      ipOk_        = false;
	bool      wifiStarted_ = false;
	uint32_t  lastPoll_    = 0;
	uint32_t  lastConnTry_ = 0;
	uint32_t  lastOk_      = 0;

	/* Instantanea de hmicfg tomada en begin() */
	char      ssid_[33] = {};
	char      pass_[65] = {};
	char      host_[41] = {};
	uint16_t  port_     = PLC_PORT;
	uint8_t   unit_     = PLC_UNIT;
	uint16_t  pollMs_   = MB_POLL_MS;

	uint16_t  hr_[NUM_WELLS][MAPB_HR_BLOCK_LEN] = {};   // bloque completo de estacion
	uint16_t  ir_[10] = {};

	StationScale scale_[NUM_WELLS];
	uint16_t     scaleBuf_[NUM_WELLS][12] = {};   // buffer de lectura async
	uint16_t     wbuf_[12] = {};                  // buffer de escritura async

	/* Restauracion automatica de calibracion (el LOGO! no la retiene tras un
	 * reinicio, ver hmi_config.h/ScaleCache): un solo intento por perdida. */
	bool scaleRestored_[NUM_WELLS] = {};
};

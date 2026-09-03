/**
 * data_source.h  -  Interfaz abstracta de una fuente de datos de planta.
 *
 * Implementaciones:
 *   - ModbusTcpSource (Fase 2, cliente Modbus TCP -> MAPA B del contrato,
 *                      LOGO! 9 real o PLC-SIM)                     [primaria]
 *   - MockSource      (datos simulados)                            [respaldo / dev]
 *   - LoraSource      (Fase 3, puerto TTL de la pasarela, opcional)
 *
 * Contrato de registros: ../../../ORCHESTRATION/REGISTER_MAP.md
 */
#pragma once
#include <Arduino.h>
#include "plant_data.h"

class DataSource {
public:
	virtual ~DataSource() {}

	/* Inicializa el transporte. Devuelve false si no se pudo abrir. */
	virtual bool begin() = 0;

	/* Refresca 'latest' leyendo del dispositivo. Se llama periodicamente. */
	virtual void poll() = 0;

	/* Envia un comando (arranque/paro/modo) a la fuente. */
	virtual bool sendCommand(const Command &cmd) = 0;

	/* true si la fuente ha respondido correctamente hace poco. */
	virtual bool isHealthy() const = 0;

	/* millis() de la ultima transaccion valida. */
	virtual uint32_t lastOkMs() const = 0;

	/* Nombre corto para diagnostico ("Modbus", "LoRa", "Sim"). */
	virtual const char *name() const = 0;

	/* Diagnostico de red (la fuente TCP los rellena; el resto por defecto). */
	virtual String localIp() const { return String("-"); }
	virtual int    linkRssi() const { return 0; }

	/* --- Bloque de escala del MAPA B (hb+20..31). Solo la fuente real lo
	 * implementa; el resto son no-ops. --- */
	virtual void         requestScale(uint8_t /*station*/) {}
	virtual bool         scaleValid(uint8_t /*station*/) const { return false; }
	virtual StationScale getScale(uint8_t /*station*/) const { return StationScale{}; }
	virtual bool         applyScale(uint8_t /*station*/, const StationScale & /*sc*/) { return false; }

	/* Ultimo snapshot leido. Lo rellena poll(). */
	PlantData latest;
};

/**
 * data_hub.h  -  Punto unico de acceso a los datos de planta para la UI.
 *
 * Mantiene el snapshot actual, elige la fuente activa (primaria con failover
 * a la de respaldo) y encola los comandos de la UI hacia esa fuente.
 */
#pragma once
#include <Arduino.h>
#include "data_source.h"

class DataHub {
public:
	static DataHub &instance();

	void begin();

	void setPrimary(DataSource *s) { primary_ = s; }
	void setBackup(DataSource *s)  { backup_  = s; }

	/* Fuente primaria (para la pagina de rangos: la escala vive en el PLC). */
	DataSource *primary() const { return primary_; }

	/* Llamar cada APP_TICK_MS desde loop(). */
	void tick();

	/* Snapshot para la UI (solo lectura). */
	const PlantData &data() const { return data_; }

	/* Pozo seleccionado en la UI (0..NUM_WELLS-1). */
	uint8_t selectedWell() const { return sel_; }
	void    setSelectedWell(uint8_t i) { sel_ = (i < NUM_WELLS) ? i : 0; }

	/* Encola un comando hacia la fuente activa, dirigido a un pozo. */
	void enqueue(CmdType t, uint8_t well);

	SrcHealth primaryHealth() const;
	SrcHealth backupHealth() const;
	bool      mqttUp() const { return false; }   /* Fase 4 */

	const char *activeSourceName() const;

	/* Contadores de diagnostico */
	uint32_t txOk()  const { return txOk_;  }
	uint32_t txErr() const { return txErr_; }

private:
	DataHub() {}
	SrcHealth healthOf(DataSource *s) const;

	PlantData   data_;
	DataSource *primary_ = nullptr;
	DataSource *backup_  = nullptr;
	DataSource *active_  = nullptr;

	static constexpr uint8_t QN = 8;
	Command  queue_[QN];
	uint8_t  qHead_ = 0, qCount_ = 0;
	uint8_t  sel_ = 0;

	uint32_t txOk_ = 0, txErr_ = 0;
};

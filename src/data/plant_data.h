/**
 * plant_data.h  -  Modelo de datos compartido por la UI y las fuentes.
 *
 * El sistema gestiona NUM_WELLS pozos. Cada pozo tiene su propio WellData;
 * PlantData agrupa todos.
 */
#pragma once
#include <stdint.h>
#include "config.h"

enum class PumpMode : uint8_t { Manual = 0, Auto = 1 };

/* Salud de una fuente de datos */
enum class SrcHealth : uint8_t { Ok, Stale, Down };

/* Comandos que la UI encola hacia la fuente activa (dirigidos a un pozo) */
enum class CmdType : uint8_t { PumpStart, PumpStop, SetModeAuto, SetModeManual };

struct Command {
	CmdType type;
	uint8_t well;   /* indice de pozo 0..NUM_WELLS-1 */
};

/* Variables de un pozo */
struct WellData {
	char  name[WELL_NAME_LEN] = "Pozo";
	float levelPct      = 0.0f;   /* nivel 0..100 %              */
	float levelM        = 0.0f;   /* nivel en metros             */
	float flowLps       = 0.0f;   /* caudal instantaneo (L/s)    */
	float flowM3h       = 0.0f;   /* caudal instantaneo (m3/h)   */
	float totalDayM3    = 0.0f;   /* acumulado del dia (m3)      */
	float totalMonthM3  = 0.0f;   /* acumulado del mes (m3)      */
	float histDayM3[HIST_DAYS] = {0}; /* acumulado por dia, [0]=mas antiguo */
	bool  pumpRun       = false;
	bool  pumpFault     = false;
	PumpMode mode       = PumpMode::Manual;
};

/* Snapshot de toda la planta */
struct PlantData {
	WellData well[NUM_WELLS];
	uint8_t  count = NUM_WELLS;
};

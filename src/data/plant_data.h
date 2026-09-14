/**
 * plant_data.h  -  Modelo de datos compartido por la UI y las fuentes.
 *
 * El sistema gestiona NUM_WELLS estaciones de bombeo. Cada una tiene su
 * WellData; PlantData agrupa todas + el estado global del PLC.
 *
 * Refleja el MAPA B del contrato ../../../ORCHESTRATION/REGISTER_MAP.md.
 */
#pragma once
#include <stdint.h>
#include "config.h"
#include "map_b.h"

/* Salud de una fuente de datos */
enum class SrcHealth : uint8_t { Ok, Stale, Down };

/* Comandos que la UI encola hacia la fuente activa (superficie del MAPA B) */
enum class CmdType : uint8_t {
	SirenOn,        /* coil cb+0 = 1  (solo en modo manual)          */
	SirenOff,       /* coil cb+0 = 0                                  */
	SirenAuto,      /* coil cb+1 = 1  (la controla la logica)        */
	SirenManual,    /* coil cb+1 = 0                                  */
	Silence,        /* coil cb+2 (pulso)                              */
	ResetDay,       /* coil cb+3 (pulso, con armado cb+9)            */
	ResetMonth,     /* coil cb+4 (pulso, con armado cb+9)            */
	AckAlarms,      /* coil cb+5 (pulso): reconocer/limpiar hb+14    */
};

struct Command {
	CmdType type;
	uint8_t well;   /* indice de estacion 0..NUM_WELLS-1 */
};

/* Parametros de escala de una variable (bloque hb+20..31 del MAPA B) */
struct ScaleVar {
	uint16_t rawMin = 800;
	uint16_t rawMax = 4000;
	int16_t  engMin = 0;      /* x100 */
	int16_t  engMax = 10000;  /* x100 */
	uint16_t unit   = 0;
	uint16_t filter = 0;
};

struct StationScale {
	ScaleVar level;
	ScaleVar flow;
	uint16_t stamp = 0;      /* hb+31: cambia cuando el PLC aplica */
	bool     valid = false;  /* true tras una lectura correcta */
};

/* Variables de una estacion (subconjunto util del MAPA B) */
struct WellData {
	char  name[WELL_NAME_LEN] = "Estacion";

	float levelEng      = 0.0f;   /* nivel escalado, en la unidad de levelUnit      */
	float flowEng       = 0.0f;   /* caudal escalado, en la unidad de flowUnit      */
	float totalDayM3    = 0.0f;
	float totalMonthM3  = 0.0f;
	float histDayM3[HIST_DAYS] = {0};
	float histMonthM3[HIST_MONTHS] = {0};
	uint8_t levelUnit   = 0;      /* codigo MAPA B hb+28 (0=%,1=m,2=cm,3=mca)       */
	uint8_t flowUnit    = 0;      /* codigo MAPA B hb+29 (0=L/s,1=m3/h,2=L/min,3=GPM)*/

	/* digitales (MAPA B: HR_STATUS / discrete inputs) */
	bool  presostato    = false;
	bool  voltLocal     = false;
	bool  tamper        = false;
	bool  sirenOn       = false;
	bool  sirenAuto     = false;
	bool  linkOk        = false;

	uint16_t alarms       = 0;    /* hb+9  bitfield (MAPB_ALM_*) — activas ahora */
	uint16_t alarmsLatched = 0;   /* hb+14 bitfield — latcheadas / sin reconocer */
	int16_t  rssi       = 0;
	uint16_t ageS       = 0;
	uint16_t levelRaw   = 0;      /* eco ADC, util en la pagina de rangos */
	uint16_t flowRaw    = 0;

	bool inAlarm()    const { return alarms != 0; }
	bool hasLatched() const { return alarmsLatched != 0; }
};

/* Snapshot de toda la planta + estado global (MAPA B IR 2000..2009) */
struct PlantData {
	WellData well[NUM_WELLS];
	uint8_t  count       = NUM_WELLS;

	uint16_t origin      = 0;     /* 0 = PLC-SIM, 1 = LOGO! real */
	uint16_t heartbeat   = 0;
	uint16_t contractVer = 0;
	uint16_t alarmOr     = 0;
};

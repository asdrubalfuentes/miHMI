/**
 * hist_log.h  -  Historico de acumulados (dia/mes) persistido en microSD.
 *
 * El PLC/nodo solo dan el acumulado del dia y del mes EN CURSO -- el HMI es
 * quien decide, con su propia hora (netclock), cuando cerro un dia o un mes,
 * apila ese cierre en el historico y lo guarda en /hist.json.
 */
#pragma once
#include "plant_data.h"

namespace histlog {

/* Carga /hist.json de la microSD (si esta montada). Llamar una vez en setup(),
 * despues de hmicfg::begin(). */
void begin();

/* Llamar cada tick de DataHub, DESPUES de refrescar data con la fuente activa:
 * detecta cruce de dia/mes (hora local) y apila el cierre; siempre deja
 * data.well[*].histDayM3 / histMonthM3 al dia con lo persistido. */
void tick(PlantData &data);

}  // namespace histlog

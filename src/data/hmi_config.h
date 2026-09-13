/**
 * hmi_config.h  -  Configuracion persistente del HMI.
 *
 * Prioridad de carga:  microSD (/hmi_config.json)  >  NVS (respaldo)  >  valores
 * por defecto de config.h / secrets.h.
 *
 * Contiene lo que antes eran #define fijos: red WiFi de planta, destino Modbus
 * del PLC, PIN de administrador, nombres de estacion y tema de la interfaz.
 */
#pragma once
#include <Arduino.h>
#include "config.h"
#include "plant_data.h"   /* ScaleVar / StationScale */

/* Adaptacion de pantalla al ambiente (LDR en GPIO34).
 *
 * Calibracion por 2 puntos: se mide el 'raw' del ADC con LUZ plena y a OSCURAS
 * (rawBright / rawDark, en cualquier orden de polaridad).  A partir de ahi el
 * firmware trabaja con "luz" 0..100 % y todos los umbrales van en % de luz.
 * Se edita por consola serie ('light', 'light cal bright|dark', 'light set ...'). */
struct LightCfg {
	uint8_t  enabled;    /* 1 = brillo automatico por LDR; 0 = fijo (blManual)      */
	uint16_t rawBright;  /* raw medido con LUZ plena   -> luz 100 %                 */
	uint16_t rawDark;    /* raw medido a OSCURAS       -> luz 0 %                   */
	uint8_t  pctClosed;  /* luz% <= esto -> CERRADO (nadie mira): backlight blClosed */
	uint8_t  pctDay;     /* luz% >= esto -> DIA FUERTE: backlight al tope (blMax)    */
	uint8_t  pctTheme;   /* luz% >= esto -> tema claro; si no -> oscuro             */
	uint8_t  hystPct;    /* histeresis en % de luz para no oscilar en el umbral     */
	uint8_t  blClosed;   /* % backlight con el tablero cerrado (0 = apagado)        */
	uint8_t  blMin;      /* % backlight minimo estando abierto                      */
	uint8_t  blMax;      /* % backlight maximo                                      */
	uint8_t  blManual;   /* % backlight fijo cuando enabled = 0                     */
};

/* Ultima calibracion de escala vista/aplicada por estacion (hb+20..31).
 * El LOGO! no la retiene tras un reinicio (sin memoria remanente para eso,
 * ver ORCHESTRATION/PLC_LOGIC.md); el HMI la guarda aqui y la reaplica sola
 * si detecta que el PLC volvio a quedar sin calibrar (rawMax de nivel = 0). */
struct ScaleCache {
	ScaleVar level;
	ScaleVar flow;
	bool     known = false;   /* true una vez que se vio/aplico un valor real */
};

struct HmiConfig {
	char     wifiSsid[33];
	char     wifiPass[65];
	char     plcHost[41];
	uint16_t plcPort;
	uint8_t  plcUnit;
	uint16_t pollMs;
	uint32_t pinHash;                 /* FNV-1a del PIN; 0 = sin PIN (concede siempre) */
	uint8_t  theme;                   /* ThemeMode: 0=auto  1=claro  2=oscuro (colores) */
	uint8_t  dispInvert;              /* inversion de color del panel (1=ON) */
	uint8_t  levelMaxM;              /* profundidad a fondo de escala (m); fallback si no hay escala del PLC */
	char     ntpServer[40];           /* servidor NTP (por defecto ntp.shoa.cl) */
	char     tz[40];                  /* zona horaria POSIX (por defecto Chile) */
	char     stationName[NUM_WELLS][WELL_NAME_LEN];
	LightCfg light;
	ScaleCache scaleCache[NUM_WELLS];
};

namespace hmicfg {

/* Monta la microSD y carga la configuracion. Llamar una vez en setup().
 * safeMode = true (MODO BASICO): no toca microSD ni NVS, solo defaults. */
bool begin(bool safeMode = false);

/* Configuracion viva (solo lectura). */
const HmiConfig &get();

/* Copia editable para la pantalla de Configuracion; aplicar con save(). */
HmiConfig editable();

/* Persiste: NVS siempre + microSD si esta montada. Devuelve true si NVS ok. */
bool save(const HmiConfig &c);

/* Igual pero solo el bloque de adaptacion de pantalla (comando serie 'light'). */
bool saveLight(const LightCfg &lc);

/* Igual pero solo el modo de tema de colores (0=auto 1=claro 2=oscuro). */
bool saveTheme(uint8_t mode);

/* Igual pero solo la inversion de color del panel (0/1). */
bool saveDispInvert(uint8_t on);

/* Igual pero solo servidor NTP + zona horaria (nullptr = no cambia ese campo). */
bool saveTime(const char *server, const char *tz);

/* Igual pero solo la profundidad a fondo de escala (m). */
bool saveLevelMaxM(uint8_t m);

/* Cachea la ultima calibracion valida de una estacion (no-op si es igual a la
 * ya cacheada, para no desgastar la microSD/NVS en cada sondeo). */
bool saveScaleCache(uint8_t station, const StationScale &sc);

/* Estado del almacenamiento. */
bool        sdMounted();
const char *source();                /* "microSD" | "NVS" | "defaults" */

/* Valores de fabrica del bloque de adaptacion de pantalla. */
LightCfg lightDefaults();

/* PIN de administrador. */
uint32_t hashPin(const char *pin);
bool     checkPin(const char *pin);  /* true si coincide, o si no hay PIN configurado */

}  // namespace hmicfg

/**
 * map_b.h  -  Offsets del MAPA B del contrato de orquestacion.
 *   ../../../ORCHESTRATION/REGISTER_MAP.md   (CONTRACT_VERSION 2)
 *
 * El HMI es CLIENTE Modbus TCP y lee este mapa del LOGO! 9 real o del PLC-SIM
 * (proyecto modbusMaster). Direcciones 0-based de protocolo.
 */
#pragma once
#include <stdint.h>
#include "config.h"

/* Bloque por estacion s = 0..NUM_WELLS-1 */
#define MAPB_HR_STRIDE   32          /* holding regs por estacion  (base s*32) */
#define MAPB_DI_STRIDE   16          /* discrete inputs por estacion (base s*16) */
#define MAPB_CO_STRIDE   16          /* coils por estacion          (base s*16) */

/* Holding registers dentro del bloque de estacion (FC03) */
enum {
    MAPB_HR_LEVEL      = 0,          /* nivel  x100 (unidad en +28) */
    MAPB_HR_FLOW       = 1,          /* caudal x100 (unidad en +29) */
    MAPB_HR_LEVEL_RAW  = 2,          /* eco ADC 0..4095 */
    MAPB_HR_FLOW_RAW   = 3,
    MAPB_HR_DAY_W0     = 4,          /* acumulado dia  m3 x10, palabra alta */
    MAPB_HR_DAY_W1     = 5,
    MAPB_HR_MON_W0     = 6,          /* acumulado mes  m3 x10, palabra alta */
    MAPB_HR_MON_W1     = 7,
    MAPB_HR_STATUS     = 8,          /* bitfield, ver MAPB_ST_* */
    MAPB_HR_ALARMS     = 9,          /* bitfield, ver MAPB_ALM_* */
    MAPB_HR_RSSI       = 10,         /* int16 dBm */
    MAPB_HR_AGE        = 11,         /* s desde la ultima respuesta del nodo */
    MAPB_HR_LINK_ADDR  = 12,
    MAPB_HR_RDERR      = 13,
    MAPB_HR_SCALE_BASE = 20,         /* +20..+31 bloque de escalado (r/w) */
    MAPB_HR_LVL_RAWMIN = 20, MAPB_HR_LVL_RAWMAX = 21,
    MAPB_HR_LVL_ENGMIN = 22, MAPB_HR_LVL_ENGMAX = 23,
    MAPB_HR_FLW_RAWMIN = 24, MAPB_HR_FLW_RAWMAX = 25,
    MAPB_HR_FLW_ENGMIN = 26, MAPB_HR_FLW_ENGMAX = 27,
    MAPB_HR_UNIT_LEVEL = 28, MAPB_HR_UNIT_FLOW  = 29,
    MAPB_HR_FILTER     = 30, MAPB_HR_CFG_STAMP  = 31,
    MAPB_HR_BLOCK_LEN  = 32,
};

/* Discrete inputs dentro del bloque de estacion (FC02) */
enum {
    MAPB_DI_PRESOSTATO = 0,
    MAPB_DI_VOLT_LOCAL = 1,
    MAPB_DI_TAMPER     = 2,
    MAPB_DI_SPARE4     = 3,
    MAPB_DI_LORA_OK    = 4,
    MAPB_DI_IN_ALARM   = 5,
    MAPB_DI_SIREN_ON   = 6,
    MAPB_DI_FRESH      = 7,
};

/* Coils dentro del bloque de estacion (FC01/05) - superficie de comandos */
enum {
    MAPB_CO_SIREN_MANUAL = 0,
    MAPB_CO_SIREN_AUTO   = 1,
    MAPB_CO_SILENCE      = 2,        /* pulso */
    MAPB_CO_RESET_DAY    = 3,        /* pulso, exige MAPB_CO_ARM_RESET */
    MAPB_CO_RESET_MONTH  = 4,        /* pulso, exige MAPB_CO_ARM_RESET */
    MAPB_CO_APPLY_SCALE  = 8,        /* pulso */
    MAPB_CO_ARM_RESET    = 9,
};

/* Bits de HR_STATUS */
enum {
    MAPB_ST_PRESOSTATO = 1 << 0,
    MAPB_ST_VOLT_LOCAL = 1 << 1,
    MAPB_ST_TAMPER     = 1 << 2,
    MAPB_ST_SPARE4     = 1 << 3,
    MAPB_ST_SIREN_ON   = 1 << 4,
    MAPB_ST_LINK_OK    = 1 << 5,
    MAPB_ST_IN_ALARM   = 1 << 6,
    MAPB_ST_SIREN_AUTO = 1 << 7,
};

/* Bits de HR_ALARMS (contrato Seccion 4.4) */
enum {
    MAPB_ALM_LEVEL_HI   = 1 << 0,
    MAPB_ALM_LEVEL_LO   = 1 << 1,
    MAPB_ALM_LEVEL_LOLO = 1 << 2,
    MAPB_ALM_NO_FLOW    = 1 << 3,
    MAPB_ALM_PRESS_FAIL = 1 << 4,
    MAPB_ALM_VOLT_LOSS  = 1 << 5,
    MAPB_ALM_TAMPER     = 1 << 6,
    MAPB_ALM_LORA_LOSS  = 1 << 7,
    MAPB_ALM_STALE      = 1 << 8,
    MAPB_ALM_SCALE_BAD  = 1 << 9,
    MAPB_ALM_OVERRANGE  = 1 << 10,
};

/* Bloque global - Holding Registers (FC03), base 96  (v2: antes en IR 2000) */
enum {
    MAPB_G_MARK        = 96,     /* 0x0B01 */
    MAPB_G_NSTATIONS   = 97,
    MAPB_G_ONLINE_BITS = 98,
    MAPB_G_ALARM_OR    = 99,
    MAPB_G_HEARTBEAT   = 100,
    MAPB_G_UPTIME_W0   = 101,
    MAPB_G_UPTIME_W1   = 102,
    MAPB_G_ORIGIN      = 103,    /* 0 = PLC-SIM, 1 = LOGO! real */
    MAPB_G_LOGIC_VER   = 104,
    MAPB_G_CONTRACT    = 105,
};
#define MAPB_MARK   0x0B01

/* Etiqueta de unidad segun el codigo del MAPA B (hb+28 nivel, hb+29 caudal). */
static inline const char *mapb_unit_level(uint16_t u) {
    static const char *N[] = {"%", "m", "cm", "mca"};
    return u < 4 ? N[u] : "?";
}
static inline const char *mapb_unit_flow(uint16_t u) {
    static const char *N[] = {"L/s", "m3/h", "L/min", "GPM"};
    return u < 4 ? N[u] : "?";
}

/* Une dos palabras de 16 bits segun el orden del contrato (Seccion 2). */
static inline uint32_t mapb_u32(uint16_t w0, uint16_t w1) {
#if MAP_B_WORD_HI_FIRST
    return ((uint32_t)w0 << 16) | w1;   /* w0 = palabra alta (dir. menor) */
#else
    return ((uint32_t)w1 << 16) | w0;
#endif
}

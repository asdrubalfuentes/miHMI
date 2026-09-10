/**
 * light_ctrl.h  -  Adaptacion de la pantalla al ambiente (LDR en GPIO34).
 *
 * Lee el LDR, lo suaviza, clasifica la escena con histeresis y ajusta el
 * backlight.  Los parametros (umbrales en cuentas ADC, limites de brillo,
 * modo de tema) salen de hmicfg (LightCfg) y se editan por consola serie.
 *
 * Escenas (se deciden sobre "luz %", no sobre el raw crudo):
 *   CERRADO     tablero cerrado, nadie mira        -> backlight blClosed (0 = off)
 *   NOCHE       abierto, poca luz                  -> backlight bajo, tema oscuro
 *   DIA         abierto, luz de dia                -> backlight medio/alto
 *   DIA_FUERTE  sol directo / reflejo intenso      -> backlight al tope, tema claro
 */
#pragma once
#include <Arduino.h>
#include "data/hmi_config.h"

namespace light {

enum Scene : uint8_t { SC_CLOSED, SC_NIGHT, SC_DAY, SC_BRIGHT };

/* Toma LightCfg de hmicfg y fija el backlight inicial. Llamar tras hmicfg::begin(). */
void begin();

/* Lee el LDR y adapta backlight/escena. Llamar en cada APP_TICK (~4 Hz). */
void tick();

int         raw();           /* ultima muestra del ADC 0..4095 (desglicheada)   */
int         luz();           /* 0..100 % de luz segun calibracion (2 puntos)    */
int         ema();           /* luz% suavizado (lo que usa la logica)           */
Scene       scene();
const char *sceneName();
uint8_t     backlight();     /* % backlight aplicado ahora                      */
bool        ambientBright(); /* respaldo de THEME_AUTO: luz ambiente alta       */

int         calSample();     /* raw actual, para 'light cal bright|dark'        */

const LightCfg &cfg();
void            setCfg(const LightCfg &lc);   /* aplica en caliente, sin persistir */

}  // namespace light

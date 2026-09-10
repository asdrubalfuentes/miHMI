/**
 * ota_hmi.h  -  Integracion del OTA "GitHub Releases pull" en el HMI.
 *
 * Usa el modulo comun net/ota_update.{h,cpp}. El progreso se pinta directo con
 * TFT_eSPI (toma la pantalla completa; no compite con LVGL). La version es
 * APP_VERSION (config.h), que el CI sobreescribe con FW_VERSION_OVERRIDE.
 */
#pragma once

namespace otaHmi {

/* Desde la pantalla de Configuracion (tras el PIN): marca una peticion; la OTA
 * corre en el proximo service() para no bloquear el manejador de eventos LVGL. */
void request();

/* Llamar en loop(). Si hay peticion -> chequea y (si aplica) actualiza mostrando
 * el progreso. Ademas, cada 6 h con WiFi arriba hace un chequeo silencioso y solo
 * toma la pantalla si hay version nueva. */
void service();

}  // namespace otaHmi

/**
 * secrets.example.h  -  Plantilla de credenciales.
 *
 * Copia este archivo a  include/secrets.h  y pon las credenciales reales de la
 * red WiFi del segmento donde estan el LOGO! 9 y la pasarela LoRa.
 *
 *   secrets.h esta en .gitignore: no se sube al repositorio.
 *
 * En operacion, la configuracion de la microSD (HMI_CFG_PATH) tiene prioridad
 * sobre estos valores; esto es solo el arranque de fabrica / respaldo.
 */
#pragma once

#define WIFI_SSID  "MiRedDePlanta"
#define WIFI_PASS  "clave-wifi"

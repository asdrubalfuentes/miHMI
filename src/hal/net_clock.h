/**
 * net_clock.h  -  Hora del HMI por NTP (SNTP del ESP32).
 *
 * Servidor y zona horaria salen de hmicfg (por defecto ntp.shoa.cl y la TZ de
 * Chile continental).  La CYD no tiene RTC con pila: tras un corte se queda sin
 * hora hasta que SNTP resincroniza con la WiFi de planta.
 */
#pragma once
#include <stddef.h>

namespace netclock {

/* Arranca SNTP con la config de hmicfg. Reentrante (re-aplica al cambiarla). */
void begin();

/* true si la hora ya se sincronizo al menos una vez. */
bool valid();

/* "HH:MM" en out (o "--:--" si aun no hay hora). Devuelve valid(). */
bool hm(char *out, size_t n);

/* Texto de estado para la consola serie. */
void status(char *out, size_t n);

}  // namespace netclock

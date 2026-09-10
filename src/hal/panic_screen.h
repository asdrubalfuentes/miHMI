/**
 * panic_screen.h  -  Aviso de fallo del HMI a pantalla completa.
 *
 *  - panic_screen_stage(): marca en que punto del arranque estamos.  El valor
 *    vive en RTC RAM, asi que sobrevive a un reset: si la placa se reinicia por
 *    panic / watchdog / brownout, el arranque siguiente sabe donde se cayo.
 *  - panic_screen_check_reset(): si el reset anterior fue anormal, pinta un
 *    aviso rojo con el motivo y en que etapa ocurrio, espera unos segundos y
 *    devuelve el control para que el arranque continue.  Devuelve true si
 *    mostro algo.
 *  - panic_screen_fatal(): condicion irrecuperable.  Pinta el motivo y detiene
 *    el equipo (bucle infinito hasta pulsar RST).  No retorna.
 *
 * Dibuja con TFT_eSPI directamente, sin LVGL: solo necesita display_hw_init().
 */
#pragma once

void panic_screen_stage(const char *name);
bool panic_screen_check_reset();
void panic_screen_fatal(const char *title,
                        const char *l1 = nullptr,
                        const char *l2 = nullptr,
                        const char *l3 = nullptr);

/* true si se acumularon BOOT_MAX_FAILS reinicios anormales seguidos: el arranque
 * debe saltar la microSD / ajustes guardados y usar solo valores por defecto.
 * Solo es valido despues de llamar a panic_screen_check_reset(). */
bool panic_safe_mode();

/* Llamar cuando el HMI lleva BOOT_STABLE_MS funcionando sin caer: da el arranque
 * por bueno y pone a cero el contador de reinicios. */
void panic_boot_ok();

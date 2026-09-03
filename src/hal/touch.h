/**
 * touch.h  -  Panel tactil resistivo XPT2046 + enlace con LVGL + calibracion.
 */
#pragma once
#include <Arduino.h>

/* Inicializa el bus SPI dedicado y el controlador XPT2046. */
void touch_hw_init();

/* Si NVS no tiene una calibracion valida, lanza la rutina (dibuja con TFT_eSPI).
 * Debe llamarse DESPUES de display_hw_init() y ANTES de lv_init(). */
void touch_calibrate_if_needed();

/* Fuerza la rutina de calibracion (opcion "Recalibrar" en Ajustes). */
void touch_force_calibrate();

/* Registra el dispositivo de entrada en LVGL. Llamar tras lv_init(). */
void touch_lvgl_init();

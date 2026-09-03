/**
 * display.h  -  Pantalla TFT (ILI9341 via TFT_eSPI) + enlace con LVGL.
 */
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

/* Inicializa el hardware de la pantalla (SPI, rotacion, backlight ON). */
void display_hw_init();

/* Registra el driver de display y los buffers en LVGL. Llamar tras lv_init(). */
void display_lvgl_init();

/* Acceso al objeto TFT_eSPI (lo usa la rutina de calibracion tactil). */
TFT_eSPI &display_tft();

/* Brillo del backlight 0..100 %. */
void display_backlight_pct(uint8_t pct);

/* Ajuste automatico de brillo a partir del LDR (llamar en app tick). */
void display_backlight_auto();

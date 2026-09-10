/**
 * lv_conf.h  -  Configuracion LVGL 8.4 para el HMI CYD (ESP32-2432S028R)
 *
 * Solo se redefinen las opciones que difieren del valor por defecto de LVGL.
 * El resto lo completa lv_conf_internal.h con #ifndef.
 * Se activa con el build flag -D LV_CONF_INCLUDE_SIMPLE  (ver platformio.ini).
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*---------------------------
 *  Color
 *--------------------------*/
#define LV_COLOR_DEPTH 16
/* TFT_eSPI + pushColors(..., swap=true): dejar el swap del lado del driver.
 * Si los colores salen invertidos (rojo<->azul), poner esto a 1. */
#define LV_COLOR_16_SWAP 0

/*---------------------------
 *  Memoria
 *--------------------------*/
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)   /* las pantallas se crean bajo demanda, ver ui.cpp */

/*---------------------------
 *  HAL / tick
 *--------------------------*/
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_DPI_DEF 130

/*---------------------------
 *  Rendimiento / features
 *--------------------------*/
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_ASSERT_MALLOC 0

/*---------------------------
 *  Log
 *--------------------------*/
#define LV_USE_LOG 1
#if LV_USE_LOG
	#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
	#define LV_LOG_PRINTF 0   /* se registra un callback propio a Serial en main.cpp */
#endif

/*---------------------------
 *  Fuentes
 *--------------------------*/
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*---------------------------
 *  Widgets (los que usa el HMI; en 8.4 ya vienen a 1 por defecto,
 *  se dejan explicitos por claridad)
 *--------------------------*/
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CHART 1
#define LV_USE_LABEL 1
#define LV_USE_MSGBOX 1
#define LV_USE_SWITCH 1
#define LV_USE_TABVIEW 1
#define LV_USE_TABLE 1

/*---------------------------
 *  Temas
 *--------------------------*/
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 1

#endif /*LV_CONF_H*/

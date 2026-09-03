/**
 * theme.h  -  Paleta y estilos reutilizables del HMI (marca AYSAFI).
 */
#pragma once
#include <lvgl.h>

/* Paleta AYSAFI (del logo) */
#define COL_TEAL     lv_color_hex(0x1499A6)
#define COL_TEAL_D   lv_color_hex(0x0E7A85)
#define COL_ORANGE   lv_color_hex(0xF15A22)
#define COL_BG       lv_color_hex(0x0B1418)
#define COL_CARD     lv_color_hex(0x14232A)
#define COL_TEXT     lv_color_hex(0xE8EEF0)
#define COL_MUTED    lv_color_hex(0x7C8B90)
#define COL_RUN      lv_color_hex(0x2FBF71)
#define COL_STOP     lv_color_hex(0xE04848)
#define COL_WARN     lv_color_hex(0xF2A900)

extern lv_style_t st_card;
extern lv_style_t st_title;
extern lv_style_t st_value;
extern lv_style_t st_unit;

void theme_init();

/* Crea un contenedor "tarjeta" con el estilo comun. */
lv_obj_t *ui_card(lv_obj_t *parent);

/* Color asociado a un estado de salud de fuente. */
lv_color_t ui_health_color(int health /* SrcHealth */);

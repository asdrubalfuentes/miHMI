/**
 * ui.h  -  Punto de entrada de la interfaz grafica y navegacion entre pantallas.
 *
 * Mapa de pantallas:
 *   splash  ->  wells (lista de pozos)  ->  well (detalle de un pozo)
 *                                             |-> history (historico del pozo)
 *                                             |-> settings -> help
 *   wells  -> help
 */
#pragma once
#include <stdint.h>

/* Crea todas las pantallas y muestra el splash. Llamar tras inicializar LVGL. */
void ui_init();

/* Refresca la pantalla activa con los datos del DataHub. Llamar en el app tick. */
void ui_tick();

/* Destruye y vuelve a crear las pantallas (tras un cambio de tema). Recarga la
 * que estaba activa. */
void ui_rebuild_all();

/* Navegacion */
void ui_show_wells();
void ui_show_well(uint8_t idx);   /* fija el pozo seleccionado y abre el detalle */
void ui_show_actions();           /* acciones de mando del pozo seleccionado */
void ui_show_history();
void ui_show_settings();
void ui_show_config();             /* configuracion de administrador (tras PIN) */
void ui_show_pin(void (*on_ok)()); /* puerta de PIN; on_ok se ejecuta al acertar */
void ui_show_help();

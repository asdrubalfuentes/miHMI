/**
 * ui_screens.h  -  API interna: cada pantalla ofrece create() y (si aplica) update().
 */
#pragma once
#include <lvgl.h>

lv_obj_t *screen_splash_create();

lv_obj_t *screen_wells_create();
void      screen_wells_update();

lv_obj_t *screen_well_create();
void      screen_well_update();

lv_obj_t *screen_history_create();
void      screen_history_update();

lv_obj_t *screen_settings_create();
void      screen_settings_update();

lv_obj_t *screen_help_create();

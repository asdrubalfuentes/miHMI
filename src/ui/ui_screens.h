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

lv_obj_t *screen_actions_create();
void      screen_actions_update();

lv_obj_t *screen_history_create();
void      screen_history_update();

lv_obj_t *screen_settings_create();
void      screen_settings_update();

lv_obj_t *screen_pin_create();
void      screen_pin_prepare(void (*on_ok)());

lv_obj_t *screen_config_create();
void      screen_config_enter();


lv_obj_t *screen_help_create();

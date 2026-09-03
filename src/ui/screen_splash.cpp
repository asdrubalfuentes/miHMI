#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include <lvgl.h>

/* Recursos de logo generados por tools/img2lvgl.py (sobre fondo blanco).
 * Si aun no existen, cada slot cae a un marcador de texto. */
#if defined(__has_include)
#  if __has_include("assets/logo_aysafi.h")
#    include "assets/logo_aysafi.h"
#    define HAVE_LOGO_AYSAFI 1
#  endif
#  if __has_include("assets/logo_cliente.h")
#    include "assets/logo_cliente.h"
#    define HAVE_LOGO_CLIENTE 1
#  endif
#endif

static void splash_timer_cb(lv_timer_t *t) {
	lv_timer_del(t);
	ui_show_wells();
}

lv_obj_t *screen_splash_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	/* Logo de la empresa cliente (arriba) */
#ifdef HAVE_LOGO_CLIENTE
	lv_obj_t *client = lv_img_create(scr);
	lv_img_set_src(client, &logo_cliente);
	lv_obj_align(client, LV_ALIGN_TOP_MID, 0, 20);
#else
	lv_obj_t *client = lv_label_create(scr);
	lv_label_set_text(client, CLIENT_NAME);
	lv_obj_set_style_text_color(client, lv_color_hex(0x555555), 0);
	lv_obj_set_style_text_font(client, &lv_font_montserrat_16, 0);
	lv_obj_align(client, LV_ALIGN_TOP_MID, 0, 24);
#endif

	/* Logo / marca del producto (centro) */
#ifdef HAVE_LOGO_AYSAFI
	lv_obj_t *logo = lv_img_create(scr);
	lv_img_set_src(logo, &logo_aysafi);
	lv_obj_align(logo, LV_ALIGN_CENTER, 0, -2);
#else
	lv_obj_t *brand = lv_label_create(scr);
	lv_label_set_text(brand, "AYSAFI");
	lv_obj_set_style_text_color(brand, COL_TEAL, 0);
	lv_obj_set_style_text_font(brand, &lv_font_montserrat_48, 0);
	lv_obj_align(brand, LV_ALIGN_CENTER, 0, -8);

	lv_obj_t *sub = lv_label_create(scr);
	lv_label_set_text(sub, "Ingenieria y Tecnologia");
	lv_obj_set_style_text_color(sub, COL_ORANGE, 0);
	lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
	lv_obj_align(sub, LV_ALIGN_CENTER, 0, 32);
#endif

	/* Pie: nombre de la aplicacion + version */
	lv_obj_t *ver = lv_label_create(scr);
	lv_label_set_text_fmt(ver, "%s   v%s", APP_NAME, APP_VERSION);
	lv_obj_set_style_text_color(ver, lv_color_hex(0x8A8A8A), 0);
	lv_obj_set_style_text_font(ver, &lv_font_montserrat_12, 0);
	lv_obj_align(ver, LV_ALIGN_BOTTOM_MID, 0, -12);

	lv_timer_create(splash_timer_cb, SPLASH_MS, nullptr);
	return scr;
}

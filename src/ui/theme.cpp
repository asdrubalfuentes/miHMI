#include "ui/theme.h"
#include "data/plant_data.h"

lv_style_t st_card;
lv_style_t st_title;
lv_style_t st_value;
lv_style_t st_unit;

void theme_init() {
	lv_style_init(&st_card);
	lv_style_set_bg_color(&st_card, COL_CARD);
	lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
	lv_style_set_radius(&st_card, 8);
	lv_style_set_border_width(&st_card, 0);
	lv_style_set_pad_all(&st_card, 8);

	lv_style_init(&st_title);
	lv_style_set_text_color(&st_title, COL_MUTED);
	lv_style_set_text_font(&st_title, &lv_font_montserrat_12);

	lv_style_init(&st_value);
	lv_style_set_text_color(&st_value, COL_TEXT);
	lv_style_set_text_font(&st_value, &lv_font_montserrat_28);

	lv_style_init(&st_unit);
	lv_style_set_text_color(&st_unit, COL_MUTED);
	lv_style_set_text_font(&st_unit, &lv_font_montserrat_14);
}

lv_obj_t *ui_card(lv_obj_t *parent) {
	lv_obj_t *c = lv_obj_create(parent);
	lv_obj_add_style(c, &st_card, 0);
	lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
	return c;
}

lv_color_t ui_health_color(int health) {
	switch ((SrcHealth)health) {
		case SrcHealth::Ok:    return COL_RUN;
		case SrcHealth::Stale: return COL_WARN;
		default:               return COL_STOP;
	}
}

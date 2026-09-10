#include "ui/theme.h"
#include "ui/ui.h"
#include "data/plant_data.h"

// Paletas de marca AYSAFI.  Ver theme.h.

lv_style_t st_card;
lv_style_t st_title;
lv_style_t st_value;
lv_style_t st_unit;

Palette PAL;   /* paleta activa */

/* ---- CLARO : proteccion visual (ISA-101) ---------------------------------
   Fondo gris de baja luminancia, texto casi negro, color solo para lo anormal. */
static const Palette PAL_LIGHT = {
	/*dark*/      false,
	/*bg*/        lv_color_hex(0xBFC5C8),
	/*card*/      lv_color_hex(0xD2D7D9),
	/*sunken*/    lv_color_hex(0xAEB4B7),
	/*line*/      lv_color_hex(0x9AA1A5),
	/*text*/      lv_color_hex(0x161C1F),
	/*muted*/     lv_color_hex(0x4E585D),
	/*accent*/    lv_color_hex(0x0E7A85),
	/*accentDim*/ lv_color_hex(0x146E78),
	/*action*/    lv_color_hex(0xC24A18),
	/*ok*/        lv_color_hex(0x1E7A34),
	/*notice*/    lv_color_hex(0x8A6D00),
	/*warn*/      lv_color_hex(0xB25A0E),
	/*alarm*/     lv_color_hex(0xB01E1E),
	/*crit*/      lv_color_hex(0x8E1D6B),
};

/* ---- OSCURO : alto contraste -------------------------------------------------
   Fondo azul petroleo; el color de la LETRA codifica la gravedad del mensaje. */
static const Palette PAL_DARK = {
	/*dark*/      true,
	/*bg*/        lv_color_hex(0x08171E),
	/*card*/      lv_color_hex(0x10242E),
	/*sunken*/    lv_color_hex(0x0B1C24),
	/*line*/      lv_color_hex(0x1F3A45),
	/*text*/      lv_color_hex(0xE9F1F3),
	/*muted*/     lv_color_hex(0x7FA0AB),
	/*accent*/    lv_color_hex(0x19B6C8),
	/*accentDim*/ lv_color_hex(0x0E5A66),
	/*action*/    lv_color_hex(0xF15A22),
	/*ok*/        lv_color_hex(0x2FE38A),
	/*notice*/    lv_color_hex(0xFFD23F),
	/*warn*/      lv_color_hex(0xFF9F1C),
	/*alarm*/     lv_color_hex(0xFF3B57),
	/*crit*/      lv_color_hex(0xFF4FD8),
};

static ThemeMode s_mode        = THEME_AUTO;
static bool      s_ambientDark = false;   /* ultimo veredicto del respaldo LDR */
static bool      s_stylesInit  = false;

static bool want_dark() {
	switch (s_mode) {
		case THEME_LIGHT: return false;
		case THEME_DARK:  return true;
		default:          return s_ambientDark;   /* AUTO: por ahora, LDR */
	}
}

static void build_styles() {
	if (!s_stylesInit) {
		lv_style_init(&st_card);
		lv_style_init(&st_title);
		lv_style_init(&st_value);
		lv_style_init(&st_unit);
		s_stylesInit = true;
	}
	lv_style_set_bg_color(&st_card, PAL.card);
	lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
	lv_style_set_radius(&st_card, 8);
	lv_style_set_border_width(&st_card, 1);
	lv_style_set_border_color(&st_card, PAL.line);
	lv_style_set_pad_all(&st_card, 8);

	lv_style_set_text_color(&st_title, PAL.muted);
	lv_style_set_text_font(&st_title, &lv_font_montserrat_12);

	lv_style_set_text_color(&st_value, PAL.text);
	lv_style_set_text_font(&st_value, &lv_font_montserrat_28);

	lv_style_set_text_color(&st_unit, PAL.muted);
	lv_style_set_text_font(&st_unit, &lv_font_montserrat_14);
}

static void apply(bool dark, bool rebuild) {
	PAL = dark ? PAL_DARK : PAL_LIGHT;
	build_styles();
	if (rebuild) ui_rebuild_all();
}

void theme_init(ThemeMode m) {
	s_mode = m;
	apply(want_dark(), false);
}

void theme_set_mode(ThemeMode m) {
	bool wasDark = PAL.dark;
	s_mode = m;
	bool d = want_dark();
	if (d != wasDark || !s_stylesInit) apply(d, true);
}

void theme_eval_auto(bool ambientBright) {
	bool nowDark = !ambientBright;
	if (nowDark == s_ambientDark) return;
	s_ambientDark = nowDark;
	if (s_mode == THEME_AUTO && want_dark() != PAL.dark) apply(want_dark(), true);
}

ThemeMode theme_mode()    { return s_mode; }
bool      theme_is_dark() { return PAL.dark; }

lv_color_t sev_color(Sev s) {
	switch (s) {
		case Sev::Ok:     return PAL.ok;
		case Sev::Notice: return PAL.notice;
		case Sev::Warn:   return PAL.warn;
		case Sev::Alarm:  return PAL.alarm;
		case Sev::Crit:   return PAL.crit;
		default:          return PAL.text;
	}
}

lv_obj_t *ui_card(lv_obj_t *parent) {
	lv_obj_t *c = lv_obj_create(parent);
	lv_obj_add_style(c, &st_card, 0);
	lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
	return c;
}

lv_color_t ui_health_color(int health) {
	switch ((SrcHealth)health) {
		case SrcHealth::Ok:    return PAL.ok;
		case SrcHealth::Stale: return PAL.warn;
		default:               return PAL.alarm;
	}
}

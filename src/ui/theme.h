/**
 * theme.h  -  Paleta de colores del HMI (marca AYSAFI: teal #1499A6 / naranja #F15A22).
 *
 * Dos temas conmutables EN CALIENTE:
 *   CLARO   - "proteccion visual" (ISA-101): fondo gris de baja luminancia,
 *             texto casi negro nitido, color reservado a lo anormal.
 *   OSCURO  - "alto contraste": fondo azul petroleo; el color de la LETRA
 *             codifica la gravedad del mensaje.
 *
 * El brillo del backlight (LDR) es independiente del tema (ver hal/light_ctrl).
 *
 * La paleta vive en runtime (struct Palette PAL).  Los nombres COL_* de siempre
 * siguen valiendo como alias, asi que el resto de pantallas no cambian su forma
 * de pedir color; solo hay que RECONSTRUIRLAS al cambiar de tema (ui_rebuild_all).
 */
#pragma once
#include <lvgl.h>

struct Palette {
	bool       dark;
	lv_color_t bg;         /* fondo de pantalla            */
	lv_color_t card;       /* tarjetas / paneles elevados  */
	lv_color_t sunken;     /* listas / zonas hundidas      */
	lv_color_t line;       /* bordes, separadores          */
	lv_color_t text;       /* texto principal              */
	lv_color_t muted;      /* texto secundario / unidades  */
	lv_color_t accent;     /* navegacion, foco (teal)      */
	lv_color_t accentDim;  /* fondo de boton secundario    */
	lv_color_t action;     /* accion destacada (naranja)   */
	/* rampa de gravedad (color de estado / de letra en OSCURO) */
	lv_color_t ok;         /* normal / en marcha           */
	lv_color_t notice;     /* aviso leve                   */
	lv_color_t warn;       /* advertencia                  */
	lv_color_t alarm;      /* alarma                       */
	lv_color_t crit;       /* critico (magenta)            */
};

extern Palette PAL;

/* Alias de compatibilidad: las pantallas siguen escribiendo COL_BG, COL_TEXT... */
#define COL_BG      PAL.bg
#define COL_CARD    PAL.card
#define COL_SUNKEN  PAL.sunken
#define COL_LINE    PAL.line
#define COL_TEXT    PAL.text
#define COL_MUTED   PAL.muted
#define COL_TEAL    PAL.accent
#define COL_TEAL_D  PAL.accentDim
#define COL_ORANGE  PAL.action
#define COL_RUN     PAL.ok
#define COL_NOTICE  PAL.notice
#define COL_WARN    PAL.warn
#define COL_STOP    PAL.alarm
#define COL_CRIT    PAL.crit

/* Gravedad de un mensaje/estado -> color de letra (segun el tema activo). */
enum class Sev : uint8_t { Info, Ok, Notice, Warn, Alarm, Crit };
lv_color_t sev_color(Sev s);

enum ThemeMode : uint8_t { THEME_AUTO = 0, THEME_LIGHT = 1, THEME_DARK = 2 };

/* Fija el modo y la paleta SIN reconstruir (aun no hay pantallas).
 * Llamar una vez antes de ui_init(). */
void theme_init(ThemeMode m);

/* Fija el modo (persistir aparte con hmicfg).  Reconstruye la UI si cambia el
 * aspecto.  En THEME_AUTO decide por 'ambientBright' (respaldo del LDR; en la
 * Etapa 2 mandara la hora NTP). */
void theme_set_mode(ThemeMode m);

/* Reevalua THEME_AUTO con la luz ambiente (llamar en el app-tick). */
void theme_eval_auto(bool ambientBright);

ThemeMode theme_mode();
bool      theme_is_dark();

extern lv_style_t st_card;
extern lv_style_t st_title;
extern lv_style_t st_value;
extern lv_style_t st_unit;

/* Crea un contenedor "tarjeta" con el estilo comun. */
lv_obj_t *ui_card(lv_obj_t *parent);

/* Color asociado a un estado de salud de fuente. */
lv_color_t ui_health_color(int health /* SrcHealth */);

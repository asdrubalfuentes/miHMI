#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include <lvgl.h>

static void on_back(lv_event_t *e) { (void)e; ui_show_wells(); }

static void section(lv_obj_t *cont, const char *title, const char *body) {
	lv_obj_t *t = lv_label_create(cont);
	lv_label_set_text(t, title);
	lv_obj_set_style_text_font(t, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(t, COL_TEAL, 0);
	lv_obj_set_style_pad_top(t, 8, 0);

	lv_obj_t *b = lv_label_create(cont);
	lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(b, LV_PCT(100));
	lv_label_set_text(b, body);
	lv_obj_set_style_text_font(b, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(b, COL_TEXT, 0);
}

lv_obj_t *screen_help_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	/* --- barra superior --- */
	lv_obj_t *top = lv_obj_create(scr);
	lv_obj_set_pos(top, 0, 0);
	lv_obj_set_size(top, SCREEN_W, 30);
	lv_obj_set_style_bg_color(top, COL_CARD, 0);
	lv_obj_set_style_radius(top, 0, 0);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *title = lv_label_create(top);
	lv_label_set_text(title, "AYUDA  -  guia rapida");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(title, COL_TEXT, 0);
	lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

	lv_obj_t *back = lv_btn_create(top);
	lv_obj_set_size(back, 40, 24);
	lv_obj_align(back, LV_ALIGN_RIGHT_MID, -6, 0);
	lv_obj_set_style_bg_color(back, COL_TEAL_D, 0);
	lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *bl = lv_label_create(back);
	lv_label_set_text(bl, LV_SYMBOL_LEFT);
	lv_obj_center(bl);

	/* --- contenido desplazable --- */
	lv_obj_t *cont = lv_obj_create(scr);
	lv_obj_set_pos(cont, 0, 30);
	lv_obj_set_size(cont, SCREEN_W, SCREEN_H - 30);
	lv_obj_set_style_bg_color(cont, COL_BG, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_style_pad_all(cont, 10, 0);
	lv_obj_set_style_pad_row(cont, 2, 0);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

	section(cont, "1. Mapa de pantallas",
	        "Splash  ->  POZOS  ->  Detalle de pozo.\n"
	        "Desde el detalle se abre Historico y Ajustes.\n"
	        "Ajustes  ->  Ayuda (esta pantalla).");

	section(cont, "2. Pantalla POZOS",
	        "Lista con todos los pozos de la red. Cada tarjeta muestra: nombre, "
	        "estado de la bomba (MARCHA / PARO / FALLO), nivel y caudal. "
	        "Toca una tarjeta para abrir su detalle. El boton de arriba a la "
	        "derecha abre esta ayuda.");

	section(cont, "3. Detalle de pozo",
	        "Barra superior:  [<] volver a la lista  |  [<] nombre [>] cambiar de "
	        "pozo  |  [engranaje] Ajustes.\n"
	        "Arco grande = NIVEL del pozo (porcentaje y metros).\n"
	        "CAUDAL = caudal instantaneo (L/s y m3/h).\n"
	        "ACUMULADO HOY = volumen del dia (m3).\n"
	        "Etiqueta arriba a la derecha = estado de la bomba.");

	section(cont, "4. Encender / Detener",
	        "Botones inferiores del detalle. Piden confirmacion (Si / No). La orden "
	        "se envia al PLC del pozo seleccionado. Hoy es simulada; el control "
	        "real por Modbus RTU llega en la Fase 2.");

	section(cont, "5. Historico",
	        "Boton [Histor.] en el detalle. Barras = m3 por dia de los ultimos "
	        "14 dias. Abajo, el acumulado del mes en curso. [Volver] regresa al "
	        "detalle del mismo pozo.");

	section(cont, "6. Barra de estado (MB / LoRa / NET)",
	        "MB   = enlace Modbus RTU / RS485 con el PLC (fuente primaria).\n"
	        "LoRa = puerto TTL de la pasarela LoRa (respaldo automatico).\n"
	        "NET  = red / MQTT (opcional).\n"
	        "Color: verde = OK, ambar = datos antiguos, rojo/gris = sin enlace.");

	section(cont, "7. Ajustes",
	        "Parametros de comunicacion (ID Modbus, baudios, puerto LoRa, MQTT), "
	        "fuente de datos activa, contador de tramas OK / ERR y 'Recalibrar "
	        "pantalla' (repite el toque de las 2 cruces).");

	section(cont, "8. Origen de los datos",
	        "El HMI consulta al PLC por Modbus RTU sobre RS485. Si el PLC no "
	        "responde, conmuta solo al puerto TTL de la pasarela LoRa. La red / "
	        "MQTT nunca es critica para operar.");

	return scr;
}

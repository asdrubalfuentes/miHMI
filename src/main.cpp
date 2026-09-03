/**
 * miHMI  -  HMI de captacion de pozos profundos
 * Placa: Cheap Yellow Display  ESP32-2432S028R  (ILI9341 320x240 + XPT2046)
 * GUI:   LVGL 8.4
 *
 * Fase 1: esqueleto + interfaz grafica (splash -> pantalla inicial) con datos
 *         simulados (MockSource).  La comunicacion de campo (Modbus RTU / RS485
 *         y pasarela LoRa) se anade en fases posteriores sobre la misma capa
 *         de abstraccion (DataSource / DataHub).
 */
#include <Arduino.h>
#include <lvgl.h>

#include "config.h"
#include "hal/display.h"
#include "hal/touch.h"
#include "ui/ui.h"
#include "data/data_hub.h"
#include "data/mock_source.h"
#include "data/modbus_tcp_source.h"

static MockSource      g_mock;   /* respaldo / desarrollo sin PLC */
static ModbusTcpSource g_plc;    /* primaria: Modbus TCP -> Mapa B (LOGO! 9 / PLC-SIM) */

#if LV_USE_LOG
static void lv_log_cb(const char *buf) {
	Serial.print("[LVGL] ");
	Serial.println(buf);
}
#endif

void setup() {
	Serial.begin(115200);
	delay(200);
	Serial.println();
	Serial.println(APP_NAME "  v" APP_VERSION);

	/* LED RGB (activo a nivel bajo en la CYD): apagar */
	pinMode(PIN_LED_R, OUTPUT); digitalWrite(PIN_LED_R, HIGH);
	pinMode(PIN_LED_G, OUTPUT); digitalWrite(PIN_LED_G, HIGH);
	pinMode(PIN_LED_B, OUTPUT); digitalWrite(PIN_LED_B, HIGH);

	/* Pantalla + tactil ANTES de LVGL (la calibracion dibuja con TFT_eSPI) */
	display_hw_init();
	touch_hw_init();
	touch_calibrate_if_needed();

	/* LVGL */
	lv_init();
#if LV_USE_LOG
	lv_log_register_print_cb(lv_log_cb);
#endif
	display_lvgl_init();
	touch_lvgl_init();

	/* Datos: primaria Modbus TCP (Mapa B); respaldo simulado con failover */
	DataHub::instance().setPrimary(&g_plc);
	DataHub::instance().setBackup(&g_mock);
	DataHub::instance().begin();

	/* Interfaz */
	ui_init();

	Serial.println("HMI listo.");
}

void loop() {
	static uint32_t t_app = 0;

	lv_timer_handler();
	g_plc.service();          /* bombea la pila Modbus TCP en cada iteracion */

	uint32_t now = millis();
	if (now - t_app >= APP_TICK_MS) {
		t_app = now;
		DataHub::instance().tick();
		ui_tick();
		display_backlight_auto();
	}

	delay(LVGL_TASK_MS);
}

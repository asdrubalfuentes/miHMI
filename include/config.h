/**
 * config.h  -  Pines, tiempos, flags y mapa Modbus (placeholder) del HMI.
 *
 * Placa: Cheap Yellow Display  ESP32-2432S028R
 * Valores de pines segun https://randomnerdtutorials.com/cheap-yellow-display-esp32-2432s028r/
 */
#pragma once

/* ============================ Branding ============================ */
#define APP_NAME        "HMI Captacion de Pozos"
#define APP_VERSION     "0.1.0"
#define CLIENT_NAME     "Empresa Cliente"          /* TODO: logo real del cliente */
#define PRODUCT_NAME    "AYSAFI  -  Ingenieria y Tecnologia"

/* ============================ Pantalla =========================== */
/* (los pines TFT se pasan por build_flags a TFT_eSPI; aqui solo resolucion util) */
#define SCREEN_W        320
#define SCREEN_H        240
#define TFT_ROTATION    1        /* apaisado */
#define PIN_TFT_BL      21       /* backlight (PWM, activo ALTO) */
#define DRAW_BUF_LINES  40       /* alto del buffer parcial de LVGL */

/* ======================== Tactil XPT2046 ======================== */
/* Bus SPI dedicado (VSPI), independiente del bus de la pantalla */
#define PIN_TOUCH_IRQ   36
#define PIN_TOUCH_MOSI  32
#define PIN_TOUCH_MISO  39
#define PIN_TOUCH_CLK   25
#define PIN_TOUCH_CS    33
#define TOUCH_ROTATION  1

/* ===================== Perifericos de la placa ================== */
#define PIN_LED_R       4
#define PIN_LED_G       16
#define PIN_LED_B       17
#define PIN_LDR         34      /* fotoresistencia -> brillo automatico */

/* ===================== Comunicacion de campo =================== */
/* RS485 -> PLC (Modbus RTU maestro).  UART1 remapeado a las E/S libres CN1/P3. */
#define PIN_RS485_TX    22
#define PIN_RS485_RX    27
#define PIN_RS485_DE    4       /* DE/RE unidos. -1 si el modulo es de direccion automatica */
#define RS485_UART_NUM  1

/* TTL -> pasarela LoRa (respaldo).  UART2 en sus pines nativos (LED verde/azul sacrificados). */
#define PIN_LORA_TX     17
#define PIN_LORA_RX     16
#define LORA_UART_NUM   2
#define LORA_BAUD       9600

/* ========================= Temporizaciones ===================== */
#define SPLASH_MS       2500
#define LVGL_TASK_MS    5
#define APP_TICK_MS     250

/* ========================= Feature flags ====================== */
#define FEATURE_MQTT        0
#define FEATURE_LORA_BACKUP 1
#define FEATURE_SD_LOG      0

/* ===================== Pozos / estaciones ===================== */
#define NUM_WELLS       2       /* 2 estaciones de bombeo (contrato Mapa B) */
#define WELL_NAME_LEN   16

/* ===================== Historico de caudal ==================== */
#define HIST_DAYS       14      /* barras del grafico "ultimos dias" */

/* ====== Fase 2: enlace de campo por Modbus TCP (MAPA B del contrato) ======= */
/* Contrato: ../ORCHESTRATION/REGISTER_MAP.md   ·   offsets en src/data/map_b.h */
#define CONTRACT_VERSION     1
#define MAP_B_WORD_HI_FIRST  1        /* 32b: palabra alta en la dir. menor (contrato Sec. 2) */

#define WIFI_SSID            ""       /* red de planta; vacio = no conecta (queda la fuente de respaldo) */
#define WIFI_PASS            ""
#define PLC_HOST             "192.168.4.50"  /* IP del LOGO! 9 real o del PLC-SIM (modbusMaster) */
#define PLC_PORT             502
#define PLC_UNIT             1        /* Unit ID (el servidor responde con cualquiera) */
#define MB_POLL_MS           750      /* periodo de sondeo del Mapa B */

#define MB_LEVEL_SCALE       100.0f   /* Mapa B: nivel y caudal viajan x100 */
#define MB_FLOW_SCALE        100.0f
#define MB_ACCUM_SCALE       10.0f    /* acumulados en m3 x10 */

/**
 * config.h  -  Pines, tiempos, flags y mapa Modbus (placeholder) del HMI.
 *
 * Placa: Cheap Yellow Display  ESP32-2432S028R
 * Valores de pines segun https://randomnerdtutorials.com/cheap-yellow-display-esp32-2432s028r/
 */
#pragma once

/* ============================ Branding ============================ */
#define APP_NAME        "HMI Captacion de Pozos"
#define APP_VERSION     "0.5.0"
/* El CI (.github/workflows/release.yml) define FW_VERSION_OVERRIDE = X.Y.Z del
 * tag; esa es la version que compara el cliente OTA (net/ota_hmi). */
#ifdef FW_VERSION_OVERRIDE
#  undef  APP_VERSION
#  define APP_VERSION FW_VERSION_OVERRIDE
#endif
#define CLIENT_NAME     "CMSG PSL"          /* TODO: logo real del cliente */
#define PRODUCT_NAME    "AYSAFI  -  Ingenieria y Tecnologia"

/* ===================== OTA (GitHub Releases pull) ============== */
/* Un tag vX.Y.Z sobre main -> Release "latest" con firmware.bin + version.txt +
 * firmware.sha256.  Particion min_spiffs.csv ya es dual-OTA (app0/app1 1.875 MB). */
#define OTA_GH_OWNER    "asdrubalfuentes"
#define OTA_GH_REPO     "miHMI"

/* ============================ Pantalla =========================== */
/* (los pines TFT se pasan por build_flags a TFT_eSPI; aqui solo resolucion util) */
#define SCREEN_W        320
#define SCREEN_H        240
#define TFT_ROTATION    1        /* apaisado */
#define PIN_TFT_BL      21       /* backlight (PWM, activo ALTO) */
#define DRAW_BUF_LINES  32       /* alto del buffer parcial de LVGL (DRAM: 320*n*2 bytes) */

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

/* ===================== microSD (config + logs) ================== */
/* Ranura TF de la CYD: bus SPI propio, separado del de pantalla/tactil. */
#define PIN_SD_CS       5
#define PIN_SD_SCK      18
#define PIN_SD_MISO     19
#define PIN_SD_MOSI     23
#define SD_SPI_HOST     HSPI    /* si la SD no monta, probar VSPI */
#define HMI_CFG_PATH    "/hmi_config.json"

/* Reinicios anormales (panic/watchdog/brownout) seguidos que se toleran antes
 * de arrancar en MODO BASICO: sin microSD ni ajustes guardados, solo defaults. */
#define BOOT_MAX_FAILS  3
#define BOOT_STABLE_MS  15000   /* uptime sin caer -> se da el arranque por bueno */

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
#define HIST_MONTHS     6       /* barras del grafico "ultimos meses" */

/* ====== Fase 2: enlace de campo por Modbus TCP (MAPA B del contrato) ======= */
/* Contrato: ../ORCHESTRATION/REGISTER_MAP.md   ·   offsets en src/data/map_b.h */
#define CONTRACT_VERSION     2
#define MAP_B_WORD_HI_FIRST  1        /* 32b: palabra alta en la dir. menor (contrato Sec. 2) */

/* Credenciales WiFi: fuera del control de versiones.  Copia
 * include/secrets.example.h a include/secrets.h y pon las de tu planta.
 * En runtime, la microSD (HMI_CFG_PATH) tiene prioridad sobre estos valores. */
#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif
#ifndef WIFI_SSID
#  define WIFI_SSID          ""       /* vacio = no conecta (queda la fuente de respaldo) */
#endif
#ifndef WIFI_PASS
#  define WIFI_PASS          ""
#endif

#define PLC_HOST             "192.168.1.56"  /* IP del LOGO! 9 real o del PLC-SIM (por defecto; la SD manda) */
#define PLC_PORT             503      /* LOGO! 9 de planta hoy en :503 (por defecto; la SD manda) */
#define PLC_UNIT             1        /* Unit ID (el servidor responde con cualquiera) */
#define MB_POLL_MS           1000     /* periodo de sondeo del Mapa B (por defecto; la SD manda) */

#define HMI_ADMIN_PIN_DEFAULT "1234"  /* PIN de administrador de fabrica; cambialo en Configuracion */

#define MB_LEVEL_SCALE       100.0f   /* Mapa B: nivel y caudal viajan x100 */
#define MB_FLOW_SCALE        100.0f
#define MB_ACCUM_SCALE       1000.0f  /* acumulados en m3 x1000 (cambio de rumbo 2026-09:
                                        * ya vienen del nodo en esa escala, sin conversion en el LOGO!) */

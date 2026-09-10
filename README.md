# miHMI — HMI de captación de pozos

HMI minimalista sobre **Cheap Yellow Display** (ESP32-2432S028R, ILI9341 320×240 +
XPT2046, LVGL 8.4). Muestra **2 estaciones de bombeo** y publica/monitorea sus
variables. Es **cliente Modbus TCP** del **MAPA B** del contrato
[`../ORCHESTRATION/REGISTER_MAP.md`](../ORCHESTRATION/REGISTER_MAP.md) — habla con
el **LOGO! 9** real o con el **PLC-SIM** (`../modbusMaster`).

## Arquitectura de datos

```
LOGO! 9 / PLC-SIM  ──(Modbus TCP :502, MAPA B)──►  miHMI
```

- `src/data/data_source.h` — interfaz `DataSource` (begin/poll/sendCommand/health).
- `src/data/data_hub.{h,cpp}` — fuente activa con **failover** primaria→respaldo y
  cola de comandos.
- `src/data/modbus_tcp_source.{h,cpp}` — **primaria**: WiFi STA + cliente Modbus
  TCP asíncrono (`emelianov/modbus-esp8266`). Lee HR `s*32+0..14` por estación
  (incluye `hb+14` alarmas latcheadas) y el bloque global `HR 96..105` (contrato
  v2). Destino/credenciales desde `hmicfg`, no de `#define`.
- `src/data/mock_source.{h,cpp}` — **respaldo / desarrollo sin PLC**.
- `src/data/map_b.h` — offsets del MAPA B (espejo del contrato).

`main.cpp`: `setPrimary(&g_plc); setBackup(&g_mock)`. Si la WiFi o el PLC no
responden, el HUD cae al simulado automáticamente.

## Configuración: microSD → NVS → defaults

`src/data/hmi_config.{h,cpp}` carga la configuración de operación con esta
prioridad:

1. **microSD** `/hmi_config.json` (ranura TF de la CYD, bus SPI propio en
   `SD_SPI_HOST`). Se siembra sola en el primer arranque si hay tarjeta.
2. **NVS** (respaldo; se actualiza cada vez que se guarda).
3. **Valores por defecto** de `include/config.h` + `include/secrets.h`.

Contiene: SSID/clave WiFi de planta, host/puerto/unit/sondeo del PLC, nombres de
estación, **PIN de administrador** (solo su hash) y tema (reservado).

Se edita en **Ajustes → Configuración** (tras el **PIN**, de fábrica `1234`).
GUARDAR escribe microSD + NVS; los cambios de red se aplican **al reiniciar**.

### `include/secrets.h` (fuera de git)

Credenciales WiFi de fábrica/arranque. Copia `include/secrets.example.h` a
`include/secrets.h` y pon las de tu planta (`secrets.h` está en `.gitignore`).
La microSD tiene prioridad sobre estos valores en operación.

### `include/config.h` (defaults y constantes fijas)

| Símbolo | Uso |
|---|---|
| `PLC_HOST` / `PLC_PORT` / `PLC_UNIT` / `MB_POLL_MS` | **por defecto** (la microSD/NVS mandan en runtime) |
| `PIN_SD_*` / `SD_SPI_HOST` / `HMI_CFG_PATH` | ranura microSD |
| `HMI_ADMIN_PIN_DEFAULT` | PIN de administrador de fábrica (`1234`) |
| `OTA_GH_OWNER` / `OTA_GH_REPO` | repo del canal OTA (GitHub Releases) |
| `BOOT_MAX_FAILS` / `BOOT_STABLE_MS` | reinicios anormales tolerados antes del **MODO BÁSICO** (`panic_screen`) |
| `NUM_WELLS` | 2 estaciones |
| `MAP_B_WORD_HI_FIRST` | orden de palabra de los 32 bits (contrato §2) |
| `DRAW_BUF_LINES` | alto del buffer parcial de LVGL (DRAM: `320·n·2` bytes) |

### Módulos de plataforma (`src/hal`, `src/net`)

- `hal/light_ctrl` — LDR (GPIO34) → brillo del backlight + escena (cerrado /
  noche / día / día fuerte). Calibración de 2 puntos + umbrales en % de luz.
  Parámetros por consola serie (`light …`), persistidos en `hmicfg.light`.
- `hal/panic_screen` — cuenta reinicios anormales (panic / watchdog / brownout);
  `BOOT_MAX_FAILS` seguidos → arranca en **MODO BÁSICO** (sin microSD ni NVS,
  solo defaults). Muestra a pantalla completa la etapa donde murió el arranque.
- `hal/net_clock` — SNTP: sincroniza la hora cuando la WiFi está arriba
  (`hmicfg.ntpServer` / `hmicfg.tz`, comando serie `time …`).
- `ui/theme` — paleta AYSAFI clara / oscura; modo `THEME_AUTO` (por luz
  ambiente) / `THEME_LIGHT` / `THEME_DARK`, persistido en `hmicfg.theme`.
- `net/ota_update` + `net/ota_hmi` — OTA (ver más abajo).

### Pantallas

- **Estaciones** (`screen_wells`): 2 tarjetas con margen superior (cabecera de
  46 px con origen SIM/LOGO + latido + alarma global). Cada tarjeta: nivel,
  caudal, presión/sirena/tapa, RSSI, estado (OK / ALARMA / SIN ENLACE).
- **Detalle** (`screen_well`): arco de nivel, caudal, acumulado del día, primera
  alarma activa (o latcheada → estado **PENDIENTE ACK**); botones **SILENCIAR**,
  **ACCIONES** y lista/histórico.
- **Acciones** (`screen_actions`, desde *Detalle*): mando por estación —
  **SILENCIAR**, **RECONOCER ALARMAS** (`cb+5`, limpia `hb+14`), **SIRENA
  AUTO/MANUAL**, **RESET DÍA / MES** (con confirmación).
- **Rangos de escala** (`screen_scale`, desde *Ajustes*): edita el bloque
  `hb+20..31` del MAPA B por estación y variable (cero/span crudo ↔ ingeniería,
  unidad, filtro) con teclado numérico, y lo **APLICA** al PLC (escribe los
  registros + pulsa el coil `cb+8`). La escala vive en el PLC; esto es el editor.
- **PIN** (`screen_pin`): puerta de administrador (teclado 0-9). 3 fallos → vuelve
  a *Ajustes*. Compara contra un hash; el PIN nunca se guarda en claro.
- **Configuración** (`screen_config`, desde *Ajustes* tras el PIN): SSID/clave
  WiFi, host/puerto/unit/sondeo del PLC, nombres de estación, tema, PIN nuevo, y
  **Buscar actualización** (OTA). **GUARDAR** persiste en microSD + NVS; ofrece
  reiniciar para aplicar la red.

`CmdType` = superficie real del MAPA B: `SirenOn/Off`, `SirenAuto/Manual`,
`Silence`, `ResetDay`, `ResetMonth`, `AckAlarms`. `WellData` refleja el MAPA B
(presostato, voltaje local, tamper, sirena, `alarms` + `alarmsLatched` bitfields,
enlace, RSSI…).

## OTA (actualización por WiFi)

Módulo `src/net/ota_update.{h,cpp}` (común, de `../ORCHESTRATION/tools/ota/`) +
`src/net/ota_hmi.{h,cpp}` — modelo "GitHub Releases pull"
([`../ORCHESTRATION/OTA_ROLLOUT.md`](../ORCHESTRATION/OTA_ROLLOUT.md)).

- **Publicar:** `git tag vX.Y.Z` sobre `main` → el workflow
  `.github/workflows/release.yml` compila con `-D FW_VERSION_OVERRIDE=X.Y.Z` y
  publica un Release `latest` (`firmware.bin` + `version.txt` + `firmware.sha256`).
  Sin secrets: la WiFi de planta vive en microSD/NVS.
- **Aplicar:** botón *Buscar actualización* en *Configuración* (tras el PIN), o el
  chequeo silencioso cada 6 h que solo interrumpe si hay versión nueva. Descarga,
  verifica el SHA-256 mientras escribe la partición OTA libre y reinicia. El
  progreso se pinta con TFT_eSPI a pantalla completa.
- `min_spiffs.csv` ya es **dual-OTA** (app0/app1 de 1.875 MB); no se toca. El
  primer equipo se flashea por USB con esta versión; a partir de ahí, OTA.

## Compilar

```bash
pio run -e esp32-2432S028R
pio run -e esp32-2432S028R -t upload
```

## Probar sin PLC real

1. Levanta el **PLC-SIM**: en `../modbusMaster`, `python app.py`, pestaña
   *PLC-SIM*, apunta al gateway (`nodeIO_master`), inicia. Anota su IP:puerto.
2. Pon esa IP en `PLC_HOST` (y `502`, o el puerto que uses) y tu WiFi en
   `WIFI_SSID`/`WIFI_PASS`. Recompila y flashea.
3. En *Ajustes / Diagnóstico* la fuente activa debe pasar a `PLC-TCP`.

# Changelog — miHMI

Versión del canal OTA: `APP_VERSION` (`config.h`), formato `MAJOR.MINOR.PATCH`.
El CI la sobreescribe desde el tag `vX.Y.Z` (`FW_VERSION_OVERRIDE`).

## 0.5.0 — cambio de rumbo: escalado en el nodo, no en el HMI

- Se elimina la página de rangos (`screen_scale`) por completo — la
  calibración de escala vive ahora en el portal del `nodeIO` remoto.
  El HMI ya no configura nada de eso; el nivel se muestra siempre en
  metros, tal cual llega ya escalado.
- Histórico de **6 meses** (`HIST_MONTHS`), además de los 14 días que
  ya existían — ambos persistidos en `/hist.json` (microSD), separado
  de la configuración. `hist_log` detecta el cierre de día/mes con la
  hora local del propio HMI y apila el valor de cierre correcto (antes
  de que el nodo/PLC lo reseteen).
- `screen_history`: botón para alternar el gráfico entre días y meses.
- `MB_ACCUM_SCALE` ×10 → ×1000: los acumulados ya vienen del nodo (vía
  gateway + LOGO!) en esa escala, sin conversión intermedia.

## 0.4.1 — respaldo de calibración de escala

- El LOGO! 9 no retiene el bloque de escala (`hb+20..31`) tras un reinicio (sin
  memoria remanente configurada para eso, ver `ORCHESTRATION/PLC_LOGIC.md`) —
  vuelve a `0` y el escalado queda en `SCALE_BAD` hasta recalibrar a mano.
- `hmi_config` (`ScaleCache`, microSD + NVS) ahora guarda la última calibración
  vista por estación. `modbus_tcp_source::applyStation()` detecta `rawMax` de
  nivel en `0` y, si hay algo cacheado, **reaplica solo** la última calibración
  conocida (`applyScale()`) — un intento por pérdida, se rearma cuando vuelve a
  verse un valor real.
- No sustituye la retentividad real para los acumulados del totalizador
  (día/mes): esos si se pierden con el PLC, se pierden — la calibración de
  escala rara vez cambia, así que reaplicar "lo último visto" siempre es
  correcto; el consumo acumulado no.

## 0.4.0 — OTA vía GitHub Releases

- **OTA "GitHub Releases pull"** (`src/net/ota_update.{h,cpp}` — módulo común de
  `../ORCHESTRATION/tools/ota/` — + `src/net/ota_hmi.{h,cpp}`): consulta
  `releases/latest/download/version.txt`; si hay versión nueva descarga
  `firmware.bin`, verifica el **SHA-256** contra `firmware.sha256` mientras
  escribe la partición OTA libre, y reinicia.
- **Cuándo:** botón **"Buscar actualización"** en *Ajustes → Configuración*
  (tras el PIN), y un chequeo **silencioso cada 6 h** con la WiFi conectada que
  solo toma la pantalla si hay versión nueva.
- El progreso se pinta directo con TFT_eSPI (pantalla completa); no compite con
  LVGL. `ota_hmi` corre desde `loop()`, no desde un manejador de eventos.
- `platformio.ini`: `${sysenv.EXTRA_BUILD_FLAGS}` en `build_flags`; se mantiene
  `min_spiffs.csv` (ya es **dual-OTA**: app0/app1 de 1.875 MB).
- `.github/workflows/release.yml`: tag `vX.Y.Z` sobre `main` → Release `latest`
  con los 3 assets. **Sin secrets**: la WiFi de planta vive en microSD/NVS, así
  que el binario de CI compila con credenciales vacías y las lee en runtime.
- `config.h`: `APP_VERSION` pasa a ser la versión del canal OTA;
  `OTA_GH_OWNER`/`OTA_GH_REPO`.

> `min_spiffs.csv` ya trae `app1`; no hace falta reflasheo destructivo. El primer
> equipo sí necesita un flasheo por USB con esta versión; a partir de ahí, OTA.

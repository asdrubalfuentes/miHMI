# Changelog — miHMI

Versión del canal OTA: `APP_VERSION` (`config.h`), formato `MAJOR.MINOR.PATCH`.
El CI la sobreescribe desde el tag `vX.Y.Z` (`FW_VERSION_OVERRIDE`).

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

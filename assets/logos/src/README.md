# Logos de origen

Archivos vectoriales originales:

- `LOGO AYSAFI - Horizontal.svg` -> marca del producto (splash, centro)
- `Logo_minera.svg`              -> empresa cliente: Minera San Geronimo (splash, arriba)

## Regenerar los recursos LVGL

Requiere: `pip install pymupdf pillow`

```
python tools/img2lvgl.py "assets/logos/src/LOGO AYSAFI - Horizontal.svg" --name logo_aysafi  --max-w 250 --max-h 120 --bg 0xFFFFFF
python tools/img2lvgl.py "assets/logos/src/Logo_minera.svg"              --name logo_cliente --max-w 236 --max-h 44  --bg 0xFFFFFF
```

Genera `src/assets/logo_aysafi.{c,h}` y `src/assets/logo_cliente.{c,h}`
(formato TRUE_COLOR 16 bits, compuestos sobre fondo blanco = fondo del splash).
`src/ui/screen_splash.cpp` los detecta con `__has_include`; si no existen,
usa marcadores de texto y el proyecto compila igual.

Notas:
- `img2lvgl.py` rasteriza el SVG con PyMuPDF e inyecta el CSS `<style>` en
  atributos de presentacion (MuPDF ignora las clases `.clsN{...}`).
- El splash tiene fondo BLANCO porque ambos logos estan disenados para claro.

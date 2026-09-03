# src/assets

Recursos LVGL generados (imagenes convertidas a C-array).

- `logo_aysafi.{c,h}`  -> generado por `tools/img2lvgl.py` a partir de
  `assets/logos/src/aysafi.png`. **No editar a mano.**
- `logo_cliente.{c,h}` -> idem, logo de la empresa cliente.

`src/ui/screen_splash.cpp` los incluye de forma condicional
(`__has_include("assets/logo_aysafi.h")`). Si el .h no existe todavia,
el splash cae a un marcador de texto y el proyecto compila igual.

#!/usr/bin/env python3
"""
img2lvgl.py  -  Convierte una imagen (PNG o SVG) a un recurso LVGL 8.x.

  - Entrada .svg  -> se rasteriza con svglib (pip install svglib)
  - Entrada .png  -> se lee con Pillow (pip install pillow)

Formato de salida (color 16 bits, LV_COLOR_16_SWAP = 0):
  - por defecto : LV_IMG_CF_TRUE_COLOR         (2 bytes/pixel, sin alpha)
  - con --alpha : LV_IMG_CF_TRUE_COLOR_ALPHA   (3 bytes/pixel)

Uso:
  python tools/img2lvgl.py assets/logos/src/aysafi.svg \
      --name logo_aysafi --max-w 240 --max-h 120 --bg 0x0B1418 --out src/assets

Genera  src/assets/<name>.c  y  src/assets/<name>.h
"""
import argparse, os, re, sys, tempfile


def _inline_css(svg_text):
    """MuPDF ignora bloques <style> con clases .clsN{...}; los pasamos a
    atributos de presentacion (fill=, stroke=, ...) en cada elemento."""
    m = re.search(r"<style[^>]*>(.*?)</style>", svg_text, re.S)
    if not m:
        return svg_text
    rules = {}
    for sel, body in re.findall(r"\.([A-Za-z0-9_-]+)\s*\{([^}]*)\}", m.group(1)):
        props = {}
        for decl in body.split(";"):
            if ":" in decl:
                k, v = decl.split(":", 1)
                props[k.strip()] = v.strip()
        rules[sel] = props

    def repl(mo):
        merged = {}
        for c in mo.group(1).split():
            merged.update(rules.get(c, {}))
        if not merged:
            return mo.group(0)
        attrs = " ".join('%s="%s"' % kv for kv in merged.items())
        return '%s class="%s"' % (attrs, mo.group(1))

    return re.sub(r'class="([^"]+)"', repl, svg_text)


def load_image(path, max_w, max_h, bg_rgb):
    from PIL import Image
    ext = os.path.splitext(path)[1].lower()

    if ext == ".svg":
        import pymupdf
        with open(path, "r", encoding="utf-8") as fh:
            svg_text = _inline_css(fh.read())
        tmp = tempfile.NamedTemporaryFile("w", suffix=".svg", delete=False, encoding="utf-8")
        tmp.write(svg_text)
        tmp.close()
        doc = pymupdf.open(tmp.name)
        page = doc[0]
        zoom = min(max_w / page.rect.width, max_h / page.rect.height)
        pix = page.get_pixmap(matrix=pymupdf.Matrix(zoom, zoom), alpha=True)
        img = Image.frombytes("RGBA", (pix.width, pix.height), pix.samples)
    else:
        img = Image.open(path).convert("RGBA")
        img.thumbnail((max_w, max_h), Image.LANCZOS)

    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("--name", default="logo")
    ap.add_argument("--max-w", type=int, default=260)
    ap.add_argument("--max-h", type=int, default=170)
    ap.add_argument("--bg", default="0x0B1418", help="color de fondo al rasterizar SVG / componer alpha")
    ap.add_argument("--alpha", action="store_true", help="conservar canal alpha (TRUE_COLOR_ALPHA)")
    ap.add_argument("--out", default="src/assets")
    args = ap.parse_args()

    bg = int(args.bg, 0)
    bg_rgb = ((bg >> 16) & 0xFF, (bg >> 8) & 0xFF, bg & 0xFF)

    img = load_image(args.src, args.max_w, args.max_h, bg_rgb)
    w, h = img.size
    px = img.load()
    name = args.name

    rows = []
    for y in range(h):
        vals = []
        for x in range(w):
            r, g, b, a = px[x, y]
            if not args.alpha and a < 255:      # componer sobre el fondo
                r = (r * a + bg_rgb[0] * (255 - a)) // 255
                g = (g * a + bg_rgb[1] * (255 - a)) // 255
                b = (b * a + bg_rgb[2] * (255 - a)) // 255
            c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            vals += [c & 0xFF, (c >> 8) & 0xFF]     # LSB, MSB (swap=0)
            if args.alpha:
                vals.append(a)
        rows.append(", ".join("0x%02x" % v for v in vals) + ",")
    data = "\n    ".join(rows)

    cf = "LV_IMG_CF_TRUE_COLOR_ALPHA" if args.alpha else "LV_IMG_CF_TRUE_COLOR"
    px_expr = f"{w * h} * LV_IMG_PX_SIZE_ALPHA_BYTE" if args.alpha else f"{w * h} * 2"

    os.makedirs(args.out, exist_ok=True)
    c_path = os.path.join(args.out, name + ".c")
    h_path = os.path.join(args.out, name + ".h")

    with open(c_path, "w", encoding="utf-8") as f:
        f.write(f"""/* Generado por tools/img2lvgl.py  ({w}x{h}, {cf}) - NO EDITAR */
#include "lvgl.h"

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST
uint8_t {name}_map[] = {{
    {data}
}};

const lv_img_dsc_t {name} = {{
    .header.cf = {cf},
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = {w},
    .header.h = {h},
    .data_size = {px_expr},
    .data = {name}_map,
}};
""")

    with open(h_path, "w", encoding="utf-8") as f:
        f.write(f'/* Generado por tools/img2lvgl.py */\n#pragma once\n#include "lvgl.h"\nextern const lv_img_dsc_t {name};\n')

    kb = os.path.getsize(c_path) / 1024.0
    print(f"OK  {c_path}  ({w}x{h})   {h_path}   [~{kb:.0f} KB de fuente]")


if __name__ == "__main__":
    main()

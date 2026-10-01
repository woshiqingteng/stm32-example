#!/usr/bin/env python3
"""Generate the ALIENTEK-style SPI-NOR image BINs for lvgl_40_img_lib.

The official `PICTURE/LVGLBIN/*.BIN` sources are not shipped, so this tool
converts pictures from the SD `PICTURE` folder into the format the port's
lv_load_img() expects:

    offset 0 : 4-byte little-endian header = cf | (w << 8) | (h << 20)
               cf = 4 (LV_IMG_CF_TRUE_COLOR, RGB565), w/h 12 bits each
    offset 4 : w * h RGB565 little-endian pixels

Usage:
    python tool/lvgl_img2bin.py <src_picture_dir> <out_lvglbin_dir>
    python tool/lvgl_img2bin.py --comprehensive <out_lvglbin_dir>

Default sources (relative to <src_picture_dir>):
    BMP/camera.bmp    -> atk05.BIN
    BMP/ALIENTEKLOGO.bmp -> atk06.BIN
    PNG/mlljt.png     -> atk07.BIN
    PNG/yifu.png      -> money.BIN

--comprehensive generates the 8 launcher icons used by lvgl_53_comprehensive.
Their official sources are not shipped, so substitutes are drawn (the launcher
recolours them to white anyway).  Sizes match the reference app_image[] table.
"""

import os
import struct
import sys

from PIL import Image

CF_TRUE_COLOR = 4
MAX_SIDE = 96

MAP = [
    ("BMP/camera.bmp", "atk05.BIN"),
    ("BMP/ALIENTEKLOGO.bmp", "atk06.BIN"),
    ("PNG/mlljt.png", "atk07.BIN"),
    ("PNG/yifu.png", "money.BIN"),
]


def to_bin(path, out_path):
    im = Image.open(path).convert("RGB")
    im.thumbnail((MAX_SIDE, MAX_SIDE), Image.LANCZOS)
    w, h = im.size
    px = im.load()

    header = (CF_TRUE_COLOR & 0xFF) | ((w & 0xFFF) << 8) | ((h & 0xFFF) << 20)

    buf = bytearray(struct.pack("<I", header))
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            buf += struct.pack("<H", c)

    with open(out_path, "wb") as f:
        f.write(buf)
    print(
        "%-24s -> %-12s %dx%d  %d bytes"
        % (os.path.relpath(path), os.path.basename(out_path), w, h, len(buf))
    )


COMPREHENSIVE = [
    ("Calculator.bin", 146, 140),
    ("File.bin", 146, 140),
    ("lv_system.bin", 146, 140),
    ("Setting.bin", 146, 140),
    ("Test.bin", 146, 140),
    ("Timer.bin", 304, 140),
    ("lv_qr.bin", 146, 140),
    ("lv_draw.bin", 304, 140),
]


def gen_icon(w, h, out_path):
    """Draw a simple substitute icon (rounded square + corner marker)."""
    im = Image.new("RGB", (w, h), (40, 60, 120))
    px = im.load()
    m = max(4, w // 8)
    for y in range(h):
        for x in range(w):
            if x < m or y < m or x >= w - m or y >= h - m:
                px[x, y] = (220, 220, 220)
            elif (x - w // 2) ** 2 + (y - h // 2) ** 2 < (min(w, h) // 3) ** 2:
                px[x, y] = (255, 255, 255)
    _write_bin(im, out_path)


def _write_bin(im, out_path):
    w, h = im.size
    px = im.load()
    header = (CF_TRUE_COLOR & 0xFF) | ((w & 0xFFF) << 8) | ((h & 0xFFF) << 20)
    buf = bytearray(struct.pack("<I", header))
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            buf += struct.pack("<H", c)
    with open(out_path, "wb") as f:
        f.write(buf)
    print("%-16s %dx%d  %d bytes" % (os.path.basename(out_path), w, h, len(buf)))


def main():
    args = sys.argv[1:]
    if args and args[0] == "--comprehensive":
        if len(args) != 2:
            print(__doc__)
            return 1
        out = args[1]
        os.makedirs(out, exist_ok=True)
        for name, w, h in COMPREHENSIVE:
            gen_icon(w, h, os.path.join(out, name))
        return 0
    if len(args) != 2:
        print(__doc__)
        return 1
    src, out = args
    os.makedirs(out, exist_ok=True)
    for rel, name in MAP:
        p = os.path.join(src, rel)
        if not os.path.exists(p):
            print("missing:", p, file=sys.stderr)
            return 2
        to_bin(p, os.path.join(out, name))
    return 0


if __name__ == "__main__":
    sys.exit(main())

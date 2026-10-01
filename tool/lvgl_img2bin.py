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

Default sources (relative to <src_picture_dir>):
    BMP/camera.bmp    -> atk05.BIN
    BMP/ALIENTEKLOGO.bmp -> atk06.BIN
    PNG/mlljt.png     -> atk07.BIN
    PNG/yifu.png      -> money.BIN
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


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    src, out = sys.argv[1], sys.argv[2]
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

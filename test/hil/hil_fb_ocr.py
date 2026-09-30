#!/usr/bin/env python3
"""OCR for the RGB panel framebuffer of the lwIP demos.

The panel is used in PORTRAIT: logical (x, y) maps to
    raster row = (pheight - 1 - x), col = y     (stride = pwidth)
with pwidth=800, pheight=480, 16 bpp RGB565.

ASCII glyphs come from bsp/openedv_stm32f4/lcdfont.h (asc2_1206/1608/2412/3216),
column-major MSB-first (see lcd.c glyph_bit()).
"""

import re
import sys
import struct

PWIDTH, PHEIGHT = 800, 480
FB_LEN = PWIDTH * PHEIGHT * 2

# size -> bytes per glyph (from lcd.c)
BYTES = {12: 12, 16: 16, 24: 36, 32: 64}


def parse_fonts(header_path):
    txt = open(header_path, "rb").read().decode("gbk", "ignore")
    fonts = {}
    for size, n in BYTES.items():
        m = re.search(
            r"asc2_%04d\[95\]\[%d\]\s*=\s*\{(.*?)\};"
            % (size * 100 + 8 if False else 0, n),
            txt,
            re.S,
        )
        # fall back to a name-based search
        m = re.search(r"asc2_[0-9]{4}\[95\]\[%d\]\s*=\s*\{(.*?)\};" % n, txt, re.S)
    # explicit names
    names = {12: "asc2_1206", 16: "asc2_1608", 24: "asc2_2412", 32: "asc2_3216"}
    for size, name in names.items():
        m = re.search(
            re.escape(name) + r"\[95\]\[%d\]\s*=\s*\{(.*?)\};" % BYTES[size], txt, re.S
        )
        if not m:
            raise SystemExit("font %s not found" % name)
        vals = [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))]
        need = 95 * BYTES[size]
        if len(vals) < need:
            raise SystemExit("font %s: got %d bytes, need %d" % (name, len(vals), need))
        fonts[size] = vals[:need]
    return fonts


def glyph_bits(data, index, size):
    """Return {(col,row): bit} for glyph `index` (0..94 == ' '..'~')."""
    nbytes = BYTES[size]
    bytes_per_col = size // 8
    cols = size // 2
    base = index * nbytes
    bits = {}
    for c in range(cols):
        for r in range(size):
            b = data[base + c * bytes_per_col + (r // 8)]
            bits[(c, r)] = (b >> (7 - (r % 8))) & 1
    return bits


def px(fb, x, y):
    if x < 0 or y < 0 or x >= PHEIGHT or y >= PWIDTH:
        return 0xFFFF
    row = (PHEIGHT - 1) - x
    col = y
    i = 2 * (PWIDTH * row + col)
    return fb[i] | (fb[i + 1] << 8)


def decode_char(fb, fonts, x0, y0, size, color):
    data = fonts[size]
    cols = size // 2
    ink = {}
    for c in range(cols):
        for r in range(size):
            ink[(c, r)] = 1 if px(fb, x0 + c, y0 + r) == color else 0
    best, best_d = -1, 1 << 30
    for gi in range(95):
        bits = glyph_bits(data, gi, size)
        d = 0
        for k in ink:
            if ink[k] != bits[k]:
                d += 1
                if d >= best_d:
                    break
        if d < best_d:
            best_d, best = d, gi
    return chr(0x20 + best) if best >= 0 else "?"


def decode_line(fb, fonts, x0, y0, size, maxlen, color):
    out = []
    for i in range(maxlen):
        x = x0 + i * (size // 2)
        ch = decode_char(fb, fonts, x, y0, size, color)
        out.append(ch)
        if ch == "\x7f":
            break
    return "".join(out)


# known colours (RGB565) used by the demos
WHITE = 0xFFFF
MAGENTA = 0xF81F
BLUE = 0x001F
RED = 0xF800
DARKBLUE = 0x01CF


if __name__ == "__main__":
    fb_path = sys.argv[1]
    hdr = sys.argv[2] if len(sys.argv) > 2 else "bsp/openedv_stm32f4/lcdfont.h"
    fb = open(fb_path, "rb").read()
    if len(fb) < FB_LEN:
        print("short framebuffer: %d" % len(fb))
    fonts = parse_fonts(hdr)
    # lines laid out by lwip_demo_ui: (x0,y0,size,color)
    lines = [
        ("title24", 6, 40, 24, 32, DARKBLUE),
        ("stm32", 6, 10, 32, 5, DARKBLUE),
        ("atom", 6, 70, 16, 13, DARKBLUE),
        ("init", 5, 110, 16, 19, MAGENTA),
        ("ip", 5, 130, 16, 20, MAGENTA),
        ("speed", 5, 150, 16, 19, MAGENTA),
        ("key", 5, 170, 16, 15, MAGENTA),
        ("rxlabel", 5, 190, 16, 13, BLUE),
        ("rx", 30, 230, 16, 40, RED),
    ]
    for name, x0, y0, size, ml, color in lines:
        print("%-8s |%s|" % (name, decode_line(fb, fonts, x0, y0, size, ml, color)))

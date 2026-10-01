#!/usr/bin/env python3
"""Convert LvglFontTool C fonts (lv_font_fmt_txt, SPARSE_TINY) to LVGL XBF.

The ALIENTEK LVGL port loads its CJK fonts from SPI-NOR using the "XBF"
layout read by the generated `FontXX.c` descriptor (see
`app/freertos/lvgl_07_xbf_font/GUI_FONT/Font12.c`):

    header  : { uint16 min; uint16 max; uint8 bpp; uint8 reserved[3]; }
    table   : (max-min+1) * uint32 offset      (0 = glyph absent)
    block   : { uint8 adv_w; uint8 box_w; uint8 box_h; int8 ofs_y; }
              followed by box_w*box_h*bpp/8 bitmap bytes

`adv_w` is in pixels; the source `lv_font_fmt_txt_glyph_dsc_t.adv_w` is 8.4,
so it is rounded with `(adv_w + 8) >> 4` (matching lv_font_fmt_txt.c).  `box_w`
is padded up to a multiple of `8 / bpp`, which keeps each bitmap row the same
byte count as LVGL's own `(box_w*bpp+7)/8` packing, so the bytes are copied
verbatim.  `ofs_x` is not representable in XBF and is forced to 0 (a warning is
printed if any glyph needs it).

Multiple source files may be given; they are merged into the union of their
codepoints (glyphs shared by several files must be identical).
"""

import argparse
import re
import sys


def _parse(path):
    text = open(path, "r", encoding="utf-8", errors="replace").read()

    m = re.search(r"glyph_bitmap\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if not m:
        raise ValueError("%s: glyph_bitmap[] not found" % path)
    bitmap = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1)))

    m = re.search(r"glyph_dsc\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if not m:
        raise ValueError("%s: glyph_dsc[] not found" % path)
    gdsc = [
        tuple(int(x) for x in g)
        for g in re.findall(
            r"\{\.bitmap_index\s*=\s*(\d+),\s*\.adv_w\s*=\s*(\d+),\s*"
            r"\.box_h\s*=\s*(\d+),\s*\.box_w\s*=\s*(\d+),\s*"
            r"\.ofs_x\s*=\s*(-?\d+),\s*\.ofs_y\s*=\s*(-?\d+)\}",
            m.group(1),
        )
    ]

    m = re.search(r"unicode_list_\d+\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if not m:
        raise ValueError("%s: unicode_list not found" % path)
    unicode_list = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]+)", m.group(1))]

    if "SPARSE_TINY" not in text:
        raise ValueError("%s: only SPARSE_TINY cmaps are supported" % path)
    bpp = int(re.search(r"\.bpp\s*=\s*(\d+)", text).group(1))
    line_height = int(re.search(r"\.line_height\s*=\s*(\d+)", text).group(1))
    base_line = int(re.search(r"\.base_line\s*=\s*(\d+)", text).group(1))

    glyphs = {}
    warn_ofs_x = 0
    for i, uni in enumerate(unicode_list):
        if i >= len(gdsc):
            break
        bmi, adv_w, box_h, box_w, ofs_x, ofs_y = gdsc[i]
        if ofs_x != 0:
            warn_ofs_x += 1
        row = (box_w * bpp + 7) // 8
        data = bitmap[bmi : bmi + box_h * row]
        if len(data) != box_h * row:
            raise ValueError("%s: glyph 0x%04X bitmap out of range" % (path, uni))
        glyphs[uni] = (adv_w, box_w, box_h, ofs_y, data)
    if warn_ofs_x:
        sys.stderr.write(
            "%s: warning: %d glyph(s) use ofs_x; XBF forces it to 0\n"
            % (path, warn_ofs_x)
        )
    return glyphs, bpp, line_height, base_line


def _merge(sources):
    glyphs = {}
    bpp = lh = bl = None
    for path in sources:
        g, b, l, base = _parse(path)
        if bpp is None:
            bpp, lh, bl = b, l, base
        elif (b, l, base) != (bpp, lh, bl):
            raise ValueError("%s: bpp/line_height/base_line mismatch" % path)
        for uni, (adv, bw, bh, oy, data) in g.items():
            if uni in glyphs:
                if glyphs[uni] != (adv, bw, bh, oy, data):
                    raise ValueError("%s: glyph 0x%04X differs" % (path, uni))
                continue
            glyphs[uni] = (adv, bw, bh, oy, data)
    return glyphs, bpp, lh, bl


def build(sources):
    glyphs, bpp, line_height, base_line = _merge(sources)
    pix_per_byte = 8 // bpp
    unis = sorted(glyphs)
    umin, umax = unis[0], unis[-1]
    table_bytes = (umax - umin + 1) * 4
    data_start = 8 + table_bytes

    blocks = []
    table = [0] * (umax - umin + 1)
    max_block = 4
    off = data_start
    for uni in unis:
        adv_w, box_w, box_h, ofs_y, data = glyphs[uni]
        box_w_pad = ((box_w + pix_per_byte - 1) // pix_per_byte) * pix_per_byte
        adv_px = (adv_w + 8) >> 4
        if adv_px > 255:
            raise ValueError("glyph 0x%04X advance %d px > 255" % (uni, adv_px))
        block = bytes([adv_px, box_w_pad, box_h, ofs_y & 0xFF]) + data
        table[uni - umin] = off
        blocks.append(block)
        off += len(block)
        max_block = max(max_block, len(block) - 4)

    out = bytearray()
    out += umin.to_bytes(2, "little")
    out += umax.to_bytes(2, "little")
    out += bytes([bpp, 0, 0, 0])
    for v in table:
        out += v.to_bytes(4, "little")
    for b in blocks:
        out += b
    return bytes(out), bpp, line_height, base_line, umin, umax, max_block


DESC = """/*
 * @file    {name}.c
 * @brief   LVGL XBF font {name} stored in the SPI-NOR font store
 *          (generated by tool/lvgl_font_c2xbf.py - do not edit).
 */

#include "lvgl.h"
#include "nor.h"
#include "text.h"

typedef struct {{
    uint16_t min;
    uint16_t max;
    uint8_t  bpp;
    uint8_t  reserved[3];
}} x_header_t;
typedef struct {{
    uint8_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t  ofs_y;
}} glyph_dsc_t;

static x_header_t __g_xbf_hd = {{0, 0, 0, {{0}}}};
static uint8_t __g_font_buf[{buf}];

static uint8_t *__user_font_getdata(int offset, int size)
{{
    nor_read(__g_font_buf, ftinfo.{field} + (uint32_t)offset, (uint16_t)size);
    return __g_font_buf;
}}

static const uint8_t *__user_font_get_bitmap(const lv_font_t *font, uint32_t unicode_letter)
{{
    (void)font;
    if (__g_xbf_hd.max == 0)
    {{
        memcpy(&__g_xbf_hd, __user_font_getdata(0, sizeof(x_header_t)), sizeof(x_header_t));
    }}
    if (unicode_letter > __g_xbf_hd.max || unicode_letter < __g_xbf_hd.min)
    {{
        return NULL;
    }}
    uint32_t unicode_offset = sizeof(x_header_t) + (unicode_letter - __g_xbf_hd.min) * 4;
    uint32_t pos = *(uint32_t *)__user_font_getdata(unicode_offset, 4);
    if (pos != 0)
    {{
        glyph_dsc_t *gdsc = (glyph_dsc_t *)__user_font_getdata(pos, sizeof(glyph_dsc_t));
        return __user_font_getdata(pos + sizeof(glyph_dsc_t), gdsc->box_w * gdsc->box_h * __g_xbf_hd.bpp / 8);
    }}
    return NULL;
}}

static bool __user_font_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out,
                                      uint32_t unicode_letter, uint32_t unicode_letter_next)
{{
    (void)font;
    (void)unicode_letter_next;
    if (__g_xbf_hd.max == 0)
    {{
        memcpy(&__g_xbf_hd, __user_font_getdata(0, sizeof(x_header_t)), sizeof(x_header_t));
    }}
    if (unicode_letter > __g_xbf_hd.max || unicode_letter < __g_xbf_hd.min)
    {{
        return false;
    }}
    uint32_t unicode_offset = sizeof(x_header_t) + (unicode_letter - __g_xbf_hd.min) * 4;
    uint32_t pos = *(uint32_t *)__user_font_getdata(unicode_offset, 4);
    if (pos != 0)
    {{
        glyph_dsc_t *gdsc = (glyph_dsc_t *)__user_font_getdata(pos, sizeof(glyph_dsc_t));
        dsc_out->adv_w = gdsc->adv_w;
        dsc_out->box_h = gdsc->box_h;
        dsc_out->box_w = gdsc->box_w;
        dsc_out->ofs_x = 0;
        dsc_out->ofs_y = gdsc->ofs_y;
        dsc_out->bpp   = __g_xbf_hd.bpp;
        return true;
    }}
    return false;
}}

lv_font_t {name} = {{
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = {lh},
    .base_line = {bl},
}};
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("sources", nargs="+", help="myFont*.c files to merge")
    ap.add_argument("--name", required=True, help="font symbol, e.g. Font14")
    ap.add_argument("--field", required=True, help="ftinfo field, e.g. lvgl_14addr")
    ap.add_argument("--out", required=True, help="output XBF .bin")
    ap.add_argument("--desc", required=True, help="output descriptor .c")
    args = ap.parse_args()

    data, bpp, lh, bl, umin, umax, max_block = build(args.sources)
    with open(args.out, "wb") as f:
        f.write(data)
    with open(args.desc, "w", encoding="utf-8", newline="\n") as f:
        f.write(
            DESC.format(name=args.name, field=args.field, buf=max_block, lh=lh, bl=bl)
        )
    print(
        "%s: %d bytes, %d bytes glyphs, bpp=%d U+%04X..U+%04X -> %s"
        % (
            args.name,
            len(data),
            len(data) - 8 - (umax - umin + 1) * 4,
            bpp,
            umin,
            umax,
            args.out,
        )
    )


if __name__ == "__main__":
    main()

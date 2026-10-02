#!/usr/bin/env bash
#
# Prepare the SD-card tree used by the LVGL apps.
#
# Usage:
#   tool/lvgl_sd_prepare.sh <alientek_sd_package_dir> <out_dir>
#
# It produces <out_dir>/ containing:
#   PICTURE/LVGLBIN/*.BIN   the SPI-NOR image store (lvgl_40 + comprehensive)
#   SYSTEM/LVFONT/Font14|18|24.BIN   the LVGL XBF fonts (regenerated from the
#                                    myFont sources kept in git history)
#   SYSTEM/... PICTURE/{BMP,PNG,GIF,JPEG}   copied from the ALIENTEK package
#
# Then flash 54_usb_device_msc (exposes NOR/NAND/SD), and copy the tree onto
# the SD card's drive.
set -euo pipefail

PKG="${1:?usage: lvgl_sd_prepare.sh <alientek_sd_package_dir> <out_dir>}"
OUT="${2:?usage: lvgl_sd_prepare.sh <alientek_sd_package_dir> <out_dir>}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$OUT/PICTURE/LVGLBIN" "$OUT/SYSTEM/LVFONT"

# 1. copy the stock SD content (GBK fonts, images, FatFs files)
cp -r "$PKG/SYSTEM/." "$OUT/SYSTEM/"
cp -r "$PKG/PICTURE/." "$OUT/PICTURE/"

# 2. image store BINs (lvgl_40's 4 + the comprehensive's 8 icons)
python "$ROOT/tool/lvgl_img2bin.py" "$PKG/PICTURE" "$OUT/PICTURE/LVGLBIN"
python "$ROOT/tool/lvgl_img2bin.py" --comprehensive "$OUT/PICTURE/LVGLBIN"

# 3. regenerate the XBF fonts from the myFont sources in git history (they were
#    removed from the tree when the fonts were externalised; the deletion
#    commit's parent still has them).
REV="$(git -C "$ROOT" log --format=%H --full-history -1 -- \
    app/freertos/lvgl_14_canvas/GUI_FONT/myFont24.c)^"
git -C "$ROOT" show "$REV:app/freertos/lvgl_06_font/GUI_FONT/myFont14.c"   > "$TMP/06_myFont14.c"
git -C "$ROOT" show "$REV:app/freertos/lvgl_14_canvas/GUI_FONT/myFont14.c" > "$TMP/14_myFont14.c"
git -C "$ROOT" show "$REV:app/freertos/lvgl_51_filemgr/GUI_FONT/myFont24.c" > "$TMP/51_myFont24.c"
git -C "$ROOT" show "$REV:app/freertos/lvgl_51_filemgr/GUI_FONT/myFont18.c" > "$TMP/51_myFont18.c"
git -C "$ROOT" show "$REV:app/freertos/lvgl_14_canvas/GUI_FONT/myFont24.c" > "$TMP/14_myFont24.c"

py="$ROOT/tool/lvgl_font_c2xbf.py"
python "$py" --name Font14 --field lvgl_14addr \
    --out "$OUT/SYSTEM/LVFONT/Font14.BIN" --desc "$TMP/Font14.c" \
    "$TMP/06_myFont14.c" "$TMP/14_myFont14.c" "$TMP/51_myFont24.c"
python "$py" --name Font18 --field lvgl_18addr \
    --out "$OUT/SYSTEM/LVFONT/Font18.BIN" --desc "$TMP/Font18.c" "$TMP/51_myFont18.c"
python "$py" --name Font24 --field lvgl_24addr \
    --out "$OUT/SYSTEM/LVFONT/Font24.BIN" --desc "$TMP/Font24.c" "$TMP/14_myFont24.c"

echo "SD tree ready in $OUT"

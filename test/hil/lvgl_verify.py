#!/usr/bin/env python3
"""HIL verification for the ported LVGL examples (app/freertos/lvgl_*).

Generic checks (all apps): the panel is drawn, no CPU fault occurred and the
LED0 heartbeat keeps toggling.  B-class apps (SD / SPI-flash resources) get
extra assertions.  Touch-driven widgets cannot be tapped from the debugger, so
they are only checked for "draws fine / does not crash".
"""

import glob
import os
import time

import hil_common as hc

ROOT = hc.ROOT


def _nonwhite(env, step=10):
    fb = env.frame()
    n = 0
    for x in range(10, 470, step):
        for y in range(10, 790, step):
            if env.px(fb, x, y) != 0xFFFF:
                n += 1
    return n


def _led_toggles(env, secs=2.6):
    bits = set()
    end = time.time() + secs
    while time.time() < end:
        bits.add((hc.led_odr(env) >> 1) & 1)  # LED0 = PB1
        time.sleep(0.2)
        if len(bits) == 2:
            break
    return len(bits) == 2


def c_draw(env, *a):
    n = _nonwhite(env)
    fault = hc.faulted(env)
    led = _led_toggles(env)
    return (n >= 20) and (not fault) and led, "drawn=%d fault=%s led=%s" % (
        n,
        fault,
        led,
    )


def c_image(env, *a):
    """A static image/font band must contain pixels that differ from the
    background (a solid background alone must not count as content)."""
    fb = env.frame()
    bg = env.px(fb, 2, 2)
    n = 0
    for x in range(40, 440, 12):
        for y in range(40, 740, 12):
            if env.px(fb, x, y) != bg:
                n += 1
    fault = hc.faulted(env)
    return (n >= 20) and (not fault), "content=%d bg=%04x fault=%s" % (n, bg, fault)


def c_gif(env, *a):
    fb1 = env.frame()
    time.sleep(1.0)
    fb2 = env.frame()
    changed = 0
    for x in range(60, 420, 30):
        for y in range(60, 420, 30):
            if env.px(fb1, x, y) != env.px(fb2, x, y):
                changed += 1
    fault = hc.faulted(env)
    return (changed >= 1) and (not fault), "changed=%d fault=%s" % (changed, fault)


CHECK = {
    "lvgl_05_fs": lambda e, *a: (not hc.faulted(e), "fs"),
    "lvgl_41_bmp": c_image,
    "lvgl_42_png": c_image,
    "lvgl_43_gif": c_gif,
    "lvgl_45_jpeg": c_image,
    "lvgl_51_filemgr": c_image,
    "lvgl_07_xbf_font": c_image,
    "lvgl_40_img_lib": c_image,
}

_SER_KW = {
    "lvgl_05_fs": "READ",
}


def default_apps():
    apps = sorted(
        os.path.basename(p) for p in glob.glob(str(ROOT / "app/freertos/lvgl_*"))
    )
    return apps


def ser_kw(app):
    return _SER_KW.get(app)


def main():
    hc.run_apps(
        default_apps(),
        CHECK,
        ser_kw_fn=ser_kw,
        title_required=False,
        report_path="test/hil/lvgl_report.md",
        title="# LVGL port - hardware verification",
        default_check=c_draw,
    )


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""HIL verification for the ported LVGL examples (app/freertos/lvgl_*).

Generic checks (all apps): the panel is drawn, no CPU fault occurred and the
LED0 heartbeat keeps toggling.  B-class apps (SD / SPI-flash resources) get
extra assertions.  Touch-driven widgets cannot be tapped from the debugger, so
they are only checked for "draws fine / does not crash".
"""

import glob
import os
import sys
import time

import hil_common as hc

ROOT = hc.ROOT

BENCH_APP = "lvgl_demo_benchmark"
BENCH_TIMEOUT = 180.0  # 49 scenes x (normal + opa) x SCENE_TIME (1 s) ~= 100 s


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
    for x in range(2, 480, 4):
        for y in range(2, 800, 8):
            if env.px(fb, x, y) != bg:
                n += 1
    fault = hc.faulted(env)
    return (n >= 8) and (not fault), "content=%d bg=%04x fault=%s" % (n, bg, fault)


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


def c_fs(env, *a):
    """lvgl_05_fs: background turns green when the SD file was read."""
    c = env.px(env.frame(), 3, 3)
    fault = hc.faulted(env)
    return (c == 0x07E0) and (not fault), "bg=%04x fault=%s" % (c, fault)


def _fault_peek(page):
    """Non-halting fault check (mdw via the AHB-AP); frame() would halt and
    disturb the measured FPS."""
    cfsr = page.peek(hc.CFSR) & 0xFFFFFFFF
    hfsr = page.peek(hc.HFSR) & 0xFFFFFFFF
    return (cfsr != 0) or (hfsr & 0x40000000) != 0


def _read_serial_until(page, boot_lines, key, timeout):
    lines = list(boot_lines)
    end = time.time() + timeout
    while time.time() < end:
        lines += page.serial_read_lines(1.0)
        if any(key in x for x in lines):
            break
    return lines


def _bench_parse(lines):
    version = None
    weighted = None
    opa = None
    scenes = []
    in_summary = False
    for raw in lines:
        ln = raw.strip()
        if ln.startswith("LVGL v") and "Benchmark" in ln:
            version = ln
        elif ln.startswith("Weighted FPS:"):
            weighted = ln.split(":", 1)[1].strip()
            in_summary = True
        elif ln.startswith("Opa. speed:"):
            opa = ln.split(":", 1)[1].strip()
        elif in_summary and "," in ln:
            name, _, val = ln.rpartition(",")
            if val.strip().isdigit():
                scenes.append((name.strip(), val.strip()))
    return version, weighted, opa, scenes


def _bench_section(version, weighted, opa, scenes):
    out = [
        hc.BENCH_BEGIN,
        "",
        "## Benchmark (`%s`)" % BENCH_APP,
        "",
        "Board: ALIENTEK Apollo STM32F429, 480x800 RGB565 (LTDC), "
        "SYSCLK 180 MHz, LVGL 8.3.11. Run: %s." % time.strftime("%Y-%m-%d"),
    ]
    if version:
        out += ["", "`%s`" % version]
    out += [
        "",
        "- Weighted FPS: **%s**" % weighted,
        "- Opa. speed: **%s**" % opa,
        "",
        "| scene | FPS |",
        "|---|---|",
    ]
    for name, fps in scenes:
        out.append("| %s | %s |" % (name, fps))
    out += ["", hc.BENCH_END]
    return "\n".join(out)


def _tap_collect(x, y):
    """Collect callback: inject a press/release at (x, y) via lv_port's test
    hook, then assert the frame changed (a widget reacted)."""

    def _run(page, _boot_lines):
        env = hc.Env(page)
        before = env.frame()
        en = page.symbol("g_lv_indev_test_en")
        pr = page.symbol("g_lv_indev_test_pr")
        pxx = page.symbol("g_lv_indev_test_x")
        pyy = page.symbol("g_lv_indev_test_y")
        page.poke(pxx, x)
        page.poke(pyy, y)
        page.poke(pr, 1)
        page.poke(en, 1)
        page.sleep(400)
        page.poke(pr, 0)
        page.poke(en, 0)
        time.sleep(0.3)
        after = env.frame()
        n = min(len(before), len(after))
        diff = sum(1 for i in range(0, n, 2) if before[i : i + 2] != after[i : i + 2])
        fault = hc.faulted(env)
        return (
            (diff >= 100) and (not fault),
            "tap(%d,%d) changed=%d fault=%s" % (x, y, diff, fault),
            None,
        )

    return _run


# Touch-driven examples: inject a point that a control reacts to (display is
# 800x480 landscape).  See tool/notes on the widget layout in each app.
TAP = {
    "lvgl_04_mouse": (400, 240),
    "lvgl_21_slider": (300, 240),
    "lvgl_29_keyboard": (40, 450),
    "lvgl_37_tabview": (400, 40),
    "lvgl_47_calculator": (400, 360),
}


def c_benchmark(page, boot_lines):
    """Collect: run until the benchmark prints its summary, record the FPS
    results (never dump the frame buffer while it runs)."""
    lines = _read_serial_until(page, boot_lines, "Weighted FPS:", BENCH_TIMEOUT)
    lines += page.serial_read_lines(2.0)  # flush the per-scene CSV lines
    version, weighted, opa, scenes = _bench_parse(lines)
    fault = _fault_peek(page)
    ok = (weighted is not None) and (opa is not None) and (not fault)
    detail = "weighted=%s opa=%s scenes=%d fault=%s" % (
        weighted,
        opa,
        len(scenes),
        fault,
    )
    return ok, detail, _bench_section(version, weighted, opa, scenes)


CHECK = {
    "lvgl_05_fs": c_fs,
    "lvgl_41_bmp": c_image,
    "lvgl_42_png": c_image,
    "lvgl_43_gif": c_gif,
    "lvgl_45_jpeg": c_image,
    "lvgl_51_filemgr": c_image,
    "lvgl_07_xbf_font": c_image,
    "lvgl_40_img_lib": c_image,
}

_SER_KW = {}


def default_apps():
    apps = sorted(
        os.path.basename(p) for p in glob.glob(str(ROOT / "app/freertos/lvgl_*"))
    )
    # The benchmark runs for ~100 s and is captured on demand (see main()).
    return [a for a in apps if a != BENCH_APP]


def ser_kw(app):
    return _SER_KW.get(app)


REPORT = "test/hil/lvgl_report.md"
TITLE = (
    "# LVGL port - hardware verification\n\n"
    "All LVGL apps run on hardware with `tool/hil.sh lvgl`. The official "
    "benchmark takes ~100 s and is measured on demand with "
    "`tool/hil.sh lvgl benchmark`, which records its FPS results in the "
    "benchmark section below."
)


def main():
    if "benchmark" in sys.argv[1:]:
        # On demand: measure and record the benchmark into lvgl_report.md
        # without touching the app table.
        hc.run_apps(
            [BENCH_APP],
            {},
            collect_map={BENCH_APP: c_benchmark},
            only_sections=True,
            title_required=False,
            report_path=REPORT,
            title=TITLE,
            apps=[BENCH_APP],
        )
        return
    apps = default_apps()
    hc.run_apps(
        apps,
        CHECK,
        ser_kw_fn=ser_kw,
        title_required=False,
        report_path=REPORT,
        title=TITLE,
        default_check=c_draw,
        collect_map={a: _tap_collect(*TAP[a]) for a in apps if a in TAP},
    )


if __name__ == "__main__":
    main()

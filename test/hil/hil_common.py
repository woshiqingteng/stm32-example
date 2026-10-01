#!/usr/bin/env python3
"""Shared HIL helpers for the ported example apps.

Reuses the project HIL framework (`test/page`, `test/config`): one persistent
OpenOCD session for the whole run (flashing, memory access, key injection), so
the CMSIS-DAP probe is initialised only once.

KEY0 = PH3, KEY1 = PH2, WK_UP = PA0 (active high).  LED0 = PB1.
"""

import pathlib
import subprocess
import sys
import time

TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
ROOT = TEST_DIR.parent
sys.path.insert(0, str(TEST_DIR))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from config import setting  # noqa: E402
from page.openedv_stm32f429 import OpenEdvSTM32F429Page  # noqa: E402
import hil_fb_ocr as h  # noqa: E402

FB = "D:/app/msys2/tmp/opencode/fv_fb.bin"
FB_ADDR, FB_LEN = "0xC0000000", "0xBB800"
APP_BIN = "build/debug/openedv_stm32f4/freertos/%s/%s.bin"

# Cortex-M4 fault status registers.
CFSR = 0xE000ED28
HFSR = 0xE000ED2C


def clear_stray():
    subprocess.run(
        ["taskkill", "/F", "/IM", "openocd.exe"], capture_output=True, timeout=10
    )
    time.sleep(0.5)


def build(app):
    return subprocess.run(
        ["bash", "tool/build.sh", "debug", app], cwd=str(ROOT), capture_output=True
    ).returncode


class Env:
    def __init__(self, page):
        self.page = page
        self.fonts = h.parse_fonts(str(ROOT / "bsp/openedv_stm32f4/lcdfont.h"))

    def frame(self):
        self.page.cmd("halt")
        self.page.cmd("dump_image %s %s %s" % (FB, FB_ADDR, FB_LEN), timeout=40)
        self.page.cmd("resume")
        return open(FB, "rb").read()

    def dec(self, fb, x, y, size, ml, color):
        return h.decode_line(fb, self.fonts, x, y, size, ml, color)

    def px(self, fb, x, y):
        return h.px(fb, x, y)

    def title(self):
        fb = self.frame()
        return (
            self.dec(fb, 10, 10, 32, 5, h.RED).strip(),
            self.dec(fb, 10, 47, 24, 18, h.RED).strip(),
        )

    def tap(self, name, hold=180, gap=150):
        self.page.tap(name, hold_ms=hold, gap_ms=gap)


def num3(env, x, y):
    return env.dec(env.frame(), x, y, 16, 3, h.BLUE).strip()


def fill_probe(env, *a):
    before = env.px(env.frame(), 100, 200)
    env.tap("KEY0")
    time.sleep(0.4)
    after = env.px(env.frame(), 100, 200)
    return (before != after), "fill %04x->%04x" % (before, after)


def gpio_odr(env):
    """Full GPIOB->ODR with the GPIOB clock forced on (idle/tickless gate it)."""
    env.page.cmd("halt")
    enr = env.page.peek(0x40023830)
    env.page.poke(0x40023830, enr | 0x00000002)  # RCC AHB1ENR: GPIOBEN
    v = env.page.peek(0x40020414)
    env.page.poke(0x40023830, enr)
    env.page.cmd("resume")
    return v


def led_odr(env):
    """LED0/1 bits of GPIOB->ODR (LED0 = PB1, LED1 = PB0)."""
    return gpio_odr(env) & 0x3


def led_probe(env):
    b = led_odr(env)
    env.tap("KEY1")
    time.sleep(0.3)
    a = led_odr(env)
    return ((b ^ a) & 0x2) != 0, "led0 %x->%x" % (b, a)


def faulted(env):
    """True if a CPU fault (HardFault/MemManage/BusFault/UsageFault) occurred."""
    cfsr = env.page.peek(CFSR) & 0xFFFFFFFF
    hfsr = env.page.peek(HFSR) & 0xFFFFFFFF
    return (cfsr != 0) or (hfsr & 0x40000000) != 0


def run_once(
    page,
    app,
    check=None,
    ser_kw=None,
    serial_set=(),
    title_required=True,
    title_ok_fn=None,
    collect=None,
):
    if build(app) != 0:
        return ("FAIL", "build rc!=0", None)
    binp = APP_BIN % (app, app)
    page.elf = str(ROOT / binp).replace(".bin", ".elf")
    try:
        page.program(str(ROOT / binp))
    except Exception as e:  # noqa: BLE001
        return ("FAIL", "flash: %s" % e, None)
    page.reset_run()
    # Capture serial from the reset so one-shot startup prints (e.g. lvgl_05's
    # "READ(...)") are not missed.
    boot_lines = page.serial_read_lines(2.5)
    if collect is not None:
        # Long-running apps (the benchmark) collect their own results.  Never
        # dump the frame buffer here: frame() halts the core and would skew
        # any timing being measured.
        try:
            ok, detail, section = collect(page, boot_lines)
        except Exception as e:  # noqa: BLE001
            ok, detail, section = False, "exc %s" % e, None
        return ("PASS" if ok else "FAIL", detail, section)
    env = Env(page)
    t, s = env.title()
    detail = "lcd='%s'/'%s'" % (t, s)
    try:
        extra_ok, extra = check(env, t, s) if check else (True, "")
    except Exception as e:  # noqa: BLE001
        extra_ok, extra = False, "exc %s" % e
    lines = boot_lines + page.serial_read_lines(1.0)
    body = [x for x in lines if x and not x.startswith("app_")]
    if ser_kw is not None:
        ser_ok = any(ser_kw in x for x in body)
        extra += " serkw '%s'=%s" % (ser_kw, ser_ok)
    elif serial_set:
        ser_ok = len(lines) > 0
    else:
        ser_ok = True
    if title_ok_fn is not None:
        title_ok = title_ok_fn(app, t, s)
    else:
        title_ok = (t == "STM32") if title_required else True
    ok = title_ok and extra_ok and ser_ok
    return ("PASS" if ok else "FAIL", detail + " " + extra, None)


BENCH_BEGIN = "<!-- BENCHMARK:BEGIN -->"
BENCH_END = "<!-- BENCHMARK:END -->"


def read_benchmark_section(report_path):
    """Return the marker-delimited benchmark block of an existing report."""
    try:
        with open(report_path, "r", encoding="utf-8") as f:
            text = f.read()
    except OSError:
        return None
    b = text.find(BENCH_BEGIN)
    e = text.find(BENCH_END)
    if b == -1 or e == -1 or e < b:
        return None
    return text[b : e + len(BENCH_END)]


def upsert_benchmark_section(report_path, section):
    """Insert/replace the benchmark block, leaving the rest of the report."""
    try:
        with open(report_path, "r", encoding="utf-8") as f:
            text = f.read()
    except OSError:
        text = ""
    b = text.find(BENCH_BEGIN)
    e = text.find(BENCH_END)
    if b != -1 and e != -1 and e > b:
        text = text[:b] + section + text[e + len(BENCH_END) :]
    else:
        if text and not text.endswith("\n"):
            text += "\n"
        text += "\n" + section + "\n"
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(text)


def _write_table_report(report_path, title, rows):
    """Write the app table, preserving any existing benchmark block."""
    section = read_benchmark_section(report_path)
    lines = [title, "", "| app | result | detail |", "|---|---|---|"]
    for a, s_, d in rows:
        lines.append("| %s | %s | %s |" % (a, s_, d))
    text = "\n".join(lines) + "\n"
    if section:
        text += "\n" + section + "\n"
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(text)


def run_apps(
    default_apps,
    check_map,
    ser_kw_fn=None,
    serial_set=(),
    title_required=True,
    title_ok_fn=None,
    report_path=None,
    title="# HIL verification",
    default_check=None,
    collect_map=None,
    only_sections=False,
    apps=None,
):
    if apps is None:
        apps = sys.argv[1:]
    if not apps:
        apps = list(default_apps)
    clear_stray()
    page = OpenEdvSTM32F429Page(setting)
    if not page.connect():
        print("adapter not reachable - re-plug CMSIS-DAP")
        return []
    page.start()
    page.serial_open()
    rows = []
    sections = {}
    try:
        for a in apps:
            kw = ser_kw_fn(a) if ser_kw_fn else None
            collect = collect_map.get(a) if collect_map else None
            status, detail, section = run_once(
                page,
                a,
                check=check_map.get(a, default_check),
                ser_kw=kw,
                serial_set=serial_set,
                title_required=title_required,
                title_ok_fn=title_ok_fn,
                collect=collect,
            )
            print("%-32s %s  %s" % (a, status, detail), flush=True)
            rows.append((a, status, detail))
            if section:
                sections[a] = section
    finally:
        page.close()
    if report_path:
        if not only_sections:
            _write_table_report(report_path, title, rows)
        for section in sections.values():
            upsert_benchmark_section(report_path, section)
    return rows

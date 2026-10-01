#!/usr/bin/env python3
"""HIL verification for the ported FreeRTOS examples.

Reuses the project HIL framework (`test/page`, `test/config`): one persistent
OpenOCD session for the whole run (flashing, memory access, key injection), so
the CMSIS-DAP probe is initialised only once.

KEY0 = PH3, KEY1 = PH2, WK_UP = PA0 (active high).  LED0 = PB1.
"""

import os
import pathlib
import re
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
FREERTOS_BIN = "build/debug/openedv_stm32f4/freertos/%s/%s.bin"

SERIAL_SET = {
    "02_freertos_port",
    "09_time_slicing",
    "11_1_task_status_info",
    "11_2_run_time_stats",
    "14_3_priority_inversion",
    "14_4_mutex",
    "07_list_item",
    "13_2_queue_set",
}


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


def led_odr(env):
    """LED0/1 bits of GPIOB->ODR, with the GPIOB clock forced on (idle hook /
    tickless may gate it, making a plain read return 0)."""
    env.page.cmd("halt")
    enr = env.page.peek(0x40023830)
    env.page.poke(0x40023830, enr | 0x00000002)  # RCC AHB1ENR: GPIOBEN
    v = env.page.peek(0x40020414) & 0x3
    env.page.poke(0x40023830, enr)
    env.page.cmd("resume")
    return v


def led_probe(env):
    b = led_odr(env)
    env.tap("KEY1")
    time.sleep(0.3)
    a = led_odr(env)
    return ((b ^ a) & 0x2) != 0, "led0 %x->%x" % (b, a)


def c_06x(env, *a):
    v1 = num3(env, 71, 111)
    time.sleep(1.3)
    v2 = num3(env, 71, 111)
    env.tap("KEY0")
    time.sleep(0.4)
    v3 = num3(env, 71, 111)
    time.sleep(1.3)
    v4 = num3(env, 71, 111)
    return (v1 != v2) and (v3 == v4), "cnt %s,%s -> %s,%s" % (v1, v2, v3, v4)


def c_063(env, *a):
    v1 = num3(env, 71, 111)
    time.sleep(1.3)
    v2 = num3(env, 71, 111)
    env.tap("KEY0")
    time.sleep(0.4)
    v3 = num3(env, 71, 111)
    time.sleep(1.3)
    v4 = num3(env, 71, 111)
    env.tap("KEY1")
    time.sleep(1.3)
    v5 = num3(env, 71, 111)
    time.sleep(1.3)
    v6 = num3(env, 71, 111)
    return (v1 != v2) and (v3 == v4) and (v5 != v6), "susp %s,%s/%s,%s res %s,%s" % (
        v1,
        v2,
        v3,
        v4,
        v5,
        v6,
    )


def c_15(env, *a):
    env.tap("KEY0")
    time.sleep(1.0)
    a1 = env.dec(env.frame(), 15, 111, 16, 11, h.BLUE).strip()
    time.sleep(2.2)
    a2 = env.dec(env.frame(), 15, 111, 16, 11, h.BLUE).strip()
    return (a1 != a2), "timer1 '%s'->'%s'" % (a1, a2)


def c_event(env, *a):
    env.tap("KEY0")
    time.sleep(0.3)
    v1 = env.dec(env.frame(), 30, 110, 16, 20, h.BLUE).strip()
    before = env.px(env.frame(), 100, 200)
    env.tap("KEY1")
    time.sleep(0.4)
    after = env.px(env.frame(), 100, 200)
    return ("1" in v1) and (before != after), "ev '%s' fill %04x->%04x" % (
        v1,
        before,
        after,
    )


def c_20(env, *a):
    env.tap("KEY0")
    time.sleep(0.3)
    v = env.dec(env.frame(), 130, 160, 16, 12, h.BLUE).strip()
    return v.startswith("0x"), "addr %s" % v


def c_04(env, *a):
    """TIM3 (pre-emption 4) keeps counting while TIM6 (pre-emption 6) is masked
    during the 5 s `portDISABLE_INTERRUPTS()` window, so after the window the
    tim3 counter leads tim6 by >= 1. Poll until the post-window line appears."""
    deadline = time.time() + 16
    best, hl, seen = 0, "", False
    while time.time() < deadline:
        for l in env.page.serial_read_lines(1.0):
            if "disable interrupts" in l:
                seen = True
            m = re.search(r"tim3=(\d+) tim6=(\d+)", l)
            if m:
                d = int(m.group(1)) - int(m.group(2))
                if d > best:
                    best, hl = d, l
        if best >= 1 and seen:
            break
    return best >= 1, "max(tim3-tim6)=%d '%s' seen_disable=%s" % (best, hl, seen)


def c_07(env, *a):
    for _ in range(6):
        env.tap("KEY0", hold=120, gap=120)
    return True, "KEY0 x6"


def c_13_2(env, *a):
    env.tap("WK_UP")
    env.tap("KEY1")
    env.tap("KEY0")
    return True, "WK_UP/KEY1/KEY0"


def c_11_1(env, *a):
    for _ in range(3):
        env.tap("KEY0")
    return True, "KEY0 x3"


def c_11_2(env, *a):
    env.tap("KEY0")
    return True, "KEY0"


def gpio_odr(env):
    """Full GPIOB->ODR with the GPIOB clock forced on (idle/tickless gate it)."""
    env.page.cmd("halt")
    enr = env.page.peek(0x40023830)
    env.page.poke(0x40023830, enr | 0x00000002)
    v = env.page.peek(0x40020414)
    env.page.poke(0x40023830, enr)
    env.page.cmd("resume")
    return v


def c_lowpower(env, *a):
    """18/19: panel blanked (LTDC off + backlight off) and LED0 toggles."""
    ltdc = env.page.peek(0x40016818) & 1  # LTDC_GCR.LTDCEN
    bl = (gpio_odr(env) >> 5) & 1  # PB5 backlight
    bits = set()
    for _ in range(24):
        bits.add((gpio_odr(env) >> 1) & 1)  # LED0 = PB1
        time.sleep(0.3)
        if len(bits) == 2:
            break
    led_ok = 0 in bits and 1 in bits
    return (ltdc == 0) and (bl == 0) and led_ok, "ltdc=%d bl=%d led0=%s" % (
        ltdc,
        bl,
        sorted(bits),
    )


CHECK = {
    "06_1_task_create_dynamic": c_06x,
    "06_2_task_create_static": c_06x,
    "06_3_task_suspend_resume": c_063,
    "13_1_queue": lambda e, *a: (fill_probe(e)[0] and led_probe(e)[0], "fill+led"),
    "14_1_binary_semaphore": fill_probe,
    "14_2_counting_semaphore": fill_probe,
    "15_software_timer": c_15,
    "16_event_group": c_event,
    "13_3_queue_set_event_flags": c_event,
    "17_4_notify_event_group": c_event,
    "17_1_notify_binary_sem": fill_probe,
    "17_3_notify_mailbox": lambda e, *a: (
        fill_probe(e)[0] and led_probe(e)[0],
        "fill+led",
    ),
    "20_memory": c_20,
    "07_list_item": c_07,
    "13_2_queue_set": c_13_2,
    "11_1_task_status_info": c_11_1,
    "11_2_run_time_stats": c_11_2,
    "18_tickless": c_lowpower,
    "19_idle_hook": c_lowpower,
    "04_interrupt": c_04,
}


def ser_keywords(app):
    return {
        "07_list_item": "list",
        "13_2_queue_set": "queue",
        "11_1_task_status_info": "finished",
        "11_2_run_time_stats": "runtime",
        "02_freertos_port": "float_num",
        "04_interrupt": "tim3=",
        "09_time_slicing": "run count",
        "14_3_priority_inversion": "running",
        "14_4_mutex": "mutex",
    }.get(app)


def run(page, app):
    if build(app) != 0:
        return ("FAIL", "build rc!=0")
    binp = FREERTOS_BIN % (app, app)
    elf = str(ROOT / binp).replace(".bin", ".elf")
    page.elf = elf
    try:
        page.program(str(ROOT / binp))
    except Exception as e:  # noqa: BLE001
        return ("FAIL", "flash: %s" % e)
    page.reset_run()
    time.sleep(2.5)
    env = Env(page)
    t, s = env.title()
    detail = "lcd='%s'/'%s'" % (t, s)
    page.serial_read_lines(0.3)  # drop boot banner
    try:
        extra_ok, extra = CHECK[app](env, t, s) if app in CHECK else (True, "")
    except Exception as e:  # noqa: BLE001
        extra_ok, extra = False, "exc %s" % e
    lines = page.serial_read_lines(1.0)
    body = [x for x in lines if x and not x.startswith("app_freertos")]
    kw = ser_keywords(app)
    if kw is not None:
        ser_ok = any(kw in x for x in body)
        extra += " serkw '%s'=%s" % (kw, ser_ok)
    elif app in SERIAL_SET:
        ser_ok = len(lines) > 0
    else:
        ser_ok = True
    title_ok = True if app in ("18_tickless", "19_idle_hook") else (t == "STM32")
    ok = title_ok and extra_ok and ser_ok
    return ("PASS" if ok else "FAIL", detail + " " + extra)


def main():
    apps = sys.argv[1:]
    if not apps:
        apps = [
            "02_freertos_port",
            "04_interrupt",
            "06_1_task_create_dynamic",
            "06_2_task_create_static",
            "06_3_task_suspend_resume",
            "07_list_item",
            "09_time_slicing",
            "11_1_task_status_info",
            "11_2_run_time_stats",
            "13_1_queue",
            "13_2_queue_set",
            "13_3_queue_set_event_flags",
            "14_1_binary_semaphore",
            "14_2_counting_semaphore",
            "14_3_priority_inversion",
            "14_4_mutex",
            "15_software_timer",
            "16_event_group",
            "17_1_notify_binary_sem",
            "17_2_notify_counting_sem",
            "17_3_notify_mailbox",
            "17_4_notify_event_group",
            "18_tickless",
            "19_idle_hook",
            "20_memory",
        ]
    clear_stray()
    page = OpenEdvSTM32F429Page(setting)
    if not page.connect():
        print("adapter not reachable - re-plug CMSIS-DAP")
        return
    page.start()
    page.serial_open()
    rows = []
    try:
        for a in apps:
            status, detail = run(page, a)
            print("%-32s %s  %s" % (a, status, detail), flush=True)
            rows.append((a, status, detail))
    finally:
        page.close()
    with open(
        "D:/app/msys2/tmp/opencode/freertos_verify.md", "w", encoding="utf-8"
    ) as f:
        f.write("# FreeRTOS port - hardware verification\n\n")
        f.write("| app | result | detail |\n|---|---|---|\n")
        for a, s_, d in rows:
            f.write("| %s | %s | %s |\n" % (a, s_, d))


if __name__ == "__main__":
    main()

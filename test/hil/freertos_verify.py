#!/usr/bin/env python3
"""HIL verification for the ported FreeRTOS examples (app/freertos/0x..20).

The shared framework lives in `hil_common.py`; this file only defines the
per-app checks for the FreeRTOS kernel demos.
"""

import re
import time

import hil_common as hc
from hil_common import Env, num3, fill_probe, led_probe, gpio_odr  # noqa: F401


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
    a1 = env.dec(env.frame(), 15, 111, 16, 11, hc.h.BLUE).strip()
    time.sleep(2.2)
    a2 = env.dec(env.frame(), 15, 111, 16, 11, hc.h.BLUE).strip()
    return (a1 != a2), "timer1 '%s'->'%s'" % (a1, a2)


def c_event(env, *a):
    env.tap("KEY0")
    time.sleep(0.3)
    v1 = env.dec(env.frame(), 30, 110, 16, 20, hc.h.BLUE).strip()
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
    v = env.dec(env.frame(), 130, 160, 16, 12, hc.h.BLUE).strip()
    return v.startswith("0x"), "addr %s" % v


def c_04(env, *a):
    """TIM3 (pre-emption 4) keeps counting while TIM6 (pre-emption 6) is masked
    during the 5 s portDISABLE_INTERRUPTS() window, so after the window the tim3
    counter leads tim6 by >= 1."""
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

_KW = {
    "07_list_item": "list",
    "13_2_queue_set": "queue",
    "11_1_task_status_info": "finished",
    "11_2_run_time_stats": "runtime",
    "02_freertos_port": "float_num",
    "04_interrupt": "tim3=",
    "09_time_slicing": "run count",
    "14_3_priority_inversion": "running",
    "14_4_mutex": "mutex",
}

DEFAULT_APPS = [
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


def ser_kw(app):
    return _KW.get(app)


def title_ok(app, t, s):
    return True if app in ("18_tickless", "19_idle_hook") else (t == "STM32")


def main():
    hc.run_apps(
        DEFAULT_APPS,
        CHECK,
        ser_kw_fn=ser_kw,
        serial_set=SERIAL_SET,
        title_ok_fn=title_ok,
        report_path="D:/app/msys2/tmp/opencode/freertos_verify.md",
        title="# FreeRTOS port - hardware verification",
    )


if __name__ == "__main__":
    main()

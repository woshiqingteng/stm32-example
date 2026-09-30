#!/usr/bin/env python3
"""Per-app HIL smoke runner for the lwIP demos.

For each app it: builds+flashes, resets, captures serial, dumps the panel
framebuffer and OCRs it, then writes a small report. PC-side peers are run
separately (see pc_peer.py) and their observations are appended manually.

Usage:
  python hil_run.py <app> [<app> ...]
  python hil_run.py all            # every app/freertos/lwip_* dir

Environment: COM4 @115200, OpenOCD with interface/cmsis-dap.cfg +
target/stm32f4x.cfg, framebuffer at 0xC0000000 (480*800*2 = 0xBB800).
"""

import os
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.environ.get("HIL_OUT", r"D:/app/msys2/tmp/opencode/hil")
FB_ADDR = "0xC0000000"
FB_LEN = "0xBB800"
COM = os.environ.get("HIL_COM", "COM4")
CAPTURE = r"D:/app/msys2/tmp/opencode/capture_baud.py"

sys.path.insert(0, os.path.dirname(__file__))
import hil_fb_ocr  # noqa: E402

OCD = ["openocd", "-f", "interface/cmsis-dap.cfg", "-f", "target/stm32f4x.cfg"]


def sh(cmd, timeout=600):
    return subprocess.run(
        cmd, cwd=ROOT, capture_output=True, text=True, timeout=timeout
    )


def ocd(cmds, timeout=60):
    base = OCD + ["-c", "init"]
    for c in cmds:
        base += ["-c", c]
    base += ["-c", "exit"]
    return subprocess.run(
        base, cwd=ROOT, capture_output=True, text=True, timeout=timeout
    )


def flash(app):
    return sh(["bash", "tool/build.sh", "debug", app, "--flash"])


def reset_run():
    ocd(["reset run"])


def capture(app, seconds=14):
    out = os.path.join(OUT, app, "serial.bin")
    base = OCD + ["-c", "init", "-c", "reset run", "-c", "exit"]
    subprocess.Popen(
        base, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    time.sleep(1.5)
    subprocess.run(
        ["python", CAPTURE, COM, "115200", str(seconds), out],
        cwd=ROOT,
        capture_output=True,
        timeout=seconds + 15,
    )
    try:
        return open(out, "rb").read().decode("latin1")
    except OSError:
        return ""


def dump_fb(app, delay=4):
    os.makedirs(os.path.join(OUT, app), exist_ok=True)
    time.sleep(delay)
    dst = os.path.join(OUT, app, "fb.bin").replace("\\", "/")
    ocd(
        [
            "halt",
            "dump_image %s %s %s" % (dst, FB_ADDR, FB_LEN),
            "resume",
        ],
        timeout=60,
    )
    return os.path.join(OUT, app, "fb.bin")


def ocr(fb):
    fonts = hil_fb_ocr.parse_fonts(os.path.join(ROOT, "bsp/openedv_stm32f4/lcdfont.h"))
    fbdata = open(fb, "rb").read()
    lines = [
        ("stm32", 6, 10, 32, 8, hil_fb_ocr.DARKBLUE),
        ("title", 6, 40, 24, 32, hil_fb_ocr.DARKBLUE),
        ("atom", 6, 70, 16, 14, hil_fb_ocr.DARKBLUE),
        ("init", 5, 110, 16, 20, hil_fb_ocr.MAGENTA),
        ("ip", 5, 130, 16, 20, hil_fb_ocr.MAGENTA),
        ("speed", 5, 150, 16, 20, hil_fb_ocr.MAGENTA),
        ("key", 5, 170, 16, 16, hil_fb_ocr.MAGENTA),
        ("rxlabel", 5, 190, 16, 14, hil_fb_ocr.BLUE),
    ]
    out = {}
    for name, x0, y0, size, ml, color in lines:
        out[name] = hil_fb_ocr.decode_line(
            fbdata, fonts, x0, y0, size, ml, color
        ).strip()
    return out


def run_app(app):
    print("== %s ==" % app)
    r = flash(app)
    rep = ["# %s" % app, "flash rc=%d" % r.returncode]
    ser = capture(app)
    rep.append("serial: " + repr(ser))
    fb = dump_fb(app)
    texts = ocr(fb)
    for k, v in texts.items():
        rep.append("%-8s |%s|" % (k, v))
    os.makedirs(os.path.join(OUT, app), exist_ok=True)
    with open(os.path.join(OUT, app, "report.txt"), "w") as f:
        f.write("\n".join(rep) + "\n")
    print("\n".join(rep))
    return rep


def main():
    apps = sys.argv[1:]
    if not apps or apps == ["all"]:
        apps = sorted(
            d
            for d in os.listdir(os.path.join(ROOT, "app/freertos"))
            if d.startswith("lwip_")
        )
    os.makedirs(OUT, exist_ok=True)
    reports = []
    for a in apps:
        reports.append(run_app(a))
    with open(os.path.join(OUT, "report.md"), "w") as f:
        f.write("\n\n".join("\n".join(r) for r in reports) + "\n")
    print("wrote", os.path.join(OUT, "report.md"))


if __name__ == "__main__":
    main()

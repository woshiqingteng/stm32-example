#!/usr/bin/env python3
"""Inject KEY0 (PH3, active low) on the board via OpenOCD and read g_lwip_send_flag.

GPIOH: MODER 0x40021C00, BSRR 0x40021C18 (set bit n -> high, reset bit n -> low).
KEY0 = PH3.

Usage: python hil_key.py <elf> [--openocd openocd]
"""

import re
import subprocess
import sys

GPIOH_MODER = 0x40021C00
GPIOH_BSRR = 0x40021C18
PH3 = 3


def nm_addr(elf, sym):
    out = subprocess.check_output(["arm-none-eabi-nm", elf], text=True)
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == sym:
            return int(parts[0], 16)
    raise SystemExit("symbol not found: %s" % sym)


def ocd(cmds, openocd="openocd"):
    base = [
        openocd,
        "-f",
        "interface/cmsis-dap.cfg",
        "-f",
        "target/stm32f4x.cfg",
        "-c",
        "init",
    ]
    for c in cmds:
        base += ["-c", c]
    base += ["-c", "exit"]
    subprocess.run(base, capture_output=True, timeout=30)


def mdw(openocd, addr):
    r = subprocess.run(
        [
            openocd,
            "-f",
            "interface/cmsis-dap.cfg",
            "-f",
            "target/stm32f4x.cfg",
            "-c",
            "init",
            "-c",
            "halt",
            "-c",
            "mdw 0x%08X 1" % addr,
            "-c",
            "exit",
        ],
        capture_output=True,
        text=True,
        timeout=30,
    )
    for line in r.stdout.splitlines():
        if "0x%08x" % addr in line:
            return int(line.split(":")[1].strip().split()[0], 16)
    return None


def press_release(openocd):
    # PH3 -> output (MODER bits[7:6]=01 => 0x40), drive low (press), high
    # (release), then back to input. OpenOCD mww takes a literal value only.
    ocd(
        [
            "mww 0x%08X 0x00000040" % GPIOH_MODER,  # PH3 = output
            "mww 0x%08X 0x00080000" % GPIOH_BSRR,  # reset PH3 -> low (press)
            "sleep 80",
            "mww 0x%08X 0x00000008" % GPIOH_BSRR,  # set PH3 -> high (release)
            "mww 0x%08X 0x00000000" % GPIOH_MODER,  # back to input
        ],
        openocd,
    )


def main():
    elf = sys.argv[1]
    openocd = "openocd"
    if "--openocd" in sys.argv:
        openocd = sys.argv[sys.argv.index("--openocd") + 1]
    addr = nm_addr(elf, "g_lwip_send_flag")
    before = mdw(openocd, addr)
    press_release(openocd)
    after = mdw(openocd, addr)
    print("g_lwip_send_flag %s -> %s" % (before, after))
    print("KEY0 ok" if (after or 0) & 0x80 else "KEY0 FAILED")


if __name__ == "__main__":
    main()

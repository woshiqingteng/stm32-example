#!/usr/bin/env python3
"""
Simplified test runner.

Reads hardware parameters from ``config/setting.py`` (board + debugger + UART),
checks the target connection, then runs the requested tests with the board's
pytest marker.

Usage (from the repository root or from this directory):
    python run.py test/test_02_key.py
    python run.py test/test_02_key.py -k key0
    python run.py --board openedv_stm32f429
    python run.py                      # whole suite for the configured board
"""

from __future__ import annotations

import pathlib
import sys
import time

TEST_DIR = pathlib.Path(__file__).resolve().parent
ROOT = TEST_DIR.parent
sys.path.insert(0, str(TEST_DIR))

import pytest  # noqa: E402

from config import setting  # noqa: E402
from page.openedv_stm32f429 import OpenEdvSTM32F429Page  # noqa: E402


def _resolve_target(target: str) -> str:
    if pathlib.Path(target).exists():
        return target
    cand = TEST_DIR / target
    if cand.exists():
        return str(cand)
    return target


def _parse(argv):
    board = setting.BOARD
    timeout = "90"
    rest = []
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg in ("--board", "-b") and i + 1 < len(argv):
            board = argv[i + 1]
            i += 2
        elif arg in ("--test-timeout", "-t") and i + 1 < len(argv):
            timeout = argv[i + 1]
            i += 2
        else:
            rest.append(arg)
            i += 1
    return board, timeout, rest


def main(argv):
    board, timeout, rest = _parse(argv[1:])
    target = rest[0] if rest else "test"
    extra = rest[1:]

    print("[run] project root : {}".format(ROOT))
    print("[run] board        : {}".format(board))
    print(
        "[run] adapter      : {} serial={!r}".format(
            setting.ADAPTER, setting.ADAPTER_SERIAL
        )
    )
    print(
        "[run] serial port  : {} @ {}".format(setting.SERIAL_PORT, setting.SERIAL_BAUD)
    )

    page = OpenEdvSTM32F429Page(setting)
    if not page.connect():
        print("[run] connection check FAILED (probe not reachable)")
        return 2
    print("[run] connection check OK")
    time.sleep(0.8)  # let the one-shot OpenOCD release the USB device

    target = _resolve_target(target)
    return pytest.main(
        ["-m", board, target, "--test-timeout={}".format(timeout), *extra]
    )


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

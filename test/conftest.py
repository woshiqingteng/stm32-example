"""
pytest fixtures for the HIL test suite.

- Adds the test dir to sys.path so `config`/`page` import.
- Sets up logging into <test>/log/.
- `board` (session) opens the OpenOCD session; the autouse `_require_board`
  skips every test when the probe/target is not reachable (no marker needed).
- `_watchdog` enforces a per-test timeout (``--test-timeout``).
"""

from __future__ import annotations

import ctypes
import logging
import pathlib
import subprocess
import sys
import threading
import time

import pytest

TEST_DIR = pathlib.Path(__file__).resolve().parent
ROOT = TEST_DIR.parent
sys.path.insert(0, str(TEST_DIR))

from config import setting  # noqa: E402
from page.openedv_stm32f429 import OpenEdvSTM32F429Page  # noqa: E402

LOG_DIR = TEST_DIR / "log"
LOG_DIR.mkdir(exist_ok=True)
RUN_LOG = LOG_DIR / ("run_{}.log".format(time.strftime("%Y%m%d_%H%M%S")))


def _setup_logging():
    logger = logging.getLogger("hil")
    logger.setLevel(logging.DEBUG)
    if logger.handlers:
        return
    fmt = logging.Formatter(
        "%(asctime)s.%(msecs)03d %(levelname)s %(name)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    fh = logging.FileHandler(RUN_LOG, encoding="utf-8")
    fh.setFormatter(fmt)
    sh = logging.StreamHandler()
    sh.setFormatter(fmt)
    logger.addHandler(fh)
    logger.addHandler(sh)


_setup_logging()
log = logging.getLogger("hil.conftest")

BIN_DIR = ROOT / "build" / "debug" / "openedv_stm32f4" / "baremetal"


def _clear_stray_openocd():
    """Kill leftover OpenOCD processes that would hold the telnet ports."""
    try:
        subprocess.run(
            ["taskkill", "/F", "/IM", "openocd.exe"], capture_output=True, timeout=10
        )
        time.sleep(0.5)
    except Exception:  # noqa: BLE001
        log.warning("could not clear stray openocd")


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "openedv_stm32f429: openedv (ALIENTEK) STM32F429 board HIL tests",
    )


def pytest_addoption(parser):
    parser.addoption(
        "--test-timeout",
        action="store",
        default="90",
        help="per-test timeout in seconds (0 disables)",
    )


class _TestTimeout(Exception):
    """Raised in the test thread on timeout."""


def _async_raise(tid, exc):
    res = ctypes.pythonapi.PyThreadState_SetAsyncExc(
        ctypes.c_long(tid), ctypes.py_object(exc)
    )
    if res != 1:  # pragma: no cover - safety reset
        ctypes.pythonapi.PyThreadState_SetAsyncExc(ctypes.c_long(tid), None)


@pytest.fixture(autouse=True)
def _watchdog(request):
    """Per-test timeout fallback (cooperative async exception)."""
    secs = float(request.config.getoption("--test-timeout"))
    if secs <= 0:
        yield
        return
    done = threading.Event()
    tid = threading.get_ident()

    def run():
        if not done.wait(secs):
            log.error("test timed out after %ss", secs)
            _async_raise(tid, _TestTimeout)

    thread = threading.Thread(target=run, daemon=True)
    thread.start()
    try:
        yield
    finally:
        done.set()


@pytest.fixture(scope="session")
def board():
    _clear_stray_openocd()
    page = OpenEdvSTM32F429Page(setting)
    log.info("session log file: %s", RUN_LOG)
    log.info(
        "board=%s adapter=%s serial=%r port=%s baud=%d",
        setting.BOARD,
        setting.ADAPTER,
        setting.ADAPTER_SERIAL,
        setting.SERIAL_PORT,
        setting.SERIAL_BAUD,
    )
    page.connected = page.connect()
    if page.connected:
        for attempt in range(5):
            try:
                page.start()
                break
            except Exception as exc:  # noqa: BLE001
                log.warning("openocd start attempt %d failed: %s", attempt + 1, exc)
                page.connected = False
                time.sleep(1.0)
    if page.connected:
        log.info("hardware session ready")
    else:
        log.error("no CMSIS-DAP/target connection")
    yield page
    page.close()


@pytest.fixture(autouse=True)
def _require_board(board):
    """Skip every test when the hardware is not connected (no marker)."""
    if not getattr(board, "connected", False):
        pytest.skip(
            "no CMSIS-DAP/target connection (adapter serial {!r})".format(
                setting.ADAPTER_SERIAL
            )
        )

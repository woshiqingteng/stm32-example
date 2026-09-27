"""05_iwdg: independent watchdog fed by WK_UP; resets if starved."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "05_iwdg"


def test_feed_prevents_reset(flashed):
    flashed.clear_reset_flags()
    for _ in range(8):
        flashed.tap("WK_UP", hold_ms=40, gap_ms=150)
    flags = flashed.reset_flags()
    assert (flags & R.RCC_CSR_IWDGRSTF) == 0, "unexpected IWDG reset while fed"


def test_starve_triggers_reset(flashed):
    flashed.clear_reset_flags()
    flashed.sleep(1600)
    flags = flashed.reset_flags()
    assert (flags & R.RCC_CSR_IWDGRSTF) != 0, "expected IWDG reset when starved"

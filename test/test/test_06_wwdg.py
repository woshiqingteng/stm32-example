"""06_wwdg: window watchdog refreshed from its early-wakeup IRQ."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "06_wwdg"


def test_led1_toggles(flashed):
    samples = flashed.sample_led_odr(2.0, 50)
    assert len({s & 0x01 for s in samples}) == 2, "LED1 should toggle"


def test_no_wwdg_reset(flashed):
    flags = flashed.reset_flags()
    assert (flags & R.RCC_CSR_WWDGRSTF) == 0, "unexpected WWDG reset"

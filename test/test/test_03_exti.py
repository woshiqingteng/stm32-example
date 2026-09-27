"""03_exti: four keys via external interrupts."""

import pytest

pytestmark = pytest.mark.openedv_stm32f429

APP = "03_exti"

EXPECTED = {
    "KEY0": 0x02,
    "KEY1": 0x00,
    "KEY2": 0x03,
    "WK_UP": 0x02,
}


def test_boot_state(flashed):
    assert flashed.led_odr() == 0x01


@pytest.mark.parametrize("key,expected", list(EXPECTED.items()))
def test_exti_key(flashed, key, expected):
    flashed.tap(key, hold_ms=60, gap_ms=150)
    assert flashed.led_odr() == expected, "after {} expected {:#x}".format(
        key, expected
    )

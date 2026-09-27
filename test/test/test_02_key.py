"""02_key: four keys and their LED effects (polled)."""

import pytest

pytestmark = pytest.mark.openedv_stm32f429

APP = "02_key"

# Boot: LED0 on, LED1 off  -> ODR bits {LED0,LB1} = 0b01
EXPECTED = {
    "KEY0": 0x02,  # both toggle
    "KEY1": 0x00,  # LED1 toggle
    "KEY2": 0x03,  # LED0 toggle
    "WK_UP": 0x02,  # LED1 toggle + LED0 = !LED1
}


def test_boot_state(flashed):
    assert flashed.led_odr() == 0x01


@pytest.mark.parametrize("key,expected", list(EXPECTED.items()))
def test_key(flashed, key, expected):
    flashed.tap(key)
    assert flashed.led_odr() == expected, "after {} expected {:#x}".format(
        key, expected
    )

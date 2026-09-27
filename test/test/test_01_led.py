"""01_led: alternate LED0/LED1."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


APP = "01_led"


def test_boot_state(flashed):
    led0, led1 = flashed.led_states()
    assert led0 and not led1


def test_both_leds_alternate(flashed):
    samples = set(flashed.sample_led_odr(2.6, 100))
    assert {0x01, 0x02} <= samples, "expected both LED phases, got {}".format(samples)

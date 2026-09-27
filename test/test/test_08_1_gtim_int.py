"""08_1_gtim_int: TIM3 update IRQ + main-loop both toggle LEDs."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


APP = "08_1_gtim_int"


def test_both_leds_toggle(flashed):
    samples = flashed.sample_led_odr(1.2, 50)
    assert len({s & 0x01 for s in samples}) == 2, "LED1 (TIM3) should toggle"
    assert len({s & 0x02 for s in samples}) == 2, "LED0 (loop) should toggle"

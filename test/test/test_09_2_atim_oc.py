"""09_2_atim_oc: TIM8 CH1..4 output-compare toggle on PC6..PC9."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "09_2_atim_oc"


def test_compare_values(flashed):
    base = R.TIM8_BASE
    assert flashed.peek(base + R.TIM_CCR1) == 249
    assert flashed.peek(base + R.TIM_CCR1 + 4) == 499
    assert flashed.peek(base + R.TIM_CCR1 + 8) == 749
    assert flashed.peek(base + R.TIM_CCR1 + 12) == 999


def test_pins_toggle(flashed):
    seen = set()
    for _ in range(40):
        seen.add((flashed.peek(R.GPIOC_BASE + R.GPIO_IDR) >> 6) & 0xF)
        flashed.sleep(20)
    assert len(seen) > 1, "PC6..PC9 should toggle"

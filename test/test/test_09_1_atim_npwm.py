"""09_1_atim_npwm: TIM8_CH1 (PC6) emits N pulses; sampled on the PC6 pin."""

import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "09_1_atim_npwm"

PC6 = 6


def _count_rising(flashed, seconds, interval_ms=30):
    prev = None
    edges = 0
    n = int(seconds * 1000 / interval_ms)
    for _ in range(n):
        bit = (flashed.peek(R.GPIOC_BASE + R.GPIO_IDR) >> PC6) & 1
        if prev == 0 and bit == 1:
            edges += 1
        prev = bit
        flashed.sleep(interval_ms)
    return edges


def test_emits_five_pulses(flashed):
    edges = _count_rising(flashed, 3.2)
    assert edges >= 4, "expected ~5 PWM pulses, saw {} edges".format(edges)


def test_key0_retriggers(flashed):
    flashed.tap("KEY0")
    edges = _count_rising(flashed, 3.2)
    assert edges >= 4, "KEY0 should (re)start the pulse burst, saw {} edges".format(
        edges
    )

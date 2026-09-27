"""08_4_gtim_cnt: TIM2 external counter (no external pulses -> injected count)."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "08_4_gtim_cnt"

TIM2_CNT = R.TIM2_BASE + R.TIM_CNT


def test_injected_count_printed(flashed):
    flashed.serial_open()
    flashed.serial_read_lines(0.3)
    flashed.poke(TIM2_CNT, 12345)
    lines = flashed.serial_read_lines(1.0)
    assert any("CNT:12345" in ln for ln in lines), lines


def test_key0_restart_clears(flashed):
    flashed.serial_open()
    flashed.serial_read_lines(0.3)
    flashed.poke(TIM2_CNT, 5000)
    flashed.serial_read_lines(0.5)
    flashed.tap("KEY0")
    lines = flashed.serial_read_lines(1.0)
    assert any("CNT:0" in ln for ln in lines), lines

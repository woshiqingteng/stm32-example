"""08_3_gtim_cap: TIM5 input capture (no external signal -> injected capture)."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


APP = "08_3_gtim_cap"


def test_injected_capture_prints_high(flashed):
    state = flashed.symbol("g_gtim_cap_state")
    over = flashed.symbol("g_gtim_cap_overflows")
    value = flashed.symbol("g_gtim_cap_value")

    flashed.serial_open()
    flashed.serial_read_lines(0.3)

    flashed.poke(over, 0)
    flashed.poke(value, 1234)
    flashed.poke(state, 2)  # GTIM_CAP_DONE

    lines = flashed.serial_read_lines(1.0)
    assert any("HIGH:1234" in ln for ln in lines), lines

"""10_tpad: capacitive touch (no finger -> injected baseline forces a touch)."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


APP = "10_tpad"


def test_led0_heartbeat(flashed):
    samples = flashed.sample_led_odr(1.0, 50)
    assert len({(s >> 1) & 1 for s in samples}) == 2, "LED0 heartbeat should toggle"


def test_injected_touch_toggles_led1(flashed):
    # Force the baseline low so the next scan exceeds baseline+gate (a "touch").
    baseline = flashed.symbol("g_tpad_default_val")
    flashed.poke16(baseline, 0)

    samples = flashed.sample_led_odr(1.5, 50)
    assert len({s & 0x01 for s in samples}) == 2, "LED1 should toggle on injected touch"

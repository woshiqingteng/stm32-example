"""09_3_atim_cplm: TIM1 complementary PWM + dead time on PE8/PE9."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "09_3_atim_cplm"


def test_deadtime_and_moe(flashed):
    bdtr = flashed.peek(R.TIM1_BASE + R.TIM_BDTR)
    assert (bdtr & 0xFF) != 0, "dead time should be non-zero"
    assert (bdtr & (1 << 15)) != 0, "MOE should be set"


def test_pins_complementary(flashed):
    seen = set()
    for _ in range(60):
        seen.add((flashed.peek(R.GPIOE_BASE + R.GPIO_IDR) >> 8) & 0x3)
        flashed.sleep(10)
    assert seen & {0b01, 0b10}, (
        "PE8/PE9 should show complementary states, saw {}".format(seen)
    )

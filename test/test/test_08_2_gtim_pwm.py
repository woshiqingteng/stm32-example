"""08_2_gtim_pwm: TIM3_CH4 breathing PWM on PB1 (duty ramps up then down)."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "08_2_gtim_pwm"


def test_ccr_register_ramps(flashed):
    vals = []
    for _ in range(70):
        vals.append(flashed.peek(R.TIM3_BASE + R.TIM_CCR4))
        flashed.sleep(80)

    assert max(vals) - min(vals) > 20, "duty should ramp, got {}..{}".format(
        min(vals), max(vals)
    )
    up = any(vals[i + 1] > vals[i] for i in range(len(vals) - 1))
    down = any(vals[i + 1] < vals[i] for i in range(len(vals) - 1))
    assert up and down, "expected ramp up then down"


def test_pwm_configured(flashed):
    ccr = flashed.peek(R.TIM3_BASE + R.TIM_CCR4)
    arr = flashed.peek(R.TIM3_BASE + R.TIM_ARR)
    assert 0 <= ccr <= arr

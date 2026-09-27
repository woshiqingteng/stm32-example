"""09_4_atim_pwmin: TIM8 PWM input measuring TIM3_CH4 (PB1).
No PB1->PC6 jumper by default, so the capture result is injected."""

import pytest

pytestmark = pytest.mark.openedv_stm32f429


from config import stm32f429 as R

APP = "09_4_atim_pwmin"


def test_tim3_test_pwm_configured(flashed):
    assert flashed.peek(R.TIM3_BASE + R.TIM_CCR4) == 2


def test_injected_pwmin_prints_freq(flashed):
    sm = flashed.symbol("g_atim_pwmin_sm")
    psc = flashed.symbol("g_atim_pwmin_psc")
    hval = flashed.symbol("g_atim_pwmin_hval")
    cval = flashed.symbol("g_atim_pwmin_cval")

    flashed.serial_open()
    flashed.serial_read_lines(0.3)

    # Stop TIM8 so its ISR cannot overwrite the injected state machine.
    tim8 = R.TIM8_BASE
    cr1 = flashed.peek(tim8 + R.TIM_CR1)
    flashed.poke(tim8 + R.TIM_CR1, cr1 & ~R.TIM_CR1_CEN)
    flashed.poke(tim8 + R.TIM_DIER, 0)

    flashed.poke16(psc, 89)  # scale = 90
    flashed.poke(hval, 10)  # htime = 10*90/180 = 5 us
    flashed.poke(cval, 20)  # ctime = 20*90/180 = 10 us -> 100 kHz
    flashed.poke(sm, 2)  # ATIM_PWMIN_SM_DONE

    lines = flashed.serial_read_lines(1.5)
    assert any("freq:100000" in ln for ln in lines), lines

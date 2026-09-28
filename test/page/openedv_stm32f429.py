"""
OpenEdvSTM32F429Page: board/chip specific helpers for the openedv (ALIENTEK)
STM32F429 board: register constants, pin injection, LED state, watchdog reset
flags and flashing. Inherits the generic OpenOCD/UART access from BasePage.

Register constants are referenced from the device file ``config/stm32f429.py``
and exposed as class attributes so tests can use e.g. ``page.GPIOB_ODR``.
"""

from __future__ import annotations

import pathlib

from config import stm32f429 as _dev

from .base import BasePage

PUPDR_PULL_UP = 0x1
PUPDR_PULL_DOWN = 0x2


class OpenEdvSTM32F429Page(BasePage):
    # -- device constants (from config/stm32f429.py) -----------------------
    GPIOA_BASE = _dev.GPIOA_BASE
    GPIOB_BASE = _dev.GPIOB_BASE
    GPIOC_BASE = _dev.GPIOC_BASE
    GPIOD_BASE = _dev.GPIOD_BASE
    GPIOE_BASE = _dev.GPIOE_BASE
    GPIOF_BASE = _dev.GPIOF_BASE
    GPIOG_BASE = _dev.GPIOG_BASE
    GPIOH_BASE = _dev.GPIOH_BASE

    GPIO_MODER = _dev.GPIO_MODER
    GPIO_PUPDR = _dev.GPIO_PUPDR
    GPIO_IDR = _dev.GPIO_IDR
    GPIO_ODR = _dev.GPIO_ODR
    GPIO_BSRR = _dev.GPIO_BSRR

    RCC_CSR = _dev.RCC_CSR
    RCC_CSR_RMVF = _dev.RCC_CSR_RMVF
    RCC_CSR_IWDGRSTF = _dev.RCC_CSR_IWDGRSTF
    RCC_CSR_WWDGRSTF = _dev.RCC_CSR_WWDGRSTF

    TIM1_BASE = _dev.TIM1_BASE
    TIM2_BASE = _dev.TIM2_BASE
    TIM3_BASE = _dev.TIM3_BASE
    TIM5_BASE = _dev.TIM5_BASE
    TIM6_BASE = _dev.TIM6_BASE
    TIM8_BASE = _dev.TIM8_BASE
    TIM_CR1 = _dev.TIM_CR1
    TIM_DIER = _dev.TIM_DIER
    TIM_SR = _dev.TIM_SR
    TIM_CCMR1 = _dev.TIM_CCMR1
    TIM_CCMR2 = _dev.TIM_CCMR2
    TIM_CCER = _dev.TIM_CCER
    TIM_CNT = _dev.TIM_CNT
    TIM_PSC = _dev.TIM_PSC
    TIM_ARR = _dev.TIM_ARR
    TIM_RCR = _dev.TIM_RCR
    TIM_CCR1 = _dev.TIM_CCR1
    TIM_CCR4 = _dev.TIM_CCR4
    TIM_BDTR = _dev.TIM_BDTR
    TIM_CR1_CEN = _dev.TIM_CR1_CEN
    TIM_CR1_OPM = _dev.TIM_CR1_OPM

    # On-board LEDs (active low): LED0 = PB1, LED1 = PB0.
    LED0_PIN = 1
    LED1_PIN = 0

    # On-board keys: name -> (port_base, pin, active_high)
    #   KEY0..KEY2 active low with pull-up, WK_UP active high with pull-down.
    KEY_DEFS = {
        "KEY0": (GPIOH_BASE, 3, False),
        "KEY1": (GPIOH_BASE, 2, False),
        "KEY2": (GPIOC_BASE, 13, False),
        "WK_UP": (GPIOA_BASE, 0, True),
    }

    BIN_DIR = (
        pathlib.Path(__file__).resolve().parents[2]
        / "build"
        / "debug"
        / "openedv_stm32f4"
        / "baremetal"
    )

    # -- flashing -----------------------------------------------------------
    def flash_app(self, app: str) -> None:
        """Flash the application ``app`` and reset so it runs from boot."""
        binpath = self.BIN_DIR / app / (app + ".bin")
        self.elf = str(binpath.with_suffix(".elf"))
        self.program(binpath.as_posix())
        self.sleep(300)
        try:
            self.clear_reset_flags()
        except Exception:  # noqa: BLE001
            self.log.warning("clear_reset_flags failed")

    # -- LEDs ---------------------------------------------------------------
    def led_odr(self) -> int:
        """Current LED bits of GPIOB->ODR (bit1 = LED0, bit0 = LED1)."""
        return self.peek(self.GPIOB_BASE + self.GPIO_ODR) & 0x3

    def led_states(self):
        """Return (led0_on, led1_on); LEDs are active low."""
        odr = self.led_odr()
        led0_on = ((odr >> self.LED0_PIN) & 1) == 0
        led1_on = ((odr >> self.LED1_PIN) & 1) == 0
        return led0_on, led1_on

    def sample_led_odr(self, seconds, interval_ms=50):
        """Sample the raw LED ODR bits over a window."""
        samples = []
        n = max(1, int(seconds * 1000 / interval_ms))
        for _ in range(n):
            samples.append(self.led_odr())
            self.sleep(interval_ms)
        return samples

    # -- keys ---------------------------------------------------------------
    def press(self, name):
        base, pin, active_high = self.KEY_DEFS[name]
        moder = self.peek(base + self.GPIO_MODER)
        moder = (moder & ~(0x3 << (2 * pin))) | (0x1 << (2 * pin))
        self.poke(base + self.GPIO_MODER, moder)
        bsrr = (1 << pin) if active_high else (1 << (pin + 16))
        self.poke(base + self.GPIO_BSRR, bsrr)

    def release(self, name):
        base, pin, active_high = self.KEY_DEFS[name]
        moder = self.peek(base + self.GPIO_MODER)
        moder &= ~(0x3 << (2 * pin))
        self.poke(base + self.GPIO_MODER, moder)
        pupdr = self.peek(base + self.GPIO_PUPDR)
        val = PUPDR_PULL_DOWN if active_high else PUPDR_PULL_UP
        pupdr = (pupdr & ~(0x3 << (2 * pin))) | (val << (2 * pin))
        self.poke(base + self.GPIO_PUPDR, pupdr)

    def tap(self, name, hold_ms=60, gap_ms=100):
        self.press(name)
        self.sleep(hold_ms)
        self.release(name)
        self.sleep(gap_ms)

    # -- watchdog / reset ---------------------------------------------------
    def reset_flags(self) -> int:
        return self.peek(self.RCC_CSR)

    def clear_reset_flags(self):
        self.poke(self.RCC_CSR, self.RCC_CSR_RMVF)

    # -- generic register / pin helpers ------------------------------------
    def gpio_read(self, port_base, mask, shift=0) -> int:
        """Read GPIOx->IDR, masked and shifted to a small field."""
        return (self.peek(port_base + self.GPIO_IDR) >> shift) & mask

    def pwm_ccr4(self) -> int:
        """TIM3_CH4 compare value (the 08_2 breathing PWM)."""
        return self.peek(self.TIM3_BASE + self.TIM_CCR4)

    def pwm_arr(self) -> int:
        return self.peek(self.TIM3_BASE + self.TIM_ARR)

    def oc_ccr(self, ch: int) -> int:
        """TIM8 output-compare value for channel ``ch`` (1..4)."""
        return self.peek(self.TIM8_BASE + self.TIM_CCR1 + (ch - 1) * 4)

    def bdtr(self) -> int:
        """TIM1 break/dead-time register."""
        return self.peek(self.TIM1_BASE + self.TIM_BDTR)

    # -- app-specific operation injection ----------------------------------
    def inject_capture(self, value: int):
        """08_3: force a completed capture (state -> CAP_READY, width=value us)."""
        self.poke(self.symbol("g_cap_width"), value)  # low 32 bits (little-endian)
        self.poke(self.symbol("g_cap_state"), 2)  # CAP_READY

    def inject_counter(self, count: int):
        """08_4: inject a TIM2 external-counter value."""
        self.poke(self.TIM2_BASE + self.TIM_CNT, count)

    def count_pc6_rising(self, seconds, interval_ms=30) -> int:
        """09_1: count rising edges on the TIM8_CH1 output pin (PC6)."""
        prev = None
        edges = 0
        for _ in range(int(seconds * 1000 / interval_ms)):
            bit = self.gpio_read(self.GPIOC_BASE, 0x1, 6)
            if prev == 0 and bit == 1:
                edges += 1
            prev = bit
            self.sleep(interval_ms)
        return edges

    def stop_tim8(self):
        """09_4: stop TIM8 (CEN=0) and disable its interrupt."""
        cr1 = self.peek(self.TIM8_BASE + self.TIM_CR1)
        self.poke(self.TIM8_BASE + self.TIM_CR1, cr1 & ~self.TIM_CR1_CEN)
        self.poke(self.TIM8_BASE + self.TIM_DIER, 0)

    def inject_pwmin(self, psc: int, hval: int, cval: int):
        """09_4: inject a PWM-input measurement (state -> DONE)."""
        self.poke16(self.symbol("g_atim_pwmin_psc"), psc)
        self.poke(self.symbol("g_atim_pwmin_hval"), hval)
        self.poke(self.symbol("g_atim_pwmin_cval"), cval)
        self.poke(self.symbol("g_atim_pwmin_sm"), 2)  # ATIM_PWMIN_SM_DONE

    def inject_touch(self):
        """10: force a 'touch' by driving the capacitive baseline to 0."""
        self.poke16(self.symbol("g_tpad_default_val"), 0)

"""
OpenEdvSTM32F429Page: board/chip specific helpers (pin injection, LED state,
watchdog reset flags) for the openedv (ALIENTEK) STM32F429 board.

Inherits the generic OpenOCD/UART access from BasePage.
"""

from __future__ import annotations

from config import stm32f429 as R

from .base import BasePage

# On-board LEDs (active low): LED0 = PB1, LED1 = PB0.
LED0_GPIO_BASE = R.GPIOB_BASE
LED0_PIN = 1
LED1_PIN = 0

# On-board keys: name -> (port_base, pin, active_high, pull_up, pull_down)
#   KEY0..KEY2 active low with pull-up, WK_UP active high with pull-down.
KEY_DEFS = {
    "KEY0": (R.GPIOH_BASE, 3, False),
    "KEY1": (R.GPIOH_BASE, 2, False),
    "KEY2": (R.GPIOC_BASE, 13, False),
    "WK_UP": (R.GPIOA_BASE, 0, True),
}

PUPDR_PULL_UP = 0x1
PUPDR_PULL_DOWN = 0x2


class OpenEdvSTM32F429Page(BasePage):
    # -- LEDs ---------------------------------------------------------------
    def led_odr(self) -> int:
        """Current LED bits of GPIOB->ODR (bit1 = LED0, bit0 = LED1)."""
        return self.peek(R.GPIOB_BASE + R.GPIO_ODR) & 0x3

    def led_states(self):
        """Return (led0_on, led1_on); LEDs are active low."""
        odr = self.led_odr()
        led0_on = ((odr >> LED0_PIN) & 1) == 0
        led1_on = ((odr >> LED1_PIN) & 1) == 0
        return led0_on, led1_on

    def sample_led_odr(self, seconds, interval_ms=50):
        """Sample the raw LED ODR bits over a window."""
        samples = []
        end = 0
        n = max(1, int(seconds * 1000 / interval_ms))
        for _ in range(n):
            samples.append(self.led_odr())
            self.sleep(interval_ms)
        return samples

    # -- keys ---------------------------------------------------------------
    def press(self, name):
        base, pin, active_high = KEY_DEFS[name]
        moder = self.peek(base + R.GPIO_MODER)
        moder = (moder & ~(0x3 << (2 * pin))) | (0x1 << (2 * pin))
        self.poke(base + R.GPIO_MODER, moder)
        bsrr = (1 << pin) if active_high else (1 << (pin + 16))
        self.poke(base + R.GPIO_BSRR, bsrr)

    def release(self, name):
        base, pin, active_high = KEY_DEFS[name]
        moder = self.peek(base + R.GPIO_MODER)
        moder &= ~(0x3 << (2 * pin))
        self.poke(base + R.GPIO_MODER, moder)
        pupdr = self.peek(base + R.GPIO_PUPDR)
        val = PUPDR_PULL_DOWN if active_high else PUPDR_PULL_UP
        pupdr = (pupdr & ~(0x3 << (2 * pin))) | (val << (2 * pin))
        self.poke(base + R.GPIO_PUPDR, pupdr)

    def tap(self, name, hold_ms=60, gap_ms=100):
        self.press(name)
        self.sleep(hold_ms)
        self.release(name)
        self.sleep(gap_ms)

    # -- watchdog / reset ---------------------------------------------------
    def reset_flags(self) -> int:
        return self.peek(R.RCC_CSR)

    def clear_reset_flags(self):
        self.poke(R.RCC_CSR, R.RCC_CSR_RMVF)

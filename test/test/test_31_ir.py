"""31_ir: TIM1_CH1 input capture. Verifies the capture interrupt path is wired
up (TIM1 uses a separate CC NVIC line, TIM1_CC_IRQn)."""

import pytest


APP = "31_ir"
pytestmark = pytest.mark.openedv_stm32f429

# STM32F429 IRQn numbers.
TIM1_UP_TIM10_IRQN = 25
TIM1_CC_IRQN = 27
NVIC_ISER0 = 0xE000E100


class Test31Ir:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_capture_interrupt_path_enabled(self):
        iser = self.board.peek(NVIC_ISER0)
        assert (iser >> TIM1_UP_TIM10_IRQN) & 1, "TIM1_UP NVIC line must be enabled"
        assert (iser >> TIM1_CC_IRQN) & 1, "TIM1_CC NVIC line must be enabled (capture)"

        dier = self.board.peek(self.board.TIM1_BASE + self.board.TIM_DIER)
        assert (dier >> 0) & 1, "DIER.UIE (update) must be set"
        assert (dier >> 1) & 1, "DIER.CC1IE (capture) must be set"

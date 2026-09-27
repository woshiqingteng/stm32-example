"""08_1_gtim_int: TIM3 update IRQ + main-loop both toggle LEDs."""

import pytest


APP = "08_1_gtim_int"
pytestmark = pytest.mark.openedv_stm32f429


class Test08_1GtimInt:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_both_leds_toggle(self):
        # 前提：08_1 运行（LED1 由 TIM3 中断翻、LED0 由主循环翻）
        # 动作：采样 ODR 1.2 s
        samples = self.board.sample_led_odr(1.2, 50)
        # 期望：LED1 与 LED0 都出现两态
        assert len({s & 0x01 for s in samples}) == 2, "LED1 (TIM3) should toggle"
        assert len({s & 0x02 for s in samples}) == 2, "LED0 (loop) should toggle"

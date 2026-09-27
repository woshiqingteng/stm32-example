"""09_2_atim_oc: TIM8 CH1..4 output-compare toggle on PC6..PC9."""

import pytest


APP = "09_2_atim_oc"
pytestmark = pytest.mark.openedv_stm32f429


class Test09_2AtimOc:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_compare_values(self):
        # 前提：09_2 运行
        # 动作：读取 TIM8 四个比较寄存器
        # 期望：CCR1..4 == 249 / 499 / 749 / 999
        assert self.board.oc_ccr(1) == 249
        assert self.board.oc_ccr(2) == 499
        assert self.board.oc_ccr(3) == 749
        assert self.board.oc_ccr(4) == 999

    def test_pins_toggle(self):
        # 前提：09_2 运行，PC6..PC9 输出比较翻转
        # 动作：采样 GPIOC IDR 的 PC6..PC9
        seen = set()
        for _ in range(40):
            seen.add(self.board.gpio_read(self.board.GPIOC_BASE, 0xF, 6))
            self.board.sleep(20)
        # 期望：出现多于一种组合（引脚在翻转）
        assert len(seen) > 1, "PC6..PC9 should toggle"

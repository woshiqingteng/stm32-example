"""09_1_atim_npwm: TIM8_CH1 (PC6) emits N pulses; sampled on the PC6 pin."""

import pytest

APP = "09_1_atim_npwm"
pytestmark = pytest.mark.openedv_stm32f429


class Test09_1AtimNpwm:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_emits_pulses(self):
        # 前提：09_1 运行，TIM8_CH1 输出 N 个脉冲到 PC6
        # 动作：在 PC6 上采样 3.2 s 统计上升沿
        # 期望：至少 4 个脉冲
        edges = self.board.count_pc6_rising(3.2)
        assert edges >= 4, "expected ~5 PWM pulses, saw {} edges".format(edges)

    def test_key0_retriggers(self):
        # 前提：09_1 运行
        # 动作：注入 KEY0 触发一轮脉冲，再采样 3.2 s
        self.board.tap("KEY0")
        edges = self.board.count_pc6_rising(3.2)
        # 期望：再次出现至少 4 个脉冲
        assert edges >= 4, "KEY0 should (re)start the burst, saw {} edges".format(edges)

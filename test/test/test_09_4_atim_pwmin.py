"""09_4_atim_pwmin: TIM8 PWM input measuring TIM3_CH4 (PB1).
No PB1->PC6 jumper by default, so the capture result is injected.
"""

import pytest


APP = "09_4_atim_pwmin"
pytestmark = pytest.mark.openedv_stm32f429


class Test09_4AtimPwmin:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_tim3_test_pwm_configured(self):
        # 前提：09_4 运行，TIM3_CH4 生成测试 PWM
        # 动作：读取 TIM3->CCR4
        # 期望：CCR4 == 2（测试脉宽）
        assert self.board.pwm_ccr4() == 2

    def test_injected_pwmin_prints_freq(self):
        # 前提：打开串口并清空；停 TIM8 免其 ISR 覆盖注入状态
        self.board.serial_open()
        self.board.serial_read_lines(0.3)
        self.board.stop_tim8()
        # 动作：注入一次 PWM 输入测量（psc=89 -> 90；hval=10 -> 5us；cval=20 -> 10us）
        self.board.inject_pwmin(psc=89, hval=10, cval=20)
        # 期望：打印 freq:100000
        lines = self.board.serial_read_lines(1.5)
        assert any("freq:100000" in ln for ln in lines), lines

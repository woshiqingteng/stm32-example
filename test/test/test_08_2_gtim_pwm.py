"""08_2_gtim_pwm: TIM3_CH4 breathing PWM on PB1 (duty ramps up then down)."""

import pytest


APP = "08_2_gtim_pwm"
pytestmark = pytest.mark.openedv_stm32f429


class Test08_2GtimPwm:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_ccr_register_ramps(self):
        # 前提：08_2 运行，duty 在 0..300 之间往返
        # 动作：采样 TIM3->CCR4 共约 5.6 s
        vals = []
        for _ in range(70):
            vals.append(self.board.pwm_ccr4())
            self.board.sleep(80)
        # 期望：数值先增后减（呼吸）
        assert max(vals) - min(vals) > 20, "duty should ramp"
        up = any(vals[i + 1] > vals[i] for i in range(len(vals) - 1))
        down = any(vals[i + 1] < vals[i] for i in range(len(vals) - 1))
        assert up and down, "expected ramp up then down"

    def test_pwm_configured(self):
        # 前提：08_2 运行
        # 动作：读取 CCR4 与 ARR
        # 期望：0 <= CCR4 <= ARR（合法占空比）
        ccr = self.board.pwm_ccr4()
        arr = self.board.pwm_arr()
        assert 0 <= ccr <= arr

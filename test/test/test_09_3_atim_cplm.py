"""09_3_atim_cplm: TIM1 complementary PWM + dead time on PE8/PE9."""

import pytest


APP = "09_3_atim_cplm"
pytestmark = pytest.mark.openedv_stm32f429


class Test09_3AtimCplm:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_deadtime_and_moe(self):
        # 前提：09_3 运行
        # 动作：读取 TIM1 的 BDTR
        # 期望：死区时间非零 且 主输出使能(MOE)
        bdtr = self.board.bdtr()
        assert (bdtr & 0xFF) != 0, "dead time should be non-zero"
        assert (bdtr & (1 << 15)) != 0, "MOE should be set"

    def test_pins_complementary(self):
        # 前提：09_3 运行，PE8(CH1N)/PE9(CH1) 互补输出
        # 动作：采样 GPIOE IDR 的 PE8/PE9
        seen = set()
        for _ in range(60):
            seen.add(self.board.gpio_read(self.board.GPIOE_BASE, 0x3, 8))
            self.board.sleep(10)
        # 期望：出现互补组合（0b01 或 0b10）
        assert seen & {0b01, 0b10}, "PE8/PE9 should be complementary, saw {}".format(
            seen
        )

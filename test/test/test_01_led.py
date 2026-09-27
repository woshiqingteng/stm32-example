"""01_led: alternate LED0/LED1."""

import pytest


APP = "01_led"
pytestmark = pytest.mark.openedv_stm32f429


class Test01Led:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_boot_state(self):
        # 前提：01_led 已上电
        # 动作：读取初始 LED 状态
        # 期望：LED0 亮、LED1 灭
        led0, led1 = self.board.led_states()
        assert led0 and not led1

    def test_both_leds_alternate(self):
        # 前提：01_led 运行中，两灯周期交替
        # 动作：采样 GPIOB ODR 2.6 s
        # 期望：出现 0x01 与 0x02 两个相位
        samples = set(self.board.sample_led_odr(2.6, 100))
        assert {0x01, 0x02} <= samples, samples

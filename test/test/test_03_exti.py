"""03_exti: four keys via external interrupts."""

import pytest


APP = "03_exti"
pytestmark = pytest.mark.openedv_stm32f429

EXPECTED = {
    "KEY0": 0x02,
    "KEY1": 0x00,
    "KEY2": 0x03,
    "WK_UP": 0x02,
}


class Test03Exti:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_boot_state(self):
        # 前提：03_exti 已上电
        # 动作：读取初始 LED 状态
        # 期望：ODR == 0x01
        assert self.board.led_odr() == 0x01

    @pytest.mark.parametrize("key,expected", list(EXPECTED.items()))
    def test_exti_key(self, key, expected):
        # 前提：LED0 亮、LED1 灭
        assert self.board.led_odr() == 0x01
        # 动作：注入按键边沿（按下->释放），触发 EXTI 回调
        self.board.tap(key, hold_ms=60, gap_ms=150)
        # 期望：ODR == expected
        assert self.board.led_odr() == expected, "after {} expected {:#x}".format(
            key, expected
        )

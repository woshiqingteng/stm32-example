"""02_key: four keys and their LED effects (polled)."""

import pytest


APP = "02_key"
pytestmark = pytest.mark.openedv_stm32f429

# Boot: LED0 on, LED1 off -> GPIOB ODR bits {LED1,LED0} = 0b01
EXPECTED = {
    "KEY0": 0x02,  # both toggle
    "KEY1": 0x00,  # LED1 toggle
    "KEY2": 0x03,  # LED0 toggle
    "WK_UP": 0x02,  # LED1 toggle + LED0 = !LED1
}


class Test02Key:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_boot_state(self):
        # 前提：02_key 已上电
        # 动作：读取初始 LED 状态
        # 期望：ODR == 0x01（LED0 亮、LED1 灭）
        assert self.board.led_odr() == 0x01

    @pytest.mark.parametrize("key,expected", list(EXPECTED.items()))
    def test_key(self, key, expected):
        # 前提：LED0 亮、LED1 灭
        assert self.board.led_odr() == 0x01
        # 动作：注入按下并释放按键 key
        self.board.tap(key)
        # 期望：ODR == expected
        assert self.board.led_odr() == expected, "after {} expected {:#x}".format(
            key, expected
        )

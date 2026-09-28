"""08_3_gtim_cap: TIM5 input capture (no external signal -> injected capture)."""

import pytest


APP = "08_3_gtim_cap"
pytestmark = pytest.mark.openedv_stm32f429


class Test08_3GtimCap:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_injected_capture_prints_high(self):
        # 前提：打开串口并清空
        self.board.serial_open()
        self.board.serial_read_lines(0.3)
        # 动作：注入一次捕获（value=1234）
        self.board.inject_capture(1234)
        # 期望：应用读取并打印 "HIGH:1.234 ms"（1234 us 自动换单位为 ms）
        lines = self.board.serial_read_lines(1.0)
        assert any("HIGH:1.234 ms" in ln for ln in lines), lines

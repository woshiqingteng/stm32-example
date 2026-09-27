"""08_4_gtim_cnt: TIM2 external counter (no external pulses -> injected count)."""

import pytest


APP = "08_4_gtim_cnt"
pytestmark = pytest.mark.openedv_stm32f429


class Test08_4GtimCnt:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_injected_count_printed(self):
        # 前提：打开串口并清空
        self.board.serial_open()
        self.board.serial_read_lines(0.3)
        # 动作：注入 TIM2 计数值 12345
        self.board.inject_counter(12345)
        # 期望：应用读到变化并打印 "CNT:12345"
        lines = self.board.serial_read_lines(1.0)
        assert any("CNT:12345" in ln for ln in lines), lines

    def test_key0_restart_clears(self):
        # 前提：注入计数 5000
        self.board.serial_open()
        self.board.serial_read_lines(0.3)
        self.board.inject_counter(5000)
        self.board.serial_read_lines(0.5)
        # 动作：注入 KEY0（重启计数）
        self.board.tap("KEY0")
        # 期望：打印 "CNT:0"
        lines = self.board.serial_read_lines(1.0)
        assert any("CNT:0" in ln for ln in lines), lines

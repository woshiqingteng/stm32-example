"""04_usart: line echo over USART1."""

import pytest


APP = "04_usart"
pytestmark = pytest.mark.openedv_stm32f429


class Test04Usart:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_echo_line(self):
        # 前提：04_usart 运行，串口已打开并清空启动横幅
        self.board.serial_open()
        self.board.serial_read_lines(0.3)
        # 动作：通过 COM 口发送一行 "abc" + CRLF
        self.board.serial_write("abc\r\n")
        # 期望：回显包含 "recv 3 bytes: abc"
        lines = self.board.serial_read_lines(2.0)
        assert any("recv 3 bytes: abc" in ln for ln in lines), lines

    def test_prompt(self):
        # 前提：04_usart 运行
        self.board.serial_open()
        # 动作：读取串口 2.5 s
        # 期望：出现周期性输入提示（CRLF 提示）
        lines = self.board.serial_read_lines(2.5)
        assert any("CRLF" in ln for ln in lines), lines

    def test_cr_without_lf_is_dropped(self):
        # 前提：04_usart 运行，串口已打开并清空启动横幅
        self.board.serial_open()
        self.board.serial_read_lines(0.3)
        # 动作：发送 "ab" + CR，随后跟一个非 LF 字节
        self.board.serial_write("ab\rX")
        # 期望：CR 后非 LF，整行丢弃，不回显 recv
        lines = self.board.serial_read_lines(1.0)
        assert not any("recv" in ln for ln in lines), lines

    def test_overlong_line_restarts(self):
        # 前提：04_usart 运行，串口已打开并清空启动横幅
        self.board.serial_open()
        self.board.serial_read_lines(0.3)
        # 动作：发送超过 LINE_MAX(199) 的一行，再发一行 "ok"
        self.board.serial_write("A" * 250 + "\r\n")
        self.board.serial_read_lines(1.0)
        self.board.serial_write("ok\r\n")
        # 期望：溢出后重新开始，后续 "ok" 行仍能正常回显
        lines = self.board.serial_read_lines(2.0)
        assert any("recv 2 bytes: ok" in ln for ln in lines), lines

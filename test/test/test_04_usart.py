"""04_usart: line echo over USART1."""
import pytest

pytestmark = pytest.mark.openedv_stm32f429


APP = "04_usart"


def test_echo_line(flashed):
    flashed.serial_open()
    flashed.serial_read_lines(0.3)  # drain banner
    flashed.serial_write("abc\r\n")
    lines = flashed.serial_read_lines(2.0)
    assert any("recv 3 bytes: abc" in ln for ln in lines), lines


def test_prompt(flashed):
    flashed.serial_open()
    lines = flashed.serial_read_lines(2.5)
    assert any("CRLF" in ln for ln in lines), lines

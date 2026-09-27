"""05_iwdg: independent watchdog fed by WK_UP; resets if starved."""

import pytest


APP = "05_iwdg"
pytestmark = pytest.mark.openedv_stm32f429


class Test05Iwdg:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_feed_prevents_reset(self):
        # 前提：清空复位标志
        self.board.clear_reset_flags()
        # 动作：周期性注入 WK_UP 喂狗（约 3 s）
        for _ in range(8):
            self.board.tap("WK_UP", hold_ms=40, gap_ms=150)
        # 期望：未发生 IWDG 复位
        flags = self.board.reset_flags()
        assert (flags & self.board.RCC_CSR_IWDGRSTF) == 0, (
            "unexpected IWDG reset while fed"
        )

    def test_starve_triggers_reset(self):
        # 前提：清空复位标志
        self.board.clear_reset_flags()
        # 动作：停止喂狗并等待 1.6 s（超过看门狗超时）
        self.board.sleep(1600)
        # 期望：发生 IWDG 复位
        flags = self.board.reset_flags()
        assert (flags & self.board.RCC_CSR_IWDGRSTF) != 0, (
            "expected IWDG reset when starved"
        )

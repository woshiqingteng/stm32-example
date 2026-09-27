"""10_tpad: capacitive touch (no finger -> injected baseline forces a touch)."""

import pytest


APP = "10_tpad"
pytestmark = pytest.mark.openedv_stm32f429


class Test10Tpad:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_led0_heartbeat(self):
        # 前提：10_tpad 运行（LED0 作为心跳）
        # 动作：采样 ODR 1 s
        # 期望：LED0(bit1) 出现两态
        samples = self.board.sample_led_odr(1.0, 50)
        assert len({(s >> 1) & 1 for s in samples}) == 2, "LED0 heartbeat should toggle"

    def test_injected_touch_toggles_led1(self):
        # 前提：10_tpad 运行
        # 动作：注入一次触摸（把基线置 0，使扫描超过 基线+门限）
        self.board.inject_touch()
        samples = self.board.sample_led_odr(1.5, 50)
        # 期望：LED1(bit0) 被翻转
        assert len({s & 0x01 for s in samples}) == 2, (
            "LED1 should toggle on injected touch"
        )

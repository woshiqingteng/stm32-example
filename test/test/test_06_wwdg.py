"""06_wwdg: window watchdog refreshed from its early-wakeup IRQ."""

import pytest


APP = "06_wwdg"
pytestmark = pytest.mark.openedv_stm32f429


class Test06Wwdg:
    @pytest.fixture(autouse=True)
    def setup_board(self, board):
        self.board = board
        self.board.flash_app(APP)
        yield board
        self.board.serial_close()

    def test_led1_toggles(self):
        # 前提：06_wwdg 运行，早唤醒中断周期翻 LED1
        # 动作：采样 ODR 2 s
        samples = self.board.sample_led_odr(2.0, 50)
        # 期望：LED1(bit0) 出现亮/灭两态
        assert len({s & 0x01 for s in samples}) == 2, "LED1 should toggle"

    def test_no_wwdg_reset(self):
        # 前提：06_wwdg 已被中断正常刷新
        # 动作：读取复位标志
        # 期望：无 WWDG 复位
        flags = self.board.reset_flags()
        assert (flags & self.board.RCC_CSR_WWDGRSTF) == 0, "unexpected WWDG reset"

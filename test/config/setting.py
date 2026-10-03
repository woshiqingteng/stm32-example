"""
Hardware parameters for the hardware-in-the-loop (HIL) test framework.

Only hardware settings live here (debugger selection, serial port, flash base).
Register/address references belong in ``config/stm32f429.py``.
"""

# --- Board -------------------------------------------------------------------
BOARD = "openedv_stm32f429"

# --- Debugger / programmer (OpenOCD) -----------------------------------------
OPENOCD = "openocd"
ADAPTER = "cmsis-dap"
# Serial number of the CMSIS-DAP probe to use. When several probes are
# connected, set this to the target probe's USB serial so OpenOCD issues
# `adapter serial <ADAPTER_SERIAL>`.
#
# NOTE: the on-board ATK CMSIS-DAP (VID_04D8/PID_00DF, composite S/N
# "ATK_20210914") does NOT expose a matchable USB serial to OpenOCD, so
# `adapter serial` fails for it; leave empty to auto-select the only probe.
ADAPTER_SERIAL = ""
TARGET_CFG = "target/stm32f4x.cfg"

# --- Target flash ------------------------------------------------------------
FLASH_ADDR = 0x08000000

# --- UART (target USART1 via the on-board USB-serial) ------------------------
SERIAL_PORT = "COM4"
SERIAL_BAUD = 115200

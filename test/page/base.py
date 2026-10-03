"""
BasePage: generic hardware access (OpenOCD connection/session, register
peek/poke, flashing, UART) shared by all board pages.
"""

from __future__ import annotations

import logging
import os
import platform
import socket
import subprocess
import threading
import time

import serial


class BasePage:
    """Generic OpenOCD + UART wrapper.

    The concrete board page (e.g. OpenEdvSTM32F429Page) subclasses this and
    adds chip/board specific helpers.
    """

    def __init__(self, setting):
        self.setting = setting
        self.log = logging.getLogger("hil.base")
        self.proc = None
        self.sock = None
        self.ser = None
        self.elf = None  # path to the current app ELF (set by the flash fixture)

    # -- OpenOCD ------------------------------------------------------------
    def _oc_args(self, extra):
        s = self.setting
        args = [
            s.OPENOCD,
            "-f",
            "interface/{}.cfg".format(s.ADAPTER),
        ]
        if getattr(s, "ADAPTER_SERIAL", ""):
            args += ["-c", "adapter serial {}".format(s.ADAPTER_SERIAL)]
        args += ["-f", s.TARGET_CFG]
        return args + list(extra)

    def connect(self, retries=2) -> bool:
        """Check that the CMSIS-DAP probe + target are reachable.

        On failure the adapter is software-reset (see reset_adapter) and the
        check is retried, which recovers a wedged probe without a re-plug.
        """
        for attempt in range(1, retries + 1):
            if self._probe_alive():
                return True
            if attempt < retries:
                self.reset_adapter()
        return False

    def _probe_alive(self) -> bool:
        """One-shot OpenOCD init/shutdown check of probe + target."""
        args = self._oc_args(["-c", "init", "-c", "shutdown"])
        self.log.info("connect: %s", " ".join(args))
        try:
            r = subprocess.run(args, capture_output=True, text=True, timeout=30)
        except Exception as exc:  # noqa: BLE001
            self.log.error("openocd exec failed: %s", exc)
            return False
        out = (r.stdout or "") + (r.stderr or "")
        self.log.debug("openocd output:\n%s", out)
        ok = ("Interface ready" in out) and ("DPIDR" in out)
        self.log.info("connect ok=%s", ok)
        return ok

    def _run_cmd(self, args, timeout=30) -> bool:
        try:
            r = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
        except Exception as exc:  # noqa: BLE001
            self.log.error("command failed: %s (%s)", args, exc)
            return False
        if r.returncode != 0:
            tail = ((r.stdout or "") + (r.stderr or "")).strip().splitlines()
            self.log.info("command rc=%d: %s", r.returncode, tail[-1] if tail else "")
        return r.returncode == 0

    def reset_adapter(self) -> bool:
        """Software-reset a wedged debug adapter (equivalent to a re-plug) by
        restarting its USB PnP device with ``pnputil /restart-device``.

        The instance id is ``setting.ADAPTER_INSTANCE``. A direct call works when
        the process is elevated; otherwise a single elevated (UAC) call is made.
        The device node is never force-removed (that detaches the probe until a
        reboot/re-plug).
        """
        if platform.system() != "Windows":
            return False
        inst = getattr(self.setting, "ADAPTER_INSTANCE", "")
        if not inst:
            return False

        sysroot = os.environ.get("SystemRoot", r"C:\Windows")
        pnputil = os.path.join(sysroot, "System32", "pnputil.exe")
        self.log.warning("resetting debug adapter: %s", inst)

        ok = self._run_cmd([pnputil, "/restart-device", inst])
        if not ok:
            self.log.info("direct pnputil reset failed; retrying elevated (UAC)")
            ps = os.path.join(
                sysroot, "System32", "WindowsPowerShell", "v1.0", "powershell.exe"
            )
            script = (
                "Start-Process -Verb RunAs -Wait -WindowStyle Hidden "
                "-FilePath '{p}' -ArgumentList '/restart-device','{i}'"
            ).format(p=pnputil, i=inst)
            ok = self._run_cmd([ps, "-NoProfile", "-Command", script], timeout=60)
        if ok:
            self.log.info("debug adapter reset ok; waiting for re-enumeration")
            time.sleep(4)
        else:
            self.log.error("debug adapter reset failed")
        return ok

    def start(self):
        """Start a persistent OpenOCD session and connect to its telnet port.

        OpenOCD's stdout is block-buffered when piped, so a background thread
        pumps it to the timestamped log while we poll the telnet port.
        """
        args = self._oc_args([])
        self.log.info("start openocd: %s", " ".join(args))

        self.proc = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        self._oc_thread = threading.Thread(
            target=self._pump_openocd, name="openocd-log", daemon=True
        )
        self._oc_thread.start()

        deadline = time.time() + 15
        self.sock = None
        last_err = None
        while time.time() < deadline:
            try:
                self.sock = socket.create_connection(("127.0.0.1", 4444), timeout=1)
                break
            except OSError as exc:
                last_err = exc
                if self.proc.poll() is not None:
                    break
                time.sleep(0.3)
        if self.sock is None:
            self._terminate_proc()
            raise RuntimeError("OpenOCD telnet not available: {}".format(last_err))

        self.sock.settimeout(5)
        self._read_prompt()
        self.cmd("init")

    def _pump_openocd(self):
        try:
            for line in self.proc.stdout:
                self.log.debug("openocd: %s", line.rstrip())
        except Exception:  # noqa: BLE001
            pass

    def _terminate_proc(self):
        if self.proc is not None:
            try:
                self.proc.terminate()
            except Exception:  # noqa: BLE001
                pass
            self.proc = None

    def _read_prompt(self, timeout=10):
        self.sock.settimeout(timeout)
        data = b""
        try:
            while not data.endswith(b"> "):
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                data += chunk
        except socket.timeout:
            pass
        return data.decode(errors="replace")

    def _drain(self):
        """Discard unsolicited OpenOCD notifications (e.g. reset detected)."""
        try:
            self.sock.setblocking(False)
            while True:
                data = self.sock.recv(4096)
                if not data:
                    break
        except (BlockingIOError, OSError):
            pass
        finally:
            self.sock.settimeout(5)

    def cmd(self, command, timeout=10):
        self._drain()
        self.log.debug("oc> %s", command)
        self.sock.sendall((command + "\n").encode())
        out = self._read_prompt(timeout)
        self.log.debug("oc< %s", out.strip())
        return out

    @staticmethod
    def _parse_mdw(out):
        for line in out.splitlines():
            line = line.strip().strip("\x00")
            if line.startswith("0x") and ":" in line:
                try:
                    return int(line.split(":")[1].split()[0], 16)
                except ValueError:
                    continue
        return None

    # -- memory -------------------------------------------------------------
    def peek(self, addr) -> int:
        for _ in range(4):
            out = self.cmd("mdw 0x{:08x} 1".format(addr))
            value = self._parse_mdw(out)
            if value is not None:
                return value
        raise RuntimeError("peek 0x{:08x} failed: {!r}".format(addr, out))

    def poke(self, addr, value):
        self.cmd("mww 0x{:08x} 0x{:08x}".format(addr, value))

    def poke16(self, addr, value):
        self.cmd("mwh 0x{:08x} 0x{:04x}".format(addr, value))

    def poke8(self, addr, value):
        self.cmd("mwb 0x{:08x} 0x{:02x}".format(addr, value))

    def symbol(self, name) -> int:
        """Address of a global/static symbol from the current app's ELF."""
        if not self.elf:
            raise RuntimeError("no ELF set for symbol lookup")
        r = subprocess.run(
            ["arm-none-eabi-nm", self.elf], capture_output=True, text=True, timeout=15
        )
        for line in r.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 3 and parts[-1] == name:
                return int(parts[0], 16)
        raise KeyError("symbol not found: {} (in {})".format(name, self.elf))

    # -- control ------------------------------------------------------------
    def program(self, binpath, retries=5):
        """Flash ``binpath``; on failure reset the target and, if the probe looks
        wedged (repeated failures), software-reset the adapter and reopen."""
        adapter_reset = False
        for attempt in range(1, retries + 1):
            try:
                self.cmd("reset halt", timeout=10)
            except Exception as exc:  # noqa: BLE001
                self.log.warning("pre-program reset halt failed: %s", exc)
            try:
                out = self.cmd(
                    "program {} 0x{:08x} verify".format(
                        binpath, self.setting.FLASH_ADDR
                    ),
                    timeout=60,
                )
            except Exception as exc:  # noqa: BLE001
                self.log.warning("program attempt %d raised: %s", attempt, exc)
                out = ""
            if "Verified OK" in out:
                self.reset_run()
                return
            self.log.warning(
                "program failed (attempt %d/%d), resetting target", attempt, retries
            )
            self.recover()
            if (attempt >= 2) and (not adapter_reset):
                adapter_reset = True
                if self.reset_adapter():
                    self.reopen()
            time.sleep(0.5)
        raise RuntimeError(
            "flash failed after {} attempts: {}".format(retries, binpath)
        )

    def reopen(self):
        """Close and re-open the persistent OpenOCD session (after an adapter
        reset the old USB handle is invalid)."""
        self.close()
        self.start()

    def recover(self):
        """Best-effort target recovery after a failed operation."""
        for cmd in ("reset halt", "reset init", "reset run"):
            try:
                self.cmd(cmd, timeout=10)
            except Exception as exc:  # noqa: BLE001
                self.log.warning("recover '%s' failed: %s", cmd, exc)
        time.sleep(0.5)

    def reset_run(self):
        self.cmd("reset run")

    def sleep(self, ms):
        time.sleep(ms / 1000.0)

    # -- UART ---------------------------------------------------------------
    def serial_open(self):
        self.ser = serial.Serial(
            self.setting.SERIAL_PORT, self.setting.SERIAL_BAUD, timeout=0.3
        )
        # The on-board USB-serial wires DTR/RTS to reset/boot: release them.
        self.ser.dtr = False
        self.ser.rts = False

    def serial_read_lines(self, seconds):
        if self.ser is None:
            self.serial_open()
        lines = []
        end = time.time() + seconds
        while time.time() < end:
            raw = self.ser.readline()
            if raw:
                lines.append(raw.decode(errors="replace").strip())
        return lines

    def serial_write(self, data):
        if self.ser is None:
            self.serial_open()
        if isinstance(data, str):
            data = data.encode()
        self.ser.write(data)

    def serial_close(self):
        if self.ser is not None:
            self.ser.close()
            self.ser = None

    # -- teardown -----------------------------------------------------------
    def close(self):
        self.serial_close()
        if self.sock is not None:
            try:
                self.cmd("shutdown", timeout=3)
            except Exception:  # noqa: BLE001
                pass
            self.sock.close()
            self.sock = None
        self._terminate_proc()

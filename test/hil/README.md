# HIL harness (hardware-in-the-loop)

Flash-and-observe checks for the FreeRTOS example apps (`app/freertos/*`) on a
real board. The harness keeps **one persistent OpenOCD session** for the whole
run (repeatedly spawning/killing OpenOCD wedges the CMSIS-DAP probe).

- **Display**: framebuffer OCR of the panel confirms the UI is drawn and checks
  the title/labels (portrait 480x800, framebuffer at `0xC0000000`).
- **Keys**: KEY0/KEY1/WKUP injected via the debugger (`page.tap`).
- **LED**: LED0 ODR sampled with the GPIOB clock forced on.
- **Low power**: `LTDC_GCR.LTDCEN` and the PB5 backlight are read back.
- **Serial**: UART keyword capture.
- **PC peers**: `pc_peer.py` provides the PC side of the lwIP protocols.

## Files

| file | purpose |
|---|---|
| `freertos_verify.py` | per-app orchestration: flash → OCR → KEY injection → serial → report (all `app/freertos/*` by default) |
| `hil_fb_ocr.py` | decode the panel framebuffer to text using `bsp/openedv_stm32f4/lcdfont.h` |
| `pc_peer.py` | PC-side UDP/TCP echo, broadcast/multicast listeners, HTTP client (lwIP end-to-end) |
| `freertos_report.md` | latest FreeRTOS-example verification results |

The harness uses the shared framework under `test/`:
`test/page/base.py` (`BasePage`: persistent OpenOCD + telnet :4444, `program`,
`peek/poke`, serial), `test/page/openedv_stm32f429.py` (`tap`, `led_odr`) and
`test/config/setting.py` (`SERIAL_PORT`, `ADAPTER_SERIAL`).

## Prerequisites

- `openocd` (`interface/cmsis-dap.cfg` + `target/stm32f4x.cfg`)
- serial capture helper (default `D:/app/msys2/tmp/opencode/capture_baud.py`, `COM4@115200`)
- `arm-none-eabi-nm`
- Board attached; a PC on the **same L2 segment/DHCP domain** for lwIP checks

## Usage

```
tool/hil.sh                    # all app/freertos/* apps
tool/hil.sh 16_event_group 18_tickless
python test/hil/freertos_verify.py 19_idle_hook

# lwIP end-to-end peers (board-initiated traffic):
python test/hil/pc_peer.py udp-echo 8080 --seconds 20
python test/hil/pc_peer.py tcp-echo 8080 --seconds 20
python test/hil/pc_peer.py udp-listen 8080 --seconds 20
python test/hil/pc_peer.py mcast-listen 224.0.1.0 8080 --seconds 20
python test/hil/pc_peer.py http <board-ip>
```

Do **not** call `terminate()`/kill OpenOCD between steps; let the single
`BasePage` session live for the whole run.

# lwIP demo HIL harness

Hardware-in-the-loop checks for the FreeRTOS lwIP demos (`app/freertos/lwip_*`):

- **Framebuffer OCR** of the panel: confirms the UI is drawn and verifies the
  text (ALIENTEK layout, portrait 480x800, framebuffer at `0xC0000000`).
- **KEY0 injection** (PH3, active low) to check the send trigger.
- **PC peers** for the end-to-end protocols (UDP/TCP echo, broadcast, multicast,
  HTTP).

## Files

| file | purpose |
|---|---|
| `hil_fb_ocr.py` | decode the panel framebuffer to text using `bsp/openedv_stm32f4/lcdfont.h` |
| `hil_key.py` | inject KEY0 via OpenOCD and read `g_lwip_send_flag` |
| `pc_peer.py` | PC-side UDP/TCP echo, broadcast/multicast listeners, HTTP client |
| `hil_run.py` | per-app orchestration: flash → serial → framebuffer OCR → report |

## Prerequisites

- `openocd` with `interface/cmsis-dap.cfg` + `target/stm32f4x.cfg`
- serial capture helper (default `D:/app/msys2/tmp/opencode/capture_baud.py`, `COM4@115200`)
- `arm-none-eabi-nm`
- The board and the PC on the **same L2 segment/DHCP domain** for end-to-end

## Usage

```
python test/hil/hil_run.py lwip_6_freertos
python test/hil/hil_run.py all
python test/hil/pc_peer.py udp-echo 8080 --seconds 20     # for the UDP peers
python test/hil/pc_peer.py tcp-echo 8080 --seconds 20     # for the TCP servers
python test/hil/pc_peer.py udp-listen 8080 --seconds 20   # board broadcast (10-1)
python test/hil/pc_peer.py mcast-listen 224.0.1.0 8080 --seconds 20
python test/hil/pc_peer.py http <board-ip>
python test/hil/hil_key.py build/debug/openedv_stm32f4/freertos/lwip_7_netconn_udp/lwip_7_netconn_udp.elf
```

Outputs land under `HIL_OUT` (default `D:/app/msys2/tmp/opencode/hil/<app>/`).

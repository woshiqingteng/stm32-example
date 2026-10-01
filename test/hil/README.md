# HIL harness (hardware-in-the-loop)

Flash-and-observe checks for the example apps on a real board. The harness keeps
**one persistent OpenOCD session** for the whole run (repeatedly spawning/killing
OpenOCD wedges the CMSIS-DAP probe).

The shared machinery lives in `hil_common.py`; each category has its own
verifier that defines the per-app checks:

| file | category | default apps |
|---|---|---|
| `freertos_verify.py` | FreeRTOS kernel demos | `app/freertos/0x..20` |
| `lvgl_verify.py` | LVGL demos (A class + SD/NOR B class) | `app/freertos/lvgl_*` |
| `lwip_verify.py` | (future) lwIP demos | `app/freertos/lwip_*` |

Shared files: `hil_common.py` (session, framebuffer OCR, key injection, LED,
runner), `hil_fb_ocr.py` (framebuffer -> text via `bsp/openedv_stm32f4/lcdfont.h`)
and `pc_peer.py` (PC-side lwIP peers). Reports: `freertos_report.md`,
`lvgl_report.md`.

Checks used: framebuffer OCR / pixel probes, KEY0/KEY1/WKUP injection (`tap`),
LED0 ODR sampling (GPIOB clock forced on), LTDC/backlight state, serial keyword
capture.

## Prerequisites

- `openocd` (`interface/cmsis-dap.cfg` + `target/stm32f4x.cfg`)
- serial capture helper (default `D:/app/msys2/tmp/opencode/capture_baud.py`, `COM4@115200`)
- `arm-none-eabi-nm`
- Board attached; for the LVGL B-class apps an SD card with the ALIENTEK
  `LVGL实验所需SD卡文件` assets and a pre-programmed SPI-NOR (XBF font / image
  store).

## Usage

```
tool/hil.sh                        # all FreeRTOS kernel demos
tool/hil.sh lvgl                   # all LVGL demos
tool/hil.sh lvgl lvgl_10_arc
tool/hil.sh 16_event_group         # category defaults to freertos
```

Do **not** call `terminate()`/kill OpenOCD between steps; let the single
`hil_common` session live for the whole run.

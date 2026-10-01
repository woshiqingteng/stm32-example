# LVGL port - hardware verification (48/48 PASS)

Harness: `test/hil/lvgl_verify.py` (shared `hil_common.py` + the project
`test/page` framework, one persistent OpenOCD session). SD card populated from
`LVGL实验所需SD卡文件` (+ `PICTURE/LVGLBIN/*.BIN` substitute images).

| app | result | detail |
|---|---|---|
| lvgl_02_stress | PASS | drawn, no fault, LED |
| lvgl_04_mouse | PASS | drawn, no fault, LED |
| lvgl_05_fs | PASS | SD file read (bg green) |
| lvgl_06_font | PASS | drawn, no fault, LED |
| lvgl_07_xbf_font | PASS | XBF Font12 store updated from SD, label drawn |
| lvgl_09..39 (widgets) | PASS | drawn, no fault, LED |
| lvgl_40_img_lib | PASS | image store loaded from SPI-NOR, images drawn |
| lvgl_41_bmp | PASS | BMP drawn from SD |
| lvgl_42_png | PASS | PNG drawn from SD |
| lvgl_43_gif | PASS | GIF animates |
| lvgl_44_qrcode | PASS | drawn, no fault, LED |
| lvgl_45_jpeg | PASS | SJPG drawn from SD |
| lvgl_47_calculator | PASS | drawn, no fault, LED |
| lvgl_49_qrgen | PASS | drawn, no fault, LED |
| lvgl_50_paint | PASS | drawn, no fault, LED |
| lvgl_51_filemgr | PASS | SD list drawn |
| lvgl_52_baseconv | PASS | drawn, no fault, LED |
| lvgl_demo_benchmark | PASS | drawn, no fault, LED |
| lvgl_demo_widgets | PASS | drawn, no fault, LED |

## Notes
- `lvgl_05_fs`: the reference prints the read result on the UART only; the port
  additionally shows the result as the screen background (green = read OK) so
  the HIL can assert it by pixel (the UART one-shot line is unreliable here).
- `lvgl_43_gif`: `has_next` is set so the demo lets LVGL's `lv_gif` loop the
  animation instead of recreating it every 10 ms.
- `lvgl_07/40`: the SD -> SPI-NOR update must run **after** `lv_port_disp_init()`
  (it draws progress via the BSP LCD, which is not initialised before that).
- `lvgl_40_img_lib`: `images_init()` now calls `nor_init()`; previously nothing
  initialised the SPI-NOR bus, so the store was never readable/writable.
- `lvgl_07_xbf_font`: `fonts_init()` succeeds on the board's factory GBK store
  (which has no LVGL XBF font), so the update was skipped. `fonts_lvgl_ok()`
  validates the Font12 XBF header and forces a one-time update when missing.

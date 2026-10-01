# LVGL port - hardware verification (46/48 PASS)

Harness: `test/hil/lvgl_verify.py` (shared `hil_common.py` + the project
`test/page` framework, one persistent OpenOCD session). SD card populated from
`LVGL实验所需SD卡文件` (+ `PICTURE/LVGLBIN/*.BIN` substitute images). `07`/`40`
need the SPI-NOR font/image store.

| app | result | detail |
|---|---|---|
| lvgl_02_stress | PASS | drawn, no fault, LED |
| lvgl_04_mouse | PASS | drawn, no fault, LED |
| lvgl_05_fs | PASS | SD file read (bg green) |
| lvgl_06_font | PASS | drawn, no fault, LED |
| lvgl_07_xbf_font | FAIL | XBF font store absent in SPI-NOR (no glyph drawn) |
| lvgl_09..39 (widgets) | PASS | drawn, no fault, LED |
| lvgl_40_img_lib | FAIL | image store not produced (NOR auto-update) |
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
- The font/image update (SD -> SPI-NOR) must run **after**
  `lv_port_disp_init()`: it draws progress via the BSP LCD, which is not
  initialised before that (created a bus fault).
- `lvgl_07` / `lvgl_40` still require a populated SPI-NOR store. The auto-update
  from SD runs but the store is not read back correctly on this board; needs
  further work on the vendor font/image store format.

# LVGL port - hardware verification

All LVGL apps run on hardware with `tool/hil.sh lvgl`. The official benchmark takes ~100 s and is measured on demand with `tool/hil.sh lvgl benchmark`, which records its FPS results in the benchmark section below.

| app | result | detail |
|---|---|---|
| lvgl_02_stress | PASS | lcd=''/'' drawn=1794 fault=False led=True |
| lvgl_04_mouse | PASS | lcd=''/'' drawn=1794 fault=False led=True |
| lvgl_05_fs | PASS | lcd=''/'' bg=07e0 fault=False |
| lvgl_06_font | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_07_xbf_font | PASS | lcd=''/'' content=29 bg=f7be fault=False |
| lvgl_09_obj | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_10_arc | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_11_bar | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_12_btn | PASS | lcd=''/'' drawn=3584 fault=False led=True |
| lvgl_13_btnmatrix | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_14_canvas | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_15_checkbox | PASS | lcd=''/'' drawn=1814 fault=False led=True |
| lvgl_16_dropdown | PASS | lcd=''/'' drawn=3253 fault=False led=True |
| lvgl_17_img | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_18_label | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_19_line | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_20_roller | PASS | lcd=''/'' drawn=3325 fault=False led=True |
| lvgl_21_slider | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_22_switch | PASS | lcd=''/'' drawn=2983 fault=False led=True |
| lvgl_23_table | PASS | lcd=''/'' drawn=2395 fault=False led=True |
| lvgl_24_textarea | PASS | lcd=''/'' drawn=2540 fault=False led=True |
| lvgl_25_calendar | PASS | lcd=''/'' drawn=2197 fault=False led=True |
| lvgl_26_chart | PASS | lcd=''/'' drawn=640 fault=False led=True |
| lvgl_27_colorwheel | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_28_imgbtn | PASS | lcd=''/'' drawn=2142 fault=False led=True |
| lvgl_29_keyboard | PASS | lcd=''/'' drawn=1239 fault=False led=True |
| lvgl_30_led | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_31_list | PASS | lcd=''/'' drawn=577 fault=False led=True |
| lvgl_32_meter | PASS | lcd=''/'' drawn=3110 fault=False led=True |
| lvgl_33_msgbox | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_34_span | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_35_spinbox | PASS | lcd=''/'' drawn=3517 fault=False led=True |
| lvgl_36_spinner | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_37_tabview | PASS | lcd=''/'' drawn=557 fault=False led=True |
| lvgl_38_tileview | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_39_win | PASS | lcd=''/'' drawn=2571 fault=False led=True |
| lvgl_40_img_lib | PASS | lcd=''/'' content=2003 bg=0000 fault=False |
| lvgl_41_bmp | PASS | lcd=''/'' content=140 bg=24be fault=False |
| lvgl_42_png | PASS | lcd=''/'' content=17 bg=f7be fault=False |
| lvgl_43_gif | PASS | lcd=''/'' changed=1 fault=False |
| lvgl_44_qrcode | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_45_jpeg | PASS | lcd=''/'' content=32 bg=24be fault=False |
| lvgl_47_calculator | PASS | lcd=''/'' drawn=2534 fault=False led=True |
| lvgl_49_qrgen | PASS | lcd=''/'' drawn=2098 fault=False led=True |
| lvgl_50_paint | PASS | lcd=''/'' drawn=104 fault=False led=True |
| lvgl_51_filemgr | PASS | lcd=''/'' content=2006 bg=9cf3 fault=False |
| lvgl_52_baseconv | PASS | lcd=''/'' drawn=361 fault=False led=True |
| lvgl_53_comprehensive | PASS | lcd=''/'' drawn=3318 fault=False led=True |
| lvgl_demo_widgets | PASS | lcd=''/'' drawn=1059 fault=False led=True |

<!-- BENCHMARK:BEGIN -->

## Benchmark (`lvgl_demo_benchmark`)

Board: ALIENTEK Apollo STM32F429, 480x800 RGB565 (LTDC), SYSCLK 180 MHz, LVGL 8.3.11. Run: 2026-10-01.

`LVGL v8.3.11  Benchmark (in csv format)`

- Weighted FPS: **21**
- Opa. speed: **90%**

| scene | FPS |
|---|---|
| Rectangle | 30 |
| Rectangle + opa | 12 |
| Rectangle rounded | 25 |
| Rectangle rounded + opa | 11 |
| Circle | 12 |
| Circle + opa | 5 |
| Border | 32 |
| Border + opa | 30 |
| Border rounded | 29 |
| Border rounded + opa | 27 |
| Circle border | 10 |
| Circle border + opa | 8 |
| Border top | 31 |
| Border top + opa | 30 |
| Border left | 31 |
| Border left + opa | 31 |
| Border top + left | 30 |
| Border top + left + opa | 28 |
| Border left + right | 29 |
| Border left + right + opa | 28 |
| Border top + bottom | 30 |
| Border top + bottom + opa | 28 |
| Shadow small | 8 |
| Shadow small + opa | 7 |
| Shadow small offset | 8 |
| Shadow small offset + opa | 6 |
| Shadow large | 2 |
| Shadow large + opa | 2 |
| Shadow large offset | 3 |
| Shadow large offset + opa | 2 |
| Image RGB | 45 |
| Image RGB + opa | 25 |
| Image ARGB | 20 |
| Image ARGB + opa | 16 |
| Image chorma keyed | 21 |
| Image chorma keyed + opa | 16 |
| Image indexed | 8 |
| Image indexed + opa | 7 |
| Image alpha only | 10 |
| Image alpha only + opa | 9 |
| Image RGB recolor | 12 |
| Image RGB recolor + opa | 12 |
| Image ARGB recolor | 12 |
| Image ARGB recolor + opa | 11 |
| Image chorma keyed recolor | 13 |
| Image chorma keyed recolor + opa | 11 |
| Image indexed recolor | 6 |
| Image indexed recolor + opa | 5 |
| Image RGB rotate | 13 |
| Image RGB rotate + opa | 9 |
| Image RGB rotate anti aliased | 7 |
| Image RGB rotate anti aliased + opa | 5 |
| Image ARGB rotate | 13 |
| Image ARGB rotate + opa | 10 |
| Image ARGB rotate anti aliased | 6 |
| Image ARGB rotate anti aliased + opa | 6 |
| Image RGB zoom | 19 |
| Image RGB zoom + opa | 13 |
| Image RGB zoom anti aliased | 10 |
| Image RGB zoom anti aliased + opa | 8 |
| Image ARGB zoom | 19 |
| Image ARGB zoom + opa | 15 |
| Image ARGB zoom anti aliased | 9 |
| Image ARGB zoom anti aliased + opa | 8 |
| Text small | 10 |
| Text small + opa | 10 |
| Text medium | 10 |
| Text medium + opa | 10 |
| Text large | 10 |
| Text large + opa | 10 |
| Text small compressed | 16 |
| Text small compressed + opa | 17 |
| Text medium compressed | 14 |
| Text medium compressed + opa | 14 |
| Text large compressed | 13 |
| Text large compressed + opa | 13 |
| Line | 23 |
| Line + opa | 23 |
| Arc think | 18 |
| Arc think + opa | 18 |
| Arc thick | 18 |
| Arc thick + opa | 17 |
| Substr. rectangle | 12 |
| Substr. rectangle + opa | 37 |
| Substr. border | 36 |
| Substr. border + opa | 37 |
| Substr. shadow | 34 |
| Substr. shadow + opa | 33 |
| Substr. image | 68 |
| Substr. image + opa | 68 |
| Substr. line | 60 |
| Substr. line + opa | 61 |
| Substr. arc | 16 |
| Substr. arc + opa | 16 |
| Substr. text | 33 |
| Substr. text + opa | 33 |

<!-- BENCHMARK:END -->

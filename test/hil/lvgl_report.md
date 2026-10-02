# LVGL port - hardware verification

All LVGL apps run on hardware with `tool/hil.sh lvgl`. The official benchmark takes ~100 s and is measured on demand with `tool/hil.sh lvgl benchmark`, which records its FPS results in the benchmark section below.

| app | result | detail |
|---|---|---|
| lvgl_02_stress | PASS | lcd=''/'' drawn=1794 fault=False led=True |
| lvgl_04_mouse | PASS | tap(400,240) changed=39027 fault=False |
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
| lvgl_21_slider | PASS | tap(300,240) changed=2761 fault=False |
| lvgl_22_switch | PASS | lcd=''/'' drawn=2983 fault=False led=True |
| lvgl_23_table | PASS | lcd=''/'' drawn=2395 fault=False led=True |
| lvgl_24_textarea | PASS | lcd=''/'' drawn=2540 fault=False led=True |
| lvgl_25_calendar | PASS | lcd=''/'' drawn=2197 fault=False led=True |
| lvgl_26_chart | PASS | lcd=''/'' drawn=640 fault=False led=True |
| lvgl_27_colorwheel | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_28_imgbtn | PASS | lcd=''/'' drawn=2142 fault=False led=True |
| lvgl_29_keyboard | PASS | tap(40,450) changed=79488 fault=False |
| lvgl_30_led | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_31_list | PASS | lcd=''/'' drawn=577 fault=False led=True |
| lvgl_32_meter | PASS | lcd=''/'' drawn=3103 fault=False led=True |
| lvgl_33_msgbox | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_34_span | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_35_spinbox | PASS | lcd=''/'' drawn=3517 fault=False led=True |
| lvgl_36_spinner | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_37_tabview | PASS | tap(400,40) changed=47012 fault=False |
| lvgl_38_tileview | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_39_win | PASS | lcd=''/'' drawn=2571 fault=False led=True |
| lvgl_40_img_lib | PASS | lcd=''/'' content=11911 bg=0000 fault=False |
| lvgl_41_bmp | PASS | lcd=''/'' content=660 bg=24be fault=False |
| lvgl_42_png | PASS | lcd=''/'' content=97 bg=f7be fault=False |
| lvgl_43_gif | PASS | lcd=''/'' changed=1 fault=False |
| lvgl_44_qrcode | PASS | lcd=''/'' drawn=3588 fault=False led=True |
| lvgl_45_jpeg | PASS | lcd=''/'' content=234 bg=24be fault=False |
| lvgl_47_calculator | PASS | tap(400,360) changed=578 fault=False |
| lvgl_49_qrgen | PASS | lcd=''/'' drawn=2098 fault=False led=True |
| lvgl_50_paint | PASS | lcd=''/'' drawn=104 fault=False led=True |
| lvgl_51_filemgr | PASS | lcd=''/'' content=10750 bg=9cf3 fault=False |
| lvgl_52_baseconv | PASS | lcd=''/'' drawn=361 fault=False led=True |
| lvgl_53_comprehensive | PASS | lcd=''/'' drawn=3318 fault=False led=True |
| lvgl_demo_widgets | PASS | lcd=''/'' drawn=1059 fault=False led=True |

<!-- BENCHMARK:BEGIN -->

## Benchmark (`lvgl_demo_benchmark`)

Board: ALIENTEK Apollo STM32F429, 480x800 RGB565 (LTDC), SYSCLK 180 MHz, LVGL 8.3.11. Run: 2026-10-02.

`LVGL v8.3.11  Benchmark (in csv format)`

- Weighted FPS: **23**
- Opa. speed: **95%**

| scene | FPS |
|---|---|
| Rectangle | 30 |
| Rectangle + opa | 22 |
| Rectangle rounded | 27 |
| Rectangle rounded + opa | 21 |
| Circle | 14 |
| Circle + opa | 14 |
| Border | 33 |
| Border + opa | 32 |
| Border rounded | 31 |
| Border rounded + opa | 29 |
| Circle border | 10 |
| Circle border + opa | 10 |
| Border top | 32 |
| Border top + opa | 32 |
| Border left | 32 |
| Border left + opa | 31 |
| Border top + left | 31 |
| Border top + left + opa | 30 |
| Border left + right | 31 |
| Border left + right + opa | 29 |
| Border top + bottom | 31 |
| Border top + bottom + opa | 30 |
| Shadow small | 10 |
| Shadow small + opa | 10 |
| Shadow small offset | 10 |
| Shadow small offset + opa | 10 |
| Shadow large | 3 |
| Shadow large + opa | 3 |
| Shadow large offset | 3 |
| Shadow large offset + opa | 3 |
| Image RGB | 59 |
| Image RGB + opa | 53 |
| Image ARGB | 20 |
| Image ARGB + opa | 16 |
| Image chorma keyed | 21 |
| Image chorma keyed + opa | 16 |
| Image indexed | 8 |
| Image indexed + opa | 7 |
| Image alpha only | 10 |
| Image alpha only + opa | 9 |
| Image RGB recolor | 19 |
| Image RGB recolor + opa | 18 |
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
| Text small | 12 |
| Text small + opa | 12 |
| Text medium | 12 |
| Text medium + opa | 12 |
| Text large | 12 |
| Text large + opa | 12 |
| Text small compressed | 9 |
| Text small compressed + opa | 9 |
| Text medium compressed | 7 |
| Text medium compressed + opa | 7 |
| Text large compressed | 4 |
| Text large compressed + opa | 4 |
| Line | 28 |
| Line + opa | 28 |
| Arc think | 20 |
| Arc think + opa | 20 |
| Arc thick | 21 |
| Arc thick + opa | 21 |
| Substr. rectangle | 27 |
| Substr. rectangle + opa | 37 |
| Substr. border | 37 |
| Substr. border + opa | 37 |
| Substr. shadow | 34 |
| Substr. shadow + opa | 34 |
| Substr. image | 69 |
| Substr. image + opa | 70 |
| Substr. line | 58 |
| Substr. line + opa | 59 |
| Substr. arc | 17 |
| Substr. arc + opa | 17 |
| Substr. text | 34 |
| Substr. text + opa | 34 |

<!-- BENCHMARK:END -->

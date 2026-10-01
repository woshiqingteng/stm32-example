# LVGL port - hardware verification

All LVGL apps run on hardware with `tool/hil.sh lvgl`. The official benchmark takes ~100 s and is measured on demand with `tool/hil.sh lvgl benchmark`, which records its FPS results in the benchmark section below.

| app | result | detail |
|---|---|---|
| lvgl_06_font | FAIL | lcd=''/'' drawn=550 fault=False led=False |
| lvgl_14_canvas | FAIL | lcd=''/'' drawn=550 fault=False led=False |
| lvgl_25_calendar | FAIL | lcd=''/'' drawn=550 fault=False led=False |
| lvgl_32_meter | FAIL | lcd=''/'' drawn=550 fault=False led=False |
| lvgl_47_calculator | FAIL | lcd=''/'' drawn=550 fault=False led=False |
| lvgl_51_filemgr | PASS | lcd=''/'' content=262 bg=ffff fault=False |
| lvgl_52_baseconv | FAIL | lcd=''/'' drawn=550 fault=False led=False |

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

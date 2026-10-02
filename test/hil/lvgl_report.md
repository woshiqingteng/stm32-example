# LVGL port - hardware verification

All LVGL apps run on hardware with `tool/hil.sh lvgl`. The official benchmark takes ~100 s and is measured on demand with `tool/hil.sh lvgl benchmark`, which records its FPS results in the benchmark section below.

| app | result | detail |
|---|---|---|
| lvgl_29_keyboard | PASS | tap(40,450) changed=79488 fault=False |

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

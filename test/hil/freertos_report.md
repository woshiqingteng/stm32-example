# FreeRTOS examples port — HIL verification

Harness: `test/hil/freertos_verify.py` on the project HIL framework
(`page/base.py` BasePage + `page/openedv_stm32f429.py`). One persistent OpenOCD
session for the whole run (the CMSIS-DAP probe wedges if re-initialised per
command). KEY0=PH3, KEY1=PH2, WK_UP=PA0 via `page.tap`; LCD via framebuffer OCR
and pixel probes; LED0=PB1 read with the GPIOB clock forced on (RCC AHB1ENR).

## Phenomenon verification (25/25 PASS)

| app | visible phenomenon / evidence |
|---|---|
| 02_freertos_port | LCD colour cycle + LED0 toggle + serial `float_num` |
| 04_interrupt | serial `max(tim3-tim6)=4` (TIM3 keeps counting, TIM6 masked) |
| 06_1_task_create_dynamic | left counter runs then stops after KEY0 |
| 06_2_task_create_static | same (static tasks) |
| 06_3_task_suspend_resume | KEY0 pauses / KEY1 resumes the counter |
| 07_list_item | serial list ops after 6× KEY0 (`list`) |
| 09_time_slicing | serial `run count` (round-robin) |
| 11_1_task_status_info | serial `finished` after 3× KEY0 |
| 11_2_run_time_stats | serial `runtime` after KEY0 |
| 13_1_queue | KEY0 fill, KEY1 LED0 |
| 13_2_queue_set | WK_UP/KEY1/KEY0 → serial `queue` |
| 13_3_queue_set_event_flags | event value 1 → fill |
| 14_1_binary_semaphore | KEY0 fill |
| 14_2_counting_semaphore | KEY0 fill |
| 14_3_priority_inversion | serial `running` |
| 14_4_mutex | serial `mutex` |
| 15_software_timer | KEY0 → Timer1 increments |
| 16_event_group | event value 1 → fill |
| 17_1_notify_binary_sem | KEY0 fill |
| 17_2_notify_counting_sem | KEY0 |
| 17_3_notify_mailbox | KEY0 fill, KEY1 LED0 |
| 17_4_notify_event_group | event value 1 → fill |
| 18_tickless | **LCD blanked (LTDC off, backlight off) + LED0 toggles (idle=on)** |
| 19_idle_hook | **LCD blanked (LTDC off, backlight off) + LED0 toggles (idle=on)** |
| 20_memory | KEY0 shows the heap address |

## Changes

### LED polarity + blanking for 18/19 (match the ALIENTEK reference)
- LCD API: `lcd_display_on()` / `lcd_display_off()` in
  `bsp/openedv_stm32f4/lcd.{c,h}` — `off` disables the LTDC controller **and**
  the PB5 backlight, `on` re-enables both.
- `18_tickless` / `19_idle_hook`: a single `lcd_display_off()` at start (panel
  dark) and LED0 busy=off / idle=on (reference `LED0(1)`/`LED0(0)`).
- HIL asserts `LTDC_GCR.LTDCEN == 0`, PB5 backlight `== 0`, and LED0 toggling.

### Earlier fixes (this port)
- `delay_us/ms` are pure SysTick busy-waits (no scheduler API) so they are safe
  with interrupts masked; `04_interrupt` uses `delay_ms(5000)` directly.
- FreeRTOS/LVGL common config lives in the port; `add_freertos_app` links
  `freertos freertos_port` directly (the port force-references
  `vApplicationStackOverflowHook`; `configCHECK_FOR_STACK_OVERFLOW = 2`).
  printf/snprintf tasks use 256-word stacks. No hook fired in any app.

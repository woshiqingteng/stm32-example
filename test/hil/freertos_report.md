# FreeRTOS examples port — HIL verification

Harness: `test/hil/freertos_verify.py` on the project HIL framework
(`test/page/base.py`, `page/openedv_stm32f429.py`). One **persistent OpenOCD
session** for the whole run (the CMSIS-DAP probe wedges if re-initialised per
command — see PLAN.md). KEY0=PH3, KEY1=PH2, WK_UP=PA0 via `page.tap`; LCD via
framebuffer OCR; LED0=PB1 read with the GPIOB clock forced on (RCC AHB1ENR).

## Result (Phase 2, after fixes) — 25/25 PASS

| app | evidence |
|---|---|
| 02_freertos_port | serial `float_num`, LCD |
| 04_interrupt | `max(tim3-tim6)=4` → TIM3 keeps counting, TIM6 masked in the window |
| 06_1_task_create_dynamic | left counter stops after KEY0 |
| 06_2_task_create_static | left counter stops after KEY0 |
| 06_3_task_suspend_resume | KEY0 pauses / KEY1 resumes |
| 07_list_item | serial `list` after 6× KEY0 |
| 09_time_slicing | serial `run count` |
| 11_1_task_status_info | serial `finished` after 3× KEY0 |
| 11_2_run_time_stats | serial `runtime` after KEY0 |
| 13_1_queue | KEY0 fill, KEY1 LED |
| 13_2_queue_set | WK_UP/KEY1/KEY0, serial `queue` |
| 13_3_queue_set_event_flags | event 1 → fill |
| 14_1_binary_semaphore | KEY0 fill |
| 14_2_counting_semaphore | KEY0 fill |
| 14_3_priority_inversion | serial `running` |
| 14_4_mutex | serial `mutex` |
| 15_software_timer | KEY0, Timer1 increments |
| 16_event_group | event 1 → fill |
| 17_1_notify_binary_sem | KEY0 fill |
| 17_2_notify_counting_sem | KEY0 |
| 17_3_notify_mailbox | KEY0 fill, KEY1 LED |
| 17_4_notify_event_group | event 1 → fill |
| 18_tickless | LED0 toggles (states 0/1), alive |
| 19_idle_hook | LED0 toggles (states 0/1), alive |
| 20_memory | KEY0 addr `0x200007c0` |

Phase 0 (before fixes) was 24/25 with 04 flagged; the fixes below resolved it.

## Fixes / refactor in this round
1. **04_interrupt / 18_tickless / 19_idle_hook**: the masked/busy phase now uses
   a scheduler-independent `delay_us()` busy wait — `delay_ms()` maps to
   `vTaskDelay()` under FreeRTOS, which is invalid with the tick/PendSV masked.
2. **Task stacks**: tasks calling `printf/snprintf` raised from 128 to 256 words.
3. **FreeRTOS config into the port** (lwIP style):
   `port/openedv_stm32f4/freertos/FreeRTOSConfig_common.h` (moved out of the
   module); the port injects its include into `freertos`.
   `lib_wrapper(lib_freertos freertos freertos_port)` with
   `vApplicationStackOverflowHook` in `port/.../freertos/vApplicationHooks.c`
   (`configCHECK_FOR_STACK_OVERFLOW = 2`). `add_freertos_app` links `lib_freertos`.
4. **LVGL config into the port**: `lv_conf_common.h` moved to
   `port/openedv_stm32f4/lvgl/` (`lvgl_tick.*` stays in the module).

No stack-overflow hook fired in any of the 25 apps.

# FreeRTOS examples port — HIL verification

Harness: `test/hil/freertos_verify.py`, built on the project HIL framework
(`test/page/base.py` BasePage + `page/openedv_stm32f429.py`). It uses **one
persistent OpenOCD session** for the whole run (flash → reset → memory →
KEY injection), because repeatedly re-initialising the CMSIS-DAP probe wedges
its firmware (documented in PLAN.md: "反复强杀 openocd …").

KEY injection: KEY0 = PH3, KEY1 = PH2, WK_UP = PA0 (via `page.tap`).
LCD via framebuffer OCR (`hil_fb_ocr`); LED0 = PB1 (GPIOB ODR, GPIOB clock
forced on through RCC AHB1ENR because the idle hook / tickless gate it).

## Phase 0 result (current code)

| app | result | evidence |
|---|---|---|
| 02_freertos_port | PASS | serial `float_num`, LCD |
| 04_interrupt | *suspect* | boots + prints `tim3=`, but the interrupt-mask window is unreliable (see below) |
| 06_1_task_create_dynamic | PASS | left counter stops after KEY0 |
| 06_2_task_create_static | PASS | left counter stops after KEY0 |
| 06_3_task_suspend_resume | PASS | KEY0 pauses / KEY1 resumes |
| 07_list_item | PASS | 6× KEY0, serial `list` |
| 09_time_slicing | PASS | serial `run count` |
| 11_1_task_status_info | PASS | 3× KEY0, serial `finished` |
| 11_2_run_time_stats | PASS | KEY0, serial `runtime` |
| 13_1_queue | PASS | KEY0 fill, KEY1 LED |
| 13_2_queue_set | PASS | WK_UP/KEY1/KEY0, serial `queue` |
| 13_3_queue_set_event_flags | PASS | event 1 → fill |
| 14_1_binary_semaphore | PASS | KEY0 fill |
| 14_2_counting_semaphore | PASS | KEY0 fill |
| 14_3_priority_inversion | PASS | serial `running` |
| 14_4_mutex | PASS | serial `mutex` |
| 15_software_timer | PASS | KEY0, Timer1 increments |
| 16_event_group | PASS | event 1 → fill |
| 17_1_notify_binary_sem | PASS | KEY0 fill |
| 17_2_notify_counting_sem | PASS | KEY0 |
| 17_3_notify_mailbox | PASS | KEY0 fill, KEY1 LED |
| 17_4_notify_event_group | PASS | event 1 → fill |
| 18_tickless | PASS | LED0 toggles, alive |
| 19_idle_hook | PASS | LED0 toggles, alive |
| 20_memory | PASS | KEY0 addr `0x20000460` |

## Phase 0 findings to fix (Phase 1)
1. **04_interrupt**: uses `delay_ms()` inside `portDISABLE_INTERRUPTS()`;
   `delay_ms` maps to `vTaskDelay` under FreeRTOS (`delay.c`), which is
   undefined while the tick/PendSV are masked. Observed: `tim3` and `tim6`
   end up equal, so the "TIM3 keeps running / TIM6 is masked" point is not
   demonstrated. → busy-wait inside the window.
2. **18_tickless / 19_idle_hook**: the "busy" phase uses `delay_ms()` →
   `vTaskDelay`, so the task blocks in both phases and the busy/idle contrast
   is lost. → busy-wait for the busy phase.
3. **Stack sizes**: tasks that call `printf/snprintf` use 128-word stacks;
   `configCHECK_FOR_STACK_OVERFLOW` is off. → bump stacks + enable the check
   with a `vApplicationStackOverflowHook` in the port.
4. **Config layering**: move the FreeRTOS/LVGL common config into the port
   (lwIP style) and expose it via `lib_wrapper(lib_freertos freertos_port)`.

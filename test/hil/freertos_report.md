# FreeRTOS examples port — verification report

All 25 examples were built (`all-freertos` RC=0, 0 warnings), flashed and
verified on hardware (LCD title via framebuffer OCR + serial). Functional
checks (KEY injection) were run on a representative subset.

## Cross-check vs the plan

| app | BSP | config overrides | boot | LCD title | notes |
|---|---|---|---|---|---|
| 02_freertos_port | lcd sdram | — | OK | STM32 / FreeRTOS Porting | serial: float_num |
| 04_interrupt | lcd sdram gtim btim | — | OK | STM32 / Interrupt | TIM3(p4)+TIM6(p6), arr 9999/psc 8999 |
| 06_1_task_create_dynamic | lcd sdram | — | OK | STM32 / Task Create & Del | KEY0/1 delete task1/2 |
| 06_2_task_create_static | lcd sdram | STATIC_ALLOCATION=1 | OK | STM32 / Task Create & Del | idle/timer mem hooks |
| 06_3_task_suspend_resume | lcd sdram | — | OK | STM32 / Task Susp & Resum | KEY0 suspend / KEY1 resume |
| 07_list_item | lcd sdram | — | OK | STM32 / List & ListItem | serial, KEY0-gated steps |
| 09_time_slicing | lcd sdram | — | OK | STM32 / FreeRTOS Round Robin | serial: task1/2 counters |
| 11_1_task_status_info | lcd sdram +malloc | — | OK | STM32 / Task Info Query | uxTaskGetSystemState/vTaskGetInfo/eTaskGetState/vTaskList |
| 11_2_run_time_stats | lcd sdram btim +malloc | GENERATE_RUN_TIME_STATS=1 | OK | STM32 / Get Run Time Stats | TIM6 10 kHz counter |
| 13_1_queue | lcd sdram | — | OK | STM32 / Message Queue | KEY0 fill, KEY1 LED |
| 13_2_queue_set | lcd sdram | — | OK | STM32 / Queue set | WKUP/KEY1 queues, KEY0 sem |
| 13_3_queue_set_event_flags | lcd sdram | — | OK | STM32 / Queue Event Group | KEY0/1 event bits |
| 14_1_binary_semaphore | lcd sdram | — | OK | STM32 / Binary Semap | KEY0 give |
| 14_2_counting_semaphore | lcd sdram | — | OK | STM32 / Count Semaphore | KEY0 give + count |
| 14_3_priority_inversion | lcd sdram | — | OK | STM32 / Priority Inversion | serial |
| 14_4_mutex | lcd sdram | — | OK | STM32 / Mutex Semaphore | serial |
| 15_software_timer | lcd sdram | — | OK | STM32 / Timer | **KEY0 start: Timer1=004, Timer2=001** |
| 16_event_group | lcd sdram | — | OK | STM32 / Event Group | KEY0/1 set bits |
| 17_1_notify_binary_sem | lcd sdram | — | OK | STM32 / Notify Bina Sem | KEY0 notify |
| 17_2_notify_counting_sem | lcd sdram | — | OK | STM32 / Notify Count Sem | KEY0 counting notify |
| 17_3_notify_mailbox | lcd sdram | — | OK | STM32 / Notify MailBox | KEY0 fill, KEY1 LED |
| 17_4_notify_event_group | lcd sdram | — | OK | STM32 / Notify Event Group | KEY0/1 bits |
| 18_tickless | lcd led sdram | TICKLESS=1 + pre/post hooks | OK | STM32 / FreeRTOS Tickless | GPIO clocks off in sleep |
| 19_idle_hook | lcd led sdram | IDLE_HOOK=1 | OK | STM32 / FreeRTOS IDLE HOOK | idle WFI |
| 20_memory | lcd sdram | heap_4 | OK | STM32 / Mem Manage | **KEY0 alloc: 0x20000460** |

## Deviations from the plan (all intentional / agreed)
- **04_interrupt** uses **TIM3 (gtim) + TIM6 (btim)** instead of TIM3/TIM5
  (no BSP change), with the reference period `arr=9999, psc=8999` and
  pre-emption priorities **4** and **6** (straddling `MAX_SYSCALL = 5`).
- **LCD / serial text is English** (the reference LCD text is English; serial
  Chinese was translated).
- Every LCD app links `sdram` and calls `sdram_init()` before `lcd_init()`
  (the panel framebuffer lives in SDRAM); this is added to the plan's BSP list.
- Heap is the common default (`configTOTAL_HEAP_SIZE = 10 KiB`, same as the
  reference); no per-app heap override was needed.
- `06_2`: the static-allocation hooks use `configSTACK_DEPTH_TYPE*` (FreeRTOS 11
  signature) instead of v10's `uint32_t*`.
- `18_tickless`: sleep hooks call no-arg functions (avoids `TickType_t` in
  FreeRTOSConfig.h); the panel is left enabled (no `lcd_display_off` in the BSP).
- `19_idle_hook`: uses CMSIS `__WFI()/__DSB()/__ISB()`.

## Tools
Verification used `test/hil/hil_fb_ocr.py` (LCD OCR), `capture_baud.py`
(serial) and openocd KEY injection (KEY0 = PH3).

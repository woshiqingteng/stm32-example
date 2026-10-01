# STM32F429 分层示例工程 — 现状说明

面向正点原子阿波罗 V2（STM32F429，`openedv_stm32f4`）的多 OS 示例工程：
裸机（`app/baremetal`）、FreeRTOS（`app/freertos`）与联网例程（`lwip_*`）。
本文件只描述**当前**架构与约定。

---

## 1. 分层与工程结构

依赖方向（严格下行）：`platform < module < bsp < port < lib < app`

```
platform/   SoC/HAL：soc（启动/向量/syscalls/HAL）、drivers
module/     上游源码（按版本目录）：stm32_hal, freertos, lwip, lvgl, mbedtls,
            fatfs, tinyusb, cherryusb, cmsis_dsp, ijg_libjpeg, tjpgd, stm32_usb_*
bsp/        板级外设驱动（openedv_stm32f4）
port/       板级“移植/配置”层：fatfs, freertos, lvgl, lwip, stm32_usb_*
lib/        仅“包装”：把 module+port 组合成 `lib_<name>`（`lib_wrapper`），不写业务
app/        应用（baremetal/ freertos/），每个 app 独立 CMake 树
```

- `lib` 只做包装，**不含链接选项/编译选项**（OS 相关选项放对应 port）。
- `port` 拥有板级配置与适配（`FreeRTOSConfig_common.h`、`lv_conf_common.h`、
  `lwipopts.h`、`sys_arch.c`、`lv_port.c` 等），可向 module 注入 include/源文件。
- app 的 `CMakeLists.txt` **不出现 `target_link_*`**，只用 `add_baremetal_app` /
  `add_freertos_app`。

## 2. 构建

```
tool/build.sh [debug|release] <app|all|all-freertos> [<app> ...] [--flash]
```

- `<app>` 自动按目录解析 OS（`app/<os>/<name>` → `APP_TARGET=app_<os>_<name>`）。
- 每个 app **独立配置/构建**到 `build/<type>/<board>/<os>/<name>/`。
- 板级：`openedv_stm32f4`；工具链：`cmake/toolchain/*`、`cmake/board/*`。

## 3. 命名约定（宏，适用于 `app/ bsp/ port/ lib/` 手写代码）

| 类别 | 后缀 | 示例 |
|---|---|---|
| 时间 | `_MS` / `_US` / `_S` | `BTIM_LOOP_MS` |
| 频率 | `_HZ` / `_KHZ` / `_MHZ` | `BSP_SYSCLK_MHZ` |
| 字节/字 | `_BYTE` / `_WORD` | `DMA_TX_CHUNK_BYTE` |
| 计数 | `_COUNT` / `_PULSE` / `_SAMPLE` / `_TICK` | `ADC_AVG_COUNT` |
| 像素/几何 | `_PX` / `_PIXEL` / `_LINE` | `LCD_WIDTH_PX` |
| 电压/温度 | `_VOLT` / `_MV` / `_DEGC` | `ADC_TEMP_OFFSET_DEGC` |
| 比例 | `_RATIO` / `_PERCENT` | `DMA_TX_PROGRESS_SCALE_PERCENT` |
| 波特率 | `_BAUD` | `RS485_BAUD` |

- 复数一律单数：`_BYTES→_BYTE`、`_TICKS→_TICK`、`_SAMPLES→_SAMPLE` 等。
- 寄存器节拍值基线名即寄存器名（`ARR/CCR/RLR/COUNTER/WINDOW/MODULUS`）不加后缀；
  描述性计时值加 `_TICK`；其它原始字段（`DTG`、`PLLN/M/P/Q`、`SDRAM_TIMING_*`、
  阈值/过滤）加 `_RAW`；纯枚举选择器不改名、仅加注释。**仅改名+注释，不改数值**。
- 仅“单位相关”宏在行尾注释物理含义/单位。

## 4. BSP 与驱动分层

规则：`X.{c,h}` = 功能接口（设备无关），`X_<chip>.{c,h}` = 具体芯片驱动
（寄存器/引脚/总线/初始化）。CMake：`bsp_X_<chip>`，`bsp_X` PUBLIC 链接之。

现有 feature/chip 对：

| feature | chip | feature | chip |
|---|---|---|---|
| `lcd` | `lcd_rgb` | `eeprom` | `eeprom_at24c02` |
| `touch` | `touch_gt9xxx` | `nor` | `nor_w25q256jv` |
| `oled` | `oled_ssd1306` | `nand` | `nand_mt29f4g08` |
| `temp` | `temp_ds18b20` | `humi` | `humi_dht11` |
| `als` | `als_ap3216c` | `mag` | `mag_st480mc` |
| `imu` | `imu_sh3001` | `wireless` | `wireless_nrf24l01` |
| `codec` | `codec_es8388` | `eth_phy` | `eth_phy_yt8512c` |
| `sdram` | `sdram_w9825g6kh.h` | | |

其余为板级单层驱动（`adc/atim/btim/can/dac/dcmi/delay/exti/ftl/gtim/i2c/iap/
internal_flash/io_expand/ir/key/led/ov5640/pwmdac/pwr/rng/rs485/rtc/sai/sdio/
spi/sys/tpad/usart/wdg`）。

- **usart 统一驱动**：`USART_ID_1/2`，tx/rx 可分别 POLL/IT/DMA；行解析在 app。
  DMA-RX 用 `USART_IT_IDLE` 驱动（循环 DMA + `NDTR`）；TX-DMA 用流 IRQ。
- **LCD**：LTDC RGB（4.3" `0x4384`），帧缓冲在 SDRAM `0xC0000000`；竖屏
  `LCD_DIR_PORTRAIT`；提供 `lcd_display_on/off()`（含背光 PB5）。

## 5. FreeRTOS 移植

- `module/freertos/11.3.1`：内核 + `portable/GCC/ARM_CM4F` + `heap_4`；目标 `freertos`。
- `port/openedv_stm32f4/freertos`：`FreeRTOSConfig_common.h`（公共配置，含
  `configCHECK_FOR_STACK_OVERFLOW=2`）+ `vApplicationHooks.c`（
  `vApplicationStackOverflowHook`，不依赖内核头）。add 到 `freertos` 的 include。
- **配置分层**：各 app `FreeRTOSConfig.h` 先写覆盖项，再 `#include
  "FreeRTOSConfig_common.h"`（同 lwIP 范式）。堆默认 10KB；lwIP 例程覆盖 32KB、
  `lv_29_keyboard` 覆盖 48KB；`06_2` 开静态分配。
- **链接**：`add_freertos_app` 直接 `target_link_libraries(... freertos
  freertos_port ...)`；`freertos_port` 用 `-Wl,-u,vApplicationStackOverflowHook`
  强制拉入 hook。无 `lib_freertos`。
- **tick**：`bsp/delay.c` 的 `SysTick_Handler` 调 `HAL_IncTick`，调度器启动后链
  `xPortSysTickHandler`。`delay_us/ms` 为**纯 SysTick 忙等**（无调度器 API，临界区/
  中断内安全）；需睡眠用 `vTaskDelay`。
- `18_tickless` 开 `configUSE_TICKLESS_IDLE` 并前置声明 sleep 钩子；
  `19_idle_hook` 开 `configUSE_IDLE_HOOK`。

## 6. lwIP 移植

- `module/lwip/2.1.2`：ST lwIP 源码（core/api/netif/apps），目标 `lwip`。
- `port/openedv_stm32f4/lwip`：`sys_arch.c`（raw-FreeRTOS）、`ethernetif.c`
  （零拷贝）、`lwip_comm.c`（netif/DHCP/回退/静态 DNS）、`lwip_crypto.c`
  （HMAC-SHA1/Base64，走 mbedTLS）、`lwipopts.h`、`arch/`。
- 要点：`NO_SYS=0`；netconn/socket；DHCP/DNS/ICMP/IGMP/MQTT/SNTP；**无 TLS**。
  **软件校验和**（STM32F4 Tx 卸载会静默丢 DHCP DISCOVER）；DHCP 在**调度器后**
  （`eth_rx` 启动 MAC 后）用 `netifapi_dhcp_start`，DHCP 时 `netif_add` 用 0.0.0.0。
  静态回退 `192.168.2.100/24`、gw/dns `192.168.2.1`；客户端例程目标 PC `192.168.2.8`。
  `LWIP_RAND` 用硬件 RNG（`drv_rng`，可关）。
- **关键修复**：`HAL_ETH_Start_IT()` 移到 `eth_rx` 任务（调度器前起 ETH 中断会在
  ISR 里 `FromISR` → HardFault）。
- `lib/lwip_mqtt_common`：17/18 共用 MQTT 回调与上报。

## 7. LVGL 移植

- `module/lvgl/8.3.11`：目标 `lvgl`（`EXCLUDE_FROM_ALL`，链 `drv_core`）。
- `port/openedv_stm32f4/lvgl`：`lv_port.c`（显示 + 触摸 + tick）、`lv_port.h`、
  `lv_conf_common.h`。`LV_TICK_CUSTOM_INCLUDE "lv_port.h"` /
  `LV_TICK_CUSTOM_SYS_TIME_EXPR (lv_port_tick_get())`（`USE_FREERTOS` 切换
  `xTaskGetTickCount()` / `HAL_GetTick()`）。`lib_wrapper(lib_lvgl lvgl_port)`。
- SDRAM 布局：帧缓冲 `0xC0000000`（预留 4MB）/ LVGL 池 `0xC0400000` /
  draw buffer `0xC0480000`（40 行）。

## 8. HIL 测试

- `test/page`：`base.py`（`BasePage`：常驻 OpenOCD + telnet:4444，`program/
  peek/poke/串口`）、`openedv_stm32f429.py`（`tap` 注入、`led_odr`）。
  **单个常驻会话**，禁止反复起停/强杀 OpenOCD（会弄死 CMSIS-DAP）。
- `test/hil/freertos_verify.py`：逐 app 烧录→帧缓冲 OCR→KEY 注入→串口→报告；
  `hil_fb_ocr.py`（读 `lcdfont.h` 解码）、`pc_peer.py`（lwIP 对端）；
  结果见 `test/hil/freertos_report.md`；入口 `tool/hil.sh`。
- `test/test`：app 01–10 的 pytest 用例；`test/run.py` + `test/conftest.py`。

## 9. 应用清单

- **baremetal（65）**：`01_led 02_key 03_exti 04_usart 05_iwdg 06_wwdg 07_btim
  08_1_gtim_int 08_2_gtim_pwm 08_3_gtim_cap 08_4_gtim_cnt 09_1_atim_npwm
  09_2_atim_oc 09_3_atim_cplm 09_4_atim_pwmin 10_tpad 11_oled 12_lcd_mcu
  13_sdram 14_lcd_rgb 15_usmart 16_rtc 17_rng 18_1_pvd 18_2_sleep 18_3_stop
  18_4_standby 19_dma 20_1_adc_single 20_2_adc_dma 20_3_adc_multi_dma
  20_4_adc_oversample 21_internal_temp 22_1_dac 22_2_dac_tri 22_3_dac_sine
  23_pwm_dac 24_i2c_eeprom 25_i2c_extend_io 26_i2c_als 27_spi_nor 28_rs485
  29_can 30_touch_screen 31_ir 32_1wire_temp 33_1wire_humi 34_i2c_magnet
  35_i2c_imu 36_spi_wireless 37_internal_flash 38_camera_stream 39_malloc
  40_sdio_sdcard 41_nand 42_fatfs 43_font 44_image 45_camera_storage 46_sai_audio
  47_sai_record 48_video 49_fpu 50_1_dsp_math 50_2_dsp_fft 53_iap 53_iap_app
  54_usb_device_msc 55_usb_device_audio 56_usb_device_cdc 57_usb_host_msc
  58_usb_host_hid`
- **freertos（25 + lv_29）**：`02_freertos_port 04_interrupt
  06_1/06_2/06_3 07_list_item 09_time_slicing 11_1 11_2 13_1 13_2 13_3
  14_1 14_2 14_3 14_4 15_software_timer 16_event_group 17_1 17_2 17_3 17_4
  18_tickless 19_idle_hook 20_memory` + `lv_29_keyboard`。
- **lwip（15）**：`lwip_6_freertos 7_netconn_udp 8_netconn_tcp_client
  9_netconn_tcp_server 10_socket_udp 10_1_udp_broadcast 10_2_udp_multicast
  11_socket_tcp_client 12_socket_tcp_server 12_1_socket_tcp_server_multi
  13_ntp 14_sntp 16_http 17_aliyun_mqtt 18_onenet_mqtt`。

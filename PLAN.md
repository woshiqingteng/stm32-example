# STM32F429 分层示例工程 — 现状说明

面向正点原子阿波罗 V2（STM32F429，`openedv_stm32f4`）的多 OS 示例工程：
裸机（`app/baremetal`）、FreeRTOS（`app/freertos`）与联网例程（`lwip_*`）。
本文件只描述**当前**架构与约定。

---

## 1. 分层与工程结构

依赖方向（严格下行）：`common < platform < module < bsp < port < lib < app`

```
common/     硬件/平台无关的纯 C 工具（`bitops.h`、`ringbuf.h`），header-only
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
  接口 `usart.h` **去 HAL**（自有帧选项枚举）；中断收发为**显式标志**实现
  （`USARTx_IRQHandler`/DMA 流 IRQ 直接判/清标志，无 `HAL_UART_IRQHandler` 与弱回调），
  HAL 仅用于 `HAL_UART_Init`/`HAL_DMA_Init`/`HAL_DMA_Abort`。DMA-RX 用
  `USART_IT_IDLE` 驱动（循环 DMA + `NDTR`，不使能流中断）；TX-DMA 用流 IRQ + `TC` 收尾。
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
 - **freertos（25 例程）**：`02_freertos_port 04_interrupt
  06_1/06_2/06_3 07_list_item 09_time_slicing 11_1 11_2 13_1 13_2 13_3
  14_1 14_2 14_3 14_4 15_software_timer 16_event_group 17_1 17_2 17_3 17_4
  18_tickless 19_idle_hook 20_memory`。
 - **LVGL（47 + 2 官方 demo）**：
  A 类（纯控件/官方 demo）：`lvgl_02_stress lvgl_04_mouse lvgl_09_obj
  lvgl_10_arc lvgl_11_bar lvgl_12_btn lvgl_13_btnmatrix lvgl_14_canvas
  lvgl_15_checkbox lvgl_16_dropdown lvgl_17_img lvgl_18_label lvgl_19_line
  lvgl_20_roller lvgl_21_slider lvgl_22_switch lvgl_23_table lvgl_24_textarea
  lvgl_25_calendar lvgl_26_chart lvgl_27_colorwheel lvgl_28_imgbtn
  lvgl_29_keyboard lvgl_30_led lvgl_31_list lvgl_32_meter lvgl_33_msgbox
  lvgl_34_span lvgl_35_spinbox lvgl_36_spinner lvgl_37_tabview lvgl_38_tileview
  lvgl_39_win lvgl_44_qrcode lvgl_47_calculator lvgl_49_qrgen lvgl_50_paint
  lvgl_52_baseconv` + `lvgl_demo_widgets lvgl_demo_benchmark`。
  B 类（SD/FATFS）：`lvgl_05_fs lvgl_06_font lvgl_41_bmp lvgl_42_png
  lvgl_43_gif lvgl_45_jpeg lvgl_51_filemgr`。
  B 类（SPI-NOR）：`lvgl_07_xbf_font lvgl_40_img_lib`。
 - **lwip（15）**：`lwip_6_freertos 7_netconn_udp 8_netconn_tcp_client
  9_netconn_tcp_server 10_socket_udp 10_1_udp_broadcast 10_2_udp_multicast
  11_socket_tcp_client 12_socket_tcp_server 12_1_socket_tcp_server_multi
  13_ntp 14_sntp 16_http 17_aliyun_mqtt 18_onenet_mqtt`。

## 10. 计划：LVGL 例程移植（FreeRTOS，A 类）+ 官方 demo

参考：ALIENTEK `…\3，扩展例程\4，LVGL例程`（LVGL **8.1.1**）；官方 demo 源
`D:\work\git\lvgl`（**8.3.11**，与 `module/lvgl` 同版本）。
目标命名：`app/freertos/lvgl_<number>_<name>`。

### 约定
- 启动层**每 app 内联**（与 FreeRTOS 例程一致）：`bsp_init`/`sdram_init`/`lv_init`
  /`lv_port_disp_init`/`lv_port_indev_init` + lvgl 任务 + LED 心跳。
- **本阶段只做 A 类（纯控件，无 SD/SPI-Flash）**；B 类（5/6/7/40-43/45/51）暂不做。
- **每 app 自带 `GUI_FONT`**（`myFont14/24`，`52` 加 `myFont18`）。
- demo 代码来自参考 `Middlewares/LVGL/GUI_APP/lv_mainstart.c`，适配 8.1.1→8.3.11。
- `lv_29_keyboard` → 重命名 `lvgl_29_keyboard` 并换成**完整参考 demo**。

### 阶段 0：公共设施
- **Vendor demos**：从 `D:\work\git\lvgl/demos` 复制到 `module/lvgl/8.3.11/demos/`：
  `widgets/`（含 `assets/img_*.c`，排除截图）、`benchmark/`（含 `assets/*.c` 及
  `*.c.c` 压缩字体，排除截图）、`stress/`、`lv_demos.h`；不复制 `music`/`keypad_encoder`。
- `module/lvgl/8.3.11/CMakeLists.txt`：`LVGL_SRC` 追加 `demos/**/*.c`
  （未启用 demo 为空 TU；assets 由 `--gc-sections` 丢弃）；demo 上游告警按需定向抑制。
- `port/openedv_stm32f4/lvgl/lv_conf_common.h`：开启 A 类基础字体
  `LV_FONT_MONTSERRAT_10/14/20/30`；`LV_USE_DEMO_*` 保持 0，由各 app 覆盖。

### 阶段 1：A 类（38 个 app）
每个 app：`main.c` + `FreeRTOSConfig.h`(heap 48KB) + `lv_conf.h`(LV_MEM_ADR/SIZE)
+ `CMakeLists.txt` + `GUI_FONT/`；额外资源 `13→img_user.c`、`17→img_gear.c`、
`28→img_cool/dry/warm.c`。
`02_stress 04_mouse 09_obj 10_arc 11_bar 12_btn 13_btnmatrix 14_canvas
15_checkbox 16_dropdown 17_img 18_label 19_line 20_roller 21_slider 22_switch
23_table 24_textarea 25_calendar 26_chart 27_colorwheel 28_imgbtn 29_keyboard
30_led 31_list 32_meter 33_msgbox 34_span 35_spinbox 36_spinner 37_tabview
38_tileview 39_win 44_qrcode 47_calculator 49_qrgen 50_paint 52_baseconv`
（`02/04` 调官方 `lv_demo_stress()`）。

### 阶段 2：官方 demo（2 个）
- `lvgl_demo_widgets`：`LV_USE_DEMO_WIDGETS=1` → `lv_demo_widgets()`。
- `lvgl_demo_benchmark`：`LV_USE_DEMO_BENCHMARK=1` + Montserrat 字体 → `lv_demo_benchmark()`。

### 阶段 3：验证
- `tool/build.sh debug all-freertos` RC=0/零告警；逐 app 烧录：帧缓冲非空 + LED
  心跳 + 无 HardFault（触摸类仅验“绘制正常不崩”）；更新 `freertos_report.md`。

## 11. 计划：LVGL B 类收尾（字库/图库/NOR/内存）

参照官方 `LVGL开发指南_V1.5.pdf`（ch 8.4.2 字库更新）与官方工程内存配置
（F429 192KB RAM）：官方每工程缩小 MALLOC 的 `MEM1(SRAMIN)` 到 20–60KB、
`MEM2(SRAMCCM)=60KB`、`MEM3(SRAMEX)=50KB`，`configTOTAL_HEAP_SIZE=46KB`，
并在调度器启动前用 `mymalloc(SRAMIN,4KB)` 小缓冲从 SD 更新 NOR。

1. **`lib/malloc/malloc.h`**：`MEM1_MAX_SIZE` 默认 160KB→96KB，并加
   `#ifndef MEM1_MAX_SIZE` 使可覆盖（全局/board 级）。
2. **`lib/text/fonts.c`**：按指南升级为 6 路径
   `{UNIGBK.BIN, GBK12/16/24/32.FON, /SYSTEM/LVFONT/Font12.BIN}`、
   `FONTSECSIZE=1633`、`fonts_update_fontx` 增 `case 5` 设 `lvgl_12addr/12size`；
   **保留 `mymalloc`**（不改静态缓冲）。
3. **`lvgl_07_xbf_font`**：加 SD 挂载；`if (fonts_init()!=0)
   fonts_update_font(0,0,16,"0:",WHITE);`（仅缺失时更新）。
4. **`lvgl_40_img_lib`**：`IMAGEINFOADDR=0x1F80000`（31.5MB，避开 25MB 字库）；
   启动 `if (images_init()!=0) images_update_image(0,0,16,"0:",WHITE);`；
   `lv_load_img` 改用自定义头（`cf | w<<8 | h<<20` + RGB565）。
   因官方 `PICTURE/LVGLBIN/*.BIN` 未随资料提供，用 `tool/lvgl_img2bin.py`
   （Pillow）以现有图片生成替代 `atk05/06/07/money.BIN`。
5. **USB MSC 拷 SD**：烧 `54_usb_device_msc` 暴露 SD，拷整包
   `LVGL实验所需SD卡文件`(48MB) + `PICTURE/LVGLBIN/*.BIN`。
6. **验证**：`all-freertos` RC=0/零告警；`tool/hil.sh lvgl` 全量 PASS 后更新
   `test/hil/lvgl_report.md`。

## 12. 计划：LVGL 配置/性能、字体外置、综合实验

目标：开启 DMA2D、记录 benchmark 性能、消除字体重复并把 CJK 字库外置到
SPI-NOR（XBF）、移植「综合实验」、资源脚本化。

### A. 配置与内存
1. `lv_conf_common.h`：`LV_USE_GPU_STM32_DMA2D 1` +
   `LV_GPU_DMA2D_CMSIS_INCLUDE "stm32f4xx.h"`（`lv_init()` 自动初始化 DMA2D）。
2. `lvgl_demo_benchmark/lv_conf.h`：`LV_USE_FONT_COMPRESSED 1`（启用压缩字场景）。
3. Montserrat 裁剪：common 默认只开实际用到的
   `{8,10,12,14,16,18,20,22,24,30,32,36,46}`；benchmark 另行开 `28`。
4. 内存单源：common 默认 `LV_MEM_ADR 0xC0400000U` / `LV_MEM_SIZE 512KB`；
   `lv_port.c` 用它推导保护地址与 draw buffer（`LV_MEM_ADR` / `LV_MEM_ADR+LV_MEM_SIZE`）。

### B. 字体外置（XBF）
5. `tool/lvgl_font_c2xbf.py`：解析 `myFont*.c` → XBF `.bin` + `FontXX.c`
   描述符（`adv_px=adv_w`——LvglFontTool 已按像素存储、`box_w` 按 `8/bpp` 补齐
   并重排位图、`line_height/base_line` 写入、`__g_font_buf` 按最大块取尺寸；
   非零 `ofs_x` 告警后继续）。
6. `lib/text`：`_font_info` 增 `lvgl_14/18`（24 复用），`FONT_GBK_PATH`/提醒表
   加 `Font14|18|24.BIN`，扩 `FONTSECSIZE`；`fonts_lvgl_ok` 扩校验 14/18/24。
7. 一次性迁移全部 myFont 应用到 XBF 描述符 + `f_mount`+`fonts_init/update`；
   删除各 app 的 `GUI_FONT/myFont*.c`。

### C. 综合实验
8. 新 `app/freertos/lvgl_53_comprehensive`：迁移参考 `GUI_APP/*` + `image` +
   `img_hand`，适配本项目头/启动/SD/字库(`Font18`)/图库（8 图，`0x1F80000`，
   含 `nor_init`）。
9. 8 个启动图标用 `tool/lvgl_img2bin.py` 生成替身（源图缺失）。

### D. HIL
10. `lv_port.c` 的 `touchpad_read` 加运行时可注入全局（`lv_indev_test_en/pr/x/y`）。
11. `lvgl_verify.py` 增 tap 断言（`04/21/29/37/47`）。
12. 重跑 `tool/hil.sh lvgl` 与 `tool/hil.sh lvgl benchmark`，更新报告。

### E. 资源与仓库
13. 资源流程：本地生成 LVGLBIN(4+8)+XBF → 烧 `54_usb_device_msc` → 拷入 SD 对应目录。
14. `.gitattributes`：生成字体/图片 `*.c` 标 `-text`。
15. `add_freertos_app` 保持原样。

### 完成情况（结果）
- A1–A4、B5–B7、C8–C9、D10–D12 均已完成；`tool/hil.sh lvgl` = 49/49 PASS，
  `tool/hil.sh lvgl benchmark` 已用 DMA2D + 压缩字库重录（Weighted FPS 23，
  Opa 95%，压缩场景已正常渲染）。
- 过程中发现并修复两个关键缺陷：
  1. 迁移应用 `MEM1_MAX_SIZE` 原为 8 KB，`fonts_update_font`/`fonts_update_fontx`
     叠加申请导致 `mymalloc` 失败、字库地址写不进去（标签显示黑方块）；提到 16 KB。
  2. 综合实验图库 `IMAGEINFOADDR=0x1F80000` + `IMAGESECSIZE=256`(1 MB) 越过 32 MB
     NOR 并回绕擦到 FatFs 区；改为 128 并去掉预擦除扫描。
- `lib/text` 同时修了 `f_open` 泄漏、去掉全量预擦除扫描、进度每 64 KB 刷新
  （首刷 ~25–95s，之后启动跳过）。
- USB MSC 三 LUN：把 `ftl_init` 从枚举路径移到 `main` 启动前，NOR/NAND/SD 均可枚举。
- 触摸注入钩子（`g_lv_indev_test_*`）+ `lvgl_04/21/29/37/47` tap 断言已加入。
- NOR 布局单一来源（`bsp/.../nor.h`）：器件容量取自 `NOR_W25Q256JV_SIZE_BYTE`，
  仅两个策略值（FatFs 20 MB、图库 512 KB），字库基址/大小自动推导并有编译期断言；
  `nor_read/nor_write/nor_erase_sector` 加了 32 MB 上界保护。MSC 的 NOR LUN = 20 MB。

### 使用须知
- **空板/空字库首次预热**：SPI-NOR 字库区为空的板子，首次运行任一迁移应用
  （`lvgl_06/14/…/53`）会做一次 SD→NOR 字库构建，LCD 显示进度，约 25–95s；
  完成后各应用共享该字库、启动很快。**跑 HIL/演示前先让一个迁移应用跑完首刷**
  （尤其改过 `NOR_FONT_BASE` 后，字库会被判失效并重建一次）。
- **USB 盘名（资源管理器）**：卷标由板侧设置——运行一次 `42_fatfs` 会把
  SD/NOR/NAND 写成 `SD`/`NOR`/`NAND`（`f_setlabel`），之后 `54_usb_device_msc`
  在电脑上即显示对应名字。
- **NAND 最大容量**：`54_usb_device_msc` 启动时若 FTL 未覆盖全部好块会执行一次
  `ftl_format()`（擦除 NAND），使 NAND LUN ≈476 MB；之后不再重格。
- **NOR 20 MB 迁移**：NOR 分区由 25 MB 改为 20 MB，旧卷不兼容；需重格一次
  （运行 `42_fatfs` 或在电脑上格式化该盘），否则资源管理器可能显示异常。

## 13. 计划：USART 驱动接口去 HAL + 完整显式中断收发

目标：`usart.h` 接口去 HAL（自有枚举）；中断收发改为显式标志，去掉
`HAL_UART_IRQHandler` 与 `HAL_UART_RxCplt/ErrorCallback`；HAL 仅保留初始化。

### 接口（`usart.h`，去 HAL）
- 去 `stm32f4xx_hal.h`，仅留 `<stdint.h>`/`<stdbool.h>`。
- 新增 `usart_word_len_t / usart_stop_bits_t / usart_parity_t / usart_mode_t /
  usart_flow_t / usart_oversampling_t`；`usart_cfg_t` 六个字段改类型；
  `USART_CFG_DEFAULT` 用自有枚举。`usart_init` 维持 `void`。

### 实现+端口（`usart.c`）
- 句柄：`+ volatile bool tx_busy; + const uint8_t *tx_ptr; + uint16_t tx_len;` 删 `rx_byte`。
- 6 个 `usart_*_to_hal()` 映射，替换 `usart_init` 的 HAL 赋值。
- HAL 仅保留 `HAL_UART_Init`/`HAL_DMA_Init`/`HAL_DMA_Abort`/`HAL_NVIC_*`/GPIO/RCC。
- TX：POLL=寄存器 TXE/TC 阻塞写；IT=TXE 逐字节+TC 收尾；DMA=`HAL_DMA_Start`
  +清流标志+`ENABLE_IT(TC)`，完成后 `HAL_DMA_Abort` 并开 `UART TC IT`。
- RX：IT 开 `RXNE`（读 DR 直取）；DMA 用 `HAL_DMA_Start`(circular)+`IDLE`（不使能流中断）。
- `usart_irq()`：TXE / TC / RXNE / ORE / IDLE 五分支。
- `usart_tx_busy`→自有 `tx_busy`；`__io_putchar` 阻塞等 `tx_busy` 后 POLL 写。
- 移除：`HAL_UART_IRQHandler`、`HAL_UART_Transmit*`、`HAL_UART_Receive_IT/DMA`、
  `HAL_UART_Abort*`、`HAL_DMA_IRQHandler`。

### 决策
D1 HAL 仅初始化；D2 POLL 用自有阻塞写；D3 `USART_IO_IT` 实现显式 TXE；
D4 自有 `tx_busy`；D5 `__io_putchar` 阻塞等待。

### 验证
`tool/build.sh debug all` 零告警；`python test/run.py test/test_04_usart.py`；
手测 15_usmart / 19_dma / rs485 / printf 横幅。

### 完成情况（结果）
- 已完成：`usart.h` 去 HAL（6 个自有帧选项枚举 + `usart_cfg_t`/默认宏改类型）；
  `usart.c` 端口层显式化——`USARTx_IRQHandler` 处理 RXNE/TXE/TC/ORE/IDLE，
  DMA 流 IRQ 处理 TC/TE/FE/DME；移除 `HAL_UART_IRQHandler` 与
  `HAL_UART_RxCplt/ErrorCallback`；HAL 仅保留 `HAL_UART_Init`/`HAL_DMA_Init`/
  `HAL_DMA_Abort`。`usart_tx_busy` 改用内部 `tx_busy`；`__io_putchar` 先等异步
  TX 结束再写。
- 关键修正（实现中发现）：仅用 `HAL_DMA_Start` 不会置位 USART 的 DMA 请求位，
  必须显式 `CR3.DMAT`（TX）/`CR3.DMAR`（RX），否则 DMA 无请求、传输不动。
- 命名注意：HAL 同步 USART 头已定义 `USART_PARITY_*`/`USART_MODE_*`/
  `USART_OVERSAMPLING_*`，故自有枚举改用 `USART_PAR_*`/`USART_DIR_*`/`USART_OS_*`。
- 验证：`tool/build.sh debug all` 仅 `lvgl_53_comprehensive` 失败，该失败在改动
  前的 HEAD 上同样存在（与本改动无关，属既有构建问题）；其余 usart 相关 app
  （`04_usart 15_usmart 19_dma 28_rs485 53_iap 38_camera_stream lvgl_40_img_lib
  lvgl_51_filemgr`）全部构建通过。`python test/run.py test/test_04_usart.py`
  = 4/4 PASS（RX IT 行回显/提示/CR 丢弃/超长重启）。`19_dma` 硬件实测：KEY0 →
  整包发送 → `progress: 100%` → `DMA TX finished`。
- 后续精简（同一节）：`usart.c` 改为**硬件描述符表 `g_hw[]`**（instance/AF/pins/
  IRQn/`DMA_TypeDef*`/TX-RX stream/channel/TX-DMA-IRQn），去掉重复的 per-id 分支；
  新增 `usart_dma_config()` 公共配置、`usart_rx_deliver()` 统一接收投递（RXNE 与
  DMA-IDLE 共用）；DMA 启动前只需清 TC 标志；`usart_dma_tx_irq()` 提取公共
  `HAL_DMA_Abort`；`0xFFFFU` → `USART_TX_MAX_WORD`。DMA 时钟使能在两个 DMA init
  内各内联一份（不新增 helper）。channel 入表：F4 各 USART 的 DMA 通道不同
  （USART1/2/3、UART4/5 为 4，USART6 为 5），故按实例存放。
- 写通路加固：`usart_write` 显式列出 POLL 分支判定（不再用隐式 `else`）；单次
  `len` 上限 64 KiB，超出直接返回 `false`；POLL 改为逐字节 100 ms
  `HAL_GetTick` 超时（`usart_tx_wait` 返回 `bool`）；DMA 使能 TE/FE/DME 中断，
  出错时 abort+清 `DMAT`+释放 `tx_busy`（修复"仅 TC 中断→错误时 `tx_busy`
  永真死等"）；不对外暴露错误查询。
- 通用工具层：新增根级 `common/`（header-only；`bitops.h` 全 `COMMON_` 前缀、
  `ringbuf.h` SPSC 透明环形缓冲），置于依赖栈最底；组件按需显式链接
  （`bsp_usart` 以 `PRIVATE common`）。`usart.c` 的内联 RX 环形缓冲迁移为
  `ringbuf_t`（head/tail/buf/size、满判据不变），行为等价。

## 14. 计划：ADC 纯驱动重构（HAL-free 接口 + 原始采样）

目标：ADC1 只做"采样"——HAL-free 接口、`adc_init(cfg)` 配置结构；**不含平均/
电压/温度等算法**（全部移到 app）。DMA 配置并入 `adc_init`，完成中断显式化
（去 HAL 弱回调）。

### 接口（`adc.h`，HAL-free）
- 自有 `adc_channel_t`（`ADC_CH0..5`、`ADC_TEMP_CH`、`ADC_CH_NUM`）与
  `ADC_SCAN_CH_NUM=6`。
- `adc_mode_t{POLL,DMA}`、`adc_dma_mode_t{ONESHOT,CIRCULAR}`、
  `adc_resolution_t`、`adc_sample_time_t`、`adc_clock_t{DIV2/4/6/8}`。
- `adc_dma_cb_t(uint16_t offset)`（半满=0，全满=dma_len/2）。
- `adc_cfg_t{mode,dma_mode,resolution,sample_time,clock,chans,nchans,dma_buf,
  dma_len,dma_half_cb,dma_cb}` + `ADC_CFG_DEFAULT`。
- API：`adc_init(cfg)`（NULL→默认；DMA 模式配置并启动首轮）、`adc_read(ch)`
  （轮询原始值，`ADC_TEMP_CH` 自动开 `TSVREFE`）、`adc_dma_start()`（ONESHOT 重装）。
- 移除：`adc_get_result_average`、`adc_temp_init`/`adc_get_temperature`、
  `adc_dma_init`/`adc_scan_dma_*`、`adc_register_dma_hook`、旧
  `adc_sample_time_t`/`ADC_SAMPLE_TIME`、`ops` 表、`stm32f4xx_hal.h`。

### 实现（`adc.c`，HAL 仅 init）
- 2 个 handle：`g_adc`(轮询) + `g_adc_dma`(DMA)；通道/分辨率/采样/时钟映射表。
- `adc_init`：PA5 模拟 + 轮询 handle；DMA 模式再加 DMA2_Stream4 配置（按
  `dma_mode` 选 CIRCULAR/NORMAL）、按 `nchans` 配扫描/rank、NVIC、缓冲/回调，
  末尾调用 `adc_dma_start()`。
- `adc_dma_start`：`__HAL_ADC_DISABLE`→清 EOC/OVR→`SET CR2.DMA`→`HAL_DMA_Start`
  →使能 DMA `TC|TE|FE|DME`(+`HT`)→`__HAL_ADC_ENABLE`→`delay_us(3)`→`SWSTART`。
- `DMA2_Stream4_IRQHandler`：HT→回调(0)；TC→ONESHOT 时 Abort+清 DMAT，回调
  (dma_len/2)；TE/FE/DME→Abort+清 DMAT。**不使能 ADC OVR 中断**（无需 ADC IRQ）。
- 约束：同一 app 不可混用 `adc_read` 与 DMA；CIRCULAR 下勿重复 `adc_dma_start`。

### 应用迁移（8 个）
- 20_1 POLL（平均/电压内联 + `BLINK_TICKS`）；20_2 CIRCULAR+HT（双缓冲）；
  20_3 CIRCULAR 扫描；20_4 ONESHOT（重装）；21 温度整数换算（app）；
  22_1/22_3/23 轮询平均（app）。

### 验证
`tool/build.sh debug 20_1_adc_single 20_2_adc_dma 20_3_adc_multi_dma
20_4_adc_oversample 21_internal_temp 22_1_dac 22_3_dac_sine 23_pwm_dac`；手测/HIL。

### 完成情况（结果）
- `adc.h` 去 HAL（自有 `adc_channel_t` + mode/dma_mode/resolution/sample_time/clock
  枚举 + `adc_cfg_t`/`ADC_CFG_DEFAULT`）；API = `adc_init(cfg)` / `adc_read(ch)` /
  `adc_dma_start()`。移除平均/电压/温度算法、旧 `adc_sample_time_t`、`ops` 表。
- DMA 并入 `adc_init`；完成中断显式（HT/TC/TE/FE/DME），ONESHOT 在 TC 处 Abort 后
  可重装，CIRCULAR 自由运行；未使能 ADC OVR 中断（无需 ADC IRQ）。
- 8 个 app 迁移：20_1 POLL（平均/电压/`BLINK_TICKS` 内联）、20_2 CIRCULAR+HT
  （双缓冲）、20_3 CIRCULAR 扫描、20_4 ONESHOT（重装）、21 温度整数换算、
  22_1/22_3/23 轮询平均内联。
- 验证：8 个 app 构建零告警；`adc.c` 强制重编零告警；HIL 串口实测 20_1
  `ch5 vol:3.29V`、20_2 `dma vol:3.29V`、20_3 六通道 `ch0..ch5`、
  20_4 `ovs raw:65430 vol:3.294V`、21 `TEMP: 39.4C`。
- 约束：同一 app 不可混用 `adc_read` 与 DMA（同属 ADC1）；CIRCULAR 下勿重复
  `adc_dma_start`。
- 提交注记：驱动与 8 个 app 相互依赖，为保持每次提交可构建，合并为**单个提交**。

## 15. ADC 驱动内聚化（adc_hw_t / adc_handle_t / id API）

目标：参照 `usart_hw_t`，把 ADC 的硬件事实集中到按 id 的描述符表，状态聚合为
单一上下文，去掉冗余 static 函数；公共 API 增加 `adc_id_t`。

### 接口（`adc.h`）
- 新增 `adc_id_t {ADC_ID_1, ADC_ID_2, ADC_ID_3, ADC_ID_NUM}`；`adc_cfg_t` 首字段
  `id`；`ADC_CFG_DEFAULT` 首项 `.id = ADC_ID_1`。
- `adc_read(adc_id_t id, adc_channel_t ch)`、`adc_dma_start(adc_id_t id)`。
- id 越界/未初始化：`adc_read` 返回 0，`adc_dma_start` 直接返回。

### 实现（`adc.c`）
- `adc_hw_t{instance,common,gpio,dma,dma_stream,dma_channel,dma_irqn,ch_hal[],ch_pin[]}`
  + `g_adc_hw[ADC_ID_NUM]`（指定初始化；**仅 ADC1 有效**，ADC2/3 预留并拒绝）。
- `adc_handle_t{hw,poll,dma,dma_stream,cfg,dma_buf,dma_cb}` + `g_adc[ADC_ID_NUM]`。
- 选项映射改文件级表 `g_res_hal/g_smp_hal/g_clk_hal`（带界钳制）；删 3 个映射函数
  与 `adc_channel_pin`；static 函数降为 4 个：`adc_instance_config`、
  `adc_channel_config`、`adc_gpio_analog`、`adc_dma_irq`。
- 中断多分支抽成 `adc_dma_irq(adc_handle_t*)`，入口守卫 `hw==0 || Instance==0`；
  向量 `DMA2_Stream4_IRQHandler` 调 `adc_dma_irq(&g_adc[ADC_ID_1])`。

### 应用
8 个 app 更新为 id API（20_1/21/22_1/22_3/23 的 `adc_read`，20_4 的
`adc_dma_start`）；20_2/20_3 用默认 `cfg.id`。

### 完成情况（结果）
- `adc.h` 增 `adc_id_t`/`cfg.id`/id API；`adc.c` 改为 `adc_hw_t` 表 + `adc_handle_t`
  上下文 + 文件级映射表；static 函数 7→4；`DMA2_Stream4_IRQHandler` 转为
  `adc_dma_irq()` + 守卫。
- ADC2/ADC3 预留（`instance=0`），`adc_init` 对未接实例直接返回。
- 8 个 app 更新为 id API；构建零告警；HIL 冒烟 20_1/20_2/20_3/20_4/21 通过。

## 16. 通用 gpio_hw_t 与 ADC 硬件内聚

目标：抽出可复用的"GPIO 硬件属性"类型到 bsp 私有共享头，供 ADC 等驱动内嵌；
ADC 的 GPIO/时钟硬件事实全部进 `adc_hw_t`。

### 新文件 `bsp/openedv_stm32f4/gpio_hw.h`（header-only）
- `gpio_hw_t { port, rcc_en, mode, pull, speed, alternate }`（配置型；引脚掩码
  由调用方按用途给出）。
- `static inline gpio_hw_setup(const gpio_hw_t*, uint16_t pins)`：使能端口
  `RCC->AHB1ENR` 时钟 + `HAL_GPIO_Init`。
- 不依赖任何驱动头；`adc_hw_t`/`usart_hw_t` 等仍各自私有。

### `adc.c`
- `adc_hw_t`：增 `adc_rcc_en`(APB2ENR_ADC1EN)、`dma_rcc_en`(AHB1ENR_DMA2EN)、
  内嵌 `gpio_hw_t gpio`、`poll_pins`；保留 `ch_pin[]`(GPIO) 与 `ch_hal[]`(ADC 通道)；
  删 `DMA_TypeDef *dma`。
- 时钟改 `SET_BIT(RCC->APB2ENR, adc_rcc_en)` + `SET_BIT(RCC->AHB1ENR, dma_rcc_en)`；
  GPIO 时钟/配置走 `gpio_hw_setup(&hw->gpio, poll_pins|pins)`。
- 删除 `adc_gpio_analog`（→ static 函数 4→3）。

### 完成情况（结果）
- 新增 `gpio_hw.h`；`adc_hw_t` 内嵌 `gpio_hw_t`，时钟/引脚事实全部来自 `g_adc_hw`
  表；`adc_gpio_analog` 删除（static 4→3）；`adc.c` 无 `__HAL_RCC_*_CLK_ENABLE`/
  `== GPIOA`/`== DMA2`。
- 8 个 app 构建零告警；`adc.c` 强制重编零告警；HIL 冒烟 20_1/20_3/21 通过。

## 17. 通用 dma_hw_t 与 ADC 内嵌

目标：参照 `gpio_hw_t`，抽出可复用的 DMA 流硬件属性类型，供 ADC 等驱动内嵌；
`dma_hw_setup()` 一次完成"时钟 + Init + HAL_DMA_Init"（NVIC 归驱动）。

### 新文件 `bsp/openedv_stm32f4/dma_hw.h`（header-only）
- `dma_hw_t { rcc_en, stream, irqn, channel, direction, periph_inc, mem_inc,
  periph_align, mem_align, mode, priority, fifo_mode, fifo_threshold, mem_burst,
  periph_burst }`（身份 + 全部 `DMA_InitTypeDef` 字段；变体 2 全通用）。
- `static inline dma_hw_setup(DMA_HandleTypeDef*, const dma_hw_t*)`：`SET_BIT` 使能
  `RCC->AHB1ENR` 时钟 + 原样填 Init + `HAL_DMA_Init`；不含 NVIC。
- 仅 `#include <stdint.h>` + `"stm32f4xx_hal.h"`，不依赖驱动头。

### `adc.c`
- `adc_hw_t`：`{dma_rcc_en, dma_stream, dma_channel, dma_irqn}` → `dma_hw_t dma`。
- 表内 `.dma = { … DMA2_Stream4 / DMA_CHANNEL_0 / PERIPH_TO_MEMORY / HALFWORD /
  NORMAL / MEDIUM / FIFO_DISABLE … }`。
- `adc_init`：`dma_hw_t dma = hw->dma; dma.mode = cfg 决定; dma_hw_setup(&h->dma_stream,
  &dma);` 替换逐字段 `Init` 赋值；NVIC 用 `hw->dma.irqn`；删除 `SET_BIT(AHB1ENR,
  hw->dma_rcc_en)`。

### 完成情况（结果）
- 新增 `dma_hw.h`；`adc_hw_t` 内嵌 `dma_hw_t`，DMA 硬件事实全部来自 `g_adc_hw` 表；
  `adc_init` 的 DMA 初始化收敛为一次 `dma_hw_setup`；NVIC 仍由驱动负责。
- 8 个 app 构建零告警；`adc.c` 强制重编零告警；HIL 冒烟 20_1/20_3/20_4/21 通过。

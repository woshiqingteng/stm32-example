# STM32 例程对齐与 LCD/RGB 策略 — 执行计划（PLAN.md）

> 目标：将 `app/` 下各例程的功能深度对齐厂商例程（寄存器版/HAL 版），修复已确认缺陷，并落实“仅 4.3" RGB 屏 / 非图像类走 printf”的策略。
> 基准路径：
> - 寄存器版：`D:/download/stm32/code/openedv/4，程序源码/1，标准例程-寄存器版本`
> - HAL 版：`D:/download/stm32/code/openedv/4，程序源码/2，标准例程-HAL库版本`

---

## 1. 锁定决策（口径）
| 项 | 决策 |
|---|---|
| LCD 驱动 | 仅保留 **4.3" RGB (LTDC)** 路径；MCU 面板(SSD1963/FMC 等)驱动删除（本仓已满足，仅核对并记录） |
| 输出策略 | **涉及图像显示**的 app 用 LCD；**其余一律 printf 串口** |
| RTOS | 不补充 FreeRTOS 例程 |
| SRAM | 当前芯片无 SRAM → **不做任何 SRAM app 相关工作** |
| `imu.c` 归属 | 新增 **`lib/imu`（`lib_imu`）** |
| `11_oled` 字库 | **全部尺寸 12/16/24**（现有仅 16） |
| `42_fatfs` | 挂载 **SD(0)+NOR(1)+NAND(2)**，NAND 卷经 FTL |
| `54_usb_device_msc` | 暴露 **三 LUN = NOR / NAND(FTL) / SD** |
| `30_touch_screen` | 电容屏 **不做校准**；补多点轨迹 |
| `53_iap` | 仅手控 + FLASH app；含擦除缺陷修复；**无 SRAM app 模式** |
| `53_iap_app` | 仅 FLASH 镜像；**无 SRAM 变体** |

## 2. 输出策略明细（LCD vs printf）
- **用 LCD**：`11_oled、12_tftlcd、14_ltdc_lcd、30_touch_screen、38_camera_stream、43_font、44_image、45_camera_storage、48_video、49_fpu`
- **用 printf**：其余全部（含 `13_sdram、16_rtc、22_1/22_3/23、25_i2c_extend_io、29_can、35_i2c_imu、42_fatfs、46_sai_audio、50_2_dsp_fft、53_iap、53_iap_app、54/55/56/57/58` 等）

## 3. 阶段总览
| 阶段 | 内容 | 提交建议 |
|---|---|---|
| P0 | 缺陷修复（正确性） | `fix: ...` |
| P1 | 屏幕/输出策略落地 | `refactor(lcd): ...` |
| P2 | 逐 app 深度补齐 | 按分组多次提交 |
| P3 | 验证与对照校验 | 收尾 |

---

## 4. P0 — 缺陷修复

| ID | 文件 | 工作内容 | 验收 |
|---|---|---|---|
| P0-1 | `bsp/openedv_stm32f4/iap.c` | 擦除逻辑从 `iap_write_appbin()` 移出；新增 `iap_erase_app(addr,len)`；接收镜像前按总长一次擦除，编程函数只写 | 多 2KB 块镜像不再互相擦除；`53_iap` 可完整烧写 |
| P0-2 | `lib/text/fonts.c` | `fonts_init()` 开头调用 `nor_init()`（lib 已依赖 nor） | 字库/NOR 读写前置初始化，`43_font` 可用 |
| P0-3 | `bsp/openedv_stm32f4/dcmi.c`(+`45_camera_storage/main.c`) | 新增 `dcmi_switch_ov5640()/dcmi_switch_sdcard()`（PC8/9/11 AF13↔AF12）；SD 存取与 DCMI 起停处切换 | 45 预览与 SD 存/读不再引脚冲突 |

## 5. P1 — 屏幕 / 输出策略
| ID | 文件 | 工作内容 | 验收 |
|---|---|---|---|
| P1-1 | `bsp/.../lcd.c` `lcd.h` | 核对无 MCU 面板残留符号 | 仅 RGB/LTDC 路径 |
| P1-2 | `verify.md` | 记录“LCD 仅 4.3" RGB；输出策略” | 文档一致 |
| P1-3 | 相关 `app/*/main.c` | 按第 2 节对齐输出（显示类用 LCD，其余 printf） | 分类一致 |

## 6. P2 — 逐 app 深度补齐

### 6.1 存储 / 文件系统
| ID | app | 工作内容 | 输出 | 验收 |
|---|---|---|---|---|
| P2-S1 | `13_sdram` | 全容量(32MB)写入/回读/计数测试，**KEY 触发** | printf | 覆盖全片；按键启动 |
| P2-S2 | `42_fatfs` | diskio 增 NAND 盘(2)（FTL）；挂载 SD0+NOR1+NAND2，格式化/卷标/总空 | printf | 三卷可见；`FF_VOLUMES` 足够 |
| P2-S3 | `54_usb_device_msc` | `usbd_storage_if.c` 三 LUN（NOR/NAND(FTL)/SD），逐 LUN 容量/读写/INQUIRY | printf | 主机识别 3 个盘 |
| P2-S4 | `53_iap` | 手控流程（WKUP 拷贝、KEY1 运行 FLASH app）；叠加 P0-1 修复 | printf | 可手控烧写并运行 |
| P2-S5 | `53_iap_app` | 保持 FLASH 镜像（`0x08010000`） | printf | 由 53_iap 跳转运行 |

### 6.2 传感器
| ID | app | 工作内容 | 输出 | 验收 |
|---|---|---|---|---|
| P2-N1 | `35_i2c_imu` + 新增 `lib/imu` | 移植 `imu.c/h`（Pitch/Roll/Yaw 融合，定周期 dt）；ANO_TC 上传帧(0xAA…)；注册 `lib_imu` | printf | 输出 RPY 与上传帧 |

### 6.3 显示
| ID | app | 工作内容 | 输出 | 验收 |
|---|---|---|---|---|
| P2-D1 | `43_font` | 整 GBK 遍历显示（12/16/24/32）；确保 nor_init | LCD | 全字符遍历 |
| P2-D2 | `11_oled` | 增 12/16/24 字形表 + `oled_show_*` 支持 | OLED | 三尺寸可用 |
| P2-D3 | `30_touch_screen` | 多点轨迹绘制（driver 已存 5 点）；不校准 | LCD | 多点可见 |
| P2-D4 | `45_camera_storage` | 移植 `sw_*_mode`；传感器原生 500W JPEG + BMP；含 P0-3 | LCD | 预览+原生拍照+落卡 |
| P2-D5 | `38_camera_stream` | 加 1:1 全尺寸、高屏(1024/1280) y 偏移/`0x3035` | LCD | 两种模式可切 |
| P2-D6 | `49_fpu` | Julia 渲染到 LCD + 缩放表/KEY 交互 | LCD | 屏上渲染+缩放 |
| P2-D7 | `44_image` / `48_video` | 字体前 `nor_init`，启用中文标签 | LCD | 中文标签可见 |

### 6.4 外设 / 模拟 / 交互
| ID | app | 工作内容 | 输出 | 验收 |
|---|---|---|---|---|
| P2-P1 | `16_rtc` | 启用 wakeup 定时中断（`rtc_set_wakeup`） | printf | 周期中断 |
| P2-P2 | `22_1_dac` / `23_pwm_dac` | 按键调节 + 读回 DAC/CCR + ADC 电压反馈 | printf | 闭环显示 |
| P2-P3 | `22_3_dac_sine` | 运行期生成正弦表 + 切频(3k↔30k) + ADC 反馈 | printf | 频率可切 |
| P2-P4 | `29_can` | 增 normal/loopback 模式切换 | printf | 两模式可用 |
| P2-P5 | `58_usb_host_hid` | 枚举超时恢复/重连 + 鼠标坐标累积 + 键盘输入串缓冲 | printf | 掉线可恢复 |
| P2-P6 | `50_2_dsp_fft` | KEY0 触发、打印全 1024 bin | printf | 全谱输出 |
| P2-P7 | `25_i2c_extend_io` | BEEP `write_bit` 演示 | printf | 蜂鸣可控 |
| P2-P8 | `46_sai_audio` | 输出时长/码率 | printf | 信息完整 |

## 7. P3 — 验证与对照校验
1. 逐 app 构建：`bash tool/build.sh debug <app>`（逐个）。
2. 全量回归：`bash tool/build.sh debug all all-freertos`，期望 **73/73**。
3. 对照本文件第 4–6 节逐项勾选验收栏。
4. 更新 `verify.md` 对应条目。
5. 分阶段提交（P0 / P1 / P2 分组 / 收尾）。

## 8. 最终校验清单
- [x] P0-1 IAP 擦除修复，`53_iap` 完整镜像可烧写（c7b518c）
- [x] P0-2 `fonts_init()` 前置 `nor_init()`（c7b518c）
- [x] P0-3 45 DCMI/SDIO 引脚切换（c7b518c）
- [x] P1 LCD 仅 RGB；非显示类全部 printf（待 P2-D6 补 49_fpu 用屏）
- [x] P2-S1..S5 存储项完成
  - S1 13_sdram 全容量按键测试
  - S2 42_fatfs SD0+NOR1+NAND2（弱钩子 + lib_nand_storage）
  - S3 54_usb_device_msc 三 LUN（NOR/NAND/SD）
  - S4 53_iap 手控（WKUP 编程 / KEY1 运行）
  - S5 53_iap_app 保持 FLASH 镜像
- [x] P2-N1 `lib/imu` 融合 + ANO_TC
- [x] P2-D1..D7 显示项完成
  - D1 43 全 GBK 遍历；D2 11 OLED 12/16/24；D3 30 多点；D4 45 原生 500W JPEG+BMP；
  - D5 38 缩放（既有 KEY_WKUP）；D6 49 LCD 渲染；D7 44/48 nor_init（fonts_init）
- [x] P2-P1..P8 外设/交互项完成
  - P1 16_rtc wakeup；P2 22_1/23 按键+ADC；P3 22_3 运行期表+切频；P4 29_can normal 模式；
  - P5 58 HID 累积/缓冲/重连；P6 50_2 FFT KEY 触发全谱；P7 25 BEEP；P8 46 时长/码率（既有）
- [x] 全量回归 73/73
- [x] `verify.md` 与本文档一致
- [x] 工作区分阶段提交、每阶段构建通过

## 9. 进度追踪
> 执行时逐项将 `[ ]` 改为 `[x]` 并注明提交号。

- [x] P0-1  [x] P0-2  [x] P0-3   (c7b518c)
- [x] P1-1  [x] P1-2  [x] P1-3
- [x] P2-S1 [x] P2-S2 [x] P2-S3 [x] P2-S4 [x] P2-S5
- [x] P2-N1
- [x] P2-D1 [x] P2-D2 [x] P2-D3 [x] P2-D4 [x] P2-D5 [x] P2-D6 [x] P2-D7
- [x] P2-P1 [x] P2-P2 [x] P2-P3 [x] P2-P4 [x] P2-P5 [x] P2-P6 [x] P2-P7 [x] P2-P8
- [x] P3 验证   (73/73, 396fb97)

## 10. 风险与备注
- `42_fatfs`/`54` 引入 NAND(FTL)：确认 `FF_VOLUMES`、port 链接 `lib_ftl`，NAND 扇区尺寸与 FTL 接口一致。
- `45` 引脚切换：确认与 RGB/LTDC、以及 SDIO 初始化顺序兼容。
- `11_oled`/`43_font` 字形表体量较大，注意 flash 占用。
- `35` 融合需较高主频与 `m`（math）链接。

---

# Style：状态机化与格式统一

## 1. 决策
- 位掩码/打包状态 → 显式状态枚举/状态机（A1–A8，含 A5 `ir.c`、A8 `g_device_state`）。
- 相机 JPEG 采集抽公共 helper：**新增 lib 层组件 `lib/cam_jpeg`（target `lib_cam_jpeg`）**，`38`/`45` 链接。
- 忙等加**有界超时**（C1 纳入）。
- `ftl` 仅格式化并**去除冗余/厂商注释**。
- 三组提交：**G1 状态枚举化 / G2 FTL 格式化 / G3 其他**；每步行为不变。

## 2. 排除项（硬件/协议寄存器位运算，保留）
`nand.c nor.c i2c.c spi ap3216c.c ds18b20.c io_expand.c touch.c qmi8658a.c
wireless.c ov5640.c codec.c lib/picture/gif.c piclib.c oled.c` CMSIS。

## 3. G1 状态枚举化
| ID | 文件 | 现状 → 目标 |
|---|---|---|
| S1 | `lib/audio/wavplay.c` | `s_dev.status` 位域 → `enum {AUDIO_STATE_IDLE,PLAYING,PAUSED}` |
| S2 | `app/baremetal/47_sai_record/recorder.c` | `g_rec_sta`(bit7/bit0) → `enum {REC_IDLE,RECORDING,PAUSED,PLAYING}` |
| S3 | `port/.../usbd_storage_if.{c,h}` + `54` | 值+错误位 → `enum usb_storage_activity_t{IDLE,READING,WRITING}` + `enum usb_storage_error_t{NONE,READ,WRITE}` |
| S4 | `port/.../usbd_cdc_if.c` | `g_usb_usart_rx_sta` 打包 → `enum {CDC_RX_IDLE,SEEN_CR,DONE}` + `g_cdc_rx_len` |
| S5 | `bsp/openedv_stm32f4/ir.{c,h}` | `g_ir_sta` 打包 → `enum {IR_IDLE,LEADER,DATA}` + edge/repeat/key/ready |
| S6 | `app/baremetal/38_camera_stream/main.c` | `g_ov_mode`/`g_jpeg_data_ok` → `enum cam_mode_t` + `enum jpeg_phase_t` |
| S7 | `app/baremetal/45_camera_storage/main.c` | `g_jpeg_ok` → `enum {JPEG_CAP_WAIT,JPEG_CAP_DONE}` |
| S8 | `port/.../usbd_conf.{c,h}` + `54/55/56` | `g_device_state` bool → `enum usbd_dev_state_t{DISCONNECTED,CONNECTED}` |

## 4. G2 FTL 格式化
| ID | 文件 | 目标 |
|---|---|---|
| F1 | `lib/ftl/ftl.h`, `lib/ftl/ftl.c` | 厂商横幅→项目 `/** @file @brief */`；TAB→4 空格；删除冗余/中文注释；逻辑不变 |

## 5. G3 其他
| ID | 文件 | 目标 |
|---|---|---|
| O1 | `lib/audio/wavplay.c`, `app/baremetal/47_sai_record/recorder.c` | 忙等加有界超时，超时退出并返回错误/停止 |
| O2 | 新增 `lib/cam_jpeg/{cam_jpeg.c,.h,CMakeLists.txt}`；重构 `38`/`45` | 抽出行缓冲+回调+`cam_jpeg_init/capture/size`；38 采集后经 USART2 发送，45 采集后写 SD；注册进 `lib/CMakeLists.txt` |
| O3 | `app/baremetal/16_rtc/main.c`、相机 `outsize` 常量 | 魔法数具名（`RTC_WAKEUP_1HZ`、`CAM_OUTSIZE_OFFSET_X` 等） |
| O4 | `verify.md` + 本文件勾选 | 记录 G1–G3 完成 |

## 6. 验证
- 逐组构建：`wavplay→46`、`recorder→47`、`storage→54`、`cdc→56`、`ir→31`、`camera→38/45`、`usbd_conf→54/55/56`。
- 全量回归 `bash tool/build.sh debug all all-freertos` → **73/73**。
- 三组分别提交（G1/G2/G3）。

## 7. 进度追踪
- [x] G1 S1 [x] S2 [x] S3 [x] S4 [x] S5 [x] S6 [x] S7 [x] S8
- [x] G2 F1
- [x] G3 O1 [x] O2 [x] O3 [x] O4
- [x] 全量回归 73/73

---

# Style（第二批）：逐 app 风格统一

## 0. 决策（锁定）
- include：统一为「system 段 (`<...>`) + 空行 + local 段 (`"..."`)」；删除被 `bsp.h` 聚合的冗余头（保留 `ltdc.h` 与 lib/USB 头）。
- 打印：仅「结果只喂 `printf`」的 `sprintf` 改直出；保留路径/文件名、LCD 显示、hex dump 等需缓冲者。
- 注释：精简冗余注释；不新增 `@brief`。
- 长行：折到 ≤100 列。
- 全局：行为不变；逐组构建 + 全量 73/73。

## 1. SG1 长行折行（≤100 列）
12_tftlcd、14_ltdc_lcd、22_1_dac、23_pwm_dac、30_touch_screen、38_camera_stream、
43_font、45_camera_storage、47_sai_record/recorder.c、48_video。

## 2. SG2 打印直出（仅 printf-only 的 sprintf）
21_internal_temp、24_i2c_eeprom、25_i2c_extend_io、26_i2c_als、28_rs485、29_can、
31_ir、32_1wire_temp、33_1wire_humi、35_i2c_imu、36_spi_wireless、39_malloc、
40_sdio_sdcard、50_1_dsp_math、54_usb_device_msc、57_usb_host_msc

## 3. SG3 include 统一
system 段 + 空行 + local 段；删除冗余 bsp 头（保留 ltdc.h 与 lib/USB 头）。

## 4. SG4 注释精简
删除复述代码的注释；保留文件级 @brief 与非直观说明；不新增 helper @brief。

## 5. 验证
逐组构建 + 全量 73/73；按 SG3 / (SG1+SG2) / SG4 提交；更新 verify.md M 章节。

## 6. 进度追踪
- [x] SG1 [x] SG2 [x] SG3 [x] SG4 [x] 全量回归 73/73

---

# Module 版本更新（参考 D:\work\git）

## 0. 决策
- 更新：freertos → V11.3.1；stm32_usb_device → v2.11.6；stm32_usb_host → v3.5.5。
- 保持：stm32_hal/stm32f4xx = v1.8.5（当前=最新 tag）。
- 不动：cmsis_dsp、fatfs、ijg_libjpeg、tjpgd（参考中更旧/缺失）。
- 旧版本目录保留。

## 1. 新增目录
module/freertos/11.3.1/、module/stm32_usb_device/2.11.6/、module/stm32_usb_host/3.5.5/

## 2. 导出（按参考 tag，裁剪所需）
- FreeRTOS @V11.3.1：tasks/queue/list/timers/event_groups/stream_buffer.c、include/*.h、
  portable/GCC/ARM_CM4F/{port.c,portmacro.h}、portable/MemMang/heap_4.c、LICENSE.md History.txt；
  复用 config/FreeRTOSConfig_common.h。
- USB device @v2.11.6：Core/{Inc,Src}、Class/{MSC,AUDIO,CDC,HID}/{Inc,Src}。
- USB host @v3.5.5：Core/{Inc,Src}、Class/{MSC,HID}/{Inc,Src}。

## 3. CMake
各新目录移植旧 CMake 源列表；版本指针 11.1.0→11.3.1、2.11.4→2.11.6、3.5.3→3.5.5。

## 4. 配置
保留 FreeRTOSConfig_common.h；USB 接口预计不变，按编译错误修正。

## 5. 验证
逐模块构建 01_led / 54-56 / 57-58；全量 73/73；更新 verify.md。

## 6. 进度追踪
- [x] 写 PLAN 章节 [x] FreeRTOS 11.3.1 [x] USB device 2.11.6 [x] USB host 3.5.5 [x] 全量回归 73/73

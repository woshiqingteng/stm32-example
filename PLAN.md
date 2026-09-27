# 逐应用对比参考程序 — 检查与改进计划

对比对象：ALIENTEK 阿波罗 V2 F429 例程
- 主参考：`…\4，程序源码\2，标准例程-HAL库版本\实验N…`（`User/main.c` + `Drivers/BSP/*`、`Drivers/SYSTEM/*`）
- 辅参考：`…\1，标准例程-寄存器版本`
- 当前：`app/baremetal/<app>/main.c` + `bsp/openedv_stm32f4/*.c`

类别：**P**=parity 缺口（必改）/ **R**=健壮性（可选建议）/ **S**=风格（可选建议）/ **I**=有意偏差（仅标注）。
状态：`[ ]` 待改、`[x]` 已改；R/S 标 `(可选)`，I 标 `(有意)`。

---

## 全局（影响所有 app）

| 项 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|
| 时钟 | `bsp/sys.c` vs `SYSTEM/sys/sys.c` | 180MHz 前缺 `HAL_PWREx_EnableOverDrive()`（VOS=SCALE1 下 >168MHz 必须使能） | P | OscConfig 与 ClockConfig 之间补 `HAL_PWREx_EnableOverDrive()` | [ ] |
| 初始化 | `bsp_init()` | 统一执行 usart_init(115200)+key_init；参考按实验按需 | I | 保留 | (有意) |
| delay | `bsp/delay.c` | FreeRTOS/uint64 计数 vs 参考 UCOS/uint32 | I | 保留 | (有意) |
| usart 重定向 | `bsp/usart.c` | `__io_putchar`+自定义 RXNE 状态机 vs 参考 fputc+Receive_IT | I | 保留 | (有意) |
| libc | `platform/soc/.../syscalls.c` | 自研 syscall、无 nosys | I | 保留 | (有意) |

## 批次 1：核心 01-07

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 01_led | 实验1 | main/led | 一致（额外 printf） | I | 无 | (有意) |
| 02_key | 实验2 | main WK_UP 分支 | 参考：`LED1_TOGGLE()` 后按 LED1 电平取反设 LED0；当前 LED0 与 LED1 同相 | P | WK_UP 时 `led0 = !led1` | [ ] |
| 02_key | 实验2 | `key.c` 扫描优先级 | 参考 WK_UP>KEY2>KEY1>KEY0；当前顺序相反 | P | 调整判定顺序 | [ ] |
| 02_key | 实验2 | `key.c` init | 参考输入引脚设 `GPIO_SPEED_FREQ_HIGH` | S | 可选 | (可选) |
| 03_exti | 实验3 | main WK_UP 中断 | 参考同时 `LED1_TOGGLE()` 且按 LED1 电平取反设 LED0；当前只翻 LED1 | P | 补 LED0 取反 | [ ] |
| 03_exti | 实验3 | `exti.c` NVIC | 参考抢占 KEY0=0/KEY1=1/KEY2=2/WK_UP=3（子均 2）；当前统一 3,2 | P | 分别配置 | [ ] |
| 03_exti | 实验3 | `exti.c` 回调 | 参考去抖 20ms 后复检引脚电平才动作 | R | 可选 | (可选) |
| 04_usart | 实验4 | `usart.c` 行结束 | 参考 CR 后必须紧跟 LF 才算一行；当前 CR 或 LF 任一即完成 | P | 改为 CR+LF | [ ] |
| 04_usart | 实验4 | main 周期输出 | 参考缺：无 50s 横幅；LED0 闪烁 300ms（`%30`）；当前 200ms 且无横幅 | P | 补横幅、改 30 次 | [ ] |
| 04_usart | 实验4 | `usart.c` 实现/日志 | 速度/回显格式差异 | I | 保留 | (有意) |
| 05_iwdg / 06_wwdg / 07_btim | 实验5/6/7 | main+wdg/btim | 一致 | P | 无 | [x] |

## 批次 2：定时器 08_*/09_*/10_tpad

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 08_1/08_2 | 8-1/8-2 | main+gtim | 一致 | P | 无 | [x] |
| 08_3_gtim_cap | 8-3 | `gtim.c` | 首次上升沿参考仅 CNT=0；当前 DISABLE→CNT=0→ENABLE | R | 可选 | (可选) |
| 08_3_gtim_cap | 8-3 | `gtim.c` | 超时阈值判定时机差 1 次溢出 | S | 可选 | (可选) |
| 08_4_gtim_cnt | 8-4 | `gtim.c` | 当前加临界保护（增强） | R | 保留 | (可选) |
| 09_1_atim_npwm | 9-1 | main | 参考显式 CCR=5000(50%)；当前驱动默认 4999 | S | 可选 | (可选) |
| 09_2/09_3 | 9-2/9-3 | main+atim | 一致 | P | 无 | [x] |
| 09_4_atim_pwmin | 9-4 | main | 整数换算可能溢出/丢小数 | R | 可选 | (可选) |
| 10_tpad | 10 | `tpad.c` | 类型宽度、参数校验差异（增强） | R/S | 保留 | (可选) |

## 批次 3：低功耗/模拟 16/17/18_*/19/20_*/21/22_*/23/37

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 16_rtc | 16 | main/rtc | 默认时间/BKP magic/初值差异 | I | 保留 | (有意) |
| 16_rtc | 16 | `rtc.c` | LSE 就绪后参考仍 OscConfig | S | 可选 | (可选) |
| 17_rng | 17 | main/rng | 触发方式、DeInit | R/S | 可选 | (可选) |
| 18_1_pvd | 18-1 | main | 参考 PVD 回调驱动 LED1；当前仅打印 | S | 可选 | (可选) |
| 18_2/18_3/18_4 | 18-2/3/4 | pwr/exti | WK_UP EXTI 优先级参考(2,2) vs 当前(3,2) | S | 可选 | (可选) |
| 18_4_standby | 18-4 | `pwr.c` | 参考复位备份域；当前保留（配合 BKP magic） | I | 保留 | (有意) |
| 19_dma | 19 | main | 缓冲/完成判定差异 | I | 保留 | (有意) |
| 20_1/20_2/20_4 | 20-1/2/4 | main+adc | 一致 | P | 无 | [x] |
| 20_3_adc_multi_dma | 20-3 | `adc.c` | EOCSelection 参考 SINGLE vs 当前 SEQ | S | 可选 | (可选) |
| 21_internal_temp | 21 | main/adc | 周期/连续模式/GPIO 速度差异 | S | 可选 | (可选) |
| 22_1_dac | 22-1 | main/dac | 按键/步进/范围差异；回读通道 PA5 vs PA4 | I | 保留 | (有意) |
| 22_2/22_3 | 22-2/3 | main/dac | 实现/频率/表值差异 | I | 保留 | (有意) |
| 23_pwm_dac | 23 | main/pwmdac | 回读通道/步进映射差异 | I | 保留 | (有意) |
| **37_internal_flash** | 37 | `internal_flash.h` | **地址越界**：`0x081E0000` > 1MB(0x100000)，写操作被地址校验拦截→空操作 | P | 改回 `0x08010000`（扇区4，16KB内） | [ ] |
| 37_internal_flash | 37 | `internal_flash.c` | 擦写未处理 ICACHE | R | 可选 | (可选) |

## 批次 4：显示 11/12/13/14/30/49

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 11_oled | 11 | main | 信息行 y=48 vs 52、CODE_X=88 vs 94（数字与 CODE 重叠） | P | 改 `OLED_INFO_Y=52`、`OLED_CODE_X=94` | [ ] |
| 11_oled | 11 | `oled.c` | 公开 API(draw_point/show_char/fill)被 static/删除 | P | 按需补回 `oled_draw_point/show_char/fill` | [ ] |
| 12_tftlcd | 12 | main/lcd | MCU 屏→LTDC RGB（有意） | I | 保留 | (有意) |
| 13_sdram | 13 | main | 参考用 LCD 显示与 KEY 交互、容量/图形测试算法；当前仅 USART 简化 | P | 评估是否对齐 LCD/算法 | [ ] |
| 14_ltdc_lcd | 14 | `ltdc.c` | 仅支持 0x4384 面板 | I | 保留 | (有意) |
| 14_ltdc_lcd | 14 | `lcd.c` | 缺 draw_line/circle/fill_circle/set_window/ram_prepare/show_xnum | P | 按需补绘图层 API | [ ] |
| 30_touch_screen | 30 | main | 无 RST 清除区、触点固定蓝色 | P | 补 RST 区与按触点取色 | [ ] |
| 30_touch_screen | 30 | `touch.c` | 竖屏映射缺 `width-` 镜像；无抽帧/越界/释放处理 | P/R | 修正竖屏映射；可选抽帧 | [ ] |
| 49_fpu | 49 | main | 缩放表(26 vs 14)、镜像方向、标题提示、计时方式差异 | P | 对齐表/镜像/标题；建议用 BTIM 0.1ms | [ ] |

## 批次 5：通讯 15/24/25/26/27/28/29/31/36

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 15_usmart | 15 | main/usmart | 无 LCD/函数注册，时基改 1us 轮询 | I | 保留 | (有意) |
| 24/25/26 | 24/25/26 | main/i2c/eeprom/io/ap3216c | 主要一致；iic_wait_ack 循环内加延时 | R | 可选 | (可选) |
| 27_spi_nor | 27 | main/spi/nor | 一致（速度档差异） | S | 可选 | (可选) |
| 28_rs485 | 28 | `rs485.c` | 发送后未清 RX 计数（可能回显自身） | R | 可选：发送后清零 | (可选) |
| 29_can | 29 | `can.c` | 发送后未等邮箱空；ExtId/GPIO 速度差异 | R/S | 可选 | (可选) |
| 31_ir | 31 | main/ir | 一致 | P | 无 | [x] |
| 36_spi_wireless | 36 | main/wireless/spi | 接收缓冲末字节、IRQ 超时、SPI 模式设置差异 | R/S | 可选 | (可选) |

## 批次 6：传感器 32/33/34/35

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 32_1wire_temp | 32 | ds18b20 | 一致 | P | 无 | [x] |
| 33_1wire_humi | 33 | dht11 | 校验和返回值语义（当前更健壮） | R | 保留 | (可选) |
| 34_i2c_magnet | REG34 | `st480mc.c` | 温度原始值用 int16 致负温异常；应用 uint16 | P | 改 uint16/float | [ ] |
| 34_i2c_magnet | REG34 | main | 方位角参考显示 `360-angle`；当前直接 angle | P | 改 360-angle 或注明 | [ ] |
| 34_i2c_magnet | REG34 | main | 校准键/节奏/显示差异 | I | 保留 | (有意) |
| 35_i2c_imu | 35 | `qmi8658a.c` | 缺片上初始化校准与运行时零偏校准，存在静差 | P | 补校准 | [ ] |
| 35_i2c_imu | 35 | main | ANO 帧格式(0x01=12B 交叉映射/0x02=18B)、波特率 500000 差异 | P | 对齐地面站帧与波特率 | [ ] |
| 35_i2c_imu | 35 | qmi/imc | 量程/单位差异 | I | 保留 | (有意) |

## 批次 7：多媒体/图像 38/43/44/45/46/47/48/50_*

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 38_camera_stream | 38 | main/dcmi/ov5640 | 初始化表一致；1:1/居中简化 | I | 保留 | (有意) |
| **43_font** | 43 | `lib/text/fonts.c` | **扇区校验只查前 1KB（应按 4KB/0xFFFFFFFF）**，后 3KB 非空时跳过擦除→可能写未擦扇区 | P | 改 `uint32_t`/`0xFFFFFFFF` 整扇区校验 | [ ] |
| 43_font | 43 | main | 扫描范围/字号/KEY0 强制刷字库差异 | P | 按需对齐 | [ ] |
| 44_image | 44 | `jpeg_dec.c` | 快/慢模式与 DMA2D 填充、定点缩放未采用（性能差距大） | P | 采用 fillcolor 快路径 | [ ] |
| 45_camera_storage | 45 | `dcmi.c` | 切 SD/NOR 未写 `0x3017/0x3018` 关/开传感器输出 | P | 补写寄存器 | [ ] |
| 45_camera_storage | 45 | main | JPEG 保存未按 `FFD8/FFD9` 截取有效长度 | P | 按 SOI/EOI 截取 | [ ] |
| 45_camera_storage | 45 | main | 预览坐标/按键重排/无蜂鸣 | P/S | 确认是否对齐 | [ ] |
| 46_sai_audio | 46 | wavplay/codec/sai | 解码/寄存器一致（多处当前更健壮） | P | 无 | [x] |
| 47_sai_record | 47 | recorder/sai | 一致 | P | 无 | [x] |
| 48_video | 48 | `mjpeg.c` | 未用 DMA2D 硬件搬运（帧率差距） | P | 接回 DMA2D | [ ] |
| 50_1_dsp_math | 50_1 | main | 参考循环反复测试 + BTIM 0.1ms 计时；当前跑一次+1ms tick | P | 恢复循环/计时 | [ ] |
| 50_2_dsp_fft | 50_2 | main | 参考 1MHz 计时/浮点幅值；当前整数截断 | P | 恢复浮点/高频计时 | [ ] |

## 批次 8：存储 39/40/41/42

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| 39_malloc | 39 | main/malloc | 交互/定位方式差异 | I | 保留 | (有意) |
| 40_sdio_sdcard | 40 | main/sdio | 交互/MSP 内联差异 | I | 保留 | (有意) |
| **41_nand** | 41 | `nand.c` | **未生成/写 spare ECC**（参考按 512B 扇区取 ECCR 并 0x85 写 spare） | P | 补 ECC 生成+写 | [ ] |
| **41_nand** | 41 | `nand.c` | **单 bit 纠正判据写成 `0xFFFF`（应为 `0xFFF`）**，纠正分支恒不触发 | P | 改回 `0xFFF` | [ ] |
| 41_nand | 41 | `nand.h` | 注释称 ECC disabled，与实现矛盾 | S | 更新注释 | (可选) |
| 42_fatfs | 42 | diskio | SD 读写无重试/重初始化 | R | 可选 | (可选) |
| 42_fatfs | 42 | ffconf | R0.16/FF_USE_LFN/NORTC 差异 | I | 保留 | (有意) |

## 批次 9：USB/IAP 53/54/55/56/57/58

| App | 参考 | 比对面 | 差异 | 类别 | 建议 | 状态 |
|---|---|---|---|---|---|---|
| **53_iap** | 53 | `iap.c` | **扇区映射错误**：硬编码 128K，`0x08010000` 得 sector 0→误擦 Bootloader/未擦 APP | P | 按地址查表(0-3:16K,4:64K,5-11:128K) | [ ] |
| **53_iap** | 53 | `iap.c` | **跳转校验错误**：校验 APP 首字在 Flash；应校验栈顶∈SRAM | P | 校验 SP∈0x20000000~0x20030000 | [ ] |
| 53_iap | 53 | main | 自定义帧协议/无 LCD/SRAM 模式 | I | 保留 | (有意) |
| 53_iap_app | 53 | main | VTOR 设置晚于 `bsp_init()`（期间中断走 Boot 向量表） | R | 提到 bsp_init 前 | (可选) |
| 54_usb_device_msc | 54 | `usbd_conf.h` | `MSC_MEDIA_PACKET=512` vs 参考 32K（吞吐低） | P | 提高到 4K/8K | [ ] |
| 54_usb_device_msc | 54 | `usbd_storage_if.c` | SD 缺失仍上报 3 LUN；无离线检测 | R | 可选 | (可选) |
| 55_usb_device_audio | 55 | `usbd_audio_if.c` | 欠载未填静音（重复旧数据） | R | 可选 | (可选) |
| 55_usb_device_audio | 55 | 新增回环/懒初始化 | I | 保留 | (有意) |
| **56_usb_device_cdc** | 56 | main | **缓冲区溢出**：`char line[48]` 但 len 最大 199 | R→P | 用 `USB_USART_REC_LEN` 或截断 | [ ] |
| 57_usb_host_msc | 57 | main | 主循环 `delay_ms(500)` vs 参考 10ms（枚举/传输迟钝） | P | 高频 `USBH_Process` | [ ] |
| 57_usb_host_msc | 57 | `usbh_diskio.c` | 未按 SCSI sense 区分 NOTRDY/WRPRT | R | 可选 | (可选) |
| 58_usb_host_hid | 58 | main | 主循环 `delay_ms(500)` vs 参考无延时（响应慢） | P | 高频处理 | [ ] |
| 58_usb_host_hid | 58 | main | 无枚举卡死检测/重连复位 | R | 可选 | (可选) |
| 53-58 | — | USB 库版本 | 设备 2.11.6/主机 3.5.5 vs 参考更旧 | I | 保留 | (有意) |

---

## 有意偏差汇总（不改）
- RGB 4.3" 屏专用（LTDC），MCU 屏/SSD1963 驱动移除；无 SRAM 运行模式。
- 无 51 手写/52 T9/59 网络/60-63 UCOS；无气压计 app（板无器件）。
- 统一 `bsp_init()`、printf/串口日志、无 LCD 的 app 用 USART 输出。
- `34_i2c_magnet` 源自寄存器版实验34（HAL 版无磁力计）。
- libc/FTL/USB 等架构级重构与版本升级（保留）。

## 进度
- Phase 0 分析：完成（9 批次，53 app）。
- Phase 1 写入本文件：完成。
- Phase 2 P 修复（按类别分组提交）：待执行。
- Phase 3 全量 73/73 + 按本表核验：待执行。

---

## 执行记录（Phase 2，按类别提交）
- 2c/之后提交：
  - `fix(core)`：sys OverDrive；key 扫描优先级；02_key/03_exti WK_UP 反向联动；exti 分键优先级；usart CR+LF；04_usart 横幅/闪烁。
  - `fix(storage)`：37 EEPROM 地址 0x08010000；41 NAND 写 spare ECC + 单 bit 判据 0xFFF。
  - `fix(usb/iap)`：53 扇区表/跳转校验；54 MSC_MEDIA_PACKET=32K；56 行缓冲；57/58 USBH 轮询 10ms。
  - `fix(display/sensor/media)`：11 OLED 版式；30 触摸竖屏映射；34 温度 uint16/方位 360-angle；43 字库整扇区校验；45 DCMI 0x3017/0x3018；syscalls 补 `_kill/_getpid`。
  - `fix(dsp)`：50_1 循环反复测试。
- **全量回归：73/73 通过**（debug）。

### 状态标记
- 已完成 [x]：全局 OverDrive；02_key（WK_UP/优先级）；03_exti（WK_UP/NVIC）；04_usart（CRLF/横幅）；37；41（ECC）；53（扇区/跳转）；54（包大小）；56（缓冲）；57/58（轮询）；11（版式）；30（竖屏映射）；34（温度/方位）；43（扇区校验）；45（DCMI 寄存器）；50_1（循环）。
- 仍待处理（较大改动，P）：11 驱动公开 API；13 SDRAM 参考算法/显示；14 绘图层 API；30 RST 清除区/按触点取色/抽帧；35 IMU 校准/ANO 帧/波特率；43 扫描范围与强制刷字库；44 JPEG 快慢模式/定点缩放；45 JPEG SOI/EOI 截取与按键重排；48 DMA2D 搬运；49 FPU 缩放表/镜像/标题/计时；50_2 计时与浮点幅值。

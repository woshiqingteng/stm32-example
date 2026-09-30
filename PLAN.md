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
| 看门狗 init | `bsp/wdg.c` | `iwdg_init()/wwdg_init()` 采用板级固定值、无参数 | I | 保留 | (有意) |
| 06_wwdg LED | `06_wwdg/main.c` | 绿灯闪烁按 EWI 软件分频(÷10)以便观察（参考为每次 EWI 翻转） | I | 保留 | (有意) |

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
| 08_1_gtim_int | 8-1 | main+gtim | 一致 | P | 无 | [x] |
| 08_2_gtim_pwm | 8-2 | main | 亮度范围 0..100%（厂商 0..60%）、整体更慢（500 步×10ms，每程 5s） | I | 保留 | (有意) |
| 08_3_gtim_cap | 8-3 | `gtim.c`+main | 重构：状态机移到 app（回调+单枚举事件+显式 switch）、捕获 16→32 位、软件溢出累计（uint64）、公共接口仅 `init`+`register`、去旧超时；打印自动换单位 us/ms/s/min/h（3 位小数） | I | 保留 | (有意) |
| 08_4_gtim_cnt | 8-4 | `gtim.c` | get_count 临界保护 + restart 清 UPDATE + 32 位 CNT/32 位溢出计数、总量 64 位（宏更名 GTIM_CNT_*） | I | 保留 | (有意) |
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
| 12_lcd_mcu | 12 | main/lcd | MCU 屏→LTDC RGB（有意） | I | 保留 | (有意) |
| 13_sdram | 13 | main | SDRAM 容量/数据测试；KEY0 容量、KEY1 数据回显 | P | 已去 LCD，改 USART（按键提示+结果） | [x] |
| 14_lcd_rgb | 14 | `lcd_rgb.c` | 仅支持 0x4384 面板 | I | 保留 | (有意) |
| 14_lcd_rgb | 14 | `lcd.c` | 缺 draw_line/circle/fill_circle/set_window/ram_prepare/show_xnum | P | 按需补绘图层 API | [ ] |
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
| 32_1wire_temp | 32 | `temp`（←ds18b20） | 一致 | P | 无 | [x] |
| 33_1wire_humi | 33 | `humi`（←dht11） | 校验和返回值语义（当前更健壮） | R | 保留 | (可选) |
| 34_i2c_magnet | REG34 | `st480mc.c` | 温度原始值用 int16 致负温异常；应用 uint16 | P | 改 uint16/float | [ ] |
| 34_i2c_magnet | REG34 | main | 方位角参考显示 `360-angle`；当前直接 angle | P | 改 360-angle 或注明 | [ ] |
| 34_i2c_magnet | REG34 | main | 校准键/节奏/显示差异 | I | 保留 | (有意) |
| 35_i2c_imu | 35 | `imu.c`(SH3001) | 上电/按键软件零偏校准（陀螺+加速度；SH3001 无片上校准寄存器） | P | 已实现 | [x] |
| 35_i2c_imu | 35 | main | 移除 ANO 帧/500000 波特率，改为 USART 文本(115200、每 500ms) | I | 保留 | (有意) |
| 35_i2c_imu | 35 | imu | 量程 ±8g / ±500dps（与现有换算一致） | I | 保留 | (有意) |
| 35_i2c_imu | - | main | 倾角补偿航向角（ST480MC，无磁校准） | 扩展 | 已实现 | [x] |
| 35_i2c_imu | - | imu | 运动中断 Tap/Free-Fall/Activity（SH3001 中断状态轮询；板上 6D_INT 边沿不可用） | 扩展 | 已实现 | [x] |
| 35_i2c_imu | - | imu | FIFO（stream，acc+gyro；KEY2 切换直读/FIFO） | 扩展 | 已实现 | [x] |
| 35_i2c_imu | - | imu | 陀螺动态零偏跟踪（静止时慢速更新） | 扩展 | 已实现 | [x] |

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

## 执行记录（Phase 2，第二批：35/49/45/50_2/13/30）
- `bc07121` 35_i2c_imu：片上校准(Ctrl9=0xA2,校验0x46)、ANO 0x01=12B/0x02=18B、500000 波特率
- `47cd7cd` 49_fpu：26 档缩放表、水平镜像、标题、TIM6 10kHz 0.1ms 计时、auto 默认关
- `c680db0` 45_camera_storage：按键 KEY0=BMP/KEY1=JPG/KEY2=对焦、JPEG 按 FFD8..FFD9 截取
- `a70d018` 50_2_dsp_fft：1MHz BTIM 计时 + 3 位小数幅值
- `091d03d` 13_sdram：参考容量算法 + LCD 显示 + 按键/数据回显
- `7c2a8ee` 30_touch_screen：按触点取色 + 右上角 RST 清除区
- **全量回归 73/73 通过**。

## 未执行（已评估，属“可选/性能”）
- `14_lcd_rgb`：补 `lcd_draw_line/circle/fill_circle/set_window/ram_prepare/show_xnum`（当前无 app 使用，属 API 完整性，非功能 parity）。
- `44_image` 快/慢模式 DMA2D 与定点缩放、`48_video` DMA2D 直写帧缓冲：大改动、性能 parity，无功能缺陷风险。
- 其余 R/S 项按约定仅列不改。

## 执行记录（Phase 2，第三批：14/44/48）
- `7a4ebd0` lcd：新增 `lcd_draw_line/lcd_draw_circle/lcd_fill_circle/lcd_set_window/lcd_write_ram_prepare`（`show_xnum` 原已存在）。
- `409eb54` 44_image：新增 **快模式**（按 MCU 目标矩形用 DMA2D `pic_phy.fillcolor` 块填充）与 **慢模式定点缩放**（`picinfo.Div_Fac` 移位，取代逐像素除法）。
- `48_video`：经核查已通过 `lcd_color_fill`（DMA2D M2M）逐行搬运，**已达 DMA2D 路径**，无需改动（且行缓冲位于 SRAMIN，DMA 可访问；参考的 CCM 缓冲不宜作 DMA 源）。
- **全量回归 73/73 通过**；`PLAN.md` 全部 P 项处理完毕。

## 执行记录（Phase 2，第四批：显示驱动分层）
- **11 OLED**：拆出 `oled_ssd1306`（SSD1306 协议 + 传输；`OLED_BUS` 宏，默认 8080、SPI 实装、I2C 占位）；`oled.c` 降为纯面板/图形。
- **12/13/14 LCD**：拆出 `lcd_rgb`（RGB 面板驱动：ID strap + 时序/PLL，单面板硬编码 4.3"/`0x4384`）；`ltdc` 降为纯控制器（`ltdc_init(const lcd_rgb_cfg_t *)`）；`lcd.h` 与 `ltdc.h` 解耦（新增 `lcd_dir_t`；`ltdc` 不再依赖 `lcd`）。
- 面板宏改名 `LTDC_PANEL_*` → `LCD_PANEL_*`；同步小改 `30_touch_screen`（`LCD_DIR_*`）、`38_camera_stream`/`45_camera_storage`（`LCD_PANEL_*`）、`touch.c`（含 `ltdc.h`）。

## 执行记录（Phase 2，第五批：显示驱动瘦身/去冗余）
- **对外接口简约化**：`lcd.h` 查询收敛为单一 `lcd_info()`（`lcd_info_t` 聚合 `pwidth/pheight/width/height/id/dir/pixsize/framebuf`）；删除 `lcd_get_width/height/id`、`_lcd_dev`、`g_point_color`/`g_back_color`（改为 `lcd_set/get_back_color()`）。
- **颜色类型统一为 `uint32_t`**（`lcd_draw_point/clear/fill/show_char/num/string`），与 `_pic_phy` 函数指针一致，避免类型不匹配。
- **删死代码**：`lcd_set_window`、`lcd_write_ram_prepare`、`lcd_show_xnum`、`lcd_draw_line/draw_circle/fill_circle`（0 调用）。
- **去重复**：`ltdc.c` 抽 `ltdc_rect_addr()`（`ltdc_fill`/`ltdc_blit` 共用）；删冗余 DMA2D 时钟使能。`lcd_color_fill` → `lcd_blit`、`ltdc_color_fill` → `ltdc_blit`。
- **健壮性**：`lcd_show_char` 增加字符范围检查（修 `text.c` 对 0x7F/0x80 的越界取字库）。
- **R7 收敛**：`ltdc` 变为 bsp 内部；`lib`（`text/piclib/bmp/mjpeg/videoplayer`）与 `app`（12/14/30/38/44/45/49）改用 `lcd_*`/`lcd_info()`，不再包含 `ltdc.h`；`38/45` 的 JPEG 缓冲基址改用 `lcd_info()->framebuf`（编译期面板尺寸经 `lcd_rgb.h`）。

## 执行记录（Phase 2，第六批：LVGL 8.3.11 移植）
- **module/lvgl/8.3.11**：按版本目录惯例（同 `freertos/11.3.1`）；目标 `lvgl`（`EXCLUDE_FROM_ALL`，仅链 `drv_core`，**不依赖 FreeRTOS**）。
- **配置分层（仿 FreeRTOS）**：公共 `module/lvgl/8.3.11/config/lv_conf_common.h`（由 `lv_conf_template.h` 生成，**每个 `#define LV_*` 均 `#ifndef` 守卫**）；单 app `app/freertos/lv_29_keyboard/lv_conf.h` 先覆盖再 `#include "lv_conf_common.h"`。
- **tick**：`LV_TICK_CUSTOM` 走 `lvgl_tick.h`/`lvgl_tick_ms()`（OS 无关）；实现 `port/openedv_stm32f4/lvgl/lv_port_tick.c`，用 `#if USE_FREERTOS` 切换：FreeRTOS=`xTaskGetTickCount()`，裸机=`HAL_GetTick()`。
- **port**：`port/openedv_stm32f4/lvgl` → `lvgl_port`（`lv_port_disp.c` flush=`lcd_blit`、`lv_port_indev.c`=`touch_*`、`lv_port_tick.c`）；链 `lvgl bsp_lcd bsp_touch bsp_delay`。`lib_wrapper(lib_lvgl lvgl_port)`。
- **app**：`app/freertos/lv_29_keyboard`（`add_freertos_app`；`BSP lcd touch sdram`；`LIB lvgl`）：`lv_keyboard` 键盘示例 + LED 心跳任务；`FreeRTOSConfig.h` 覆盖 `configTOTAL_HEAP_SIZE=48KB`。
- **内存**：SDRAM `0xC0000000` 帧缓冲 / `0xC0100000` LVGL 池(512KB) / `0xC0200000` draw buffer(64KB/40 行)；FreeRTOS 堆 48KB（内部 SRAM）。
- 裸机复用同一套 module/port/配置（`USE_FREERTOS=0` 时 tick 自动走 `HAL_GetTick`），无需改动。
- **tick 注入（无反向依赖）**：`lvgl` 模块内定义 `g_lvgl_tick_fn`（`config/lvgl_tick.c`），`lv_conf_common.h` 的 `LV_TICK_CUSTOM_SYS_TIME_EXPR=(g_lvgl_tick_fn())`；由 **port** 的 `lv_port_tick_init()` 注入（`#if USE_FREERTOS` → `xTaskGetTickCount()`，否则 `HAL_GetTick()`）。依赖严格下行（`lvgl→drv_core`；`lvgl_port→lvgl/bsp_*`），**无 module→port 反向边**。
- **验证（`lv_29_keyboard`，自动）**：`build` RC=0（text≈211KB / bss≈55KB；`-Wall` 仅 LVGL 上游 5 条告警：`extra/libs/qrcode/qrcodegen.c`×2、`misc/lv_tlsf.c`×3）。串口横幅 `app_freertos_lv_29_keyboard`；`g_lvgl_tick_fn=0x08003ad9(=&tick_read) ≠ &lvgl_tick_default`；`xTickCount` **580→1578ms**（Δ≈1000ms/1s，tick 前进）；帧缓冲 `0xC0000000` **非空**（`f7be…`，UI 已绘制）；`PC` 位于 idle 任务（**非 HardFault**）。触摸交互未自动核验（GT9147 需真实触摸）。

## 执行记录（Phase 2，第七批：审查发现修复）
- **F1 字号 32 字库步长**：`lcdfont.h` `asc2_3216[95][128]→[95][64]`、`lcd.c` `LCD_FONT_3216_BYTE 128U→64U`（重构把 `font->bytes` 当尺寸+步长；数据实为 64B/字形，参考实现 csize=64）。节省 **6080B** 常量（`nm` 实测 `asc2_3216=0x17c0`）。`43_font` 增加内置 ASCII-32 示例行以便目视。
- **F2（先删后修）**：`module/lvgl` 依赖回退为 `drv_core`（lvgl **可**依赖 HAL；**不**直接依赖 `soc_${MCU_FAMILY}`）；移除 drv_core 会导致 lvgl 丢失 FPU ABI 编译选项而链接失败。
- **F3 撤销**：保留 app 侧 `target_include_directories(lvgl PUBLIC ...)` 注入（与 `add_freertos_app` 对 `freertos` 的做法一致；每 app 独立 CMake 树，无冲突），不用 `LV_CONF_PATH`。
- **F4 SDRAM 布局**：`lv_conf.h` pool `0xC0100000→0xC0400000`、`lv_port_disp.c` draw `0xC0200000→0xC0480000`（帧缓冲 0xC0000000 预留 4MB）；`lv_port_disp_init()` 加帧缓冲越界 `printf` 防呆。
- **F5**：删除 `lv_29_keyboard` 自定义键盘回调（v8 中 `LV_SYMBOL_KEYBOARD` 内置为 CANCEL；模式切换用内置键 `1#`/`abc`/`ABC`）。
- **F6**：对 `qrcodegen.c` 加 `-Wno-type-limits`、`lv_tlsf.c` 加 `-Wno-unused-parameter`（定向抑制上游告警）。
- **验证**：`build all` / `build all-freertos` RC=0、**0 告警**；`43_font` 烧录运行（`page 0: GBK FONT OK`）；`lv_29_keyboard` 自动核验：`g_lvgl_tick_fn=&tick_read`、`xTickCount` **689→1695ms**、帧缓冲非空、非 HardFault。

## 执行记录（Phase 2，第八批：`ltdc` 并入 `lcd_rgb` + 应用改名）
- **合并**：删除 `ltdc.{c,h}`，其面板探测 + LTDC 控制器内容并入 `lcd_rgb.{c,h}`（模块名 `lcd_rgb`）；`lcd_rgb.h` 现直接 `#include "stm32f4xx_hal.h"`（消除 `RCC_PLLSAIDIVR_4` 依赖包含顺序的隐患）。
- **标识符不改**：`ltdc_*`、`LTDC_*`（自定义宏/枚举）、`lcdltdc`、`g_ltdc_handle`、`g_ltdc_framebuf`、`g_dma2d_handle`、`lcd_rgb_cfg_t`、`lcd_rgb_probe`、`LCD_PANEL_*` 全部沿用。
- **CMake**：删除 `bsp_ltdc`；`bsp_lcd_rgb` 承接 `drv_ltdc drv_dma2d`；`bsp_lcd PUBLIC bsp_lcd_rgb`；`bsp_touch PUBLIC bsp_lcd`（修正：touch 仅用 `lcd_info()`）；`bsp_all` 去 `bsp_ltdc`。
- **包含**：`bsp/.../lcd.c` 改 `#include "lcd_rgb.h"`；`38/45` 仍含 `lcd_rgb.h`（不变）；`platform/**`（`LTDC_IRQHandler`）与 `module/stm32_hal/**` 不动。
- **应用改名**：`12_tftlcd`→`12_lcd_mcu`、`14_ltdc_lcd`→`14_lcd_rgb`（目录/`add_baremetal_app`/`@brief`/`printf`/交叉注释/PLAN 名引用）。
- **验证**：`build all` / `build all-freertos` RC=0、**0 告警**（含 `12_lcd_mcu`/`14_lcd_rgb`/`30/38/43/45` 全量重建）。烧录 `14_lcd_rgb`：横幅 `app_baremetal_14_lcd_rgb`、`14_lcd_rgb ready, LCD ID:4384`、颜色循环正常。烧录 `lv_29_keyboard` 自动核验：`g_lvgl_tick_fn=&tick_read`、`xTickCount` **718→1722ms**、帧缓冲 `0xC0000000` 非空、非 HardFault。

## 执行记录（Phase 2，第九批：`lcd_rgb` 全局收敛 + 类型改名）
- **类型**：`_ltdc_dev`（文件作用域 `_` 前缀为 C 保留标识符）→ **`ltdc_dev_t`**。
- **全局内聚**：`lcd_rgb.c` 内 `g_ltdc_dev`（原名 `lcdltdc`）、`g_ltdc_handle`、`g_dma2d_handle`、`g_ltdc_framebuf[2]` 全省为 `static`；`lcd_rgb.h` 删除对应 4 行 `extern`。
- **访问器**：新增 `const ltdc_dev_t *ltdc_info(void)` 与 `uint32_t ltdc_framebuf(void)`（活动层帧缓冲基址），替代对外暴露全局。
- **`lcd.c`**：`lcd_sync_info()` 改用 `ltdc_info()`/`ltdc_framebuf()`；5 处 `pwidth!=0` 守卫改 `ltdc_info()->pwidth`。
- **验证**：`build all` / `build all-freertos` RC=0、**0 告警**；烧录 `14_lcd_rgb`（`LCD ID:4384`、颜色循环正常）；`lv_29_keyboard` 自动核验：`g_lvgl_tick_fn=&tick_read`、`xTickCount` **704→1708ms**、帧缓冲非空。

## 执行记录（Phase 2，第十批：对外接口与具体驱动解耦，仿 `oled`/`oled_ssd1306`）
- **规则**：`X.{c,h}`=对外接口（函数名不变，消费方免改）+ 设备无关逻辑；`X_<chip>.{c,h}`=具体驱动（芯片常量/寄存器/引脚/总线/初始化/原始访问）。CMake `bsp_X_<chip>` + `bsp_X` PUBLIC 链接之。
- **touch**：新增 `touch_gt9xxx.{c,h}`（`touch_gt9xxx_init/read`；CT_IIC/GT9XXX 寄存器/读点为原始点，**无 `g_touch`/`lcd`**）；`touch.{c,h}` 保留公共 API + `touch_map_raw`（坐标映射）。**`ct_iic_delay()` 删除，22 处直接 `delay_us(GT9XXX_IIC_DELAY_US)`**。
- **eeprom**：新增 `eeprom_at24cxx.{c,h}`（`AT24*`/`EE_TYPE`/`read_one_byte`/`write_one_byte`/`check`）；`eeprom.{c,h}` 转发 + 块读写。
- **codec**：新增 `codec_es8388.{c,h}`（`ES8388_ADDR` + 全部寄存器级配置）；`codec.{c,h}` 为转发层。
- **imu**：新增 `imu_sh3001.{c,h}`（`IMU_ADDR`/`IMU_CHIP_ID_VAL`/寄存器图/原始读/温度/中断/FIFO）；`imu.{c,h}` 保留标定/动态偏置，`IMU_STATUS_*`/`IMU_FIFO_SAMPLE_LEN`/`IMU_ACC_1G_COUNT`/`IMU_CAL_*`/`IMU_DYN_*` 留公共。
- **wireless**：新增 `wireless_nrf24l01.{c,h}`（引脚/SPI 命令/寄存器图/宽度）；`wireless.{c,h}` 转发；`36_spi_wireless/main.c` 增 `#include "wireless_nrf24l01.h"`（唯一消费方改动）。
- **CMake**：`bsp_touch_gt9xxx`(bsp_delay)+`bsp_touch`(→bsp_touch_gt9xxx,bsp_lcd)；`bsp_eeprom_at24cxx`(bsp_i2c,bsp_delay)+`bsp_eeprom`；`bsp_codec_es8388`(bsp_i2c,bsp_delay)+`bsp_codec`；`bsp_imu_sh3001`(bsp_i2c)+`bsp_imu`(→bsp_imu_sh3001,m,bsp_delay)；`bsp_wireless_nrf24l01`(bsp_spi)+`bsp_wireless`。`bsp_all` 不变。
- **验证**：`build all`/`all-freertos` RC=0（新/改文件 **0 告警**；全量重编暴露 3 处**既有**告警：`24_i2c_eeprom/main.c:26` 字符串初始值、`wavplay.c:131/133` 枚举、`usbd_def.h` `-Wundef`，均非本次引入）。烧录自检：`24_i2c_eeprom`（`24C02 ready`/读写 `OK`）、`35_i2c_imu`（`SH3001 ready`/标定/温度/acc-gyro）、`30_touch_screen`（`touch ready`）、`47_sai_record`（SD/menu）、`36_spi_wireless`（无模块→`NRF24L01 not found!`，初始化路径正常）。公共头经核对**无芯片细节泄漏**。

## 执行记录（Phase 2，第十一批：app 仅经功能头接触器件）
- **R1 wireless**：`wireless.h` 增 `WIRELESS_PLOAD_WIDTH 32U`（功能契约）；`wireless.c` 加 `_Static_assert` 与 `NRF24L01_TX/RX_PLOAD_WIDTH` 一致；`36_spi_wireless` 改用该宏并删除 `wireless_nrf24l01.h`。
- **R2 lcd 几何**：`lcd.h` 增 `LCD_WIDTH_PX 800U`/`LCD_HEIGHT_PX 480U`（原生光栅，供编译期缓冲尺寸）；`lcd_rgb.h` 删 `LCD_PANEL_WIDTH/HEIGHT_PX`（时序仍私有）；`lcd_rgb.c` 含 `lcd.h` 用新宏；`38_camera_stream`/`45_camera_storage` 换宏并删 `lcd_rgb.h`。
- **R3 传感器功能层**：新增 `mag.{c,h}`（←`st480mc`：`mag_init/read/read_average/read_temperature`）与 `als.{c,h}`（←`ap3216c`：`als_init`、`als_read(ir,ps,light)`）。改 `34_i2c_magnet`/`35_i2c_imu`（`mag.h`）、`26_i2c_als`（`als.h`）；app BSP 改 `mag`/`imu mag`/`als`。
- **CMake**：`bsp_als`→`bsp_ap3216c`；`bsp_mag`→`bsp_st480mc`；`bsp_all` 追加 `bsp_als bsp_mag`。芯片头 `st480mc.h`/`ap3216c.h` 保持私有。
- **暂不改**：`ds18b20.h`/`dht11.h`/`ov5640.h`（含 `lib/usmart`）及 `temp/humi/ov` 相关 app（按决定保留现状）。
- **验证**：`build all`/`all-freertos` RC=0、新/改文件 **0 告警**；烧录自检 `26_i2c_als`（IR/PS/ALS 数据）、`34_i2c_magnet`（`ST480MC ready`+MagX/Y/Z）、`35_i2c_imu`（`SH3001 ready`+`ST480MC ready`）、`36_spi_wireless`（无模块→not found）、`38_camera_stream`（`OV5640 error`＝相机未接，编译/启动正常）。

## 执行记录（Phase 2，第十二批：`temp`/`humi` 功能接口）
- **新增** `temp.{c,h}`（←`ds18b20`）：`temp_init()`、`temp_read()`（0.1°C，int16）。
- **新增** `humi.{c,h}`（←`dht11`）：`humi_init()`、`humi_read(uint8_t *temp_c, uint8_t *rh)`。
- **app**：`32_1wire_temp`（include `temp.h`；`ds18b20_*`→`temp_*`；BSP `ds18b20`→`temp`）、`33_1wire_humi`（include `humi.h`；`dht11_*`→`humi_*`；BSP `dht11`→`humi`）。
- **CMake**：`bsp_temp`(→`bsp_ds18b20`)、`bsp_humi`(→`bsp_dht11`)；`bsp_all` 追加。芯片头 `ds18b20.h`/`dht11.h` 保持私有（单线时序/编解码留芯片，不抽 `singlewire`）。
- **待办（未执行）**：相机解耦——`camera.{c,h}`（←`ov5640`，外加 `dcmi` 采集不变），改 `38/45`（BSP `ov5640`→`camera`）与 `lib/usmart`；按决定**暂不执行**。
- **验证**：`build all`/`all-freertos` RC=0、新/改文件 **0 告警**；烧录 `32_1wire_temp`/`33_1wire_humi` 启动正常（传感器未接→`DS18B20 not found!`/`DHT11 not found!`，`temp_init`/`humi_init` 路径已执行）。

## 执行记录（Phase 2，第十三批：非显示应用去 LCD）
- **原则**：仅“图像/视频/相机/触摸/LVGL/字库”等**必要**用 LCD 的应用保留 `lcd`；其余改 `printf` 串口输出。
- **`13_sdram`**：删除 `#include "lcd.h"` 与全部 `lcd_*` 调用；菜单/容量结果改 `printf`。**去掉正点原子横幅**（`STM32`/`ATOM@ALIENTEK`/`SDRAM TEST`），仅保留必要信息：`APP_BANNER` + `KEY0: capacity test  KEY1: data dump` + `SDRAM Capacity:<n>KB`。CMake `BSP sdram lcd`→`BSP sdram`。
- **`49_fpu` 保留 LCD**（Julia 分形渲染类，去屏即失去意义）。
- **保留清单**：`12_lcd_mcu`/`14_lcd_rgb`/`30_touch_screen`/`43_font`/`44_image`/`48_video`/`38/45_camera`/`lv_29_keyboard`。
- **验证**：`13_sdram` 构建 RC=0、0 告警；烧录串口仅 `app_baremetal_13_sdram` + `KEY0: capacity test  KEY1: data dump`（无厂商横幅）。

## 执行记录（Phase 2，第十四批：非 LCD/OLED 应用去厂商/演示横幅）
- **原则**：除 LCD/OLED 类外，去除厂商/演示字样（含 `STM32` 前缀）；**源码注释保留**。
- **`31_ir`（中性名）**：`ir.h` `IR_KEY_ALIENTEK=71`→`IR_KEY_MENU=71`；`31_ir/main.c` `"ALIENTEK"`→`"MENU"`。
- **串口/测试串**：`19_dma` `"STM32F429 USART1 TX DMA demo - …"`→`"USART1 TX DMA: 0123456789\r\n"`；`24_i2c_eeprom` `"STM32 IIC TEST"`→`"IIC TEST"`；`37_internal_flash` `"STM32 FLASH TEST"`→`"FLASH TEST"`；`42_fatfs` `ALIENTEK.TXT`→`FATFS.TXT`、`"ALIENTEK FATFS TEST"`→`"FATFS TEST"`；`56_usb_device_cdc` 去 `STM32 `。
- **保留**：LCD/OLED 类横幅；所有源码注释（`35/fusion.c`、`39_malloc`、`lib/*`、`53_iap`、`ir.h`）。
- **验证**：`build all`/`all-freertos` RC=0；**`24_i2c_eeprom` 的 `-Wunterminated-string-initialization` 告警消失**；烧录抽查 `19_dma`/`24_i2c_eeprom`(`String: IIC TEST (OK)`)/`37_internal_flash`(`"FLASH TEST"`)/`31_ir`(干净横幅)/`42_fatfs`(`FATFS.TXT`/`FATFS TEST`)/`56_usb_device_cdc`(USB CDC) 均无厂商/演示字样。

## 执行记录（Phase 2，第十五批：按键/LED 提示统一）
- **规则**：提示行统一 `KEY0: <a>  KEY1: <b>  WKUP: <c>`（冒号后 1 空格、条目间 2 空格、唤醒键统一 `WKUP`）；置于**所有外设初始化之后**；内嵌提示的 ready 行**整行删除**；LCD 类不改；`34_i2c_magnet` 保留其特定动作提示。
- **A 类（改文案/去 ready）**：`18_2/18_3/18_4`（`WKUP` 标签）、`19_dma`(去 ready→`KEY0: send`)、`22_1/23`(`WKUP: +  KEY0: -`)、`22_3`(`KEY0: switch frequency`)、`29_can`(`WKUP: toggle mode`)、`35_i2c_imu`(`KEY0: recalibrate`)、`36_spi_wireless`(`KEY0: RX  KEY1: TX`)、`47_sai_record`、`50_2_dsp_fft`(`KEY0: run %u-point FFT`)、`53_iap`(`WKUP: receive+program  KEY1: run app`，frame 行独立)。
- **B 类（LED 演示新增）**：`02_key`/`03_exti`（`... WKUP: both (opposite)`，WKUP 使两灯互补）、`09_1_atim_npwm`(`KEY0: reset pulse count`)。
- **C 类（新增+统一）**：`05_iwdg`(`WKUP: feed watchdog`)、`08_4_gtim_cnt`(`KEY0: restart count`)、`41_nand`(`KEY0: read  KEY1: write  KEY2: restore`)、`55_usb_device_audio`(`KEY0: vol+  KEY2: vol-  WKUP: default vol`)。
- **验证**：`build all`/`all-freertos` RC=0、0 告警；烧录抽查 `02_key`/`09_1`/`19_dma`/`22_1`/`29_can`/`35_i2c_imu`/`55_usb_device_audio` 串口均显示统一提示。

## 执行记录（Phase 2，第十六批：SDRAM 通用/器件参数分离）
- **方案 B（轻量抽常量）**：`sdram.h` 新增 `sdram_cfg_t`（FMC 几何/时序/CAS/刷新/mode 的通用类型）；新增 `sdram_w9825g6kh.h`（型号私有 `static const g_sdram_w9825g6kh`，仅 `sdram.c` 包含）；`sdram.c` 保留引脚+FMC 编程+JEDEC 序列+缓冲访问，改从 cfg 取值。对外 API/调用**不变**（app 与 `usmart_config.c` 免改）。
- **注释纠正**：`sdram.c` “IS42S16400 style” → **“W9825G6KH-6 (8192×512×16, 32 MB)”**（含文件头 `@brief`）。依据芯片资料 `W9825G6KH.pdf`：阵列 8192×512×16、8K Refresh/64ms、-6 档 tRC60/tRAS42/tRCD18/tRP18/tWR2tCK/tXSR72。
- **时序**：SDCLK=90MHz 下 TMRD=2、TXSR=7、**TRAS=4**（手册最小）、TRC=6、TWR=2、TRP=2、TRCD=2（均满足且 FMC 域范围 1..16、TRC≥TRAS+TRP）。
- **刷新重算**：改为运行期公式 `count = tREF*(HAL_RCC_GetHCLKFreq()/div/1000)/rows − 20` → 90MHz 得 **683**（原写死 730/96MHz 的 TODO 删除）。
- **验证**：`build all`/`all-freertos` RC=0、0 告警；烧录 `13_sdram`（`SDRAM Capacity:32768KB`、`KEY1` 图案 `0000 0001 …`）、`12_lcd_mcu`（`LCD ID:4384`，帧缓冲 `0xC0000000=ffffffff`）、`lv_29_keyboard`（横幅，帧缓冲 `f7be…`）均正常。

## 执行记录（Phase 2，第十七批：SDRAM 时序按 ns 存、运行期换算）
- **动机**：原时序是写死的周期数（按 90MHz），换钟（如 PLLN=336/392）会失配；改为**绝对时间存 ns、相对时间存 tCK**，运行期按实际 SDCLK 换算。
- **`sdram_cfg_t`**：`tmrd_cycle`/`twr_cycle`（相对，tCK）+ `trcd_ns`/`trp_ns`/`trc_ns`/`tras_ns`/`txsr_ns`（绝对，ns）。
- **`sdram.c`**：`#include <stdio.h>`；新增 `ns_to_cycle(ns, sdclk_hz)`（`ceil(ns*f)`、夹取 1..16，`>16` 打印告警并夹取）；`sdclk_hz=HAL_RCC_GetHCLKFreq()/div` 在组装前读取；`TRC ≥ TRAS+TRP` 守卫；刷新沿用同一 `sdclk_hz`。前提写入头注释：**先配时钟、后 `sdram_init()`**。
- **预期换算**（PLLM=25、PLLP=2、AHB/1、SDCLK=HCLK/2）：PLLN360(HCLK180/SDCLK90)→`2/2/2/2/6/4/7`+683；PLLN336(168/84)→同周期+636；PLLN392(超频 196/98)→`trcd2/trp2/trc6/tras5/txsr8`+745（全部 ≤16、COUNT ≤8191）。
- **验证**：`build all`/`all-freertos` RC=0、0 告警；默认钟（180MHz/SDCLK90）烧录 `13_sdram`（`32768KB`+干净图案）、`12_lcd_mcu`（帧缓冲 `0xC0000000=ffffffff`）无回归。

---

## 执行记录（Phase 2，第十八批：存储/传感器驱动拆分 + 命名统一）
- **EEPROM 重命名**：`eeprom_at24cxx.{c,h}`→`eeprom_at24c02.{c,h}`（函数/宏/头保护/CMake `bsp_eeprom_at24c02`）；保留 `AT24C01..AT24C256` 表与 `EE_TYPE=AT24C02`；`eeprom.c` 转发改名。
- **NOR 拆分（A 式，芯片持总线）**：`nor.h`（功能）仅留尺寸宏 + `nor_init/nor_read_id/nor_read/nor_write/nor_erase_sector`（去 CS/ID 宏、`g_nor_type`、HAL）；新增 `nor_w25q256jv.{c,h}`（芯片：CS(PF6)、`_cs_low/high`、`_spi_rw`、`_probe`(0x90)、`_addr_bytes`(=4)、`_dev_init` 含 4 字节模式）；`nor.c` 算法经芯片 cs/spi。CMake `bsp_nor_w25q256jv`(PUBLIC `bsp_spi bsp_delay`) → `bsp_nor`。
- **NAND 拆分（A 式，功能 + 器件）**：`nand.h` 删 `nand_info_t`/`nand_get_info`、器件 ID 宏、`extern g_nand_handle`、`NAND_RB_*`、HAL include；新增 `nand_mt29f4g08.{c,h}`（`nand_device_t`+`nand_mt29f4g08_probe(id)`，仅 `<stdint.h>`）；`nand.c` 加 HAL、`NAND_RB_*` 内移、`g_nand_handle` static、`nand_init` 用 probe 填 `nand_dev`（未知→返回 1）、`nand_eraseblock`→`blocknum*nand_dev.block_pagenum`、spare 偏移用器件值；`ftl` 不动。CMake `bsp_nand_mt29f4g08` → `bsp_nand`。
- **传感器命名统一（4 组，含器件内部宏前缀）**：`dht11→humi_dht11`、`ds18b20→temp_ds18b20`、`ap3216c→als_ap3216c`、`st480mc→mag_st480mc`（文件/库/函数/头保护/器件宏 `HUMI_DHT11_*`/`TEMP_DS18B20_*`/`ALS_AP3216C_*`/`MAG_ST480MC_*`）；`imu_sh3001` 内部宏 `IMU_ADDR/IMU_REG_*/IMU_CHIP_ID_VAL`→`IMU_SH3001_*`（`imu.h` 的 `IMU_STATUS_*/IMU_FIFO_*` 等保留）。
- **`bsp_all`**：保持“全部”语义（feature + 芯片），仅同步被重命名的 4 个传感器芯片目标名。
- **不改**：`fatfs_*`、`stm32_usb_device_msc`、`lib/text`、app 的 `nor/nand/ftl/eeprom/humi/temp/als/mag/imu` 名称。
- **验证**：`build all`/`all-freertos` RC=0、0 告警（`ftl.c` 有 6 处历史遗留 `-Wsign-compare`/`-Wformat`，按“ftl 不动”未触及）；真机 `27_spi_nor`（ID `0xEF18`、Write/Read OK）、`41_nand`（4096 块/512MB、FTL OK）、`42_fatfs`（SD/NOR/NAND 三盘挂载+读写 OK）、`54_usb_device_msc`（启动）、`24_i2c_eeprom`（24C02 ready）、`26_i2c_als`/`34_i2c_magnet`/`35_i2c_imu`（数据正常）、`33_1wire_humi`/`32_1wire_temp`（外部 1-wire 传感器未接，探测路径正常，与改名无关）。

---

# 计划：`test/` pytest 硬件在环全自动验证（01–10）

## 目标
- 对 app `01_led` … `10_tpad` 共 **16 个**做**烧录后自动验证**；**全自动、无人操作**（不按键/不触摸）。
- 以 pytest + OpenOCD(CMSIS-DAP, 按序列号指定) + pyserial 实现。

## 目录（`PLAN.md` 在仓库根；其余测试相关在 `<root>/test/` 下）
```
<root>/PLAN.md
<root>/test/
  requirements.txt          # pytest, pyserial
  run.py                    # 读 config + 传入测试文件 → 连接检测 → -m hw 运行
  conftest.py               # -m hw 门控 + 自动连接检测(autouse) + flash fixture + 日志
  pytest.ini                # markers: hw
  config/
    setting.py              # 硬件参数：ADAPTER_SERIAL="ATK_20210914" / SERIAL_PORT="COM4" / 115200 / FLASH_ADDR
    stm32f429.py            # 设备文件：仅地址/偏移参考（CMSIS 风格）
  page/
    __init__.py
    base.py                 # class BasePage：连接/OpenOCD/串口等通用
    openedv_stm32f429.py    # class OpenEdvSTM32F429Page(BasePage)：pins/odr 板级特异
  log/                      # 测试日志（与 page/ 同级）
  test/                     # 具体测试用例（每 app 一个文件，文件内可多项）
    test_01_led.py … test_10_tpad.py
```

## 类职责
- **BasePage**：`connect()`（`openocd -f interface/cmsis-dap.cfg -c "adapter serial <sn>" -f target/stm32f4x.cfg -c "init" -c "shutdown"`，判 `Interface ready/DPIDR`）；常驻会话 telnet:4444；`program/reset_run/halt/resume/sleep/peek/poke`；串口读写。
- **OpenEdvSTM32F429Page**：绑定 `config/*`；`press()/release()` 引脚注入（MODER/PUPDR/BSRR + 复原）；`led_odr()`；`reset_flags()`。

## 逐 app 验证矩阵（16）
| App | 方法 | 类型 | 状态 |
|---|---|---|---|
| 01_led | 采样 ODR，两灯交替 | 真功能 | [ ] |
| 02_key | 注入 4 键，读 ODR 序列 | 真功能 | [ ] |
| 03_exti | 注入 4 键（EXTI 边沿） | 真功能 | [ ] |
| 04_usart | COM4 发 `abc\r\n` 读回显 | 真功能 | [ ] |
| 05_iwdg | 周期喂狗不复位 / 停喂触发 IWDGRSTF | 真功能 | [ ] |
| 06_wwdg | 采样 LED1 + WWDGRSTF | 真功能 | [ ] |
| 07_btim | 采样 ODR（500/200ms） | 真功能 | [ ] |
| 08_1_gtim_int | 采样 ODR | 真功能 | [ ] |
| 08_2_gtim_pwm | 采样 TIM3->CCR4 呼吸 | 真功能 | [ ] |
| 08_3_gtim_cap | 注入 app 捕获态（poke `g_cap_state`/`g_cap_width`）→ 断言 `HIGH:1.234 ms` | 状态注入 | [ ] |
| 08_4_gtim_cnt | 注入 TIM2 CNT → 断言 CNT:；KEY0 清零 | 寄存器注入 | [ ] |
| 09_1_atim_npwm | 采样 PC6（≈0.5s）数 5 脉冲后停 + KEY0 重触发 | 真功能 | [ ] |
| 09_2_atim_oc | 读 CCMR/CCR1..4 + 采样 PC6..9 | 真功能 | [ ] |
| 09_3_atim_cplm | 读 TIM1/BDTR + 采样 PE8/PE9 互补 | 真功能 | [ ] |
| 09_4_atim_pwmin | 读 COM4 freq: 行 | 真功能 | [ ] |
| 10_tpad | 注入阈值/基准模拟触摸 → 断言 LED1 翻转 | 寄存器注入 | [ ] |

## 运行
- `python run.py test/test_02_key.py [-k …]`（内部 `-m hw`；先自动连接检测）。
- 无连接/无板自动 skip；IWDG/WWDG 用例期间不 halt。

## 执行记录
- [x] 写 PLAN.md
- [x] 冒烟：烧录 02_key + pyserial 读 COM4
- [x] 实现 config/page/conftest/run.py/requirements.txt
- [x] 16 个 test/test_*.py
- [x] 预构建 16 app 并运行
- [x] 对照本表核验 + 提交

## 结果与说明
- **35 passed**（连续两次全绿，单次 ~95s）；`python test/run.py`（内部 `-m openedv_stm32f429`）。
- 板级 marker `openedv_stm32f429`；`run.py --board <id>` 选择板子跑对应用例；hw 检测在 conftest（autouse，无 hw marker）。
- 超时兜底：pytest `--test-timeout`（默认 90s，watchdog 线程注入异常）+ 命令级超时。
- 烧录健壮性：`reset halt`→`program verify`，失败则 `reset halt/init/run` 复位芯片并重试（最多 5 次）。
- 说明：本机 ATK 板载 CMSIS-DAP（VID_04D8/PID_00DF，复合 S/N `ATK_20210914`）不向 OpenOCD 暴露可匹配序列号，`adapter serial` 会 “no matching device”；故 `ADAPTER_SERIAL` 默认留空=自动选择（`config/setting.py` 内已注明）；当前探针为 CMSIS-DAP **v2**，未降速。
- 一个实测坑：反复强杀 openocd 会让 DAP 进入 “CMSIS-DAP command mismatch”，需重启该 USB 设备；框架已在会话开始前清理残留 openocd。
- 三个物理不可达用例的注入策略：08_3（TIM5 捕获）、08_4（TIM2 计数）、10（tpad 基线）经 `nm` 解析符号地址后注入；09_4 注入前先关 TIM8 中断。

## 提交
- `test: add pytest HIL checks for apps 01-10`（`test/` 全量 + 根 `PLAN.md` + `.gitignore`）

---

# 计划：USART 统一驱动重构（传输/解析分层 + poll/IT/DMA）

## 目标
- BSP `usart` 只做**传输**（按 id：`USART_ID_1/2`），统一 `write/read/busy`，并入 poll/IT/DMA；**行解析上移到 app**。
- 无兼容别名；配置用结构体 + 默认宏 `USART_CFG_DEFAULT`。
- 每阶段独立提交并可回归。

## API（`bsp/openedv_stm32f4/usart.h`）
```c
typedef enum { USART_ID_1 = 0, USART_ID_2 = 1, USART_ID_NUM } usart_id_t;
typedef enum { USART_IO_POLL = 0, USART_IO_IT, USART_IO_DMA } usart_io_t;

typedef struct {
    usart_id_t id; uint32_t baudrate;
    uint32_t word_length, stop_bits, parity, mode, hw_flow_ctl, oversampling;
    usart_io_t tx, rx;            /* USART_ID_2 的 DMA：reserved，回退 POLL */
    uint8_t *rx_buf; uint16_t rx_size;   /* M1：外部接收缓冲 */
    uint32_t irq_preempt, irq_sub;
} usart_cfg_t;

#define USART_CFG_DEFAULT(inst) /* 8N1@115200、无流控、16 过采样、tx=POLL、rx=POLL、IRQ 3/3 */

void      usart_init(const usart_cfg_t *cfg);
bool      usart_write(usart_id_t id, const uint8_t *data, uint32_t len);
bool      usart_tx_busy(usart_id_t id);
uint32_t  usart_read(usart_id_t id, uint8_t *data, uint32_t len, uint32_t timeout);
typedef void (*usart_rx_cb_t)(uint8_t byte);
void      usart_set_rx_cb(usart_id_t id, usart_rx_cb_t cb);
```
删除：`usart_init(uint32_t)`、`usart2_init`、`usart_register_rx_byte_hook`、`usart_tx_dma*`、`usart_rx_state/len/buf/clear`、`usart_rx_state_t`、`USART_REC_LEN`、`usart_poll`、`usart_read_flush`、`extern g_uart1/2_handle`。保留 `__io_putchar`（固定阻塞）。

## 规则
- `rx=IT/DMA` 且无 `rx_buf` → RX 不使能（安全）。
- `id=USART_ID_2` 且 `tx/rx=DMA` → 回退 POLL（reserved）。
- 排空惯用法（替代 flush）：`while (usart_read(id,&b,1,0)==1) {}`。
- `__io_putchar` 恒阻塞（printf 安全）。

## 模式语义
| 方向 | POLL | IT | DMA |
|---|---|---|---|
| TX | 阻塞 | `Transmit_IT` | `Transmit_DMA` |
| RX | `read` 读硬件 | RXNE ISR→`rx_buf` 环形+回调 | 循环 DMA 写 `rx_buf` + `USART_IT_IDLE`→回调 |

## DMA
- USART1 TX=DMA2_Stream7（现有）；USART1 RX=DMA2_Stream2（阶段 2）。
- USART2 TX/RX DMA **本轮不做**（保留参数入口，回退 POLL）。

## 迁移清单
- `bsp/.../usart.[ch]`：新驱动；`USART1/2_IRQHandler`、`DMA2_Stream7_IRQHandler`（+阶段2 `DMA2_Stream2_IRQHandler`）；handles 改 `static`。
- `bsp/.../bsp.c`：控制台 `rx=POLL`（只发，无缓冲）；接收类 app（04/15_usmart/rs485）自设 `rx=IT`+缓冲。
- `bsp/.../rs485.[ch]`：删自建 MSP 与 `USART2_IRQHandler`；`usart_set_rx_cb(USART_ID_2,…)`；`rs485_send`→`usart_write`。
- `lib/usmart/usmart_port.c`：`usart_set_rx_cb(USART_ID_1,…)`。
- `bsp/.../iap.c`：改 `usart_read`；`AbortReceive`→排空循环。
- `app/04_usart`：app 内 CR/LF 解析 + `usart_set_rx_cb`。
- `app/19_dma`：cfg tx=DMA + `usart_write` + `usart_tx_busy`。
- `app/35_i2c_imu`、`app/38_camera_stream`、`app/54–58`：改新 API。

## 阶段与工作流
1. 写本计划（完成）。
2. 阶段 1：新 API + 全量迁移 + 04 解析入 app + rs485 回调。校验：构建受影响 app + 04 HIL（35 passed）。
3. 阶段 2：USART1 RX-DMA（Stream2 + IDLE）。校验：回归。
4. 最后校验：受影响集合全量构建 + HIL（35 passed）。

## 执行记录
- [x] 写入本计划
- [x] 阶段 1（`bd20a67`）：新 API + 全量迁移 + 04 解析入 app + rs485 回调；HIL 35 passed。
- [x] 阶段 2（本提交）：USART1 RX-DMA（DMA2_Stream2 + IDLE 回填/回调）；HIL 35 passed。
- [x] 最后校验：全量构建 `all` 通过（70 app，无失败）+ HIL 35 passed。

---

# 计划：USART DMA 补全 + 驱动修正（Option S）

## 决策
- **RX-DMA 用 `USART_IT_IDLE` 驱动，不使用任何 RX 流 IRQ**（循环 DMA 硬件搬运 + `NDTR` 计算）；IDLE 分支顺带清一次 RX 流 TE 标志。
- **TX-DMA 需要流 IRQ**（完成置 `gState=READY`）：USART1 `DMA2_Stream7`、USART2 `DMA1_Stream6`（均不与其它驱动共享）。
- RX 流：USART1 `DMA2_Stream2`、USART2 `DMA1_Stream5`（后者与 DAC 共用同一流，不同时启用 → 无链接冲突，`dac.c` 保留其 handler）。
- `usart_write` 不加 timeout 参数；POLL 内部改 1000ms。
- 内部类型重命名 `usart_ctx_t → usart_handle_t`。
- P1：`usart_init()` 重初始化前 `HAL_UART_AbortReceive()` + 关 `RXNE/IDLE` IT。
- P3：更新注释（`hdma_rx`、`usart_read` 字节间超时、DMA-RX 覆盖风险）。

## 改动
- `bsp/openedv_stm32f4/usart.c`：重命名；P1；`usart_dma_tx_init/rx_init` 按 id 参数化；USART1/2 的 IDLE 分支（+TE 清标志）；删 `DMA2_Stream2_IRQHandler`、增 `DMA1_Stream6_IRQHandler`；POLL 写超时 1000ms。
- `bsp/openedv_stm32f4/usart.h`：去 “USART2 DMA reserved”，注明字节间超时。
- `dac.c` 不改（无冲突）。

## 验证（只验对应文件）
- 构建 `04_usart`、`28_rs485`、`19_dma`、`22_1_dac`（无重复符号）。
- HIL `test/test/test_04_usart.py`（预期 2 passed）。

## 执行记录
- [x] 写入本计划
- [x] 修改 `usart.c/.h`（重命名、P1、USART2 TX/RX DMA、IDLE+TE 清标志）
- [x] 构建对应文件（`04_usart`/`28_rs485`/`19_dma`/`22_1_dac`/`35`/`38`，无告警）+ 04 HIL（2 passed）

---

## 命名约定（宏）

适用于手写代码（`app/`、`bsp/`、`lib/`、`port/`）；`platform/`、`module/` 等第三方保持原样。

### 物理量单位后缀（单数）
| 类别 | 后缀 | 示例 |
|---|---|---|
| 时间 | `_MS` / `_US` / `_S` | `BTIM_LOOP_MS` |
| 频率 | `_HZ` / `_KHZ` / `_MHZ` | `BSP_SYSCLK_MHZ` |
| 字节/字 | `_BYTE` / `_WORD` | `DMA_TX_CHUNK_BYTE` |
| 计数 | `_COUNT` / `_PULSE` / `_SAMPLE` / `_TICK` | `ADC_AVG_COUNT` |
| 像素/几何 | `_PX` / `_PIXEL` / `_LINE` | `LTDC_PANEL_WIDTH_PX` |
| 电压/温度 | `_VOLT` / `_MV` / `_DEGC` | `ADC_TEMP_OFFSET_DEGC` |
| 比例 | `_RATIO` / `_PERCENT` | `DMA_TX_PROGRESS_SCALE_PERCENT` |
| 波特率 | `_BAUD` | `RS485_BAUD` |

- 复数一律改单数：`_BYTES→_BYTE`、`_WORDS→_WORD`、`_TICKS→_TICK`、`_SAMPLES→_SAMPLE`、`_TIMES→_COUNT`、`_PULSES→_PULSE_COUNT`。
- 仅**单位相关**宏在行尾加 `/*!< 物理含义 + 单位 */` 说明；无单位类（instance/port/pin/IRQ/flag/color/address/magic）不加单位后缀。

### 寄存器值命名（不暴露原始值，仅命名）
- 计时器节拍值：基线名已是寄存器名（`ARR / CCR / RLR / COUNTER / WINDOW / MODULUS`）→ 不加后缀；描述性计时值 → 加 `_TICK`（如 `IR_PERIOD_TICK`）
- 其它原始字段（`DTG`、`PLLN/M/P/Q`、`SDRAM_TIMING_*`、阈值/过滤等）→ 加 `_RAW`
- 纯枚举选择器（`PVD_LEVEL`、`RTC_WAKEUP_*`）→ 不改名，仅加注释
- **仅改名+注释，不改数值、不做换算**。


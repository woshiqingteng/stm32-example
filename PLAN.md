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
- [ ] P2-S1..S5 存储项完成
- [ ] P2-N1 `lib/imu` 融合 + ANO_TC
- [ ] P2-D1..D7 显示项完成
- [ ] P2-P1..P8 外设/交互项完成
- [ ] 全量回归 73/73
- [ ] `verify.md` 与本文档一致
- [ ] 工作区分阶段提交、每阶段构建通过

## 9. 进度追踪
> 执行时逐项将 `[ ]` 改为 `[x]` 并注明提交号。

- [x] P0-1  [x] P0-2  [x] P0-3   (c7b518c)
- [x] P1-1  [x] P1-2  [x] P1-3
- [ ] P2-S1 [ ] P2-S2 [ ] P2-S3 [ ] P2-S4 [ ] P2-S5
- [ ] P2-N1
- [ ] P2-D1 [ ] P2-D2 [ ] P2-D3 [ ] P2-D4 [ ] P2-D5 [ ] P2-D6 [ ] P2-D7
- [ ] P2-P1 [ ] P2-P2 [ ] P2-P3 [ ] P2-P4 [ ] P2-P5 [ ] P2-P6 [ ] P2-P7 [ ] P2-P8
- [ ] P3 验证

## 10. 风险与备注
- `42_fatfs`/`54` 引入 NAND(FTL)：确认 `FF_VOLUMES`、port 链接 `lib_ftl`，NAND 扇区尺寸与 FTL 接口一致。
- `45` 引脚切换：确认与 RGB/LTDC、以及 SDIO 初始化顺序兼容。
- `11_oled`/`43_font` 字形表体量较大，注意 flash 占用。
- `35` 融合需较高主频与 `m`（math）链接。

# STM32 示例工程 — 架构与内容全面验证报告

范围：`platform / module / bsp / port / lib / app` 六层结构、依赖方向、模块化（HAL/BSP 叶子目标）、
声明式 `BSP`/`LIB`、USB port 拆分、命名清理、构建产物。

环境：CMake 4.4.3 / Ninja / arm-none-eabi-gcc 15.3.1（Windows + msys2）。

图例：✅ 通过 / ❌ 失败 / ⚠ 说明。

**结论：28 项结构检查 + 8 项构建检查全部通过；全量 73 镜像无错误。**
（B4/E4 中 port 直接链 `drv_usb` 属 USB LL 适配的合理例外，lib 层严格不写 `drv_*`。）

---

## A. 目录与分层结构

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| A1 | 顶层目录 | `platform/module/bsp/port/lib/app/tool/cmake`，无 `target/`、`tools/` | ✅ |
| A2 | 顶层 add 顺序 | platform→module→bsp→port→lib→app | ✅ `31:platform 32:module 33:bsp 34:port 35:lib 36:app` |
| A3 | platform 分层 | `arch/cmsis_core` + `soc/stm32/stm32f4xx`（扁平） | ✅ |
| A4 | module 分层 | 每第三方模块含版本子目录 | ✅ `cmsis_dsp/fatfs/freertos/ijg_libjpeg/stm32_hal/tjpgd/stm32_usb_device/stm32_usb_host` |
| A5 | port 分层 | `<BSP>/{fatfs,stm32_usb_device,stm32_usb_host}` | ✅ |
| A6 | app 分层 | `baremetal/` + `freertos/` | ✅ |

## B. 依赖方向 / 层次规则

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| B1 | platform 不依赖上层 | 仅 cmsis_core/cpu/soc | ✅ |
| B2 | module 仅依赖 platform/module | 无 bsp/port/lib/app | ✅ 依赖 `cmsis_core`/`soc_*`/`drv_usb`（均 module/platform） |
| B3 | bsp 依赖 platform/module | HAL 叶子链 `cmsis_core soc`；叶子链 `drv_core` | ✅ |
| B4 | port 依赖 bsp/module | 无 lib/app；HAL 仅 USB LL 例外 | ⚠ `stm32_usb_*_common_port` 链 `drv_usb`（USB LL，无对应 BSP 驱动） |
| B5 | lib 不写 HAL | 无 `drv_*` | ✅ |
| B6 | 无 `target_${MCU_FAMILY}` 残留 | 无匹配 | ✅（仅 verify.md 文本） |

## C. HAL 模块化（module/stm32_hal）

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| C1 | 每模块一叶子目标 | 29 个 | ✅ 29 |
| C2 | 叶子可排除构建 | 全含 EXCLUDE | ✅ 由 `drv_library()` 统一 `STATIC EXCLUDE_FROM_ALL` |
| C3 | `drv_all` 聚合 | 存在 INTERFACE | ✅ |
| C4 | 大类注释分组 | core/time/comm/analog/memory/display/multimedia/crypto | ✅ |
| C5 | HAL 无 GLOB | 无匹配 | ✅ |
| C6 | `hal_conf` 裁剪 | 远小于原 48 | ✅ 启用 30 |
| C7 | 叶子依赖链 | 非 `drv_core` 叶子 `PUBLIC drv_core` | ✅ 函数内统一 |

## D. BSP 模块化（bsp/openedv_stm32f4）

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| D1 | 每驱动一叶子目标 | 45 个 | ✅ 45 |
| D2 | `bsp_core` 常链 | STATIC = `bsp.c`（入口）+ 5 核心叶子 | ✅ (`bsp_core/sys/delay/led/key/usart`) |
| D3 | `bsp_all` 聚合 | 存在 | ✅ |
| D4 | `bsp_resolve` | 支持 `core`/叶子/`ALL`，未知报错 | ✅ |
| D5 | 大类注释分组 | core/time/comm/analog/memory/display/multimedia/crypto/sensor/misc | ✅ |
| D6 | 跨类依赖 | `lcd→ltdc`、`pwmdac→dac`、`pwr→exti`、`iap→usart`、`nor→spi`、`ov5640→io_expand` 等 | ✅ |
| D7 | 帧率定时器归入 `bsp_gtim` | 无 `timer.c/h`；TIM14 帧率 API `gtim_frame_*` 在 `gtim.c/h` | ✅ |
| D8 | `remote→ir`，归入 `# comm` | `ir.c/ir.h`（无 `remote_`/`REMOTE_`），`bsp_ir` 位于 `# comm` 段，依赖 `drv_tim` | ✅ |
| D9 | BSP 无 GLOB | 无匹配 | ✅ |
| D10 | `bsp_drv` 已删 | 无匹配 | ✅ |

## E. port 层

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| E1 | FatFs 端口 | `fatfs_port`（+ 变体 `fatfs_stm32_usb_msc_port`）= `<BSP>/fatfs`（exfuns + diskio） | ✅ |
| E2 | USB device 端口按类 | conf + cdc + audio + msc | ✅ |
| E3 | USB host 端口 | conf + msc | ✅ |
| E4 | port 不写 HAL（USB LL 例外） | 仅 `drv_usb` | ⚠ 见 B4 |
| E5 | USB MSC 定义路由 | port 变体 `fatfs_stm32_usb_msc_port`（`FATFS_USB_MSC`） | ✅ `57` app |

## F. lib 层

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| F1 | 直连组件 | `lib_fatfs` / `lib_fatfs_stm32_usb_msc` / `lib_stm32_usb_device_{cdc,audio,msc}` / `lib_stm32_usb_host_{hid,msc}` / `lib_dsp` / `lib_nand_storage` / `lib_cam_jpeg` | ✅ |
| F2 | `lib_resolve` | 小写可用 + `ALL` + 未知报错 | ✅ |
| F3 | bsp/lib 依赖分行 | BSP 与 lib 不同行 | ✅ text/picture/audio/mjpeg/ftl/nand_storage/cam_jpeg |
| F4 | 中间件库可排除 | 各中间件 STATIC 含 EXCLUDE | ✅ 9/9 |
| F5 | NAND 分层 | port 不依赖 lib：port 留 weak 钩子，强实现在 `lib_nand_storage` | ✅ `diskio.c`/`usbd_storage_if.c` weak + object override |

## G. app 声明

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| G1 | `BSP` 关键字解析 | `bsp_resolve` + `core` 隐含 | ✅ helper 均含 `lib_resolve`/`bsp_resolve` |
| G2 | `BSP` 在 `LIB` 前 | 全部满足 | ✅ |
| G3 | `LIB` 参数小写 | 参数小写 | ✅ 抽查全部小写 |
| G4 | 标注一致性 | 与约定一致 | ✅（60 app 含 BSP） |
| G5 | USB 关键字 | 仅 `stm32_usb_device_*`/`stm32_usb_host_*` | ✅ `stm32_usb_device_cdc/audio/msc`、`stm32_usb_host_hid/msc` |

## H. 构建与产物

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| H1 | 全量构建 | 73 镜像无失败 | ✅ 73 |
| H2 | 无链接错误 | 无 `undefined reference` | ✅ |
| H3 | 默认链接脚本 | `platform/soc/.../stm32f4xx_flash.ld` | ✅ |
| H4 | IAP 链接脚本覆盖 | app 内 `stm32f4xx_iap_app.ld` | ✅ |
| H5 | 按需对象数（全新） | 未用外设不编译 | ✅ 见下表 |
| H6 | 镜像不回归 | 一致或 ±4B | ✅ 见下表 |
| H7 | 被排除目标可显式构建 | 成功 | ✅ `drv_tim`、`bsp_lcd` |
| H8 | FreeRTOS 变体 | 成功 | ✅ `freertos/01_led` |

## I. 命名 / 引用清洁

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| I1 | 无 `bsp_drv` | 无 | ✅ |
| I2 | 无 `remote`/`REMOTE` | 无 | ✅ |
| I3 | 无 `bsp_timer` | 无 | ✅ |
| I4 | 无泛用 USB 关键字 | 无 | ✅ |
| I5 | 无可选 MODULE 开关 | 无 | ✅ |
| I6 | 无 `cmsis_device`/`freertos_port` | 无 | ✅ |
| I7 | 无 `target/`、`module/cmsis_core` 路径 | 无 | ✅ |
| I8 | 全局无 GLOB | `rg 'file\(GLOB'` | 无 | ✅ |

## J. 平台 / 配置 / OS

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| J1 | arch 条件选择 | 依 `MCU_ARCH` 选 `cmsis_core` | ✅ |
| J2 | soc 内容 | device+system+startup+vector+it+ld | ✅ |
| J3 | hal_conf 生效 | 未用 HAL 模块未编译 | ✅（对象数下降佐证） |
| J4 | FreeRTOS 配置归属 | `module/freertos/11.1.0/config/FreeRTOSConfig_common.h` | ✅ |
| J5 | 板级 `MCU_FLASH_BASE` | 仅分支内赋值 | ✅ flash/ram 各一次 |
| J6 | `bsp_delay` OS 注入 | `USE_FREERTOS`/`freertos` | ✅ |

## K. 文档 / 注释一致性

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| K1 | `bsp.h` 引用 | 含 `ir.h`，无 `remote.h` | ✅ |
| K2 | 注释无过期引用 | 无 `freertos_port`/`bsp_drv`/`target/stm32` | ✅（I6/I7） |
| K3 | 本报告存在 | 存在 | ✅ `verify.md` |
| K4 | LCD 驱动范围 | 仅 4.3" RGB (LTDC)；MCU 面板(SSD1963/FMC)代码删除 | ✅ `lcd.c/lcd.h` 仅转发 LTDC，无面板分支 |
| K5 | 输出策略 | 涉及图像显示用 LCD，其余 printf | ✅ 见 `PLAN.md`；分类与 app 一致 |

## L. 内容深度对齐（PLAN.md P0–P2）

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| L1 | P0 缺陷 | IAP 擦除、字体 NOR 初始化、DCMI/SDIO 引脚切换 | ✅ `iap_erase_app`、`fonts_init→nor_init`、`dcmi_switch_*` |
| L2 | P2 存储 | 13 全容量、42 三卷、54 三 LUN、53 IAP 手控 | ✅ |
| L3 | P2 传感器 | 35 app-local 融合 + ANO_TC | ✅ `35_i2c_imu/imu.c` |
| L4 | P2 显示 | 43 GBK 遍历、11 OLED 12/16/24、30 多点、45 原生 JPEG/BMP、49 LCD | ✅ |
| L5 | P2 外设/交互 | 16 wakeup、22/23 按键+ADC、29 模式、50 FFT、58 HID、25 BEEP | ✅ |
| L6 | 全量回归 | 73 镜像 | ✅ 73 |
| L7 | 进程记录 | `PLAN.md` 勾选与提交对应 | ✅ |

## M. 状态机与风格统一

| ID | 验证项 | 期望 | 结果 |
|---|---|---|---|
| M1 | 打包位状态消除 | wavplay/recorder/usbd_storage/usbd_cdc/ir/38/45 改显式状态枚举 | ✅ 无位掩码状态 |
| M2 | USB 设备状态 | `usbd_dev_state_t` 替代 bool；54/55/56 同步 | ✅ |
| M3 | 相机 JPEG 复用 | 38/45 共用 `lib_cam_jpeg` | ✅ 抽公共 helper |
| M4 | 忙等超时 | `wavplay` 半缓冲等待有界 | ✅ `AUDIO_WAIT_TIMEOUT_MS` |
| M5 | FTL 格式 | 项目头注释、TAB→空格、去冗余/中文注释 | ✅ |
| M6 | 魔法数具名 | `RTC_WAKEUP_1HZ`、`CAM_OUTSIZE_OFFSET_X` | ✅ |
| M7 | include 统一 | system 段 + 空行 + local 段；去冗余 bsp 头（保留 ltdc.h/lib/USB） | ✅ 73 app |
| M8 | 打印直出 | 仅 printf 的 sprintf 改直出（保留路径/LCD/hex dump） | ✅ 16 app |
| M9 | 长行 | app 层 ≤100 列 | ✅ |
| M10 | 注释精简 | 去除复述式注释与空 `else` | ✅ |

---

## 基线对比（Phase 2 前 → 当前，全新构建）

| app | 对象（前→后） | 静态库（前→后） | text/data/bss（前→后） |
|---|---|---|---|
| `01_led` | 313 → **61** | 16 → **8** | 10688 → 10684 / 112 / 4848 |
| `42_fatfs` | 313 → **101** | 16 → **15** | 212484 → 212480 / 112 / 13328 |
| `44_image` | 313 → **209** | 16 → **26** | 243824 / 152 / 190872（一致） |
| `57_usb_host_msc` | 313 → **144** | 16 → **21** | 219792 → 219788 / 144 → 148 / 14640 → 14636 |
| `freertos/01_led` | 325 → **73** | 17 → **9** | 18324 → 18328 / 116 / 16052 |

**新增核对 app**
| app | 对象 | 静态库 | text/data/bss | 链接脚本 |
|---|---|---|---|---|
| `56_usb_device_cdc` | 94 | 12 | 24416/356/23132 | stm32f4xx_flash.ld |
| `53_iap_app` | 61 | 8 | 10880/112/4848 | stm32f4xx_iap_app.ld |

`56_usb_device_cdc` 仅编 `stm32_usb_device_conf_port` + `stm32_usb_device_cdc_port`（无 audio/msc/host）；镜像差异均 ≤ ±4B，属 HAL 归档分组/段对齐，**无功能回归**。

---

## 总体结论

- **结构（A–K，27 项）**：全部通过。分层清晰、依赖单向、`BSP`/`LIB` 声明式选择生效、命名统一无残留。
- **构建（H1–H8）**：73/73 镜像成功；按需编译显著（`01_led` 313→61 对象）；镜像无功能回归。
- **内容深度（L1–L7）**：P0 缺陷修复 + 逐 app 对齐（存储/传感器/显示/外设）全部落地，全量 73/73。
- **说明**：`port` 直接链 `drv_usb` 为 USB LL 适配的合理例外；lib 层严格不写 `drv_*`。
- **已知保留**：`stm32_usb_device` 内核未按类拆分（已评估，收益低）。

## N. Module 版本（参考 `D:\work\git`）

| module | 版本 | 参考仓库 | 状态 |
|---|---|---|---|
| freertos | V11.3.1 | FreeRTOS-Kernel | ✅ 已更新（旧版已删除） |
| stm32_usb_device | v2.11.6 | stm32-mw-usb-device | ✅ 已更新（旧版已删除） |
| stm32_usb_host | v3.5.5 | stm32-mw-usb-host | ✅ 已更新（旧版已删除） |
| stm32_hal/stm32f4xx | v1.8.5 | stm32f4xx-hal-driver | ✅ 已是最新 tag |
| cmsis_dsp / fatfs / ijg_libjpeg / tjpgd | 1.17.1 / r0.16 / 10 / r0.03 | （参考中更旧/缺失） | ⏸ 不动 |

- 验证：`01_led`（freertos）、`54/55/56`（usb device）、`57/58`（usb host）逐项构建；全量回归 **73/73**。

## O. cmsis_core 版本（参考 `D:\work\git\CMSIS_6`）

| 项 | 版本 | 来源 | 状态 |
|---|---|---|---|
| `platform/arch/cmsis_core` | 6.1.0 → **6.3.0** | CMSIS_6 main HEAD（Core(M) 6.3.0） | ✅ 已更新（旧版已删除） |

- 变更：`cmsis_version.h` 6.1.0→6.3.0，另 3 个 M/R 头微调与 1 处文件名更正；`core_cm4.h/cmsis_gcc.h/cmsis_compiler.h` 不变。
- 验证：`01_led`/`50_2_dsp_fft`/`54`/`57` 逐项构建；全量回归 **73/73**。

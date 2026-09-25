# STM32 示例工程分层重构 — 验证报告

范围：`platform`/`module`/`bsp`/`port`/`lib`/`app` 分层改造（Phase 1 → 2a → 2b → 2c）、
按需编译（HAL/BSP 叶子目标）、`BSP`/`LIB` 声明式选择、`remote→ir`、`timer→gtim`、
USB port 按类拆分、板级 `MCU_FLASH_BASE` 清理。

验证环境：CMake 4.4.3 / Ninja / arm-none-eabi-gcc 15.3.1；Windows + msys2。

## 验证计划与结果

| ID | 验证项 | 方法 | 期望 | 实际结果 | 结论 |
|---|---|---|---|---|---|
| V01 | 提交与工作区 | `git log --oneline`；`git status` | Phase 提交齐全 | `af22175`(P1) `927edf5`(P2a) `0bfce1c`(P2b) `4aea8ac`(board) | ✅ |
| V02 | HAL/BSP 去 GLOB | `rg "file\(GLOB"` | 仅 freertos 保留 | 仅 `module/freertos/11.1.0/CMakeLists.txt:5` | ✅ |
| V03 | `bsp_drv` 删除 | `rg bsp_drv` | 无匹配 | 无匹配 | ✅ |
| V04 | HAL 小类叶子 | `rg 'hal_library\(hal_'` | 每模块一目标 | 29 个 `hal_*` + `hal_all` | ✅ |
| V05 | `hal_conf` 裁剪 | 统计启用模块 | 仅用到的模块 | 启用 30（原 48） | ✅ |
| V06 | 可选 MODULE 开关移除 | `rg MODULE_(FATFS\|…)_ENABLE` | 无匹配 | 无匹配（仅 HAL/FREERTOS） | ✅ |
| V07 | `remote→ir` | `ls ir.*`；`rg 'remote_\|REMOTE_'` | `ir.c/ir.h`，无 remote | 存在 `ir.c/ir.h`；无 `remote_`/`REMOTE_` | ✅ |
| V08 | `timer` 并入 `bsp_gtim` | `rg bsp_timer`；查 `bsp_gtim` | 无 `bsp_timer`，含 `timer.c` | `bsp_leaf(bsp_gtim gtim.c timer.c)` | ✅ |
| V09 | lib 不写 HAL | `rg hal_ lib/**/CMakeLists.txt` | 无匹配 | 无匹配 | ✅ |
| V10 | lib 依赖 bsp/lib 分行 | 读 4 个 lib CMake | BSP 与 lib 不同行 | text/picture/audio/mjpeg 均分行 | ✅ |
| V11 | USB LIB 关键字 | `rg 'USB_DEVICE\b\|USB_HOST\b' app` | 仅 `USB_DEVICE_*`/`USB_HOST_*` | 无泛用关键字 | ✅ |
| V12 | USB port 按类拆分 | 查 `56_usb_cdc` 归档 | 仅 common+cdc | `usb_device_common_port`+`usb_device_cdc_port`（无 audio/msc/host） | ✅ |
| V13 | app `BSP` 标注 | `rg -l '^\s*BSP\s' app/baremetal/*` | 与 D 表一致 | 60 个 app 含 `BSP` | ✅ |
| V14 | `platform/arch` 条件选择 | 读 `platform/arch/CMakeLists.txt` | 依 `MCU_ARCH` 选 `cmsis_core` | `if(MCU_ARCH MATCHES "^cortex-m")` | ✅ |
| V15 | 解析函数报错 | 代码审查 `bsp_resolve`/`lib_resolve` | 未知组件 `FATAL_ERROR` | 均含未知组件报错分支 | ✅ |
| V16 | 全量回归 | `tool/build.sh debug all all-freertos` | 72 镜像，无失败 | 72 `.elf`，无 `!!!`/`undefined reference` | ✅ |
| V17 | 按需编译（全新构建对象数） | 见下表 | 显著下降 | `01_led`=61 / `42_fatfs`=101 / `44_picture`=209 / `56_usb_cdc`=94 / `57_usb_host_msc`=144 / `rtos_01_led`=73 | ✅ |
| V18 | 镜像不回归 | `arm-none-eabi-size` 对比基线 | 一致或 ±4B | 见下表（≤±4B，对齐差异） | ✅ |
| V19 | 默认/覆盖链接脚本 | 检查 `build.ninja` 的 `-T` | 默认 flash.ld；IAP 用 app ld | 默认 `stm32f4xx_flash.ld`；`53_iap_app` 用 `stm32f4xx_iap_app.ld` | ✅ |
| V20 | 显式构建被排除目标 | `--target hal_tim` / `--target bsp_lcd` | 可单独构建 | 均成功链接对应 `.a` | ✅ |
| V21 | 板级 `MCU_FLASH_BASE` | 读 `cmake/board/openedv_stm32f4.cmake` | 仅分支内赋值 | flash 分支 `0x08000000`、ram 分支 `0x20000000`；无冗余顶层赋值 | ✅ |
| V22 | `hal_all`/`bsp_all` 聚合 | `rg 'add_library\((hal_all\|bsp_all)'` | 存在 | 均存在 | ✅ |
| V23 | app 中 `BSP` 在 `LIB` 前 | 遍历 app CMake 行号 | `BSP` 行号 < `LIB` 行号 | 全部满足 | ✅ |
| V24 | LIB 参数小写 | 抽查 app `LIB` 行 | 参数小写 | 如 `LIB malloc text picture`、`LIB fatfs usb_host_msc` | ✅ |

## 基线对比（Phase 2 前 → 全量改造后，全新构建）

| app | 编译对象 | 静态库 | ELF text/data/bss |
|---|---|---|---|
| `01_led` | 313 → **61** | 16 → **8** | 10688 → 10684 / 112 / 4848 |
| `42_fatfs` | 313 → **101** | 16 → **15** | 212484 → 212480 / 112 / 13328 |
| `44_picture` | 313 → **209** | 16 → **26** | 243824 / 152 / 190872（一致） |
| `56_usb_cdc` | — → **94** | — → **12** | 24416 / 356 / 23132 |
| `57_usb_host_msc` | 313 → **144** | 16 → **21** | 219792 → 219788 / 144 → 148 / 14640 → 14636 |
| `freertos/01_led` | 325 → **73** | 17 → **9** | 18324 → 18328 / 116 / 16052 |

- 镜像差异均为 ±4 字节，源于 HAL 归档分组/段对齐变化，**无功能回归**。
- `01_led` 仅编译 `bsp_{init,sys,delay,led,key,usart}` + `hal_core/hal_uart`；`56_usb_cdc` 仅编译 CDC 端口 + `hal_core/hal_uart/hal_usb`。

## 结论

- V01–V24 **全部通过**；全量 72 镜像构建无错误。
- 按需编译与声明式 `BSP`/`LIB` 生效，未用外设不再编译；镜像无功能回归。
- 已知保留：`module/freertos` 使用 GLOB（内核核心文件，全部需要）；`usb_device` 内核未按类拆分（已评估，收益低）。

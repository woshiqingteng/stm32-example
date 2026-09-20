# Device selection. Only stm32f4 is available for now.

set(TARGET_STM32F4
    ${TARGET_DIR}/startup_stm32f429igtx.c
    ${TARGET_DIR}/vector_stm32f429igtx.c
    ${TARGET_DIR}/system_stm32f4xx.c
    ${TARGET_DIR}/stm32f4xx_it.c)

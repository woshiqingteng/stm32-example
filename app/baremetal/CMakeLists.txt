# Baremetal app tree. Common modules are declared here; each app adds only extras.

set(BAREMETAL_COMMON TARGET_STM32F4 HAL_CORE HAL_UART BSP_CORE)

add_subdirectory(01_led)

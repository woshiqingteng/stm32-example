# BSP exposed modules: CACHE INTERNAL variables carrying their source files.

set(BSP_DIR "${CMAKE_SOURCE_DIR}/bsp" CACHE INTERNAL "" FORCE)

set(BSP_EXPOSED_MODULES BSP_CORE BSP_EXTI BSP_WDG BSP_TIM BSP_TPAD
    CACHE INTERNAL "" FORCE)

set(BSP_CORE
    ${BSP_DIR}/bsp.c
    ${BSP_DIR}/syscalls.c
    ${BSP_DIR}/sys.c
    ${BSP_DIR}/delay.c
    ${BSP_DIR}/usart.c
    ${BSP_DIR}/led.c
    ${BSP_DIR}/key.c
    CACHE INTERNAL "" FORCE)
set(BSP_EXTI ${BSP_DIR}/exti.c CACHE INTERNAL "" FORCE)
set(BSP_WDG  ${BSP_DIR}/wdg.c CACHE INTERNAL "" FORCE)
set(BSP_TIM  ${BSP_DIR}/btim.c ${BSP_DIR}/gtim.c ${BSP_DIR}/atim.c CACHE INTERNAL "" FORCE)
set(BSP_TPAD ${BSP_DIR}/tpad.c CACHE INTERNAL "" FORCE)

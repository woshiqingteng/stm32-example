add_library(target_stm32f4 STATIC
    startup_stm32f429ig.c
    vector_stm32f429ig.c
    stm32f4xx_it.c
    system_stm32f4xx.c
)

target_compile_options(target_stm32f4 PUBLIC ${MCU_ARCH})
target_link_options(target_stm32f4 PUBLIC ${MCU_ARCH})

target_include_directories(target_stm32f4
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(target_stm32f4 PUBLIC ${STM32_DEVICE_MACRO})
target_link_libraries(target_stm32f4 PUBLIC cmsis_core)

set(MCU_LD flash CACHE STRING "Linker script variant (flash|ram)")
set_property(CACHE MCU_LD PROPERTY STRINGS flash ram)
if(MCU_LD STREQUAL "ram")
    target_link_options(target_stm32f4 INTERFACE "-Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/stm32f429ig_ram.ld")
else()
    target_link_options(target_stm32f4 INTERFACE "-Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/stm32f429ig_flash.ld")
endif()

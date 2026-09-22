add_library(${TARGET_LIB} STATIC
    ${STM32_STARTUP_SRC}
    ${STM32_VECTOR_SRC}
    ${STM32_IT_SRC}
    ${STM32_SYSTEM_SRC}
)

target_compile_options(${TARGET_LIB} PUBLIC ${MCU_ARCH})
target_link_options(${TARGET_LIB} PUBLIC ${MCU_ARCH})

target_include_directories(${TARGET_LIB}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${TARGET_LIB} PUBLIC
    ${STM32_DEVICE_MACRO}
    HSE_VALUE=${STM32_HSE_VALUE}
    HSI_VALUE=${STM32_HSI_VALUE}
)
target_link_libraries(${TARGET_LIB} PUBLIC cmsis_core)

set(MCU_LD flash CACHE STRING "Linker script variant (flash|ram)")
set_property(CACHE MCU_LD PROPERTY STRINGS flash ram)
if(MCU_LD STREQUAL "ram")
    target_link_options(${TARGET_LIB} INTERFACE "-Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/${STM32_LD_RAM}")
else()
    target_link_options(${TARGET_LIB} INTERFACE "-Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/${STM32_LD_FLASH}")
endif()

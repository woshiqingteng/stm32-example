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


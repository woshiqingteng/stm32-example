add_library(target_${MCU_FAMILY} INTERFACE)

target_sources(target_${MCU_FAMILY} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/startup_${MCU_FAMILY}.c
    ${CMAKE_CURRENT_LIST_DIR}/vector_${MCU_FAMILY}.c
    ${CMAKE_CURRENT_LIST_DIR}/system_${MCU_FAMILY}.c
    ${CMAKE_CURRENT_LIST_DIR}/${MCU_FAMILY}_it.c
)

target_include_directories(target_${MCU_FAMILY} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(target_${MCU_FAMILY} INTERFACE
    ${MCU_MACRO}
    HSE_VALUE=${MCU_HSE_VALUE}
    HSI_VALUE=${MCU_HSI_VALUE}
)

# A single -T is emitted: the app may override the script by setting the
# MCU_LD_SCRIPT property on its own target (e.g. an IAP image at 0x08010000),
# otherwise the board's default script for the current boot mode is used.
target_link_options(target_${MCU_FAMILY} INTERFACE
    "$<$<BOOL:$<TARGET_PROPERTY:${APP_TARGET},MCU_LD_SCRIPT>>:-T$<TARGET_PROPERTY:${APP_TARGET},MCU_LD_SCRIPT>>"
    "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:${APP_TARGET},MCU_LD_SCRIPT>>>:-T${CMAKE_CURRENT_LIST_DIR}/${MCU_FAMILY}_${MCU_LD_SUFFIX}.ld>"
    "-Wl,-Map=$<TARGET_FILE_DIR:${APP_TARGET}>/${APP_TARGET}.map"
)

target_link_libraries(target_${MCU_FAMILY} INTERFACE
    cmsis_core
    ${MCU_ARCH_INTERFACE}
)

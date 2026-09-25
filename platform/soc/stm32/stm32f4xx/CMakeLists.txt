add_library(soc_${MCU_FAMILY} INTERFACE)

target_sources(soc_${MCU_FAMILY} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/startup_${MCU_FAMILY}.c
    ${CMAKE_CURRENT_LIST_DIR}/vector_${MCU_FAMILY}.c
    ${CMAKE_CURRENT_LIST_DIR}/system_${MCU_FAMILY}.c
    ${CMAKE_CURRENT_LIST_DIR}/${MCU_FAMILY}_it.c
)

target_include_directories(soc_${MCU_FAMILY} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(soc_${MCU_FAMILY} INTERFACE
    ${MCU_MACRO}
    HSE_VALUE=${MCU_HSE_VALUE}
    HSI_VALUE=${MCU_HSI_VALUE}
)

target_link_options(soc_${MCU_FAMILY} INTERFACE
    "$<$<BOOL:$<TARGET_PROPERTY:${APP_TARGET},MCU_LD_SCRIPT>>:-T$<TARGET_PROPERTY:${APP_TARGET},MCU_LD_SCRIPT>>"
    "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:${APP_TARGET},MCU_LD_SCRIPT>>>:-T${CMAKE_CURRENT_LIST_DIR}/${MCU_FAMILY}_${MCU_LD_SUFFIX}.ld>"
    "-Wl,-Map=$<TARGET_FILE_DIR:${APP_TARGET}>/${APP_TARGET}.map"
)

target_link_libraries(soc_${MCU_FAMILY} INTERFACE
    cmsis_core
    ${MCU_ARCH_INTERFACE}
)

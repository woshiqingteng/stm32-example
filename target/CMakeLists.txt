# Target layer: chip support package (CSP) selected by STM32_SERIES.
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${STM32_SERIES})
    message(FATAL_ERROR "No target support for series '${STM32_SERIES}'")
endif()
add_subdirectory(${STM32_SERIES})

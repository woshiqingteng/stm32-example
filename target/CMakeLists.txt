# Target layer: chip support package (CSP) selected by STM32_TARGET_DIR.
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${STM32_TARGET_DIR})
    message(FATAL_ERROR "No target support: target/${STM32_TARGET_DIR}")
endif()
add_subdirectory(${STM32_TARGET_DIR})

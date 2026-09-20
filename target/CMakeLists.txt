# Device selection. Only stm32f4 is available for now.

# Append startup/vector/system/it + device headers to the calling app (once).
macro(use_target)
    if(NOT _TARGET_ADDED)
        list(APPEND APP_SOURCES
            "${TARGET_DIR}/startup_stm32f429igtx.c"
            "${TARGET_DIR}/vector_stm32f429igtx.c"
            "${TARGET_DIR}/system_stm32f4xx.c"
            "${TARGET_DIR}/stm32f4xx_it.c")
        list(APPEND APP_INCLUDES "${TARGET_DIR}")
        list(APPEND APP_DEFINES "${DEVICE_DEFINE}")
        set(_TARGET_ADDED TRUE)
    endif()
endmacro()

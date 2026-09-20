# Device selection. Only stm32f4 is available for now.

# Append startup/vector/system/it + device headers to the calling app (once).
macro(use_target)
    if(NOT _TARGET_ADDED)
        cache_list_append(APP_SOURCES "${TARGET_DIR}/startup_stm32f429igtx.c")
        cache_list_append(APP_SOURCES "${TARGET_DIR}/vector_stm32f429igtx.c")
        cache_list_append(APP_SOURCES "${TARGET_DIR}/system_stm32f4xx.c")
        cache_list_append(APP_SOURCES "${TARGET_DIR}/stm32f4xx_it.c")
        cache_list_append(APP_INCLUDES "${TARGET_DIR}")
        cache_list_append(APP_DEFINES "${DEVICE_DEFINE}")
        set(_TARGET_ADDED TRUE CACHE INTERNAL "" FORCE)
    endif()
endmacro()

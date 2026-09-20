# Baremetal apps. Exposed modules are variables carrying their source files;
# app_baremetal_add resolves the requested tokens and collects the common ones.

set(BAREMETAL_COMMON TARGET_STM32F4 HAL_CORE HAL_UART BSP_CORE)

# app_baremetal_add(NAME SOURCES <file>... [MODULES <token>...])
macro(app_baremetal_add NAME)
    set(_sources "")
    set(_modules "")
    set(_kw "")

    foreach(_a ${ARGN})
        if(_a STREQUAL "SOURCES" OR _a STREQUAL "MODULES")
            set(_kw "${_a}")
        elseif(_kw STREQUAL "SOURCES")
            list(APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/${_a}")
        elseif(_kw STREQUAL "MODULES")
            list(APPEND _modules "${_a}")
        endif()
    endforeach()

    set(_srcs ${_sources})
    foreach(_m ${BAREMETAL_COMMON} ${_modules})
        list(APPEND _srcs ${${_m}})
    endforeach()

    set(_tgt "app_${OS}_${NAME}")
    add_executable(${_tgt} ${_srcs})
    target_include_directories(${_tgt} PRIVATE
        "${TARGET_DIR}" "${BSP_DIR}" "${CMSIS_CORE_INCLUDE}" "${HAL_INC}")
    target_compile_definitions(${_tgt} PRIVATE "USE_HAL_DRIVER" "${DEVICE_DEFINE}")
    target_link_options(${_tgt} PRIVATE
        -T${TARGET_LD_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_DIR:${_tgt}>/${_tgt}.map)
    set_target_properties(${_tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OS}/${NAME}"
        OUTPUT_NAME "${_tgt}")

    flow_add_artifacts(${_tgt})
    flow_add_flash(${NAME} $<TARGET_FILE_DIR:${_tgt}>/${_tgt}.bin ${_tgt})
endmacro()

add_subdirectory(01_led)

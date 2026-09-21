set(MCU_FLOAT_ABI hard CACHE STRING "FPU ABI (hard|softfp|soft)")
set_property(CACHE MCU_FLOAT_ABI PROPERTY STRINGS hard softfp soft)

set(_mcu_arch -mcpu=cortex-m4 -mthumb -mfloat-abi=${MCU_FLOAT_ABI})
if(NOT MCU_FLOAT_ABI STREQUAL "soft")
    list(APPEND _mcu_arch -mfpu=fpv4-sp-d16)
endif()
set(MCU_ARCH ${_mcu_arch} CACHE INTERNAL "" FORCE)

message(STATUS "Float ABI : ${MCU_FLOAT_ABI}")

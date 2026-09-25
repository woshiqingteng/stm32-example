# Vendor module: Arm CMSIS-DSP (source build). Board independent. Only the
# sources needed by the DSP experiment are vendored, so the precompiled vendor
# .lib is not used. The public Include/ tree plus PrivateInclude/ (internal
# helpers) are shipped; the compiled subset is listed in CMSIS_DSP_SRC.
#
# Required for Cortex-M4F:
#   ARM_MATH_CM4            - select the Cortex-M4 build in arm_math.h
#   ARM_MATH_LOOPUNROLL     - enable the unrolled kernels
#   ARM_MATH_MATRIX_CHECK   - argument checking on matrix ops
#   ARM_MATH_ROUNDING       - rounding on fixed-point support functions
#   __FPU_PRESENT / __FPU_USED - declare the FPU to the CMSIS core headers

set(CMSIS_DSP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/BasicMathFunctions/arm_dot_prod_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/CommonTables/arm_common_tables.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/CommonTables/arm_const_structs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/FastMathFunctions/arm_cos_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/FastMathFunctions/arm_sin_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/FilteringFunctions/arm_fir_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/FilteringFunctions/arm_fir_init_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/StatisticsFunctions/arm_max_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/StatisticsFunctions/arm_mean_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/TransformFunctions/arm_bitreversal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/TransformFunctions/arm_cfft_radix4_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/TransformFunctions/arm_cfft_radix4_init_f32.c)

add_library(cmsis_dsp STATIC EXCLUDE_FROM_ALL ${CMSIS_DSP_SRC})
target_include_directories(cmsis_dsp
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PrivateInclude)

target_compile_definitions(cmsis_dsp PRIVATE
    ARM_MATH_CM4
    ARM_MATH_LOOPUNROLL
    ARM_MATH_MATRIX_CHECK
    ARM_MATH_ROUNDING
    __FPU_PRESENT=1
    __FPU_USED=1)

target_link_libraries(cmsis_dsp PUBLIC cmsis_core soc_${MCU_FAMILY})

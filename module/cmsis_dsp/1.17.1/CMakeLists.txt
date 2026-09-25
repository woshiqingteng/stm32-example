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
    ARM_MATH_LOOPUNROLL)

target_link_libraries(cmsis_dsp PUBLIC cmsis_core soc_${MCU_FAMILY} m)

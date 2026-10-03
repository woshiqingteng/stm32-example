/**
 * @file    bitops.h
 * @brief   Hardware/platform independent bit and small utility macros.
 *
 * Every macro is COMMON_ prefixed to avoid clashing with the many MIN/MAX/BIT
 * definitions found in third-party headers (STM32 USB, lvgl, cmsis_dsp, ...).
 */

#ifndef COMMON_BITOPS_H
#define COMMON_BITOPS_H

#include <stdint.h>
#include <stddef.h>

#define COMMON_BIT(n)          (1UL << (n))
#define COMMON_BIT_MASK(n)     (COMMON_BIT(n) - 1UL)
#define COMMON_GENMASK(h, l)   (((~0UL) << (l)) & (~0UL >> (31U - (h))))
#define COMMON_ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define COMMON_MIN(a, b)       (((a) < (b)) ? (a) : (b))
#define COMMON_MAX(a, b)       (((a) > (b)) ? (a) : (b))
#define COMMON_CLAMP(v, lo, hi) COMMON_MIN(COMMON_MAX((v), (lo)), (hi))
#define COMMON_ROUND_UP(x, a)  (((x) + (a) - 1U) / (a) * (a))

#define COMMON_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif /* COMMON_BITOPS_H */

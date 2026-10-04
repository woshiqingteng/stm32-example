p = r"D:/work/stm32-example/bsp/openedv_stm32f4/i2c.c"
BS = chr(92)
NL = BS + "r" + BS + "n"
s = open(p, encoding="utf-8").read()
if "#include <stdio.h>" not in s:
    s = s.replace('#include "dma_hw.h"', '#include "dma_hw.h"\n#include <stdio.h>', 1)

# trace buffers
s = s.replace(
    "static volatile bool     g_dma_err[2];",
    "static volatile bool     g_dma_err[2];\n"
    "volatile uint8_t g_evt[24];\nvolatile uint8_t g_evt_n;",
    1,
)

# record SR1 at each EV ISR entry
old = "void I2C2_EV_IRQHandler(void)\n{\n"
new = (
    old
    + "    if (g_evt_n < 24U) { g_evt[g_evt_n++] = (uint8_t)(g_i2c_hw.instance->SR1 & 0xFFU); }\n"
)
assert old in s
s = s.replace(old, new, 1)

# print the trace on IT timeout
old2 = (
    "            __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);\n"
    "            return false;\n"
    "        }\n"
    "    }\n"
    "    return !g_it.error;"
)
new2 = (
    "            static uint32_t nt;\n"
    "            if (nt < 3U) { uint8_t q; nt++;\n"
    '                printf("ITTO k=%u p=%u s2=%02lX ev=", (unsigned)g_it.kind, (unsigned)g_it.phase,\n'
    "                       (unsigned long)(g_i2c_hw.instance->SR2 & 0xFFU));\n"
    '                for (q = 0U; q < g_evt_n; q++) { printf("%02X ", g_evt[q]); }\n'
    '                printf("' + NL + '"); }\n'
    "            g_evt_n = 0U;\n"
    "            __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);\n"
    "            return false;\n"
    "        }\n"
    "    }\n"
    "    return !g_it.error;"
)
assert old2 in s, "itw"
s = s.replace(old2, new2, 1)

# reset the trace at each transaction
old3 = (
    "    NVIC_ClearPendingIRQ(I2C2_EV_IRQn);\n    NVIC_ClearPendingIRQ(I2C2_ER_IRQn);"
)
new3 = (
    "    g_evt_n = 0U;\n"
    "    NVIC_ClearPendingIRQ(I2C2_EV_IRQn);\n"
    "    NVIC_ClearPendingIRQ(I2C2_ER_IRQn);"
)
assert old3 in s, "start"
s = s.replace(old3, new3, 1)

open(p, "w", encoding="utf-8").write(s)
print("ok")

/**
 * @file    main.c
 * @brief   50_1_dsp_math: DSP BasicMath benchmark (vendor experiment 50_1).
 *          A sin/cos identity (sin^2 + cos^2 = 1) is evaluated with plain libm
 *          and with the CMSIS-DSP kernels; the runtimes are reported over
 *          USART1.
 */

#include <stdio.h>
#include <math.h>

#include "bsp.h"
#include "arm_math.h"
#define DELTA           0.0001f     /* Maximum allowed sin^2 + cos^2 error */
#define SIN_COS_TIMES   200000U     /* Iterations per run */

/* 0 = plain sinf/cosf, 1 = CMSIS-DSP arm_sin_f32/arm_cos_f32. Returns 0xFF
 * when a result deviates from 1 by more than DELTA. */
static uint8_t sin_cos_test(float angle, uint32_t times, uint8_t dsp)
{
    float    sinx;
    float    cosx;
    float    result;
    uint32_t i;

    for (i = 0U; i < times; i++)
    {
        if (dsp == 0U)
        {
            cosx = cosf(angle);
            sinx = sinf(angle);
        }
        else
        {
            cosx = arm_cos_f32(angle);
            sinx = arm_sin_f32(angle);
        }

        result = (sinx * sinx) + (cosx * cosx);
        result = fabsf(result - 1.0f);

        if (result > DELTA)
        {
            return 0xFFU;
        }

        angle += 0.001f;
    }

    return 0U;
}

int main(void)
{
    uint32_t t0;
    uint32_t elapsed;
    uint8_t  res;

    bsp_init();

    printf("50_1_dsp_math ready\r\n");

    for (;;)
    {
        t0      = sys_get_tick();
        res     = sin_cos_test(PI / 6.0f, SIN_COS_TIMES, 0U);
        elapsed = sys_get_tick() - t0;
        printf("sin/cos noDSP : %lu ms (%s)\r\n",
               (unsigned long)elapsed, (res == 0U) ? "OK" : "error");

        t0      = sys_get_tick();
        res     = sin_cos_test(PI / 6.0f, SIN_COS_TIMES, 1U);
        elapsed = sys_get_tick() - t0;
        printf("sin/cos DSP   : %lu ms (%s)\r\n\r\n",
               (unsigned long)elapsed, (res == 0U) ? "OK" : "error");

        led_toggle(LED0);
        delay_ms(500U);
    }
}

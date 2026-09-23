/**
 * @file    main.c
 * @brief   50_dsp: ALIENTEK DSP test ported to a source build of CMSIS-DSP.
 *          The BasicMath sin/cos benchmark runs with and without the DSP
 *          kernels, a 1024 point complex FFT and an FIR filter are measured,
 *          with the runtimes and results shown on the RGB panel and USART1.
 */

#include <stdio.h>
#include <math.h>
#include "bsp.h"
#include "arm_math.h"

#define DELTA           0.0001f     /* Maximum allowed sin^2 + cos^2 error */
#define SIN_COS_TIMES   200000U     /* Iterations per sin/cos run */
#define FFT_LENGTH      1024U       /* FFT length: 16, 64, 256 or 1024 */
#define FFT_RUNS        100U        /* FFT repetitions used for the timing */
#define FFT_SIGNAL_LEN  32U         /* Serial dump of the first N magnitudes */
#define FIR_NUM_TAPS    29U         /* FIR moving-average length */
#define FIR_BLOCK       256U        /* FIR block size */
#define FIR_RUNS        500U        /* FIR repetitions used for the timing */

#define TEXT_X          30U
#define TEXT_WIDTH      300U

/** @brief  sin/cos implementation under test. */
typedef enum
{
    DSP_MATH_PLAIN = 0,   /*!< plain sinf/cosf */
    DSP_MATH_CMSIS = 1    /*!< CMSIS-DSP arm_sin_f32/arm_cos_f32 */
} dsp_math_mode_t;

/* FFT input (complex pairs) and output (magnitudes). */
static float g_fft_inputbuf[FFT_LENGTH * 2U];
static float g_fft_outputbuf[FFT_LENGTH];

/* FIR coefficients, state and working buffers. */
static float g_fir_state[FIR_NUM_TAPS + FIR_BLOCK - 1U];
static float g_fir_coeffs[FIR_NUM_TAPS];
static float g_fir_in[FIR_BLOCK];
static float g_fir_out[FIR_BLOCK];

/* 0 = plain sinf/cosf, 1 = CMSIS-DSP arm_sin_f32/arm_cos_f32. Returns 0xFF
 * when a result deviates from 1 by more than DELTA. */
static uint8_t sin_cos_test(float angle, uint32_t times, dsp_math_mode_t mode)
{
    float    sinx;
    float    cosx;
    float    result;
    uint32_t i;

    for (i = 0U; i < times; i++)
    {
        if (mode == DSP_MATH_PLAIN)
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

/* Fill the complex FFT input with a multi tone test signal; the imaginary
 * part stays zero. */
static void fft_signal_fill(void)
{
    uint32_t i;

    for (i = 0U; i < FFT_LENGTH; i++)
    {
        g_fft_inputbuf[2U * i] =
            100.0f +
            10.0f * arm_sin_f32(2.0f * PI * (float)i / (float)FFT_LENGTH) +
            30.0f * arm_sin_f32(2.0f * PI * (float)i * 4.0f / (float)FFT_LENGTH) +
            50.0f * arm_cos_f32(2.0f * PI * (float)i * 8.0f / (float)FFT_LENGTH);
        g_fft_inputbuf[(2U * i) + 1U] = 0.0f;
    }
}

/* Fill the FIR input with a positive (DC biased) two tone test signal. */
static void fir_signal_fill(void)
{
    uint32_t i;

    for (i = 0U; i < FIR_BLOCK; i++)
    {
        g_fir_in[i] = 1.0f +
                      0.5f * arm_sin_f32(2.0f * PI * (float)i / 16.0f) +
                      0.25f * arm_cos_f32(2.0f * PI * (float)i / 4.0f);
    }
}

/* Render a non-negative value as "int.frac" (two fractional digits); float
 * printf is not available with the newlib-nano configuration. */
static void fixed2_to_str(float value, char *out)
{
    uint32_t scaled;

    if (value <= 0.0f)
    {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    scaled = (uint32_t)((value * 100.0f) + 0.5f);
    sprintf(out, "%lu.%02lu", (unsigned long)(scaled / 100U),
            (unsigned long)(scaled % 100U));
}

int main(void)
{
    arm_cfft_radix4_instance_f32 scfft;
    uint32_t t0;
    uint32_t elapsed;
    uint32_t i;
    uint32_t peak;
    uint8_t  res;
    char     line[64];
    char     num[24];

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DSP TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    printf("50_dsp ready\r\n");

    /* BasicMath benchmark: plain libm, then the CMSIS-DSP kernels. */
    t0      = sys_get_tick();
    res     = sin_cos_test(PI / 6.0f, SIN_COS_TIMES, DSP_MATH_PLAIN);
    elapsed = sys_get_tick() - t0;
    sprintf(line, "Math noDSP:%lu ms", (unsigned long)elapsed);
    printf("sin/cos noDSP : %s (%s)\r\n", line, (res == 0U) ? "OK" : "error");
    lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line,
                    (res == 0U) ? BLUE : RED);

    t0      = sys_get_tick();
    res     = sin_cos_test(PI / 6.0f, SIN_COS_TIMES, DSP_MATH_CMSIS);
    elapsed = sys_get_tick() - t0;
    sprintf(line, "Math DSP  :%lu ms", (unsigned long)elapsed);
    printf("sin/cos DSP   : %s (%s)\r\n", line, (res == 0U) ? "OK" : "error");
    lcd_show_string(TEXT_X, 120U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line,
                    (res == 0U) ? BLUE : RED);

    /* FFT benchmark. */
    if (arm_cfft_radix4_init_f32(&scfft, (uint16_t)FFT_LENGTH, 0U, 1U) != ARM_MATH_SUCCESS)
    {
        printf("FFT init failed\r\n");
        lcd_show_string(TEXT_X, 150U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "FFT init failed", RED);
    }
    else
    {
        t0 = sys_get_tick();

        for (i = 0U; i < FFT_RUNS; i++)
        {
            fft_signal_fill();
            arm_cfft_radix4_f32(&scfft, g_fft_inputbuf);
        }

        elapsed = sys_get_tick() - t0;

        sprintf(line, "FFT %u pts:%lu ms", (unsigned)FFT_LENGTH, (unsigned long)elapsed);
        printf("%u point FFT x%u: %lu ms total\r\n", (unsigned)FFT_LENGTH,
               (unsigned)FFT_RUNS, (unsigned long)elapsed);
        lcd_show_string(TEXT_X, 150U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);

        arm_cmplx_mag_f32(g_fft_inputbuf, g_fft_outputbuf, FFT_LENGTH);

        peak = 0U;

        for (i = 1U; i < (FFT_LENGTH / 2U); i++)
        {
            if (g_fft_outputbuf[i] > g_fft_outputbuf[peak])
            {
                peak = i;
            }
        }

        sprintf(line, "FFT peak bin:%lu", (unsigned long)peak);
        printf("FFT peak bin %lu magnitude %lu\r\n", (unsigned long)peak,
               (unsigned long)g_fft_outputbuf[peak]);
        lcd_show_string(TEXT_X, 170U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);

        for (i = 0U; i < FFT_SIGNAL_LEN; i++)
        {
            printf("g_fft_outputbuf[%lu]:%lu\r\n", (unsigned long)i,
                   (unsigned long)g_fft_outputbuf[i]);
        }
    }

    /* FIR benchmark plus statistics on the filter output. */
    for (i = 0U; i < FIR_NUM_TAPS; i++)
    {
        g_fir_coeffs[i] = 1.0f / (float)FIR_NUM_TAPS;
    }

    fir_signal_fill();

    {
        arm_fir_instance_f32 fir;
        float32_t            mean;
        float32_t            maxval;
        uint32_t             maxidx;
        float32_t            energy;

        arm_fir_init_f32(&fir, (uint16_t)FIR_NUM_TAPS, g_fir_coeffs, g_fir_state,
                         (uint32_t)FIR_BLOCK);

        t0 = sys_get_tick();

        for (i = 0U; i < FIR_RUNS; i++)
        {
            arm_fir_f32(&fir, g_fir_in, g_fir_out, (uint32_t)FIR_BLOCK);
        }

        elapsed = sys_get_tick() - t0;

        sprintf(line, "FIR %u taps:%lu ms", (unsigned)FIR_NUM_TAPS, (unsigned long)elapsed);
        printf("%u tap FIR x%u: %lu ms total\r\n", (unsigned)FIR_NUM_TAPS,
               (unsigned)FIR_RUNS, (unsigned long)elapsed);
        lcd_show_string(TEXT_X, 190U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);

        arm_mean_f32(g_fir_out, (uint32_t)FIR_BLOCK, &mean);
        arm_max_f32(g_fir_out, (uint32_t)FIR_BLOCK, &maxval, &maxidx);
        arm_dot_prod_f32(g_fir_out, g_fir_out, (uint32_t)FIR_BLOCK, &energy);

        fixed2_to_str(mean, num);
        printf("FIR out mean %s", num);
        fixed2_to_str(maxval, num);
        printf(" max %s [%lu]", num, (unsigned long)maxidx);
        fixed2_to_str(energy, num);
        printf(" energy %s\r\n", num);

        fixed2_to_str(maxval, num);
        sprintf(line, "FIR max:%s idx:%lu", num, (unsigned long)maxidx);
        lcd_show_string(TEXT_X, 210U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}

/**
 * @file    main.c
 * @brief   50_2_dsp_fft: DSP FFT benchmark (vendor experiment 50_2).
 *          A 1024-point complex FFT of a multi-tone signal is measured and
 *          the magnitudes/peak bin are reported over USART1.
 */

#include <stdio.h>
#include <math.h>

#include "bsp.h"
#include "btim.h"
#include "arm_math.h"

#define FFT_LENGTH_SAMPLE      1024U       /* FFT length: 16, 64, 256 or 1024 */
#define FFT_RUN_COUNT        100U        /* FFT repetitions used for the timing */
#define FFT_SIGNAL_LEN_SAMPLE  32U         /* Serial dump of the first N magnitudes */

/* FFT input (complex pairs) and output (magnitudes). */
static float g_fft_inputbuf[FFT_LENGTH_SAMPLE * 2U];
static float g_fft_outputbuf[FFT_LENGTH_SAMPLE];

static volatile uint8_t g_timeout;

static void on_tim6(void)
{
    g_timeout++;
}

/* Fill the complex FFT input with a multi tone test signal; the imaginary
 * part stays zero. */
static void fft_signal_fill(void)
{
    uint32_t i;

    for (i = 0U; i < FFT_LENGTH_SAMPLE; i++)
    {
        g_fft_inputbuf[2U * i] =
            100.0f +
            10.0f * arm_sin_f32(2.0f * PI * (float)i / (float)FFT_LENGTH_SAMPLE) +
            30.0f * arm_sin_f32(2.0f * PI * (float)i * 4.0f / (float)FFT_LENGTH_SAMPLE) +
            50.0f * arm_cos_f32(2.0f * PI * (float)i * 8.0f / (float)FFT_LENGTH_SAMPLE);
        g_fft_inputbuf[(2U * i) + 1U] = 0.0f;
    }
}

int main(void)
{
    arm_cfft_radix4_instance_f32 scfft;
    uint32_t t0;
    uint32_t i;
    uint32_t peak;

    bsp_init();

    btim_timx_int_init(65535U, 90U - 1U); /* 1 MHz, ~65 ms overflow */
    btim_timx_int_register(&on_tim6);

    printf(APP_BANNER "\r\n");

    if (arm_cfft_radix4_init_f32(&scfft, (uint16_t)FFT_LENGTH_SAMPLE, 0U, 1U) != ARM_MATH_SUCCESS)
    {
        printf("FFT init failed\r\n");

        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    printf("KEY0: run %u-point FFT\r\n", (unsigned)FFT_LENGTH_SAMPLE);

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            fft_signal_fill();

            TIM6->CNT  = 0U;
            g_timeout  = 0U;
            arm_cfft_radix4_f32(&scfft, g_fft_inputbuf);
            t0 = (uint32_t)TIM6->CNT + ((uint32_t)g_timeout * 65536U); /* us */

            arm_cmplx_mag_f32(g_fft_inputbuf, g_fft_outputbuf, FFT_LENGTH_SAMPLE);

            peak = 0U;

            for (i = 1U; i < (FFT_LENGTH_SAMPLE / 2U); i++)
            {
                if (g_fft_outputbuf[i] > g_fft_outputbuf[peak])
                {
                    peak = i;
                }
            }

            printf("%u point FFT runtime:%lu.%03lu ms\r\n", (unsigned)FFT_LENGTH_SAMPLE,
                   (unsigned long)(t0 / 1000U), (unsigned long)(t0 % 1000U));
            printf("FFT peak bin %lu magnitude %lu.%03lu\r\n", (unsigned long)peak,
                   (unsigned long)g_fft_outputbuf[peak],
                   (unsigned long)((g_fft_outputbuf[peak] -
                                    (float)(unsigned long)g_fft_outputbuf[peak]) * 1000.0f));

            for (i = 0U; i < FFT_LENGTH_SAMPLE; i++)
            {
                unsigned long mi = (unsigned long)g_fft_outputbuf[i];
                unsigned long mf = (unsigned long)((g_fft_outputbuf[i] - (float)mi) * 1000.0f);

                printf("g_fft_outputbuf[%lu]:%lu.%03lu\r\n", (unsigned long)i, mi, mf);
            }
        }

        led_toggle(LED0);
        delay_ms(100U);
    }
}

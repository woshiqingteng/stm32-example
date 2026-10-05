/**
 * @file    main.c
 * @brief   35_i2c_imu: SH3001 six-axis attitude (Madgwick) test. Temperature,
 *          Euler angles, accelerometer (g) and gyroscope (dps) are printed on
 *          USART1 (115200). KEY0 toggles the ANO_TC telemetry upload.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bsp.h"
#include "imu.h"
#include "fusion.h"
#include "ano.h"

#define SAMPLE_PERIOD_MS    1U
#define REPORT_TICKS        200U       /* 200 * 1 ms = 0.2 s report period */
#define ACC_LSB_PER_G       4096.0f    /* accelerometer configured for +/-8g  */
#define GYRO_LSB_PER_DPS    65.536f    /* gyroscope configured for +/-500dps  */

static int16_t  g_acc[3];
static int16_t  g_gyro[3];
static float    g_af[3];               /* accelerometer, g */
static float    g_gf[3];               /* gyroscope, rad/s */
static float    g_gdps[3];             /* gyroscope, degrees/s */
static float    g_rpy[3];              /* pitch, roll, yaw (degrees) */
static bool     g_imu_ok;
static bool     g_ano_on;
static uint16_t g_ticks;

/* Print a signed value scaled by 100 as "<int>.<frac>" (e.g. -123 -> -1.23). */
static void print_x100(int32_t v)
{
    if (v < 0)
    {
        printf("-");
        v = -v;
    }
    printf("%d.%02d", (int)(v / 100), (int)(v % 100));
}

/* Scale a float to a signed x100 integer, rounded to nearest. */
static int32_t scale100(float v)
{
    return (int32_t)((v < 0.0f) ? (v * 100.0f - 0.5f) : (v * 100.0f + 0.5f));
}

/* Print three values as x100. */
static void print_vec_x100(float x, float y, float z)
{
    print_x100(scale100(x));
    printf(" ");
    print_x100(scale100(y));
    printf(" ");
    print_x100(scale100(z));
}

/* Read the sensor, track the bias and update the attitude estimate. */
static void imu_update(void)
{
    uint8_t i;

    if (g_imu_ok)
    {
        fusion_read_xyz(g_acc, g_gyro);
        fusion_update_dynamic_bias(g_acc, g_gyro);
    }
    else
    {
        for (i = 0U; i < 3U; i++)
        {
            g_acc[i] = 0;
            g_gyro[i] = 0;
        }
    }

    for (i = 0U; i < 3U; i++)
    {
        g_af[i]   = (float)g_acc[i] / ACC_LSB_PER_G;
        g_gdps[i] = (float)g_gyro[i] / GYRO_LSB_PER_DPS;
        g_gf[i]   = g_gdps[i] * DEG2RAD;
    }

    fusion_get_eulerian_angles(g_af, g_gf, g_rpy, (float)SAMPLE_PERIOD_MS / 1000.0f);
}

/* Print temperature, attitude and the raw accelerometer/gyroscope values. */
static void report_show(void)
{
    printf("Temp : ");
    print_x100(g_imu_ok ? (int32_t)(imu_read_temperature() * 100.0f) : 0);
    printf(" C\r\n");

    printf("Pitch: ");
    print_x100(scale100(g_rpy[0]));
    printf("  Roll: ");
    print_x100(scale100(g_rpy[1]));
    printf("  Yaw: ");
    print_x100(scale100(g_rpy[2]));
    printf("\r\n");

    printf("acc(g):    ");
    print_vec_x100(g_af[0], g_af[1], g_af[2]);
    printf("   gyro(dps): ");
    print_vec_x100(g_gdps[0], g_gdps[1], g_gdps[2]);
    printf("\r\n");
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    g_imu_ok = (imu_init() == 0U);

    if (g_imu_ok)
    {
        printf("SH3001 ready\r\n");
        printf("Calibrating: keep the board still and level...\r\n");
        fusion_calibrate();
        printf("Calibration done\r\n");
    }
    else
    {
        printf("SH3001 check failed\r\n");
    }

    printf("KEY0: toggle ANO upload\r\n");

    for (;;)
    {
        imu_update();

        if (key_scan(false) == KEY0)
        {
            g_ano_on = !g_ano_on;

            /* KEY0 also switches the baud rate used for the ANO frames. */
            usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1),
                                       .baudrate = g_ano_on ? 921600U : 115200U });
        }

        if (g_ano_on)
        {
            /* ~100 Hz ANO_TC telemetry (suppresses the text report). */
            ano_report_raw(g_acc, g_gyro);
            ano_report_imu(g_rpy[1], g_rpy[0], g_rpy[2]);   /* roll, pitch, yaw */
        }
        else if (++g_ticks >= REPORT_TICKS)
        {
            g_ticks = 0U;
            report_show();
            led_toggle(LED0);
        }

        delay_ms(SAMPLE_PERIOD_MS);
    }
}

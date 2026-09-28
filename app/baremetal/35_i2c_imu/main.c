/**
 * @file    main.c
 * @brief   35_i2c_imu: SH3001 six-axis test with attitude fusion (Mahony).
 *          Accelerometer/gyroscope counts are converted to physical units and
 *          fused into Euler angles; temperature, angles (x100), acc (g) and
 *          gyro (dps) are printed on USART1 (115200) every 500 ms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bsp.h"
#include "fusion.h"

#define SAMPLE_PERIOD_MS    10U
#define REPORT_PERIOD_MS    500U
#define PI_F                3.14159265f
#define ACC_LSB_PER_G       4096.0f    /* accelerometer configured for +/-8g  */
#define GYRO_LSB_PER_DPS    65.536f    /* gyroscope configured for +/-500dps  */

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

static bool time_due(uint32_t *next, uint32_t period)
{
    if ((int32_t)(HAL_GetTick() - *next) >= 0)
    {
        *next += period;
        return true;
    }
    return false;
}

int main(void)
{
    int16_t  acc[3];
    int16_t  gyro[3];
    float    af[3];
    float    gf[3];
    float    rpy[3] = { 0.0f, 0.0f, 0.0f };
    int16_t  r100[3];
    uint32_t next_report;
    uint16_t i;
    bool     imu_ok = false;

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (imu_init() != 0U)
    {
        printf("SH3001 check failed\r\n");
    }
    else
    {
        imu_ok = true;
        printf("SH3001 ready\r\n");
        printf("Calibrating: keep the board still...\r\n");
        imu_calibrate();
        printf("Calibration done\r\n");
    }

    printf("35_i2c_imu ready (RPY fusion, KEY0: recalibrate)\r\n");

    next_report = HAL_GetTick() + REPORT_PERIOD_MS;

    for (;;)
    {
        if (imu_ok && (key_scan(false) == KEY0))
        {
            printf("Recalibrating: keep the board still...\r\n");
            delay_ms(100);
            imu_calibrate();
            printf("Calibration done\r\n");
        }

        imu_read_xyz(acc, gyro);

        if (imu_ok)
        {
            imu_update_dynamic_bias(acc, gyro);
        }

        for (i = 0U; i < 3U; i++)
        {
            af[i] = (float)acc[i] / ACC_LSB_PER_G;
            gf[i] = ((float)gyro[i] * PI_F) / (GYRO_LSB_PER_DPS * 180.0f);
        }

        fusion_get_eulerian_angles(af, gf, rpy, (float)SAMPLE_PERIOD_MS / 1000.0f);

        for (i = 0U; i < 3U; i++)
        {
            r100[i] = (int16_t)(rpy[i] * 100.0f);
        }

        if (time_due(&next_report, REPORT_PERIOD_MS))
        {
            printf("Temp : ");
            print_x100((int32_t)(imu_read_temperature() * 100.0f));
            printf(" C\r\n");

            printf("Pitch: %d  Roll: %d  Yaw: %d\r\n",
                   (int)r100[0], (int)r100[1], (int)r100[2]);

            printf("acc(g): ");
            print_x100(scale100(af[0]));
            printf(" ");
            print_x100(scale100(af[1]));
            printf(" ");
            print_x100(scale100(af[2]));
            printf("   gyro(dps): ");
            print_x100(scale100((float)gyro[0] / GYRO_LSB_PER_DPS));
            printf(" ");
            print_x100(scale100((float)gyro[1] / GYRO_LSB_PER_DPS));
            printf(" ");
            print_x100(scale100((float)gyro[2] / GYRO_LSB_PER_DPS));
            printf("\r\n");

            led_toggle(LED0);
        }

        delay_ms(SAMPLE_PERIOD_MS);
    }
}

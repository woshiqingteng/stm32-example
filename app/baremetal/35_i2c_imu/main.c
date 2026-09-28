/**
 * @file    main.c
 * @brief   35_i2c_imu: SH3001 six-axis fusion (Mahony) + ST480MC compass.
 *          Temperature, Euler angles (x100), acc (g), gyro (dps) and the
 *          tilt-compensated heading are printed on USART1 (115200) every
 *          500 ms. KEY0 re-runs the gyro/acc zero-bias calibration.
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bsp.h"
#include "fusion.h"

#define SAMPLE_PERIOD_MS    10U
#define REPORT_TICKS        50U        /* 50 * 10 ms = 500 ms report period */
#define PI_F                3.14159265f
#define ACC_LSB_PER_G       4096.0f    /* accelerometer configured for +/-8g  */
#define GYRO_LSB_PER_DPS    65.536f    /* gyroscope configured for +/-500dps  */
#define MAG_LSB_PER_GAUSS_XY 667.0f    /* ST480MC X/Y sensitivity */
#define MAG_LSB_PER_GAUSS_Z  400.0f    /* ST480MC Z sensitivity   */

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

int main(void)
{
    int16_t  acc[3];
    int16_t  gyro[3];
    int16_t  mag[3];
    float    af[3];
    float    gf[3];
    float    rpy[3] = { 0.0f, 0.0f, 0.0f };
    int16_t  r100[3];
    uint16_t i;
    uint16_t ticks = 0U;
    bool     imu_ok;
    bool     mag_ok;

    bsp_init();
    printf(APP_BANNER "\r\n");

    imu_ok = (imu_init() == 0U);

    if (!imu_ok)
    {
        printf("SH3001 check failed\r\n");
    }
    else
    {
        printf("SH3001 ready\r\n");
        printf("Calibrating: keep the board still...\r\n");
        imu_calibrate();
        printf("Calibration done\r\n");
    }

    mag_ok = (st480mc_init() == 0U);

    if (!mag_ok)
    {
        printf("ST480MC check failed\r\n");
    }
    else
    {
        printf("ST480MC ready\r\n");
    }

    (void)io_expand_init();
    exti_io_expand_init();

    if (imu_ok)
    {
        imu_motion_int_enable();
    }

    printf("35_i2c_imu ready (SH3001+ST480MC, KEY0: recalibrate)\r\n");

    for (;;)
    {
        if (exti_io_expand_pending())
        {
            uint8_t st;

            (void)io_expand_read_byte();      /* clear the PCF8574 INT */
            st = imu_motion_int_status();     /* clear the SH3001 latched INT */

            if ((st & IMU_STATUS_TAP) != 0U)
            {
                printf("EVENT: TAP\r\n");
            }
            if ((st & IMU_STATUS_FREEFALL) != 0U)
            {
                printf("EVENT: FREE-FALL\r\n");
            }
            if ((st & IMU_STATUS_ACTIVITY) != 0U)
            {
                printf("EVENT: ACTIVITY\r\n");
            }
        }

        if (imu_ok)
        {
            imu_read_xyz(acc, gyro);
            imu_update_dynamic_bias(acc, gyro);
        }
        else
        {
            for (i = 0U; i < 3U; i++)
            {
                acc[i] = 0;
                gyro[i] = 0;
            }
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

        if (key_scan(false) == KEY0)
        {
            if (imu_ok)
            {
                printf("Recalibrating: keep the board still...\r\n");
                delay_ms(100);
                imu_calibrate();
                printf("Calibration done\r\n");
            }
        }

        ticks++;

        if (ticks >= REPORT_TICKS)
        {
            uint8_t mag_ret = 1U;

            ticks = 0U;

            if (mag_ok)
            {
                uint8_t k;

                for (k = 0U; (k < 20U) && (mag_ret != 0U); k++)
                {
                    mag_ret = st480mc_read_magdata(&mag[0], &mag[1], &mag[2]);
                }
            }

            printf("Temp : ");
            print_x100(imu_ok ? (int32_t)(imu_read_temperature() * 100.0f) : 0);
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

            if (mag_ret == 0U)
            {
                float pitch = rpy[0] * DEG2RAD;
                float roll = rpy[1] * DEG2RAD;
                float mxg = (float)mag[0] / MAG_LSB_PER_GAUSS_XY;
                float myg = (float)mag[1] / MAG_LSB_PER_GAUSS_XY;
                float mzg = (float)mag[2] / MAG_LSB_PER_GAUSS_Z;
                float xh = mxg * cosf(pitch) + mzg * sinf(pitch);
                float yh = mxg * sinf(roll) * sinf(pitch) + myg * cosf(roll) -
                           mzg * sinf(roll) * cosf(pitch);
                float heading = atan2f(-yh, xh) * RAD2DEG;

                if (heading < 0.0f)
                {
                    heading += 360.0f;
                }

                printf("Heading: ");
                print_x100(scale100(heading));
                printf("\r\n");
            }

            led_toggle(LED0);
        }

        delay_ms(SAMPLE_PERIOD_MS);
    }
}

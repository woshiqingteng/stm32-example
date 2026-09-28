/**
 * @file    main.c
 * @brief   34_i2c_magnet: ST480MC 3-axis magnetometer test. Raw X/Y/Z counts,
 *          the temperature and the compass heading are printed over USART1.
 *          KEY0 runs a horizontal (min/max) compass calibration.
 */

#include <stdio.h>
#include <math.h>

#include "bsp.h"
#include "st480mc.h"

#define SAMPLE_PERIOD_MS   200U

static int16_t g_magx_offset;
static int16_t g_magy_offset;

/* Print a float with one fractional digit (float printf is unavailable). */
static void print_fixed1(const char *label, const char *unit, float value)
{
    int32_t x10 = (int32_t)((value * 10.0f) + ((value < 0.0f) ? -0.5f : 0.5f));
    int32_t mag = (x10 < 0) ? -x10 : x10;

    printf("%s%s%d.%d%s\r\n", label, (x10 < 0) ? "-" : "",
           (int)(mag / 10), (int)(mag % 10), unit);
}

static float compass_get_angle(void)
{
    float   angle = 0.0f;
    int16_t magx;
    int16_t magy;
    int16_t magz;

    if (st480mc_read_magdata_average(&magx, &magy, &magz, 10U) != 0U)
    {
        return 0.0f;
    }

    magx = (int16_t)(magx - g_magx_offset);
    magy = (int16_t)(magy - g_magy_offset);

    if ((magx > 0) && (magy > 0))
    {
        angle = (float)((atan((double)magy / (double)magx) * 180.0) / 3.14159);
    }
    else if ((magx > 0) && (magy < 0))
    {
        angle = 360.0f + (float)((atan((double)magy / (double)magx) * 180.0) / 3.14159);
    }
    else if ((magx == 0) && (magy > 0))
    {
        angle = 90.0f;
    }
    else if ((magx == 0) && (magy < 0))
    {
        angle = 270.0f;
    }
    else if (magx < 0)
    {
        angle = 180.0f + (float)((atan((double)magy / (double)magx) * 180.0) / 3.14159);
    }
    else
    {
        angle = 0.0f;
    }

    if (angle > 360.0f)
    {
        angle = 360.0f;
    }
    if (angle < 0.0f)
    {
        angle = 0.0f;
    }

    return angle;
}

/* Horizontal min/max calibration: rotate a full turn, then press KEY0. */
static void compass_calibration(void)
{
    int16_t x_min = 0;
    int16_t x_max = 0;
    int16_t y_min = 0;
    int16_t y_max = 0;
    int16_t magx;
    int16_t magy;
    int16_t magz;

    printf("Compass calibration: rotate horizontally, press KEY0 when done\r\n");

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            break;
        }

        if (st480mc_read_magdata(&magx, &magy, &magz) == 0U)
        {
            if (magx > x_max) { x_max = magx; }
            if (magx < x_min) { x_min = magx; }
            if (magy > y_max) { y_max = magy; }
            if (magy < y_min) { y_min = magy; }
            led_toggle(LED0);
        }

        delay_ms(50U);
    }

    g_magx_offset = (int16_t)((x_max + x_min) / 2);
    g_magy_offset = (int16_t)((y_max + y_min) / 2);

    printf("x_min:%d x_max:%d\r\n", (int)x_min, (int)x_max);
    printf("y_min:%d y_max:%d\r\n", (int)y_min, (int)y_max);
    printf("offset x:%d y:%d\r\n", (int)g_magx_offset, (int)g_magy_offset);
}

int main(void)
{
    int16_t magx;
    int16_t magy;
    int16_t magz;
    float   temperature;
    float   angle;
    uint8_t t = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (st480mc_init() != 0U)
    {
        printf("ST480MC check failed\r\n");
    }
    else
    {
        printf("ST480MC ready\r\n");
    }

    printf("34_i2c_magnet ready\r\n");

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            compass_calibration();
        }

        t++;

        if (t >= 5U)                                /* ~1 s */
        {
            t = 0U;

            angle = compass_get_angle();
            print_fixed1("Angle: ", "", 360.0f - angle);

            if (st480mc_read_temperature(&temperature) == 0U)
            {
                print_fixed1("Temp: ", " C", temperature);
            }

            if (st480mc_read_magdata(&magx, &magy, &magz) == 0U)
            {
                printf("MagX:%d\r\n", (int)magx);
                printf("MagY:%d\r\n", (int)magy);
                printf("MagZ:%d\r\n", (int)magz);
            }

            led_toggle(LED0);
        }

        delay_ms(SAMPLE_PERIOD_MS);
    }
}

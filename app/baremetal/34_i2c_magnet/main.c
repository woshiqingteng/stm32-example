/**
 * @file    main.c
 * @brief   34_i2c_magnet: ST480MC 3-axis magnetometer test. Raw X/Y/Z counts,
 *          the temperature and the compass heading are printed over USART1.
 *          KEY0 runs a horizontal (min/max) compass calibration.
 */

#include <stdio.h>
#include <math.h>

#include "bsp.h"
#include "mag.h"

#define SAMPLE_PERIOD_MS   200U
#define REPORT_TICK        5U            /* 5 * 200 ms = 1 s */
#define AVERAGE_TIMES      10U
#define PI_F               3.14159265f

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

/* Average @p times single-shot samples (retrying transient read errors). */
static uint8_t mag_read_average(int16_t *x, int16_t *y, int16_t *z, uint8_t times)
{
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sz = 0;
    uint8_t got = 0U;
    uint8_t err = 0U;
    int16_t mx;
    int16_t my;
    int16_t mz;

    while (got < times)
    {
        if (mag_read(&mx, &my, &mz) == 0U)
        {
            sx += mx;
            sy += my;
            sz += mz;
            got++;
            err = 0U;
        }
        else
        {
            err++;
            delay_ms(10U);

            if (err > 100U)
            {
                return 0xFFU;
            }
        }
    }

    *x = (int16_t)(sx / times);
    *y = (int16_t)(sy / times);
    *z = (int16_t)(sz / times);

    return 0U;
}

/* Compass heading [0, 360): atan2 of the offset-corrected X/Y. */
static float compass_get_angle(void)
{
    int16_t magx;
    int16_t magy;
    int16_t magz;
    float   angle;

    if (mag_read_average(&magx, &magy, &magz, AVERAGE_TIMES) != 0U)
    {
        return 0.0f;
    }

    angle = atan2f((float)(magy - g_magy_offset),
                   (float)(magx - g_magx_offset)) * (180.0f / PI_F);

    return (angle < 0.0f) ? (angle + 360.0f) : angle;
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

        if (mag_read(&magx, &magy, &magz) == 0U)
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

/* One report: heading, temperature and the raw X/Y/Z counts. */
static void mag_show(void)
{
    int16_t magx;
    int16_t magy;
    int16_t magz;
    float   temperature;

    print_fixed1("Angle: ", "", 360.0f - compass_get_angle());

    if (mag_read_temp(&temperature) == 0U)
    {
        print_fixed1("Temp: ", " C", temperature);
    }

    if (mag_read(&magx, &magy, &magz) == 0U)
    {
        printf("MagX:%d\r\n", (int)magx);
        printf("MagY:%d\r\n", (int)magy);
        printf("MagZ:%d\r\n", (int)magz);
    }
}

int main(void)
{
    uint8_t t = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (mag_init() != 0U)
    {
        printf("ST480MC check failed\r\n");
    }

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            compass_calibration();
        }

        if (++t >= REPORT_TICK)
        {
            t = 0U;
            mag_show();
            led_toggle(LED0);
        }

        delay_ms(SAMPLE_PERIOD_MS);
    }
}

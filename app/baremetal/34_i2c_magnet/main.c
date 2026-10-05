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
static uint8_t mag_read_average(mag_data_t *avg, uint8_t times)
{
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sz = 0;
    float   st = 0.0f;
    uint8_t got = 0U;
    uint8_t err = 0U;
    mag_data_t d;

    while (got < times)
    {
        if (mag_read(&d) == 0U)
        {
            sx += d.x;
            sy += d.y;
            sz += d.z;
            st += d.temperature;
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

    avg->x = (int16_t)(sx / times);
    avg->y = (int16_t)(sy / times);
    avg->z = (int16_t)(sz / times);
    avg->temperature = st / (float)times;

    return 0U;
}

/* Compass heading [0, 360) from the offset-corrected X/Y. */
static float compass_get_angle(const mag_data_t *d)
{
    float heading = atan2f((float)(d->y - g_magy_offset),
                           (float)(d->x - g_magx_offset)) * (180.0f / PI_F);

    return (heading < 0.0f) ? (heading + 360.0f) : heading;
}

/* Horizontal min/max calibration: rotate a full turn, then press KEY0. */
static void compass_calibration(void)
{
    int16_t x_min = 0;
    int16_t x_max = 0;
    int16_t y_min = 0;
    int16_t y_max = 0;
    mag_data_t d;

    printf("Compass calibration: rotate horizontally, press KEY0 when done\r\n");

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            break;
        }

        if (mag_read(&d) == 0U)
        {
            if (d.x > x_max) { x_max = d.x; }
            if (d.x < x_min) { x_min = d.x; }
            if (d.y > y_max) { y_max = d.y; }
            if (d.y < y_min) { y_min = d.y; }
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

/* One report: heading, temperature and the averaged raw X/Y/Z counts. */
static void mag_show(void)
{
    mag_data_t d;

    if (mag_read_average(&d, AVERAGE_TIMES) != 0U)
    {
        printf("Angle: --\r\n");
        printf("Temp: -- C\r\n");
        printf("MagX:--\r\n");
        printf("MagY:--\r\n");
        printf("MagZ:--\r\n");
        return;
    }

    print_fixed1("Angle: ", "", 360.0f - compass_get_angle(&d));
    print_fixed1("Temp: ", " C", d.temperature);
    printf("MagX:%d\r\n", (int)d.x);
    printf("MagY:%d\r\n", (int)d.y);
    printf("MagZ:%d\r\n", (int)d.z);
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

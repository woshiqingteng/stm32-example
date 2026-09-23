/**
 * @file    main.c
 * @brief   35_qmi8658a: QMI8658A six-axis test. Raw accelerometer/gyroscope
 *          counts and the die temperature are printed over USART1. No attitude
 *          fusion is used.
 */

#include <stdio.h>
#include "bsp.h"

#define SAMPLE_PERIOD   100U

static void format_temp(char *buf, int16_t temp_x100)
{
    int16_t mag = (temp_x100 < 0) ? (int16_t)(-temp_x100) : temp_x100;

    sprintf(buf, "TEMP: %s%d.%02d C", (temp_x100 < 0) ? "-" : "",
            (int)(mag / 100), (int)(mag % 100));
}

int main(void)
{
    char line[56];
    int16_t acc[3];
    int16_t gyro[3];

    bsp_init();

    if (qmi8658a_init() != 0U)
    {
        printf("QMI8658A check failed\r\n");
    }
    else
    {
        printf("QMI8658A ready\r\n");
    }

    printf("35_qmi8658a ready\r\n");

    for (;;)
    {
        qmi8658a_read_xyz(acc, gyro);

        sprintf(line, "ACC: %d %d %d", (int)acc[0], (int)acc[1], (int)acc[2]);
        printf("%s\r\n", line);

        sprintf(line, "GYR: %d %d %d", (int)gyro[0], (int)gyro[1], (int)gyro[2]);
        printf("%s\r\n", line);

        format_temp(line, (int16_t)(qmi8658a_read_temperature() * 100.0f));
        printf("%s\r\n", line);

        led_toggle(LED0);
        delay_ms(SAMPLE_PERIOD);
    }
}

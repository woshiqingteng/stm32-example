/**
 * @file    main.c
 * @brief   35_i2c_imu: QMI8658A six-axis test with attitude fusion. The raw
 *          accelerometer/gyroscope counts are converted to physical units and
 *          fed to the Mahony filter (lib_imu); the roll/pitch/yaw angles are
 *          printed on USART1 and streamed as ANO_TC frames (0xAA 0xAA ...).
 */

#include <stdio.h>
#include <stdint.h>

#include "bsp.h"
#include "imu.h"
#define SAMPLE_PERIOD_MS    10U
#define PI_F                3.14159265f
#define ACC_LSB_PER_G       4096.0f    /* accelerometer configured for +/-8g  */
#define GYRO_LSB_PER_DPS    64.0f      /* gyroscope configured for +/-512dps  */

static void format_temp(char *buf, int16_t temp_x100)
{
    int16_t mag = (temp_x100 < 0) ? (int16_t)(-temp_x100) : temp_x100;

    sprintf(buf, "TEMP: %s%d.%02d C", (temp_x100 < 0) ? "-" : "",
            (int)(mag / 100), (int)(mag % 100));
}

static void uart1_write(const uint8_t *buf, uint16_t len)
{
    (void)HAL_UART_Transmit(&g_uart1_handle, (uint8_t *)buf, len, 100U);
}

/* ANO_TC frame: AA AA <fun> <len> <data...> <sum8>. */
static void ano_tc_send(uint8_t fun, const uint8_t *data, uint8_t len)
{
    uint8_t buf[32];
    uint8_t i;
    uint8_t sum = 0U;

    buf[0] = 0xAAU;
    buf[1] = 0xAAU;
    buf[2] = fun;
    buf[3] = len;

    for (i = 0U; i < len; i++)
    {
        buf[4U + i] = data[i];
    }

    for (i = 0U; i < (uint8_t)(len + 4U); i++)
    {
        sum = (uint8_t)(sum + buf[i]);
    }

    buf[len + 4U] = sum;
    uart1_write(buf, (uint16_t)(len + 5U));
}

int main(void)
{
    char     line[64];
    int16_t  acc[3];
    int16_t  gyro[3];
    float    af[3];
    float    gf[3];
    float    rpy[3] = { 0.0f, 0.0f, 0.0f };
    uint8_t  tbuf[18];
    uint16_t i;
    int16_t  r100[3];

    bsp_init();

    if (qmi8658a_init() != 0U)
    {
        printf("QMI8658A check failed\r\n");
    }
    else
    {
        printf("QMI8658A ready\r\n");
    }

    printf("35_i2c_imu ready (RPY fusion + ANO_TC)\r\n");

    for (;;)
    {
        qmi8658a_read_xyz(acc, gyro);

        for (i = 0U; i < 3U; i++)
        {
            af[i] = (float)acc[i] / ACC_LSB_PER_G;
            gf[i] = ((float)gyro[i] * PI_F) / (GYRO_LSB_PER_DPS * 180.0f);
        }

        imu_get_eulerian_angles(af, gf, rpy, (float)SAMPLE_PERIOD_MS / 1000.0f);

        for (i = 0U; i < 3U; i++)
        {
            r100[i] = (int16_t)(rpy[i] * 100.0f);
        }

        sprintf(line, "RPY(x100): %d %d %d", (int)r100[0], (int)r100[1], (int)r100[2]);
        printf("%s\r\n", line);

        format_temp(line, (int16_t)(qmi8658a_read_temperature() * 100.0f));
        printf("%s\r\n", line);

        /* 0x01: roll/pitch/yaw (deg*100) + 4 reserved bytes. */
        for (i = 0U; i < 3U; i++)
        {
            tbuf[2U * i]     = (uint8_t)((uint16_t)r100[i] >> 8);
            tbuf[2U * i + 1] = (uint8_t)((uint16_t)r100[i] & 0xFFU);
        }

        tbuf[6] = 0U;
        tbuf[7] = 0U;
        tbuf[8] = 0U;
        tbuf[9] = 0U;
        ano_tc_send(0x01U, tbuf, 10U);

        /* 0x02: raw accelerometer + gyroscope counts. */
        for (i = 0U; i < 3U; i++)
        {
            tbuf[2U * i]       = (uint8_t)((uint16_t)acc[i] >> 8);
            tbuf[2U * i + 1U]  = (uint8_t)((uint16_t)acc[i] & 0xFFU);
            tbuf[6U + 2U * i]     = (uint8_t)((uint16_t)gyro[i] >> 8);
            tbuf[6U + 2U * i + 1U] = (uint8_t)((uint16_t)gyro[i] & 0xFFU);
        }

        ano_tc_send(0x02U, tbuf, 12U);

        led_toggle(LED0);
        delay_ms(SAMPLE_PERIOD_MS);
    }
}

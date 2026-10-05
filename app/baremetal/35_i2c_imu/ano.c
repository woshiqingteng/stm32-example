/**
 * @file    ano.c
 * @brief   ANO_TC v4 telemetry upload over USART1.
 *
 * Frame: [0xAA][0xAA][fun][len][data...][checksum], checksum = sum of the
 * first (len + 4) bytes. Matches the ALIENTEK "实验35" reporting so the data
 * can be viewed with the ANO_TC host software.
 */

#include <stddef.h>

#include "usart.h"
#include "ano.h"

/* Send one frame (max 28 payload bytes). */
static void ano_send(uint8_t fun, const uint8_t *data, uint8_t len)
{
    uint8_t buf[32];
    uint8_t sum = 0U;
    uint8_t i;

    if (len > 28U)
    {
        return;
    }

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

    (void)usart_write(USART_ID_1, buf, (uint32_t)(len + 5U));
}

void ano_report_imu(float roll, float pitch, float yaw)
{
    /* fun 0x01: roll, pitch, yaw (0.01 deg, big-endian int16),
     * prs (int32) = 0, fly_mode = 0, armed = 0. */
    int16_t r = (int16_t)(roll * 100.0f);
    int16_t p = (int16_t)(pitch * 100.0f);
    int16_t y = (int16_t)(yaw * 100.0f);
    uint8_t data[12];

    data[0]  = (uint8_t)(r >> 8);
    data[1]  = (uint8_t)r;
    data[2]  = (uint8_t)(p >> 8);
    data[3]  = (uint8_t)p;
    data[4]  = (uint8_t)(y >> 8);
    data[5]  = (uint8_t)y;
    data[6]  = 0U;   /* prs[31:24] */
    data[7]  = 0U;   /* prs[23:16] */
    data[8]  = 0U;   /* prs[15:8]  */
    data[9]  = 0U;   /* prs[7:0]   */
    data[10] = 0U;   /* fly_mode   */
    data[11] = 0U;   /* armed      */

    ano_send(0x01U, data, 12U);
}

void ano_report_raw(const int16_t acc[3], const int16_t gyro[3])
{
    /* fun 0x02: acc x/y/z + gyro x/y/z (big-endian int16, raw counts) + 6 x 0. */
    uint8_t data[18];
    uint8_t i;

    for (i = 0U; i < 3U; i++)
    {
        data[i * 2U]      = (uint8_t)(acc[i] >> 8);
        data[i * 2U + 1U] = (uint8_t)acc[i];
    }

    for (i = 0U; i < 3U; i++)
    {
        data[6U + i * 2U]      = (uint8_t)(gyro[i] >> 8);
        data[6U + i * 2U + 1U] = (uint8_t)gyro[i];
    }

    for (i = 12U; i < 18U; i++)
    {
        data[i] = 0U;
    }

    ano_send(0x02U, data, 18U);
}

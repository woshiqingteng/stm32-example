/**
 * @file    touch_gt9xxx.c
 * @brief   GT9xxx capacitive touch controller.
 *
 * The controller has its own software IIC bus (CT_SCL = PH6, CT_SDA = PI3) that
 * is separate from the general-purpose IIC on PH4/PH5. RST is on PI8 and INT on
 * PH7, following the vendor TOUCH driver.
 */

#include <string.h>

#include "stm32f4xx_hal.h"
#include "touch_gt9xxx.h"
#include "delay.h"

/* --- capacitive touch IIC pins --- */
#define CT_IIC_SCL_GPIO_PORT    GPIOH
#define CT_IIC_SCL_GPIO_PIN     GPIO_PIN_6
#define CT_IIC_SDA_GPIO_PORT    GPIOI
#define CT_IIC_SDA_GPIO_PIN     GPIO_PIN_3

/* --- controller reset / interrupt pins --- */
#define GT9XXX_RST_GPIO_PORT    GPIOI
#define GT9XXX_RST_GPIO_PIN     GPIO_PIN_8
#define GT9XXX_INT_GPIO_PORT    GPIOH
#define GT9XXX_INT_GPIO_PIN     GPIO_PIN_7

/* --- GT9xxx register map --- */
#define GT9XXX_CMD_WR           0x28U
#define GT9XXX_CMD_RD           0x29U
#define GT9XXX_CTRL_REG         0x8040U
#define GT9XXX_PID_REG          0x8140U
#define GT9XXX_GSTID_REG        0x814EU
#define GT9XXX_TP1_REG          0x8150U
#define GT9XXX_TP_REG_STEP      0x0008U

#define GT9XXX_IIC_DELAY_US     2U
#define GT9XXX_ACK_TIMEOUT_COUNT      250U
#define GT9XXX_RESET_DELAY_MS   10U
#define GT9XXX_PROBE_DELAY_MS   100U
#define GT9XXX_CTRL_RESET       0x02U
#define GT9XXX_CTRL_NORMAL      0x00U
#define GT9XXX_STATUS_READY     0x80U
#define GT9XXX_STATUS_COUNT_MSK 0x0FU

#define CT_SCL(x)   do { (x) ? HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); } while (0)
#define CT_SDA(x)   do { (x) ? HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); } while (0)
#define CT_READ_SDA HAL_GPIO_ReadPin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN)

#define GT9XXX_RST(x) do { (x) ? HAL_GPIO_WritePin(GT9XXX_RST_GPIO_PORT, GT9XXX_RST_GPIO_PIN, GPIO_PIN_SET) \
                               : HAL_GPIO_WritePin(GT9XXX_RST_GPIO_PORT, GT9XXX_RST_GPIO_PIN, GPIO_PIN_RESET); } while (0)

static const uint16_t g_gt_tp_reg[TOUCH_GT9XXX_MAX_POINT_COUNT] =
{
    GT9XXX_TP1_REG,
    GT9XXX_TP1_REG + GT9XXX_TP_REG_STEP,
    GT9XXX_TP1_REG + (2U * GT9XXX_TP_REG_STEP),
    GT9XXX_TP1_REG + (3U * GT9XXX_TP_REG_STEP),
    GT9XXX_TP1_REG + (4U * GT9XXX_TP_REG_STEP),
};

static void ct_iic_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_init.Pin   = CT_IIC_SCL_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CT_IIC_SCL_GPIO_PORT, &gpio_init);

    gpio_init.Pin = CT_IIC_SDA_GPIO_PIN;
    HAL_GPIO_Init(CT_IIC_SDA_GPIO_PORT, &gpio_init);

    CT_SDA(1);
    CT_SCL(1);
}

static void ct_iic_start(void)
{
    CT_SDA(1);
    CT_SCL(1);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SDA(0);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SCL(0);
    delay_us(GT9XXX_IIC_DELAY_US);
}

static void ct_iic_stop(void)
{
    CT_SDA(0);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SCL(1);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SDA(1);
    delay_us(GT9XXX_IIC_DELAY_US);
}

static uint8_t ct_iic_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    CT_SDA(1);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SCL(1);
    delay_us(GT9XXX_IIC_DELAY_US);

    while (CT_READ_SDA)
    {
        waittime++;

        if (waittime > GT9XXX_ACK_TIMEOUT_COUNT)
        {
            ct_iic_stop();
            rack = 1;
            break;
        }

        delay_us(GT9XXX_IIC_DELAY_US);
    }

    CT_SCL(0);
    delay_us(GT9XXX_IIC_DELAY_US);

    return rack;
}

static void ct_iic_ack(void)
{
    CT_SDA(0);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SCL(1);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SCL(0);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SDA(1);
    delay_us(GT9XXX_IIC_DELAY_US);
}

static void ct_iic_nack(void)
{
    CT_SDA(1);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SCL(1);
    delay_us(GT9XXX_IIC_DELAY_US);
    CT_SCL(0);
    delay_us(GT9XXX_IIC_DELAY_US);
}

static void ct_iic_send_byte(uint8_t data)
{
    uint8_t t;

    for (t = 0; t < 8U; t++)
    {
        CT_SDA((data & 0x80U) >> 7);
        delay_us(GT9XXX_IIC_DELAY_US);
        CT_SCL(1);
        delay_us(GT9XXX_IIC_DELAY_US);
        CT_SCL(0);
        data <<= 1;
    }

    CT_SDA(1);
}

static uint8_t ct_iic_read_byte(uint8_t ack)
{
    uint8_t i;
    uint8_t receive = 0;

    for (i = 0; i < 8U; i++)
    {
        receive <<= 1;
        CT_SCL(1);
        delay_us(GT9XXX_IIC_DELAY_US);

        if (CT_READ_SDA != 0U)
        {
            receive++;
        }

        CT_SCL(0);
        delay_us(GT9XXX_IIC_DELAY_US);
    }

    if (ack != 0U)
    {
        ct_iic_ack();
    }
    else
    {
        ct_iic_nack();
    }

    return receive;
}

static void gt9xxx_wr_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    ct_iic_start();
    ct_iic_send_byte(GT9XXX_CMD_WR);
    ct_iic_wait_ack();
    ct_iic_send_byte((uint8_t)(reg >> 8));
    ct_iic_wait_ack();
    ct_iic_send_byte((uint8_t)(reg & 0xFFU));
    ct_iic_wait_ack();

    for (i = 0U; i < len; i++)
    {
        ct_iic_send_byte(buf[i]);

        if (ct_iic_wait_ack() != 0U)
        {
            break;
        }
    }

    ct_iic_stop();
}

static void gt9xxx_rd_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    ct_iic_start();
    ct_iic_send_byte(GT9XXX_CMD_WR);
    ct_iic_wait_ack();
    ct_iic_send_byte((uint8_t)(reg >> 8));
    ct_iic_wait_ack();
    ct_iic_send_byte((uint8_t)(reg & 0xFFU));
    ct_iic_wait_ack();

    ct_iic_start();
    ct_iic_send_byte(GT9XXX_CMD_RD);
    ct_iic_wait_ack();

    for (i = 0U; i < len; i++)
    {
        buf[i] = ct_iic_read_byte((i == (uint8_t)(len - 1U)) ? 0U : 1U);
    }

    ct_iic_stop();
}

static uint8_t gt9xxx_check_id(void)
{
    uint8_t pid[5];

    gt9xxx_rd_reg(GT9XXX_PID_REG, pid, 4U);
    pid[4] = 0U;

    if ((strncmp((char *)pid, "9147", 4U) == 0) ||
        (strncmp((char *)pid, "1158", 4U) == 0) ||
        (strncmp((char *)pid, "9271", 4U) == 0) ||
        (strncmp((char *)pid, "911", 3U) == 0))
    {
        return 0U;
    }

    return 1U;
}

uint8_t touch_gt9xxx_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint8_t ctrl;
    uint8_t ret;

    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio_init.Pin   = GT9XXX_RST_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GT9XXX_RST_GPIO_PORT, &gpio_init);

    gpio_init.Pin  = GT9XXX_INT_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GT9XXX_INT_GPIO_PORT, &gpio_init);

    ct_iic_init();

    GT9XXX_RST(0);
    delay_ms(GT9XXX_RESET_DELAY_MS);
    GT9XXX_RST(1);
    delay_ms(GT9XXX_RESET_DELAY_MS);

    gpio_init.Pin  = GT9XXX_INT_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GT9XXX_INT_GPIO_PORT, &gpio_init);

    delay_ms(GT9XXX_PROBE_DELAY_MS);
    ret = gt9xxx_check_id();

    ctrl = GT9XXX_CTRL_RESET;
    gt9xxx_wr_reg(GT9XXX_CTRL_REG, &ctrl, 1U);
    delay_ms(GT9XXX_RESET_DELAY_MS);

    ctrl = GT9XXX_CTRL_NORMAL;
    gt9xxx_wr_reg(GT9XXX_CTRL_REG, &ctrl, 1U);

    return ret;
}

bool touch_gt9xxx_read(uint16_t *x, uint16_t *y, uint8_t max_points, uint8_t *count)
{
    uint8_t status;
    uint8_t n;
    uint8_t buf[4];
    uint8_t i;

    gt9xxx_rd_reg(GT9XXX_GSTID_REG, &status, 1U);

    n = (uint8_t)(status & GT9XXX_STATUS_COUNT_MSK);

    if (((status & GT9XXX_STATUS_READY) != 0U) && (n <= TOUCH_GT9XXX_MAX_POINT_COUNT))
    {
        uint8_t clear = 0U;
        gt9xxx_wr_reg(GT9XXX_GSTID_REG, &clear, 1U);
    }

    if ((n != 0U) && (n <= TOUCH_GT9XXX_MAX_POINT_COUNT) && ((status & GT9XXX_STATUS_READY) != 0U))
    {
        if (n > max_points)
        {
            n = max_points;
        }

        for (i = 0U; i < n; i++)
        {
            gt9xxx_rd_reg(g_gt_tp_reg[i], buf, 4U);

            x[i] = (uint16_t)(((uint16_t)buf[1] << 8) | buf[0]);
            y[i] = (uint16_t)(((uint16_t)buf[3] << 8) | buf[2]);
        }

        *count = n;
        return true;
    }

    *count = 0U;
    return false;
}

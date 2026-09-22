/**
 * @file    ov5640.c
 * @brief   OV5640 camera sensor driver.
 *
 * Ported from the ALIENTEK experiment 38 vendor driver. SCCB is bit-banged on
 * PB4 (SCL) / PB3 (SDA) with a ~5 us half period, the sensor reset line is PA15
 * and power-down is routed through the PCF8574 IO expander.
 */

#include "stm32f4xx_hal.h"
#include "ov5640.h"
#include "pcf8574.h"
#include "delay.h"
#include "ov5640cfg.h"
#include "ov5640af.h"

#define OV5640_SCCB_DELAY_US   5U
#define OV5640_RESET_LOW_MS    20U
#define OV5640_RESET_HIGH_MS   20U
#define OV5640_PWDN_SETTLE_MS  5U
#define OV5640_FOCUS_TIMEOUT   1000U
#define OV5640_FOCUS_POLL_MS   5U

/* ------------------------------------------------------------------------- */
/* SCCB transport (PB4 = SCL, PB3 = SDA)                                     */
/* ------------------------------------------------------------------------- */

#define SCCB_SCL(x)  do { (x) ? HAL_GPIO_WritePin(OV5640_SCL_GPIO_PORT, OV5640_SCL_GPIO_PIN, GPIO_PIN_SET) \
                              : HAL_GPIO_WritePin(OV5640_SCL_GPIO_PORT, OV5640_SCL_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define SCCB_SDA(x)  do { (x) ? HAL_GPIO_WritePin(OV5640_SDA_GPIO_PORT, OV5640_SDA_GPIO_PIN, GPIO_PIN_SET) \
                              : HAL_GPIO_WritePin(OV5640_SDA_GPIO_PORT, OV5640_SDA_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define SCCB_READ_SDA HAL_GPIO_ReadPin(OV5640_SDA_GPIO_PORT, OV5640_SDA_GPIO_PIN)

static void sccb_delay(void)
{
    delay_us(OV5640_SCCB_DELAY_US);
}

static void sccb_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin   = OV5640_SCL_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(OV5640_SCL_GPIO_PORT, &gpio_init);

    gpio_init.Pin  = OV5640_SDA_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(OV5640_SDA_GPIO_PORT, &gpio_init);
}

static void sccb_start(void)
{
    SCCB_SDA(1);
    SCCB_SCL(1);
    sccb_delay();
    SCCB_SDA(0);
    sccb_delay();
    SCCB_SCL(0);
}

static void sccb_stop(void)
{
    SCCB_SDA(0);
    sccb_delay();
    SCCB_SCL(1);
    sccb_delay();
    SCCB_SDA(1);
    sccb_delay();
}

static void sccb_nack(void)
{
    sccb_delay();
    SCCB_SDA(1);
    SCCB_SCL(1);
    sccb_delay();
    SCCB_SCL(0);
    sccb_delay();
    SCCB_SDA(0);
    sccb_delay();
}

static uint8_t sccb_send_byte(uint8_t data)
{
    uint8_t t;
    uint8_t res;

    for (t = 0U; t < 8U; t++)
    {
        SCCB_SDA((data & 0x80U) >> 7);
        sccb_delay();
        SCCB_SCL(1);
        sccb_delay();
        SCCB_SCL(0);
        data <<= 1;
    }

    SCCB_SDA(1);
    sccb_delay();
    SCCB_SCL(1);
    sccb_delay();

    res = (SCCB_READ_SDA != 0U) ? 1U : 0U;

    SCCB_SCL(0);

    return res;
}

static uint8_t sccb_read_byte(void)
{
    uint8_t i;
    uint8_t receive = 0U;

    for (i = 0U; i < 8U; i++)
    {
        receive <<= 1;
        SCCB_SCL(1);
        sccb_delay();

        if (SCCB_READ_SDA != 0U)
        {
            receive++;
        }

        SCCB_SCL(0);
        sccb_delay();
    }

    return receive;
}

/* ------------------------------------------------------------------------- */
/* Register access                                                           */
/* ------------------------------------------------------------------------- */

uint8_t ov5640_read_reg(uint16_t reg)
{
    uint8_t data;

    sccb_start();
    (void)sccb_send_byte(OV5640_SCCB_ADDR);
    (void)sccb_send_byte((uint8_t)(reg >> 8));
    (void)sccb_send_byte((uint8_t)reg);
    sccb_stop();

    sccb_start();
    (void)sccb_send_byte(OV5640_SCCB_ADDR | 0x01U);
    data = sccb_read_byte();
    sccb_nack();
    sccb_stop();

    return data;
}

uint8_t ov5640_write_reg(uint16_t reg, uint8_t data)
{
    uint8_t res = 0U;

    sccb_start();

    if (sccb_send_byte(OV5640_SCCB_ADDR) != 0U)
    {
        res = 1U;
    }

    if (sccb_send_byte((uint8_t)(reg >> 8)) != 0U)
    {
        res = 1U;
    }

    if (sccb_send_byte((uint8_t)reg) != 0U)
    {
        res = 1U;
    }

    if (sccb_send_byte(data) != 0U)
    {
        res = 1U;
    }

    sccb_stop();

    return res;
}

void ov5640_pwdn_set(uint8_t sta)
{
    pcf8574_write_bit(PCF8574_DCMI_PWDN_IO, sta);
}

uint16_t ov5640_read_id(void)
{
    uint16_t id;

    id  = (uint16_t)((uint16_t)ov5640_read_reg(OV5640_CHIPIDH) << 8);
    id |= (uint16_t)ov5640_read_reg(OV5640_CHIPIDL);

    return id;
}

/* ------------------------------------------------------------------------- */
/* Initialisation and mode selection                                         */
/* ------------------------------------------------------------------------- */

uint8_t ov5640_init(void)
{
    uint16_t i;
    uint16_t id;
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Pin   = OV5640_RESET_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OV5640_RESET_GPIO_PORT, &gpio_init);

    (void)pcf8574_init();

    HAL_GPIO_WritePin(OV5640_RESET_GPIO_PORT, OV5640_RESET_GPIO_PIN, GPIO_PIN_RESET);
    delay_ms(OV5640_RESET_LOW_MS);

    ov5640_pwdn_set(0U);                            /* power on */
    delay_ms(OV5640_PWDN_SETTLE_MS);

    HAL_GPIO_WritePin(OV5640_RESET_GPIO_PORT, OV5640_RESET_GPIO_PIN, GPIO_PIN_SET);
    delay_ms(OV5640_RESET_HIGH_MS);

    sccb_init();
    delay_ms(OV5640_PWDN_SETTLE_MS);

    id = ov5640_read_id();

    if (id != OV5640_CHIPID)
    {
        return 1U;
    }

    (void)ov5640_write_reg(0x3103, 0X11);
    (void)ov5640_write_reg(0X3008, 0X82);
    delay_ms(10);

    for (i = 0U; i < (uint16_t)(sizeof(ov5640_init_reg_tbl) / sizeof(ov5640_init_reg_tbl[0])); i++)
    {
        (void)ov5640_write_reg(ov5640_init_reg_tbl[i][0], (uint8_t)ov5640_init_reg_tbl[i][1]);
    }

    ov5640_flash_ctrl(1U);
    delay_ms(50);
    ov5640_flash_ctrl(0U);

    return 0U;
}

void ov5640_jpeg_mode(void)
{
    uint16_t i;

    for (i = 0U; i < (uint16_t)(sizeof(ov5640_jpeg_reg_tbl) / sizeof(ov5640_jpeg_reg_tbl[0])); i++)
    {
        (void)ov5640_write_reg(ov5640_jpeg_reg_tbl[i][0], (uint8_t)ov5640_jpeg_reg_tbl[i][1]);
    }
}

void ov5640_rgb565_mode(void)
{
    uint16_t i;

    for (i = 0U; i < (uint16_t)(sizeof(ov5640_rgb565_reg_tbl) / sizeof(ov5640_rgb565_reg_tbl[0])); i++)
    {
        (void)ov5640_write_reg(ov5640_rgb565_reg_tbl[i][0], (uint8_t)ov5640_rgb565_reg_tbl[i][1]);
    }

    /* RGB panel path: enable horizontal mirror. */
    (void)ov5640_write_reg(0X3821, 0X06);
}

/* ------------------------------------------------------------------------- */
/* Image adjustments                                                         */
/* ------------------------------------------------------------------------- */

static const uint8_t ov5640_exposure_tbl[7][6] =
{
    { 0x10, 0x08, 0x10, 0x08, 0x20, 0x10 },     /* -3 */
    { 0x20, 0x18, 0x41, 0x20, 0x18, 0x10 },     /* -2 */
    { 0x30, 0x28, 0x61, 0x30, 0x28, 0x10 },     /* -1 */
    { 0x38, 0x30, 0x61, 0x38, 0x30, 0x10 },     /*  0 */
    { 0x40, 0x38, 0x71, 0x40, 0x38, 0x10 },     /* +1 */
    { 0x50, 0x48, 0x90, 0x50, 0x48, 0x20 },     /* +2 */
    { 0x60, 0x58, 0xa0, 0x60, 0x58, 0x20 }      /* +3 */
};

void ov5640_exposure(uint8_t exposure)
{
    (void)ov5640_write_reg(0x3212, 0x03);
    (void)ov5640_write_reg(0x3a0f, ov5640_exposure_tbl[exposure][0]);
    (void)ov5640_write_reg(0x3a10, ov5640_exposure_tbl[exposure][1]);
    (void)ov5640_write_reg(0x3a1b, ov5640_exposure_tbl[exposure][2]);
    (void)ov5640_write_reg(0x3a1e, ov5640_exposure_tbl[exposure][3]);
    (void)ov5640_write_reg(0x3a11, ov5640_exposure_tbl[exposure][4]);
    (void)ov5640_write_reg(0x3a1f, ov5640_exposure_tbl[exposure][5]);
    (void)ov5640_write_reg(0x3212, 0x13);
    (void)ov5640_write_reg(0x3212, 0xa3);
}

static const uint8_t ov5640_lightmode_tbl[5][7] =
{
    { 0x04, 0X00, 0X04, 0X00, 0X04, 0X00, 0X00 },   /* Auto */
    { 0x06, 0X1C, 0X04, 0X00, 0X04, 0XF3, 0X01 },   /* Sunny */
    { 0x05, 0X48, 0X04, 0X00, 0X07, 0XCF, 0X01 },   /* Office */
    { 0x06, 0X48, 0X04, 0X00, 0X04, 0XD3, 0X01 },   /* Cloudy */
    { 0x04, 0X10, 0X04, 0X00, 0X08, 0X40, 0X01 }    /* Home */
};

void ov5640_light_mode(uint8_t mode)
{
    uint8_t i;

    (void)ov5640_write_reg(0x3212, 0x03);

    for (i = 0U; i < 7U; i++)
    {
        (void)ov5640_write_reg((uint16_t)(0x3400 + i), ov5640_lightmode_tbl[mode][i]);
    }

    (void)ov5640_write_reg(0x3212, 0x13);
    (void)ov5640_write_reg(0x3212, 0xa3);
}

static const uint8_t ov5640_saturation_tbl[7][6] =
{
    { 0X0C, 0x30, 0X3D, 0X3E, 0X3D, 0X01 },     /* -3 */
    { 0X10, 0x3D, 0X4D, 0X4E, 0X4D, 0X01 },     /* -2 */
    { 0X15, 0x52, 0X66, 0X68, 0X66, 0X02 },     /* -1 */
    { 0X1A, 0x66, 0X80, 0X82, 0X80, 0X02 },     /*  0 */
    { 0X1F, 0x7A, 0X9A, 0X9C, 0X9A, 0X02 },     /* +1 */
    { 0X24, 0x8F, 0XB3, 0XB6, 0XB3, 0X03 },     /* +2 */
    { 0X2B, 0xAB, 0XD6, 0XDA, 0XD6, 0X04 }      /* +3 */
};

void ov5640_color_saturation(uint8_t sat)
{
    uint8_t i;

    (void)ov5640_write_reg(0x3212, 0x03);
    (void)ov5640_write_reg(0x5381, 0x1c);
    (void)ov5640_write_reg(0x5382, 0x5a);
    (void)ov5640_write_reg(0x5383, 0x06);

    for (i = 0U; i < 6U; i++)
    {
        (void)ov5640_write_reg((uint16_t)(0x5384 + i), ov5640_saturation_tbl[sat][i]);
    }

    (void)ov5640_write_reg(0x538b, 0x98);
    (void)ov5640_write_reg(0x538a, 0x01);
    (void)ov5640_write_reg(0x3212, 0x13);
    (void)ov5640_write_reg(0x3212, 0xa3);
}

void ov5640_brightness(uint8_t bright)
{
    uint8_t brtval;

    if (bright < 4U)
    {
        brtval = (uint8_t)(4U - bright);
    }
    else
    {
        brtval = (uint8_t)(bright - 4U);
    }

    (void)ov5640_write_reg(0x3212, 0x03);
    (void)ov5640_write_reg(0x5587, (uint8_t)(brtval << 4));

    if (bright < 4U)
    {
        (void)ov5640_write_reg(0x5588, 0x09);
    }
    else
    {
        (void)ov5640_write_reg(0x5588, 0x01);
    }

    (void)ov5640_write_reg(0x3212, 0x13);
    (void)ov5640_write_reg(0x3212, 0xa3);
}

void ov5640_contrast(uint8_t contrast)
{
    uint8_t reg0val = 0X00;
    uint8_t reg1val = 0X20;

    switch (contrast)
    {
        case 0U:
            reg0val = 0X14;
            reg1val = 0X14;
            break;

        case 1U:
            reg0val = 0X18;
            reg1val = 0X18;
            break;

        case 2U:
            reg0val = 0X1C;
            reg1val = 0X1C;
            break;

        case 4U:
            reg0val = 0X10;
            reg1val = 0X24;
            break;

        case 5U:
            reg0val = 0X18;
            reg1val = 0X28;
            break;

        case 6U:
            reg0val = 0X1C;
            reg1val = 0X2C;
            break;

        default:
            break;
    }

    (void)ov5640_write_reg(0x3212, 0x03);
    (void)ov5640_write_reg(0x5585, reg0val);
    (void)ov5640_write_reg(0x5586, reg1val);
    (void)ov5640_write_reg(0x3212, 0x13);
    (void)ov5640_write_reg(0x3212, 0xa3);
}

void ov5640_sharpness(uint8_t sharp)
{
    if (sharp < 33U)
    {
        (void)ov5640_write_reg(0x5308, 0x65);
        (void)ov5640_write_reg(0x5302, sharp);
    }
    else
    {
        (void)ov5640_write_reg(0x5308, 0x25);
        (void)ov5640_write_reg(0x5300, 0x08);
        (void)ov5640_write_reg(0x5301, 0x30);
        (void)ov5640_write_reg(0x5302, 0x10);
        (void)ov5640_write_reg(0x5303, 0x00);
        (void)ov5640_write_reg(0x5309, 0x08);
        (void)ov5640_write_reg(0x530a, 0x30);
        (void)ov5640_write_reg(0x530b, 0x04);
        (void)ov5640_write_reg(0x530c, 0x06);
    }
}

static const uint8_t ov5640_effects_tbl[7][3] =
{
    { 0X06, 0x40, 0X10 },   /* Normal */
    { 0X1E, 0xA0, 0X40 },   /* Cool */
    { 0X1E, 0x80, 0XC0 },   /* Warm */
    { 0X1E, 0x80, 0X80 },   /* B&W */
    { 0X1E, 0x40, 0XA0 },   /* Yellowish */
    { 0X40, 0x40, 0X10 },   /* Inverse */
    { 0X1E, 0x60, 0X60 }    /* Greenish */
};

void ov5640_special_effects(uint8_t eft)
{
    (void)ov5640_write_reg(0x3212, 0x03);
    (void)ov5640_write_reg(0x5580, ov5640_effects_tbl[eft][0]);
    (void)ov5640_write_reg(0x5583, ov5640_effects_tbl[eft][1]);
    (void)ov5640_write_reg(0x5584, ov5640_effects_tbl[eft][2]);
    (void)ov5640_write_reg(0x5003, 0x08);
    (void)ov5640_write_reg(0x3212, 0x13);
    (void)ov5640_write_reg(0x3212, 0xa3);
}

void ov5640_test_pattern(uint8_t mode)
{
    if (mode == 0U)
    {
        (void)ov5640_write_reg(0X503D, 0X00);
    }
    else if (mode == 1U)
    {
        (void)ov5640_write_reg(0X503D, 0X80);
    }
    else if (mode == 2U)
    {
        (void)ov5640_write_reg(0X503D, 0X82);
    }
    else
    {
        /* nothing */
    }
}

void ov5640_flash_ctrl(uint8_t sw)
{
    (void)ov5640_write_reg(0x3016, 0X02);
    (void)ov5640_write_reg(0x301C, 0X02);

    if (sw != 0U)
    {
        (void)ov5640_write_reg(0X3019, 0X02);
    }
    else
    {
        (void)ov5640_write_reg(0X3019, 0X00);
    }
}

uint8_t ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)
{
    (void)ov5640_write_reg(0X3212, 0X03);

    (void)ov5640_write_reg(0x3808, (uint8_t)(width >> 8));
    (void)ov5640_write_reg(0x3809, (uint8_t)(width & 0xffU));
    (void)ov5640_write_reg(0x380a, (uint8_t)(height >> 8));
    (void)ov5640_write_reg(0x380b, (uint8_t)(height & 0xffU));

    (void)ov5640_write_reg(0x3810, (uint8_t)(offx >> 8));
    (void)ov5640_write_reg(0x3811, (uint8_t)(offx & 0xffU));
    (void)ov5640_write_reg(0x3812, (uint8_t)(offy >> 8));
    (void)ov5640_write_reg(0x3813, (uint8_t)(offy & 0xffU));

    (void)ov5640_write_reg(0X3212, 0X13);
    (void)ov5640_write_reg(0X3212, 0Xa3);

    return 0U;
}

uint8_t ov5640_image_window_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)
{
    uint16_t xst;
    uint16_t yst;
    uint16_t xend;
    uint16_t yend;

    xst = offx;
    yst = offy;
    xend = (uint16_t)(offx + width - 1U);
    yend = (uint16_t)(offy + height - 1U);

    (void)ov5640_write_reg(0X3212, 0X03);
    (void)ov5640_write_reg(0X3800, (uint8_t)(xst >> 8));
    (void)ov5640_write_reg(0X3801, (uint8_t)(xst & 0XFFU));
    (void)ov5640_write_reg(0X3802, (uint8_t)(yst >> 8));
    (void)ov5640_write_reg(0X3803, (uint8_t)(yst & 0XFFU));
    (void)ov5640_write_reg(0X3804, (uint8_t)(xend >> 8));
    (void)ov5640_write_reg(0X3805, (uint8_t)(xend & 0XFFU));
    (void)ov5640_write_reg(0X3806, (uint8_t)(yend >> 8));
    (void)ov5640_write_reg(0X3807, (uint8_t)(yend & 0XFFU));
    (void)ov5640_write_reg(0X3212, 0X13);
    (void)ov5640_write_reg(0X3212, 0Xa3);

    return 0U;
}

/* ------------------------------------------------------------------------- */
/* Auto focus (firmware loaded into the sensor MCU)                          */
/* ------------------------------------------------------------------------- */

uint8_t ov5640_focus_init(void)
{
    uint16_t i;
    uint16_t addr = 0x8000U;
    uint8_t state = 0x8FU;

    (void)ov5640_write_reg(0x3000, 0x20);                   /* reset MCU */

    for (i = 0U; i < (uint16_t)sizeof(ov5640_af_config); i++)
    {
        (void)ov5640_write_reg(addr, ov5640_af_config[i]);
        addr++;
    }

    (void)ov5640_write_reg(0x3022, 0x00);
    (void)ov5640_write_reg(0x3023, 0x00);
    (void)ov5640_write_reg(0x3024, 0x00);
    (void)ov5640_write_reg(0x3025, 0x00);
    (void)ov5640_write_reg(0x3026, 0x00);
    (void)ov5640_write_reg(0x3027, 0x00);
    (void)ov5640_write_reg(0x3028, 0x00);
    (void)ov5640_write_reg(0x3029, 0x7f);
    (void)ov5640_write_reg(0x3000, 0x00);

    i = 0U;
    do
    {
        state = ov5640_read_reg(0x3029);
        delay_ms(OV5640_FOCUS_POLL_MS);
        i++;

        if (i > OV5640_FOCUS_TIMEOUT)
        {
            return 1U;
        }
    } while (state != 0x70);

    return 0U;
}

uint8_t ov5640_focus_single(void)
{
    uint8_t temp;
    uint16_t retry = 0U;

    (void)ov5640_write_reg(0x3022, 0x03);                   /* trigger single AF */

    for (;;)
    {
        retry++;
        temp = ov5640_read_reg(0x3029);

        if (temp == 0x10)
        {
            break;
        }

        delay_ms(OV5640_FOCUS_POLL_MS);

        if (retry > OV5640_FOCUS_TIMEOUT)
        {
            return 1U;
        }
    }

    return 0U;
}

uint8_t ov5640_focus_constant(void)
{
    uint8_t temp = 0U;
    uint16_t retry = 0U;

    (void)ov5640_write_reg(0x3023, 0x01);
    (void)ov5640_write_reg(0x3022, 0x08);                   /* trigger idle */

    do
    {
        temp = ov5640_read_reg(0x3023);
        retry++;

        if (retry > OV5640_FOCUS_TIMEOUT)
        {
            return 2U;
        }

        delay_ms(OV5640_FOCUS_POLL_MS);
    } while (temp != 0x00);

    (void)ov5640_write_reg(0x3023, 0x01);
    (void)ov5640_write_reg(0x3022, 0x04);                   /* start continuous AF */
    retry = 0U;

    do
    {
        temp = ov5640_read_reg(0x3023);
        retry++;

        if (retry > OV5640_FOCUS_TIMEOUT)
        {
            return 2U;
        }

        delay_ms(OV5640_FOCUS_POLL_MS);
    } while (temp != 0x00);

    return 0U;
}

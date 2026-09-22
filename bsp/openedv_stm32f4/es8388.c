/**
 * @file    es8388.c
 * @brief   ES8388 audio codec control over the bit-bang IIC bus.
 */

#include "es8388.h"
#include "iic.h"
#include "delay.h"

uint8_t es8388_init(void)
{
    iic_init();                     /* initialise the IIC interface */

    es8388_write_reg(0, 0x80);      /* software reset */
    es8388_write_reg(0, 0x00);
    delay_ms(100);                  /* wait for the reset */

    es8388_write_reg(0x01, 0x58);
    es8388_write_reg(0x01, 0x50);
    es8388_write_reg(0x02, 0xF3);
    es8388_write_reg(0x02, 0xF0);

    es8388_write_reg(0x03, 0x09);   /* mic bias off */
    es8388_write_reg(0x00, 0x06);   /* reference 500K, slow */
    es8388_write_reg(0x04, 0x00);   /* DAC power, all outputs off */
    es8388_write_reg(0x08, 0x00);   /* MCLK divider */
    es8388_write_reg(0x2B, 0x80);   /* DACLRC = ADCLRC */

    es8388_write_reg(0x09, 0x88);   /* ADC L/R PGA gain +24 dB */
    es8388_write_reg(0x0C, 0x4C);   /* ADC data select, 16-bit */
    es8388_write_reg(0x0D, 0x02);   /* ADC MCLK / sample rate = 256 */
    es8388_write_reg(0x10, 0x00);   /* ADC L input attenuation, minimum */
    es8388_write_reg(0x11, 0x00);   /* ADC R input attenuation, minimum */

    es8388_write_reg(0x17, 0x18);   /* DAC 16-bit */
    es8388_write_reg(0x18, 0x02);   /* DAC MCLK / sample rate = 256 */
    es8388_write_reg(0x1A, 0x00);   /* DAC L input attenuation, minimum */
    es8388_write_reg(0x1B, 0x00);   /* DAC R input attenuation, minimum */
    es8388_write_reg(0x27, 0xB8);   /* L mixer */
    es8388_write_reg(0x2A, 0xB8);   /* R mixer */
    delay_ms(100);

    return 0;
}

uint8_t es8388_write_reg(uint8_t reg, uint8_t val)
{
    iic_start();

    iic_send_byte((ES8388_ADDR << 1) | 0);  /* device address + write */
    if (iic_wait_ack())
    {
        return 1;
    }

    iic_send_byte(reg);                     /* register address */
    if (iic_wait_ack())
    {
        return 2;
    }

    iic_send_byte(val & 0xFF);              /* data */
    if (iic_wait_ack())
    {
        return 3;
    }

    iic_stop();

    return 0;
}

uint8_t es8388_read_reg(uint8_t reg)
{
    uint8_t temp = 0;

    iic_start();

    iic_send_byte((ES8388_ADDR << 1) | 0);  /* device address + write */
    if (iic_wait_ack())
    {
        return 1;
    }

    iic_send_byte(reg);                     /* register address */
    if (iic_wait_ack())
    {
        return 1;
    }

    iic_start();
    iic_send_byte((ES8388_ADDR << 1) | 1);  /* device address + read */
    if (iic_wait_ack())
    {
        return 1;
    }

    temp = iic_read_byte(0);

    iic_stop();

    return temp;
}

void es8388_sai_cfg(uint8_t fmt, uint8_t len)
{
    fmt &= 0x03;
    len &= 0x07;                                    /* clamp the range */
    es8388_write_reg(23, (fmt << 1) | (len << 3));  /* R23: SAI format */
}

void es8388_hpvol_set(uint8_t volume)
{
    if (volume > 33)
    {
        volume = 33;
    }

    es8388_write_reg(0x2E, volume);
    es8388_write_reg(0x2F, volume);
}

void es8388_spkvol_set(uint8_t volume)
{
    if (volume > 33)
    {
        volume = 33;
    }

    es8388_write_reg(0x30, volume);
    es8388_write_reg(0x31, volume);
}

void es8388_3d_set(uint8_t depth)
{
    depth &= 0x7;                           /* clamp the range */
    es8388_write_reg(0x1D, depth << 2);     /* R7: 3D depth */
}

void es8388_adda_cfg(uint8_t dacen, uint8_t adcen)
{
    uint8_t tempreg = 0;

    tempreg |= ((!dacen) << 0);
    tempreg |= ((!adcen) << 1);
    tempreg |= ((!dacen) << 2);
    tempreg |= ((!adcen) << 3);
    es8388_write_reg(0x02, tempreg);
}

void es8388_output_cfg(uint8_t o1en, uint8_t o2en)
{
    uint8_t tempreg = 0;
    tempreg |= o1en * (3 << 4);
    tempreg |= o2en * (3 << 2);
    es8388_write_reg(0x04, tempreg);
}

void es8388_mic_gain(uint8_t gain)
{
    gain &= 0x0F;
    gain |= gain << 4;
    es8388_write_reg(0x09, gain);       /* R9: input PGA gain */
}

void es8388_alc_ctrl(uint8_t sel, uint8_t maxgain, uint8_t mingain)
{
    uint8_t tempreg = 0;

    tempreg = sel << 6;
    tempreg |= (maxgain & 0x07) << 3;
    tempreg |= mingain & 0x07;
    es8388_write_reg(0x12, tempreg);     /* R18: ALC control */
}

void es8388_input_cfg(uint8_t in)
{
    es8388_write_reg(0x0A, (5 * in) << 4);   /* ADC1 L/R input select */
}

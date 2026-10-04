/**
 * @file    als_ap3216c.c
 * @brief   AP3216C ambient light / proximity sensor driver.
 */

#include <stddef.h>

#include "i2c.h"
#include "als_ap3216c.h"
#include "delay.h"

#define ALS_AP3216C_RESET_DELAY_MS  50U
#define ALS_AP3216C_DATA_LEN_BYTE        6U

/* Register map. */
#define ALS_AP3216C_SYS_REG     0x00U
#define ALS_AP3216C_DATA_REG    0x0AU
#define ALS_AP3216C_RESET       0x04U   /* software reset       */
#define ALS_AP3216C_ALS_PS_IR   0x03U   /* enable ALS + PS + IR */

/* Data registers 0x0A..0x0F, read in order into buf[0..5]:
 *   0x0A IR  Data Low  : bit7 = IR_OF (1 = invalid), bits[1:0] = IR[1:0]
 *   0x0B IR  Data High : bits[7:0] = IR[9:2]
 *   0x0C ALS Data Low  : bits[7:0] = ALS[7:0]
 *   0x0D ALS Data High : bits[7:0] = ALS[15:8]
 *   0x0E PS  Data Low  : bit6 = PS_OF (1 = invalid), bits[3:0] = PS[3:0]
 *   0x0F PS  Data High : bits[5:0] = PS[9:4]
 * IR/PS are 10-bit, ALS is 16-bit; the low register holds the low bits:
 *   IR  = (IR_H  << 2) | (IR_L  & 0x03)
 *   ALS = (ALS_H << 8) |  ALS_L
 *   PS  = ((PS_H & 0x3F) << 4) | (PS_L & 0x0F) */
#define AP3216C_IR_OF     0x80U
#define AP3216C_PS_OF     0x40U
#define AP3216C_IR_L_MASK 0x03U
#define AP3216C_PS_L_MASK 0x0FU
#define AP3216C_PS_H_MASK 0x3FU

static void ap3216c_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = data;
    (void)i2c_write(I2C_DEV_ALS, buf, 2U);
}

static uint8_t ap3216c_read_reg(uint8_t reg)
{
    uint8_t res = 0U;

    (void)i2c_write_read(I2C_DEV_ALS, &reg, 1U, &res, 1U);
    return res;
}

uint8_t als_ap3216c_init(void)
{
    uint8_t temp;

    i2c_init(NULL);

    ap3216c_write_reg(ALS_AP3216C_SYS_REG, ALS_AP3216C_RESET);
    delay_ms(ALS_AP3216C_RESET_DELAY_MS);
    ap3216c_write_reg(ALS_AP3216C_SYS_REG, ALS_AP3216C_ALS_PS_IR);

    temp = ap3216c_read_reg(ALS_AP3216C_SYS_REG);

    return (temp == ALS_AP3216C_ALS_PS_IR) ? 0U : 1U;
}

void als_ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als)
{
    uint8_t buf[ALS_AP3216C_DATA_LEN_BYTE];
    uint8_t ir_l, ir_h, als_l, als_h, ps_l, ps_h;
    uint8_t i;

    for (i = 0U; i < ALS_AP3216C_DATA_LEN_BYTE; i++)
    {
        buf[i] = ap3216c_read_reg((uint8_t)(ALS_AP3216C_DATA_REG + i));
    }

    ir_l  = buf[0];
    ir_h  = buf[1];
    als_l = buf[2];
    als_h = buf[3];
    ps_l  = buf[4];
    ps_h  = buf[5];

    *ir  = ((ir_l & AP3216C_IR_OF) != 0U) ? 0U
          : (uint16_t)(((uint16_t)ir_h << 2) | (ir_l & AP3216C_IR_L_MASK));
    *als = (uint16_t)(((uint16_t)als_h << 8) | als_l);
    *ps  = ((ps_l & AP3216C_PS_OF) != 0U) ? 0U
          : (uint16_t)(((uint16_t)(ps_h & AP3216C_PS_H_MASK) << 4) | (ps_l & AP3216C_PS_L_MASK));
}

/**
 * @file    codec.c
 * @brief   Audio codec control interface (delegates to the chip driver).
 */

#include "codec.h"
#include "codec_es8388.h"

uint8_t codec_init(void)
{
    return codec_es8388_init();
}

uint8_t codec_write_reg(uint8_t reg, uint8_t val)
{
    return codec_es8388_write_reg(reg, val);
}

uint8_t codec_read_reg(uint8_t reg)
{
    return codec_es8388_read_reg(reg);
}

void codec_sai_cfg(uint8_t fmt, uint8_t len)
{
    codec_es8388_sai_cfg(fmt, len);
}

void codec_hpvol_set(uint8_t volume)
{
    codec_es8388_hpvol_set(volume);
}

void codec_spkvol_set(uint8_t volume)
{
    codec_es8388_spkvol_set(volume);
}

void codec_3d_set(uint8_t depth)
{
    codec_es8388_3d_set(depth);
}

void codec_adda_cfg(uint8_t dacen, uint8_t adcen)
{
    codec_es8388_adda_cfg(dacen, adcen);
}

void codec_output_cfg(uint8_t o1en, uint8_t o2en)
{
    codec_es8388_output_cfg(o1en, o2en);
}

void codec_mic_gain(uint8_t gain)
{
    codec_es8388_mic_gain(gain);
}

void codec_alc_ctrl(uint8_t sel, uint8_t maxgain, uint8_t mingain)
{
    codec_es8388_alc_ctrl(sel, maxgain, mingain);
}

void codec_input_cfg(uint8_t in)
{
    codec_es8388_input_cfg(in);
}

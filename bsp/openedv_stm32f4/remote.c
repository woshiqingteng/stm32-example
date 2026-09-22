/**
 * @file    remote.c
 * @brief   NEC infrared remote receiver, ported from the vendor REMOTE example.
 *          TIM1_CH1 (PA8) is used in input-capture mode with 1 us resolution;
 *          the capture/update interrupts are handled directly.
 */

#include "stm32f4xx_hal.h"
#include "remote.h"

#define REMOTE_PRESCALER        (180U - 1U) /* 1 tick = 1 us at 180 MHz */
#define REMOTE_PERIOD           10000U
#define REMOTE_IC_FILTER        0x03U
#define REMOTE_NVIC_PREEMP      1U
#define REMOTE_NVIC_UP_SUB      3U
#define REMOTE_NVIC_CC_SUB      2U

#define REMOTE_STA_HIGH         0x10U /*!< a high level has been captured */
#define REMOTE_STA_KEY          0x40U /*!< a decoded key is available    */
#define REMOTE_STA_READY        0x80U /*!< a full frame has been received */
#define REMOTE_STA_TIME_MASK    0x0FU
#define REMOTE_STA_ALL_FLAGS    0xF0U
#define REMOTE_REPEAT_MAX       14U

#define REMOTE_BIT0_MIN         300U
#define REMOTE_BIT0_MAX         800U
#define REMOTE_BIT1_MIN         1400U
#define REMOTE_BIT1_MAX         1800U
#define REMOTE_REPEAT_MIN       2000U
#define REMOTE_REPEAT_MAX_VAL   3000U
#define REMOTE_LEAD_MIN         4200U
#define REMOTE_LEAD_MAX         4700U

static TIM_HandleTypeDef g_remote_handle;
static uint8_t           g_remote_sta;
static uint32_t          g_remote_data;

uint8_t g_remote_cnt;

void remote_init(void)
{
    GPIO_InitTypeDef   gpio_init = {0};
    TIM_IC_InitTypeDef ic        = {0};

    /* ---- MSP begin: TIM1 clock + PA8 + NVIC ---- */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Pin       = REMOTE_IN_GPIO_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = REMOTE_IN_GPIO_AF;
    HAL_GPIO_Init(REMOTE_IN_GPIO_PORT, &gpio_init);

    HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, REMOTE_NVIC_PREEMP, REMOTE_NVIC_UP_SUB);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
    HAL_NVIC_SetPriority(TIM1_CC_IRQn, REMOTE_NVIC_PREEMP, REMOTE_NVIC_CC_SUB);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
    /* ---- MSP end ---- */

    g_remote_handle.Instance               = REMOTE_IN_TIMX;
    g_remote_handle.Init.Prescaler         = REMOTE_PRESCALER;
    g_remote_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_remote_handle.Init.Period            = REMOTE_PERIOD;
    g_remote_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_remote_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    (void)HAL_TIM_IC_Init(&g_remote_handle);

    ic.ICPolarity  = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = REMOTE_IC_FILTER;
    (void)HAL_TIM_IC_ConfigChannel(&g_remote_handle, &ic, REMOTE_IN_TIMX_CHY);

    g_remote_sta  = 0U;
    g_remote_data = 0U;
    g_remote_cnt  = 0U;

    __HAL_TIM_ENABLE_IT(&g_remote_handle, TIM_IT_UPDATE);
    (void)HAL_TIM_IC_Start_IT(&g_remote_handle, REMOTE_IN_TIMX_CHY);
}

static void remote_update_isr(void)
{
    if ((g_remote_sta & REMOTE_STA_READY) != 0U)
    {
        g_remote_sta &= (uint8_t)~REMOTE_STA_HIGH;

        if ((g_remote_sta & REMOTE_STA_TIME_MASK) == 0U)
        {
            g_remote_sta |= REMOTE_STA_KEY;
        }

        if ((g_remote_sta & REMOTE_STA_TIME_MASK) < REMOTE_REPEAT_MAX)
        {
            g_remote_sta++;
        }
        else
        {
            g_remote_sta &= (uint8_t)~REMOTE_STA_READY;
            g_remote_sta &= REMOTE_STA_ALL_FLAGS;
        }
    }
}

static void remote_capture_isr(void)
{
    uint16_t dval;

    if (HAL_GPIO_ReadPin(REMOTE_IN_GPIO_PORT, REMOTE_IN_GPIO_PIN) != GPIO_PIN_RESET)
    {
        /* rising edge: restart timing */
        __HAL_TIM_SET_CAPTUREPOLARITY(&g_remote_handle, REMOTE_IN_TIMX_CHY,
                                      TIM_INPUTCHANNELPOLARITY_FALLING);
        __HAL_TIM_SET_COUNTER(&g_remote_handle, 0U);
        g_remote_sta |= REMOTE_STA_HIGH;
    }
    else
    {
        /* falling edge: the high-level width carries the bit value */
        dval = (uint16_t)HAL_TIM_ReadCapturedValue(&g_remote_handle, REMOTE_IN_TIMX_CHY);
        __HAL_TIM_SET_CAPTUREPOLARITY(&g_remote_handle, REMOTE_IN_TIMX_CHY,
                                      TIM_INPUTCHANNELPOLARITY_RISING);

        if ((g_remote_sta & REMOTE_STA_HIGH) != 0U)
        {
            if ((g_remote_sta & REMOTE_STA_READY) != 0U)
            {
                if ((dval > REMOTE_BIT0_MIN) && (dval < REMOTE_BIT0_MAX))
                {
                    g_remote_data >>= 1;
                    g_remote_data &= ~(0x80000000U);
                }
                else if ((dval > REMOTE_BIT1_MIN) && (dval < REMOTE_BIT1_MAX))
                {
                    g_remote_data >>= 1;
                    g_remote_data |= 0x80000000U;
                }
                else if ((dval > REMOTE_REPEAT_MIN) && (dval < REMOTE_REPEAT_MAX_VAL))
                {
                    g_remote_cnt++;
                    g_remote_sta &= REMOTE_STA_ALL_FLAGS;
                }
            }
            else if ((dval > REMOTE_LEAD_MIN) && (dval < REMOTE_LEAD_MAX))
            {
                g_remote_sta |= REMOTE_STA_READY;
                g_remote_cnt = 0U;
            }
        }

        g_remote_sta &= (uint8_t)~REMOTE_STA_HIGH;
    }
}

void TIM1_CC_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_remote_handle, TIM_FLAG_CC1) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_remote_handle, TIM_FLAG_CC1);
        remote_capture_isr();
    }
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_remote_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_remote_handle, TIM_FLAG_UPDATE);
        remote_update_isr();
    }
}

uint8_t remote_parse(uint32_t frame)
{
    uint8_t addr;
    uint8_t addr_inv;
    uint8_t cmd;
    uint8_t cmd_inv;

    addr     = (uint8_t)frame;
    addr_inv = (uint8_t)(frame >> 8);

    if ((addr != (uint8_t)~addr_inv) || (addr != REMOTE_ID))
    {
        return 0U;
    }

    cmd     = (uint8_t)(frame >> 16);
    cmd_inv = (uint8_t)(frame >> 24);

    if (cmd != (uint8_t)~cmd_inv)
    {
        return 0U;
    }

    return cmd;
}

uint8_t remote_scan(void)
{
    uint8_t key = 0U;

    if ((g_remote_sta & REMOTE_STA_KEY) != 0U)
    {
        key = remote_parse(g_remote_data);

        if ((key == 0U) || ((g_remote_sta & REMOTE_STA_READY) == 0U))
        {
            g_remote_sta &= (uint8_t)~REMOTE_STA_KEY;
            g_remote_cnt = 0U;
        }
    }

    return key;
}

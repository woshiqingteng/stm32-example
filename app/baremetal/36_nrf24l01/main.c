/**
 * @file    main.c
 * @brief   36_nrf24l01: NRF24L01 wireless link test. The role (RX/TX) can be
 *          selected with KEY0 / KEY1; when no key is pressed the default is RX.
 *          The transmitted / received payload is printed and shown on the RGB
 *          panel.
 */

#include <stdio.h>
#include "bsp.h"

#define NRF_MODE_SELECT_MS  5000U
#define NRF_TX_PERIOD_MS    500U

#define TEXT_X              30U
#define TEXT_WIDTH          300U

/** @brief  Selected NRF24L01 role. */
typedef enum
{
    NRF_MODE_RX = 0,
    NRF_MODE_TX = 1
} nrf_mode_t;

static void nrf_run_rx(void)
{
    uint8_t payload[NRF24L01_RX_PLOAD_WIDTH];
    char    line[48];

    lcd_show_string(TEXT_X, 170U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Mode: RX", BLUE);
    printf("NRF24L01 RX mode\r\n");
    nrf24l01_rx_mode();

    for (;;)
    {
        if (nrf24l01_rx_packet(payload) == 0U)
        {
            payload[NRF24L01_RX_PLOAD_WIDTH - 1U] = '\0';
            sprintf(line, "RX: %s", (char *)payload);
            lcd_show_string(TEXT_X, 210U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
            printf("%s\r\n", line);
            led_toggle(LED1);
        }
        else
        {
            delay_us(100U);
        }
    }
}

static void nrf_run_tx(void)
{
    uint8_t  payload[NRF24L01_TX_PLOAD_WIDTH];
    uint32_t count = 0U;
    char     line[48];
    uint8_t  i;

    lcd_show_string(TEXT_X, 170U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Mode: TX", BLUE);
    printf("NRF24L01 TX mode\r\n");
    nrf24l01_tx_mode();

    for (;;)
    {
        for (i = 0U; i < NRF24L01_TX_PLOAD_WIDTH; i++)
        {
            payload[i] = (uint8_t)' ';
        }

        (void)snprintf((char *)payload, NRF24L01_TX_PLOAD_WIDTH, "NRF TX #%lu",
                       (unsigned long)count);

        if (nrf24l01_tx_packet(payload) == 0U)
        {
            sprintf(line, "TX: %s", (char *)payload);
            lcd_show_string(TEXT_X, 210U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
            printf("%s\r\n", line);
            led_toggle(LED0);
        }
        else
        {
            lcd_show_string(TEXT_X, 210U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "TX failed", RED);
            printf("NRF24L01 TX failed\r\n");
        }

        count++;
        delay_ms(NRF_TX_PERIOD_MS);
    }
}

int main(void)
{
    uint8_t  key;
    uint32_t waited = 0U;
    nrf_mode_t mode = NRF_MODE_RX; /* RX by default */

    bsp_init();
    sdram_init();
    lcd_init();
    nrf24l01_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "NRF24L01 TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    while (nrf24l01_check() != 0U)
    {
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "NRF24L01 Error!", RED);
        printf("NRF24L01 not found!\r\n");
        delay_ms(200U);
    }

    lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "NRF24L01 Ready!", BLUE);
    lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "KEY0:RX  KEY1:TX", RED);
    printf("36_nrf24l01 ready\r\n");

    while (waited < NRF_MODE_SELECT_MS)
    {
        key = key_scan(false);

        if (key == KEY0)
        {
            mode = NRF_MODE_RX;
            break;
        }
        else if (key == KEY1)
        {
            mode = NRF_MODE_TX;
            break;
        }

        waited += 10U;
        delay_ms(10U);
    }

    if (mode == NRF_MODE_RX)
    {
        nrf_run_rx();
    }
    else
    {
        nrf_run_tx();
    }

    return 0;
}

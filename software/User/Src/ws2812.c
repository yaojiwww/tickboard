#include "ws2812.h"

/*
 * WS2812 时序（DWT 周期计数实现）
 *   0 码: 高 0.35us + 低 0.90us
 *   1 码: 高 0.70us + 低 0.50us
 *   复位: 低电平 >= 50us
 * 高电平宽度决定 0/1，低电平稍长不影响。
 */
#define T0H_NS      350U
#define T0L_NS      900U
#define T1H_NS      700U
#define T1L_NS      500U
#define RESET_NS    300000U

static uint8_t  s_led[RGB_LED_COUNT][3];   /* [0]=G, [1]=R, [2]=B */
static uint32_t s_t0h, s_t0l, s_t1h, s_t1l;

static uint32_t ns_to_cycles(uint32_t ns)
{
    uint64_t cycles = ((uint64_t)SystemCoreClock * ns) / 1000000000ULL;
    return (uint32_t)cycles;
}

static inline __attribute__((always_inline)) void ws_delay_cycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < cycles)
    {
    }
}

void WS2812_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin   = RGB_DATA_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RGB_DATA_PORT, &gpio);

    Board_PinLow(RGB_DATA_PORT, RGB_DATA_PIN);

    /* 减去函数调用/读 DWT 的少量固定开销 */
    s_t0h = ns_to_cycles(T0H_NS);
    s_t0l = ns_to_cycles(T0L_NS);
    s_t1h = ns_to_cycles(T1H_NS);
    s_t1l = ns_to_cycles(T1L_NS);

    if (s_t0h > 6U) { s_t0h -= 4U; }
    if (s_t1h > 6U) { s_t1h -= 4U; }

    WS2812_Clear();
    WS2812_Show();
}

void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= RGB_LED_COUNT)
    {
        return;
    }

    s_led[index][0] = g;
    s_led[index][1] = r;
    s_led[index][2] = b;
}

void WS2812_Fill(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t i;

    for (i = 0; i < RGB_LED_COUNT; i++)
    {
        WS2812_SetPixel(i, r, g, b);
    }
}

void WS2812_Clear(void)
{
    WS2812_Fill(0U, 0U, 0U);
}

static void ws_send_byte(uint8_t dat)
{
    uint8_t i;

    for (i = 0; i < 8U; i++)
    {
        if (dat & 0x80U)
        {
            Board_PinHigh(RGB_DATA_PORT, RGB_DATA_PIN);
            ws_delay_cycles(s_t1h);
            Board_PinLow(RGB_DATA_PORT, RGB_DATA_PIN);
            ws_delay_cycles(s_t1l);
        }
        else
        {
            Board_PinHigh(RGB_DATA_PORT, RGB_DATA_PIN);
            ws_delay_cycles(s_t0h);
            Board_PinLow(RGB_DATA_PORT, RGB_DATA_PIN);
            ws_delay_cycles(s_t0l);
        }
        dat <<= 1U;
    }
}

void WS2812_Show(void)
{
    uint16_t i;

    /* 传输期间关中断，避免 SysTick 打断造成颜色错乱 */
    __disable_irq();

    Board_PinLow(RGB_DATA_PORT, RGB_DATA_PIN);
    ws_delay_cycles(ns_to_cycles(80000U));      /* 80us 复位 */

    for (i = 0; i < RGB_LED_COUNT; i++)
    {
        ws_send_byte(s_led[i][0]);              /* G */
        ws_send_byte(s_led[i][1]);              /* R */
        ws_send_byte(s_led[i][2]);              /* B */
    }

    Board_PinLow(RGB_DATA_PORT, RGB_DATA_PIN);
    __enable_irq();

    ws_delay_cycles(ns_to_cycles(RESET_NS));    /* 锁存 */
}

void WS2812_ColorWheel(uint16_t index, uint16_t phase,
                       uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t pos = (uint8_t)((index * 8U + phase) & 0xFFU);

    if (pos < 85U)
    {
        *r = (uint8_t)(255U - pos * 3U);
        *g = (uint8_t)(pos * 3U);
        *b = 0U;
    }
    else if (pos < 170U)
    {
        pos = (uint8_t)(pos - 85U);
        *r = 0U;
        *g = (uint8_t)(255U - pos * 3U);
        *b = (uint8_t)(pos * 3U);
    }
    else
    {
        pos = (uint8_t)(pos - 170U);
        *r = (uint8_t)(pos * 3U);
        *g = 0U;
        *b = (uint8_t)(255U - pos * 3U);
    }
}

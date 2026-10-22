#include "app.h"

#include "audio.h"
#include "board.h"
#include "keypad.h"
#include "power.h"
#include "ws2812.h"

#define APP_POLL_MS     10U

/*
 * 开机：
 *   - PG1 保持高电平（电源自锁）
 *   - 30 颗 WS2812 全亮白色
 * 按键：
 *   - 按第 K 键 -> 第 K 颗灯翻转（亮/灭）
 * 关机：
 *   - 长按电源键 2 秒 -> 全灭、拉低 PG1，停在关机循环
 */

static uint8_t  s_led_state[RGB_LED_COUNT];   /* 1 = 亮, 0 = 灭 */
static uint32_t s_last_poll;

/* 排查用：记录 PB12/PB13/PB15 拉高后悬空掉到低电平所需的 DWT 周期数 */
volatile uint32_t g_pin_decay[4];

static uint32_t decay_test(uint16_t pin)
{
    uint32_t crh = GPIOB->CRH;
    uint32_t shift = (uint32_t)(pin - 8U) * 4U;
    uint32_t t0;

    /* 推挽输出，拉高 1ms */
    GPIOB->CRH = (crh & ~(0xFU << shift)) | (0x3U << shift);
    GPIOB->BSRR = pin;
    t0 = DWT->CYCCNT;
    while ((DWT->CYCCNT - t0) < 72000U)
    {
    }

    /* 切成高阻输入，测量掉到低电平的时间 */
    GPIOB->CRH = (crh & ~(0xFU << shift)) | (0x4U << shift);
    t0 = DWT->CYCCNT;

    while ((GPIOB->IDR & pin) != 0U)
    {
        if ((DWT->CYCCNT - t0) > 720000U)   /* 10ms 超时 */
        {
            return 0xFFFFFFFFU;
        }
    }

    return DWT->CYCCNT - t0;
}

static void pin_decay_probe(void)
{
    g_pin_decay[0] = decay_test(GPIO_PIN_12);
    g_pin_decay[1] = decay_test(GPIO_PIN_13);
    g_pin_decay[2] = decay_test(GPIO_PIN_15);   /* 悬空对照 */
    g_pin_decay[3] = decay_test(GPIO_PIN_12);
}

static void led_apply(void)
{
    uint16_t i;

    for (i = 0; i < RGB_LED_COUNT; i++)
    {
        if (s_led_state[i] != 0U)
        {
            WS2812_SetPixel(i, 255U, 255U, 255U);
        }
        else
        {
            WS2812_SetPixel(i, 0U, 0U, 0U);
        }
    }
    WS2812_Show();
}

void APP_Init(void)
{
    uint16_t i;

    Board_Init();
    pin_decay_probe();
    Power_Init();       /* 先锁存电源，再做外设初始化 */
    WS2812_Init();
    Keypad_Init();

    /* 开机默认 30 颗全亮 */
    for (i = 0; i < RGB_LED_COUNT; i++)
    {
        s_led_state[i] = 1U;
    }
    led_apply();

    /*
     * 开机播放第 0 段。WT588F 的语音工程可能是单字节，也可能是双字节
     * （厂家默认双字节），这里两种格式都发一遍，互不冲突。
     */
    Audio_Init();
    Audio_SendCommand16(0xFFEFU);   /* 双字节：最大音量 */
    Audio_SendCommand16(0x0000U);   /* 双字节：播放第 0 段 */
    Audio_SetVolume(0xEFU);         /* 单字节：最大音量 */
    Audio_SendCommand(0x00U);       /* 单字节：播放第 0 段 */

    s_last_poll = HAL_GetTick();
}

void APP_Run(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t rising;
    uint16_t i;

    if ((now - s_last_poll) < APP_POLL_MS)
    {
        return;
    }
    s_last_poll = now;

    /* 长按 2 秒：全灭，拉低 PG1；松开按键后电源断开 */
    if (Power_Poll() != 0U)
    {
        WS2812_Clear();
        WS2812_Show();
        Power_Release();

        while (1)
        {
        }
    }

    /* 按键矩阵：新按下的键 -> 对应灯翻转 */
    if (Keypad_Scan() != 0U)
    {
        rising = Keypad_GetRisingMask();

        for (i = 0; i < RGB_LED_COUNT; i++)
        {
            if (rising & (1UL << i))
            {
                s_led_state[i] ^= 1U;
            }
        }

        led_apply();
    }
}

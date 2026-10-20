#include "app.h"

#include "board.h"
#include "power.h"
#include "ws2812.h"

#define APP_POLL_MS     10U

static uint32_t s_last_poll;
static uint8_t  s_led_pressed = 0xFFU;   /* 0xFF = 尚未刷新 */

/*
 * LED 指示：
 *   正常       -> 第一颗白色，其余熄灭
 *   电源键按下 -> 第一颗红色（便于确认 PG2 检测正常）
 *   关机       -> 全部熄灭
 */
static void led_update(void)
{
    uint8_t pressed;

    /* 开机那一下先保持白色，松开并完成武装后才显示按键状态 */
    pressed = (Power_IsArmed() != 0U) ? Power_KeyIsDown() : 0U;

    if (pressed == s_led_pressed)
    {
        return;
    }
    s_led_pressed = pressed;

    WS2812_Clear();
    if (pressed != 0U)
    {
        WS2812_SetPixel(0U, 255U, 0U, 0U);
    }
    else
    {
        WS2812_SetPixel(0U, 255U, 255U, 255U);
    }
    WS2812_Show();
}

void APP_Init(void)
{
    Board_Init();
    Power_Init();       /* 先锁存电源，再做外设初始化 */

    WS2812_Init();
    s_led_pressed = 0xFFU;
    led_update();       /* 开机只亮第一颗白色 LED */

    s_last_poll = HAL_GetTick();
}

void APP_Run(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - s_last_poll) < APP_POLL_MS)
    {
        return;
    }
    s_last_poll = now;

    led_update();

    if (Power_Poll() != 0U)
    {
        /* 长按 2 秒：全灭，拉低 PG1；松开按键后电源断开 */
        WS2812_Clear();
        WS2812_Show();
        Power_Release();

        while (1)
        {
        }
    }
}

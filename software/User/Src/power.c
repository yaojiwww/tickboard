#include "power.h"

#define PWR_DEBOUNCE_MS     20U

static uint8_t  s_raw;              /* 原始电平，1 = 按下 */
static uint8_t  s_stable;           /* 消抖后的状态 */
static uint32_t s_raw_changed_at;   /* 原始电平最后一次变化的时间 */
static uint32_t s_pressed_at;       /* 消抖后按下的时间 */
static uint8_t  s_armed;            /* 开机那次按下松开后才允许长按关机 */
static uint8_t  s_fired;            /* 长按事件只触发一次 */

void Power_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PG1：先写入高电平，再切为推挽输出，避免切换瞬间出现低电平 */
    HAL_GPIO_WritePin(PWR_HOLD_PORT, PWR_HOLD_PIN, GPIO_PIN_SET);
    gpio.Pin   = PWR_HOLD_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PWR_HOLD_PORT, &gpio);
    HAL_GPIO_WritePin(PWR_HOLD_PORT, PWR_HOLD_PIN, GPIO_PIN_SET);

    /* PG2：按键输入，外部 R3 10k 上拉，按下接地 */
    gpio.Pin  = PWR_KEY_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(PWR_KEY_PORT, &gpio);

    s_raw            = Power_KeyIsDown();
    s_stable         = s_raw;
    s_raw_changed_at = HAL_GetTick();
    s_pressed_at     = s_raw_changed_at;
    /* 开机时如果按键仍按着，必须先松开一次，避免“按住开机”被当成 2 秒长按 */
    s_armed          = (uint8_t)(s_raw == 0U);
    s_fired          = 0U;
}

void Power_Hold(void)
{
    HAL_GPIO_WritePin(PWR_HOLD_PORT, PWR_HOLD_PIN, GPIO_PIN_SET);
}

void Power_Release(void)
{
    HAL_GPIO_WritePin(PWR_HOLD_PORT, PWR_HOLD_PIN, GPIO_PIN_RESET);
}

uint8_t Power_KeyIsDown(void)
{
    return (HAL_GPIO_ReadPin(PWR_KEY_PORT, PWR_KEY_PIN) == GPIO_PIN_RESET)
               ? 1U : 0U;
}

uint8_t Power_Poll(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t  down = Power_KeyIsDown();

    if (s_fired != 0U)
    {
        return 0U;
    }

    if (down != s_raw)
    {
        s_raw = down;
        s_raw_changed_at = now;
    }
    else if (((now - s_raw_changed_at) >= PWR_DEBOUNCE_MS) &&
             (s_stable != s_raw))
    {
        s_stable = s_raw;

        if (s_stable != 0U)
        {
            s_pressed_at = now;
        }
        else
        {
            /* 开机那一下松开后，后面的按下才是真正的“关机长按” */
            s_armed = 1U;
        }
    }

    if ((s_armed != 0U) && (s_stable != 0U) &&
        ((now - s_pressed_at) >= PWR_LONG_PRESS_MS))
    {
        s_fired = 1U;
        return 1U;
    }

    return 0U;
}

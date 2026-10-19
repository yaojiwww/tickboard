#include "encoder.h"

/* 正交解码表：索引 = (上一次状态 << 2) | 当前状态，状态 = (A << 1) | B */
static const int8_t s_quad_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static uint8_t  s_prev_state;
static int32_t  s_position;
static uint8_t  s_sw_raw;
static uint8_t  s_sw_stable;
static uint8_t  s_sw_count;
static uint8_t  s_press_event;

void Encoder_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* A/B/按键均为输入上拉，按下接地为低电平 */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;

    gpio.Pin = ENC_A_PIN | ENC_B_PIN;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = ENC_SW_PIN;
    HAL_GPIO_Init(GPIOA, &gpio);

    s_prev_state = (uint8_t)((Board_PinRead(ENC_A_PORT, ENC_A_PIN) << 1) |
                              Board_PinRead(ENC_B_PORT, ENC_B_PIN));
    s_position   = 0;
    s_sw_raw     = 0U;
    s_sw_stable  = 0U;
    s_sw_count   = 0U;
    s_press_event = 0U;
}

void Encoder_Poll(void)
{
    uint8_t state = (uint8_t)((Board_PinRead(ENC_A_PORT, ENC_A_PIN) << 1) |
                               Board_PinRead(ENC_B_PORT, ENC_B_PIN));
    uint8_t idx = (uint8_t)((s_prev_state << 2) | state);

    s_prev_state = state;
    s_position += s_quad_table[idx];

    /* 按键消抖 + 下降沿事件 */
    {
        uint8_t pressed = (uint8_t)(!Board_PinRead(ENC_SW_PORT, ENC_SW_PIN));

        if (pressed != s_sw_raw)
        {
            s_sw_raw = pressed;
            s_sw_count = 0U;
        }
        else if (s_sw_count < 5U)
        {
            s_sw_count++;
        }

        if ((s_sw_count >= 4U) && (s_sw_stable != s_sw_raw))
        {
            s_sw_stable = s_sw_raw;
            if (s_sw_stable != 0U)
            {
                s_press_event = 1U;
            }
        }
    }
}

int32_t Encoder_GetPosition(void)
{
    return s_position;
}

int32_t Encoder_GetDetents(void)
{
    return s_position / 4;
}

uint8_t Encoder_SwitchIsDown(void)
{
    return s_sw_stable;
}

uint8_t Encoder_TakePressEvent(void)
{
    uint8_t ev = s_press_event;

    s_press_event = 0U;
    return ev;
}

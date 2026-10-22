#include "keypad.h"

/*
 * 3 行 x 10 列矩阵（2026-10-20 原理图）。
 * 行线经按键和 1N4148WS 二极管接到列线，方向 ROW -> COL，因此扫描方式：
 *   当前行输出高电平，其余行输出低电平；列线下拉输入，读到高电平即按下。
 * K1..K30 = ROW1..ROW3 的 COL1..COL10，K(row,col) = row*10 + col + 1，
 * 第 K 键对应第 K 颗 WS2812（数组下标 K-1）。
 */

static GPIO_TypeDef *const s_row_port[KEY_ROW_NUM] = {
    GPIOB, GPIOC, GPIOC
};
static const uint16_t s_row_pin[KEY_ROW_NUM] = {
    GPIO_PIN_11, GPIO_PIN_6, GPIO_PIN_7
};
static GPIO_TypeDef *const s_col_port[KEY_COL_NUM] = {
    GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC,
    GPIOB, GPIOB, GPIOB, GPIOB
};
static const uint16_t s_col_pin[KEY_COL_NUM] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5,
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_10
};

static uint16_t s_candidate[KEY_ROW_NUM];
static uint8_t  s_candidate_cnt[KEY_ROW_NUM];
static uint16_t s_stable[KEY_ROW_NUM];
static uint32_t s_mask;
static uint32_t s_last_mask;
static uint32_t s_rising;

void Keypad_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 行线：推挽输出，默认低 */
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin   = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, gpio.Pin, GPIO_PIN_RESET);

    /* 列线：下拉输入（COL1-6 在 GPIOC，COL7-10 在 GPIOB） */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &gpio);

    s_mask      = 0U;
    s_last_mask = 0U;
    s_rising    = 0U;
}

static void scan_raw(uint16_t *raw)
{
    uint8_t r, c, i;

    for (r = 0; r < KEY_ROW_NUM; r++)
    {
        /* 先全部拉低，再拉高当前行 */
        for (i = 0; i < KEY_ROW_NUM; i++)
        {
            s_row_port[i]->BRR = s_row_pin[i];
        }
        s_row_port[r]->BSRR = s_row_pin[r];

        Board_DelayUs(5U);

        raw[r] = 0U;
        for (c = 0; c < KEY_COL_NUM; c++)
        {
            if (s_col_port[c]->IDR & s_col_pin[c])
            {
                raw[r] |= (uint16_t)(1U << c);
            }
        }
    }

    for (i = 0; i < KEY_ROW_NUM; i++)
    {
        s_row_port[i]->BRR = s_row_pin[i];
    }
}

uint8_t Keypad_Scan(void)
{
    uint16_t raw[KEY_ROW_NUM];
    uint8_t r, c;
    uint8_t changed = 0U;

    scan_raw(raw);

    for (r = 0; r < KEY_ROW_NUM; r++)
    {
        if (raw[r] == s_candidate[r])
        {
            if (s_candidate_cnt[r] < 3U)
            {
                s_candidate_cnt[r]++;
            }
        }
        else
        {
            s_candidate[r] = raw[r];
            s_candidate_cnt[r] = 0U;
        }

        /* 连续 2 次 (20ms) 一致才认为稳定 */
        if ((s_candidate_cnt[r] >= 2U) && (s_stable[r] != s_candidate[r]))
        {
            s_stable[r] = s_candidate[r];
            changed = 1U;
        }
    }

    if (changed)
    {
        s_mask = 0U;
        for (r = 0; r < KEY_ROW_NUM; r++)
        {
            for (c = 0; c < KEY_COL_NUM; c++)
            {
                if (s_stable[r] & (uint16_t)(1U << c))
                {
                    s_mask |= (1UL << (r * KEY_COL_NUM + c));
                }
            }
        }

        s_rising    = s_mask & ~s_last_mask;
        s_last_mask = s_mask;
    }

    return changed;
}

uint32_t Keypad_GetMask(void)
{
    return s_mask;
}

uint32_t Keypad_GetRisingMask(void)
{
    return s_rising;
}

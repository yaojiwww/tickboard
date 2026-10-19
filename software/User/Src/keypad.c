#include "keypad.h"

/*
 * 7 行 x 6 列，行线经按键和 1N4148WS 二极管接到列线。
 * 二极管方向：ROW -> COL，因此扫描方式为：
 *   当前行输出高电平，其余行输出低电平；列线下拉输入，读到高电平即按下。
 * K1..K30 = ROW1..ROW5 的 6 列；K31 = ROW6/COL1；K32 = ROW7/COL1。
 */
static GPIO_TypeDef *const s_row_port[KEY_ROW_NUM] = {
    GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOC, GPIOC
};
static const uint16_t s_row_pin[KEY_ROW_NUM] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_10,
    GPIO_PIN_11, GPIO_PIN_6, GPIO_PIN_7
};
static GPIO_TypeDef *const s_col_port[KEY_COL_NUM] = {
    GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC
};
static const uint16_t s_col_pin[KEY_COL_NUM] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5
};

static uint8_t s_candidate[KEY_ROW_NUM];
static uint8_t s_candidate_cnt[KEY_ROW_NUM];
static uint8_t s_stable[KEY_ROW_NUM];
static uint32_t s_mask;
static uint32_t s_last_mask;
static uint8_t  s_last_key;

void Keypad_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 行线：推挽输出，默认低 */
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, gpio.Pin, GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, gpio.Pin, GPIO_PIN_RESET);

    /* 列线：下拉输入 */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gpio);
}

/* 行/列 -> 键号，未安装的位置返回 0 */
static uint8_t key_number(uint8_t row, uint8_t col)
{
    if (row < 5U)
    {
        return (uint8_t)(row * KEY_COL_NUM + col + 1U);
    }
    if (col == 0U)
    {
        return (row == 5U) ? 31U : 32U;
    }
    return 0U;
}

static void scan_raw(uint8_t *raw)
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
                raw[r] |= (uint8_t)(1U << c);
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
    uint8_t raw[KEY_ROW_NUM];
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
                if (s_stable[r] & (1U << c))
                {
                    uint8_t key = key_number(r, c);
                    if (key != 0U)
                    {
                        s_mask |= (1UL << (key - 1U));
                    }
                }
            }
        }

        /* 记录新按下的键，便于界面显示 */
        {
            uint32_t newly = s_mask & ~s_last_mask;
            if (newly != 0U)
            {
                for (c = 0U; c < 32U; c++)
                {
                    if (newly & (1UL << c))
                    {
                        s_last_key = (uint8_t)(c + 1U);
                        break;
                    }
                }
            }
            s_last_mask = s_mask;
        }
    }

    return changed;
}

uint32_t Keypad_GetMask(void)
{
    return s_mask;
}

uint8_t Keypad_GetPressedCount(void)
{
    uint32_t m = s_mask;
    uint8_t n = 0U;

    while (m != 0U)
    {
        m &= (m - 1U);
        n++;
    }
    return n;
}

uint8_t Keypad_GetLastKey(void)
{
    return s_last_key;
}

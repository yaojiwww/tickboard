#include "audio.h"

/*
 * WT588F04A-8S-A2 两线串口协议：
 *   - CLK/DATA 空闲为高
 *   - 每次发码前 CLK 拉低 5ms 作为起始条件
 *   - 低位在前，CLK 上升沿采样 DATA
 *   - 半周期 300us，命令之间间隔 >= 5ms
 *   - BUSY 为芯片输出（PB14），不要加上拉
 */
#define AUDIO_START_MS      5U
#define AUDIO_HALF_US       300U

void Audio_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* CLK / DATA 推挽输出，初始为高（芯片空闲电平） */
    gpio.Pin   = AUDIO_CLK_PIN | AUDIO_DATA_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    Board_PinHigh(AUDIO_CLK_PORT, AUDIO_CLK_PIN);
    Board_PinHigh(AUDIO_DATA_PORT, AUDIO_DATA_PIN);

    /* BUSY 输入，按数据手册要求不加内部上拉 */
    gpio.Pin  = AUDIO_BUSY_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* 芯片上电初始化时间约 200ms */
    HAL_Delay(250U);
}

/* 两线发送，允许指定 CLK / DATA 引脚（用于排查接反） */
static void audio_send_two_line(uint8_t data, uint16_t clk_pin, uint16_t data_pin)
{
    uint8_t i;

    /* 若 CLK 为低，先拉高 >= 5ms */
    Board_PinHigh(AUDIO_CLK_PORT, clk_pin);
    Board_PinHigh(AUDIO_DATA_PORT, data_pin);
    HAL_Delay(AUDIO_START_MS);

    /* 起始条件：CLK 低 5ms */
    Board_PinLow(AUDIO_CLK_PORT, clk_pin);
    HAL_Delay(AUDIO_START_MS);

    for (i = 0; i < 8U; i++)
    {
        Board_PinLow(AUDIO_CLK_PORT, clk_pin);

        if (data & 0x01U)
        {
            Board_PinHigh(AUDIO_DATA_PORT, data_pin);
        }
        else
        {
            Board_PinLow(AUDIO_DATA_PORT, data_pin);
        }

        Board_DelayUs(AUDIO_HALF_US);
        Board_PinHigh(AUDIO_CLK_PORT, clk_pin);   /* 上升沿锁存 */
        Board_DelayUs(AUDIO_HALF_US);

        data >>= 1U;
    }

    Board_PinHigh(AUDIO_DATA_PORT, data_pin);
    Board_PinHigh(AUDIO_CLK_PORT, clk_pin);
    HAL_Delay(2U);
}

void Audio_SendByte(uint8_t data)
{
    audio_send_two_line(data, AUDIO_CLK_PIN, AUDIO_DATA_PIN);
}

void Audio_SendCommand(uint8_t cmd)
{
    Audio_SendByte(cmd);
    HAL_Delay(5U);      /* 命令间隔 */
}

/* 排查用：CLK/DATA 对调 */
void Audio_SendCommandSwapped(uint8_t cmd)
{
    audio_send_two_line(cmd, AUDIO_DATA_PIN, AUDIO_CLK_PIN);
    HAL_Delay(5U);
}

/*
 * 双字节（16 位）命令：先发高字节，两个字节之间 CLK/DATA 拉高 2ms + CLK 低 5ms。
 * 音量 FFEF = 最大，播放地址 0000 = 第 0 段。
 */
static void audio_send_two_line16(uint16_t data, uint16_t clk_pin, uint16_t data_pin)
{
    uint8_t j;

    /* 双字节协议：先发高字节。交换后按 LSB first 发送即可 */
    data = (uint16_t)((data >> 8) | (data << 8));

    Board_PinHigh(AUDIO_CLK_PORT, clk_pin);
    Board_PinHigh(AUDIO_DATA_PORT, data_pin);
    HAL_Delay(5U);

    Board_PinLow(AUDIO_CLK_PORT, clk_pin);      /* 起始：CLK 低 5ms */
    HAL_Delay(5U);

    for (j = 0U; j < 16U; j++)
    {
        if (j == 8U)
        {
            Board_PinHigh(AUDIO_CLK_PORT, clk_pin);
            Board_PinHigh(AUDIO_DATA_PORT, data_pin);
            HAL_Delay(2U);
            Board_PinLow(AUDIO_CLK_PORT, clk_pin);
            HAL_Delay(5U);
        }

        Board_PinLow(AUDIO_CLK_PORT, clk_pin);

        if (data & 0x0001U)
        {
            Board_PinHigh(AUDIO_DATA_PORT, data_pin);
        }
        else
        {
            Board_PinLow(AUDIO_DATA_PORT, data_pin);
        }

        Board_DelayUs(200U);
        Board_PinHigh(AUDIO_CLK_PORT, clk_pin);
        Board_DelayUs(200U);

        data >>= 1U;
    }

    Board_PinHigh(AUDIO_DATA_PORT, data_pin);
    Board_PinHigh(AUDIO_CLK_PORT, clk_pin);
    HAL_Delay(2U);
}

void Audio_SendCommand16(uint16_t cmd)
{
    audio_send_two_line16(cmd, AUDIO_CLK_PIN, AUDIO_DATA_PIN);
    HAL_Delay(20U);     /* 非播放地址指令前后建议加 20ms */
}

void Audio_SendCommand16Swapped(uint16_t cmd)
{
    audio_send_two_line16(cmd, AUDIO_DATA_PIN, AUDIO_CLK_PIN);
    HAL_Delay(20U);
}

void Audio_SetVolume(uint8_t volume)
{
    if (volume < 0xE0U) { volume = 0xE0U; }
    if (volume > 0xEFU) { volume = 0xEFU; }

    Audio_SendCommand(volume);
}

/*
 * 一线串口测试：芯片若被烧录为一线上位机模式，数据走 DATA1（pin2 = PB12）。
 *   0 码 = 高 200us + 低 600us
 *   1 码 = 高 600us + 低 200us
 *   起始：DATA 先拉高 >=5ms，再拉低 5ms
 */
static void audio_send_one_line_byte(uint8_t data, uint16_t pins)
{
    uint8_t i;

    /* 若 DATA 当前为低，先拉高 >= 5ms */
    Board_PinHigh(AUDIO_CLK_PORT, pins);
    HAL_Delay(5U);

    /* 起始条件：DATA 低 5ms */
    Board_PinLow(AUDIO_CLK_PORT, pins);
    HAL_Delay(5U);

    for (i = 0; i < 8U; i++)
    {
        if (data & 0x01U)
        {
            Board_PinHigh(AUDIO_CLK_PORT, pins);
            Board_DelayUs(600U);
            Board_PinLow(AUDIO_CLK_PORT, pins);
            Board_DelayUs(200U);
        }
        else
        {
            Board_PinHigh(AUDIO_CLK_PORT, pins);
            Board_DelayUs(200U);
            Board_PinLow(AUDIO_CLK_PORT, pins);
            Board_DelayUs(600U);
        }

        data >>= 1U;
    }

    Board_PinHigh(AUDIO_CLK_PORT, pins);
    HAL_Delay(2U);
}

/* 排查用：在指定引脚（PB12/PB13）上按一线协议发送音量 + 播放第 0 段 */
void Audio_TestOneLine(uint16_t pins)
{
    if (pins == 0U)
    {
        return;
    }

    Board_PinHigh(AUDIO_CLK_PORT, pins);
    HAL_Delay(10U);

    audio_send_one_line_byte(0xEFU, pins);    /* 最大音量 */
    HAL_Delay(5U);
    audio_send_one_line_byte(0x00U, pins);    /* 播放第 0 段 */
    HAL_Delay(5U);
}

/* 一线 16 位：先发高字节 */
static void audio_send_one_line_word(uint16_t data, uint16_t pins)
{
    uint8_t j;

    /* 双字节协议：先发高字节 */
    data = (uint16_t)((data >> 8) | (data << 8));

    Board_PinHigh(AUDIO_CLK_PORT, pins);
    HAL_Delay(5U);

    Board_PinLow(AUDIO_CLK_PORT, pins);         /* 起始：DATA 低 5ms */
    HAL_Delay(5U);

    for (j = 0U; j < 16U; j++)
    {
        if (j == 8U)
        {
            Board_PinHigh(AUDIO_CLK_PORT, pins);
            HAL_Delay(2U);
            Board_PinLow(AUDIO_CLK_PORT, pins);
            HAL_Delay(5U);
        }

        if (data & 0x0001U)
        {
            Board_PinHigh(AUDIO_CLK_PORT, pins);
            Board_DelayUs(600U);
            Board_PinLow(AUDIO_CLK_PORT, pins);
            Board_DelayUs(200U);
        }
        else
        {
            Board_PinHigh(AUDIO_CLK_PORT, pins);
            Board_DelayUs(200U);
            Board_PinLow(AUDIO_CLK_PORT, pins);
            Board_DelayUs(600U);
        }

        data >>= 1U;
    }

    Board_PinHigh(AUDIO_CLK_PORT, pins);
    HAL_Delay(2U);
}

void Audio_TestOneLine16(uint16_t pins, uint16_t addr)
{
    if (pins == 0U)
    {
        return;
    }

    HAL_Delay(20U);
    audio_send_one_line_word(0xFFEFU, pins);    /* 最大音量 */
    HAL_Delay(20U);
    audio_send_one_line_word(addr, pins);       /* 播放指定段 */
    HAL_Delay(20U);
}

void Audio_Play16(uint16_t addr)
{
    Audio_SendCommand16(0xFFEFU);
    Audio_SendCommand16(addr);
}

void Audio_Play16Swapped(uint16_t addr)
{
    Audio_SendCommand16Swapped(0xFFEFU);
    Audio_SendCommand16Swapped(addr);
}

/* 排查用：模拟按键触发，把指定引脚拉低 2 秒（兼容电平触发） */
void Audio_TriggerPulse(uint16_t pin)
{
    Board_PinLow(AUDIO_CLK_PORT, pin);
    HAL_Delay(2000U);
    Board_PinHigh(AUDIO_CLK_PORT, pin);
    HAL_Delay(50U);
}

uint8_t Audio_IsBusy(void)
{
    return Board_PinRead(AUDIO_BUSY_PORT, AUDIO_BUSY_PIN);
}

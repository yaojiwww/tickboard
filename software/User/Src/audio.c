#include "audio.h"

/*
 * WT588F04A-8S-A2 两线串口协议：
 *   - CLK/DATA 空闲为高
 *   - 每次发码前 CLK 拉低 5ms 作为起始条件
 *   - 低位在前，CLK 上升沿采样 DATA
 *   - 半周期 300us，命令之间间隔 >= 5ms
 *   - BUSY 为芯片输出（原理图 PB14），不要加上拉
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

    /* 芯片上电初始化时间 */
    HAL_Delay(150U);
}

void Audio_SendByte(uint8_t data)
{
    uint8_t i;

    /* 若 CLK 为低，先拉高 >= 5ms */
    Board_PinHigh(AUDIO_CLK_PORT, AUDIO_CLK_PIN);
    Board_PinHigh(AUDIO_DATA_PORT, AUDIO_DATA_PIN);
    HAL_Delay(AUDIO_START_MS);

    /* 起始条件：CLK 低 5ms */
    Board_PinLow(AUDIO_CLK_PORT, AUDIO_CLK_PIN);
    HAL_Delay(AUDIO_START_MS);

    for (i = 0; i < 8U; i++)
    {
        Board_PinLow(AUDIO_CLK_PORT, AUDIO_CLK_PIN);

        if (data & 0x01U)
        {
            Board_PinHigh(AUDIO_DATA_PORT, AUDIO_DATA_PIN);
        }
        else
        {
            Board_PinLow(AUDIO_DATA_PORT, AUDIO_DATA_PIN);
        }

        Board_DelayUs(AUDIO_HALF_US);
        Board_PinHigh(AUDIO_CLK_PORT, AUDIO_CLK_PIN);   /* 上升沿锁存 */
        Board_DelayUs(AUDIO_HALF_US);

        data >>= 1U;
    }

    Board_PinHigh(AUDIO_DATA_PORT, AUDIO_DATA_PIN);
    Board_PinHigh(AUDIO_CLK_PORT, AUDIO_CLK_PIN);
    HAL_Delay(2U);
}

void Audio_SendCommand(uint8_t cmd)
{
    Audio_SendByte(cmd);
    HAL_Delay(5U);      /* 命令间隔 */
}

void Audio_SetVolume(uint8_t volume)
{
    if (volume < 0xE0U) { volume = 0xE0U; }
    if (volume > 0xEFU) { volume = 0xEFU; }

    Audio_SendCommand(volume);
}

uint8_t Audio_IsBusy(void)
{
    return Board_PinRead(AUDIO_BUSY_PORT, AUDIO_BUSY_PIN);
}

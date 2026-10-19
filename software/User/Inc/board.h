#ifndef __BOARD_H
#define __BOARD_H

#include "main.h"

/*
 * tickboard V1.0 引脚定义（依据 2026-09-14 原理图）
 * MCU: STM32F103RCT6
 */

/* WS2812 灯链：32 颗，数据经 74HCT1G125 缓冲到 LED DIN */
#define RGB_DATA_PORT       GPIOA
#define RGB_DATA_PIN        GPIO_PIN_1
#define RGB_LED_COUNT       32U

/* 按键矩阵 */
#define KEY_ROW_NUM         7U
#define KEY_COL_NUM         6U

/* EC11 编码器（A/B 外部 2.7k 上拉，公共端接地，低电平有效） */
#define ENC_A_PORT          GPIOB
#define ENC_A_PIN           GPIO_PIN_6
#define ENC_B_PORT          GPIOB
#define ENC_B_PIN           GPIO_PIN_7
#define ENC_SW_PORT         GPIOA
#define ENC_SW_PIN          GPIO_PIN_0

/* WT588F04A-8S-A2 语音芯片（两线串口模式） */
#define AUDIO_BUSY_PORT     GPIOB
#define AUDIO_BUSY_PIN      GPIO_PIN_14
#define AUDIO_DATA_PORT     GPIOB
#define AUDIO_DATA_PIN      GPIO_PIN_13
#define AUDIO_CLK_PORT      GPIOB
#define AUDIO_CLK_PIN       GPIO_PIN_12

/* 1 = HSE 16MHz 晶振，0 = HSI/2 x16 内部时钟 */
extern uint8_t g_clock_hse_ok;

void Board_Init(void);
void Board_DelayUs(uint32_t us);

/* 强制内联：WS2812 / 语音芯片时序对指令开销敏感，-O0 下也不能变成函数调用 */
static inline __attribute__((always_inline)) void Board_PinHigh(GPIO_TypeDef *port, uint16_t pin)
{
    port->BSRR = pin;
}

static inline __attribute__((always_inline)) void Board_PinLow(GPIO_TypeDef *port, uint16_t pin)
{
    port->BRR = pin;
}

static inline __attribute__((always_inline)) uint8_t Board_PinRead(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->IDR & pin) ? 1U : 0U;
}

#endif /* __BOARD_H */

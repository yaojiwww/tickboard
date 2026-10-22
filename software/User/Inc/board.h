#ifndef __BOARD_H
#define __BOARD_H

#include "main.h"

/*
 * tickboard V1.0 引脚定义（依据 2026-10-20 原理图）
 * MCU: STM32F103RCT6
 */

/* WS2812 灯链：30 颗（LED1-LED30），数据经 74HCT1G125 缓冲 */
#define RGB_DATA_PORT       GPIOA
#define RGB_DATA_PIN        GPIO_PIN_1
#define RGB_LED_COUNT       30U

/* 按键矩阵：3 行 x 10 列，K1..K30 对应第 1..30 颗灯 */
#define KEY_ROW_NUM         3U
#define KEY_COL_NUM         10U

/* WT588F04A-8S-A2 语音芯片（两线串口） */
#define AUDIO_BUSY_PORT     GPIOB
#define AUDIO_BUSY_PIN      GPIO_PIN_14
#define AUDIO_DATA_PORT     GPIOB
#define AUDIO_DATA_PIN      GPIO_PIN_13
#define AUDIO_CLK_PORT      GPIOB
#define AUDIO_CLK_PIN       GPIO_PIN_12

/* 1 = HSE 8MHz 晶振可用，0 = 使用 HSI/2 x16 内部时钟 */
extern uint8_t g_clock_hse_ok;

void Board_Init(void);
void Board_DelayUs(uint32_t us);

/* 强制内联：WS2812 位时序对指令开销敏感，-O0 下也不能变成函数调用 */
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

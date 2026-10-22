#ifndef __POWER_H
#define __POWER_H

#include "main.h"

/*
 * tickboard 2026-10-20 硬件电源控制：
 *   PG1 (PC8) 高电平 -> Q3(S8050) 导通 -> Q1(AO3401A) 栅极拉低 -> 维持供电
 *   PG2 (PC9) 电源按键，外部 10k 上拉，按下为低电平
 */

#define PWR_HOLD_PORT       GPIOC
#define PWR_HOLD_PIN        GPIO_PIN_8
#define PWR_KEY_PORT        GPIOC
#define PWR_KEY_PIN         GPIO_PIN_9

#define PWR_LONG_PRESS_MS   2000U

void Power_Init(void);

/* 拉高 / 拉低 PG1，控制电源自锁 */
void Power_Hold(void);
void Power_Release(void);

/* 电源按键当前是否按下（低电平有效） */
uint8_t Power_KeyIsDown(void);

/*
 * 每 10ms 调用一次。
 * 返回 1 表示长按达到 PWR_LONG_PRESS_MS，可以执行关机。
 * 上电时如果按键仍按着，必须先松开一次，之后再次按下才开始计时，
 * 避免“按住电源键开机”被误判成长按关机。
 */
uint8_t Power_Poll(void);

#endif /* __POWER_H */

#ifndef __KEYPAD_H
#define __KEYPAD_H

#include "board.h"

void Keypad_Init(void);

/* 每 10ms 调用一次，返回 1 表示按键状态有变化 */
uint8_t Keypad_Scan(void);

/* 当前按下的键位图：bit0 = K1 ... bit29 = K30 */
uint32_t Keypad_GetMask(void);

/* 本次扫描中新按下的键位图（按下沿），用于边沿触发动作 */
uint32_t Keypad_GetRisingMask(void);

#endif /* __KEYPAD_H */

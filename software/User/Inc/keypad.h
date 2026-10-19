#ifndef __KEYPAD_H
#define __KEYPAD_H

#include "board.h"

void Keypad_Init(void);
uint8_t Keypad_Scan(void);          /* 返回 1 表示按键状态有变化 */
uint32_t Keypad_GetMask(void);      /* bit0 = K1 ... bit31 = K32 */
uint8_t Keypad_GetPressedCount(void);
uint8_t Keypad_GetLastKey(void);    /* 最近按下的键号 1..32，0 表示无 */

#endif /* __KEYPAD_H */

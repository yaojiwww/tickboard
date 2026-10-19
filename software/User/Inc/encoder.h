#ifndef __ENCODER_H
#define __ENCODER_H

#include "board.h"

void Encoder_Init(void);
void Encoder_Poll(void);            /* 建议 1~2ms 调用一次 */

int32_t Encoder_GetPosition(void);  /* 正交计数，4 个计数 = 1 格 */
int32_t Encoder_GetDetents(void);   /* 格数 */
uint8_t Encoder_SwitchIsDown(void);
uint8_t Encoder_TakePressEvent(void);

#endif /* __ENCODER_H */

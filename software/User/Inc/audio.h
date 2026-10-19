#ifndef __AUDIO_H
#define __AUDIO_H

#include "board.h"

void Audio_Init(void);
void Audio_SendByte(uint8_t data);
void Audio_SendCommand(uint8_t cmd);
void Audio_SetVolume(uint8_t volume);   /* 0xE0 最小 ~ 0xEF 最大 */
uint8_t Audio_IsBusy(void);

#endif /* __AUDIO_H */

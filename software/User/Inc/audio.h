#ifndef __AUDIO_H
#define __AUDIO_H

#include "board.h"

/*
 * WT588F04A-8S-A2 两线串口：
 *   发送 0x00-0xDF 播放对应语音段
 *   发送 0xE0-0xEF 设置音量（E0 最小，EF 最大）
 *   发送 0xFE     停止播放
 */
void Audio_Init(void);
void Audio_SendByte(uint8_t data);
void Audio_SendCommand(uint8_t cmd);
void Audio_SetVolume(uint8_t volume);
void Audio_SendCommandSwapped(uint8_t cmd);
void Audio_SendCommand16(uint16_t cmd);
void Audio_SendCommand16Swapped(uint16_t cmd);
void Audio_TestOneLine(uint16_t pins);
void Audio_TestOneLine16(uint16_t pins, uint16_t addr);
void Audio_Play16(uint16_t addr);
void Audio_Play16Swapped(uint16_t addr);
void Audio_TriggerPulse(uint16_t pin);
uint8_t Audio_IsBusy(void);

#endif /* __AUDIO_H */

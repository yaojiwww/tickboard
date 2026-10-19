#ifndef __WS2812_H
#define __WS2812_H

#include "board.h"

void WS2812_Init(void);
void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_Fill(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Clear(void);
void WS2812_Show(void);
void WS2812_ColorWheel(uint16_t index, uint16_t phase,
                       uint8_t *r, uint8_t *g, uint8_t *b);

#endif /* __WS2812_H */

#ifndef __SPI_H
#define __SPI_H

#include "main.h"

/*
 * tickboard LCD 4 线 SPI 引脚（与厂商例程不同，按实际 PCB 修改）
 * LCD: ST7789 2.25" 284x76
 *   背光 LEDA 经 200R 直接接 +5V，无 BLK 控制引脚
 */
#define LCD_SCK_PORT        GPIOA
#define LCD_SCK_PIN         GPIO_PIN_5
#define LCD_MOSI_PORT       GPIOA
#define LCD_MOSI_PIN        GPIO_PIN_7
#define LCD_RES_PORT        GPIOA
#define LCD_RES_PIN         GPIO_PIN_2
#define LCD_DC_PORT         GPIOA
#define LCD_DC_PIN          GPIO_PIN_3
#define LCD_CS_PORT         GPIOA
#define LCD_CS_PIN          GPIO_PIN_4

/* 直接写 BSRR/BRR，避免 HAL_GPIO_WritePin 的函数调用开销 */
#define LCD_SCK_Set()       (LCD_SCK_PORT->BSRR = LCD_SCK_PIN)
#define LCD_SCK_Clr()       (LCD_SCK_PORT->BRR  = LCD_SCK_PIN)
#define LCD_MOSI_Set()      (LCD_MOSI_PORT->BSRR = LCD_MOSI_PIN)
#define LCD_MOSI_Clr()      (LCD_MOSI_PORT->BRR  = LCD_MOSI_PIN)
#define LCD_RES_Set()       (LCD_RES_PORT->BSRR = LCD_RES_PIN)
#define LCD_RES_Clr()       (LCD_RES_PORT->BRR  = LCD_RES_PIN)
#define LCD_DC_Set()        (LCD_DC_PORT->BSRR = LCD_DC_PIN)
#define LCD_DC_Clr()        (LCD_DC_PORT->BRR  = LCD_DC_PIN)
#define LCD_CS_Set()        (LCD_CS_PORT->BSRR = LCD_CS_PIN)
#define LCD_CS_Clr()        (LCD_CS_PORT->BRR  = LCD_CS_PIN)

/* 背光在板上常亮，保留空宏以兼容厂商 lcd_init.c */
#define LCD_BLK_CLK_ENABLE()    ((void)0)
#define LCD_BLK_Set()           ((void)0)
#define LCD_BLK_Clr()           ((void)0)

void LCD_GPIOInit(void);
void LCD_WR_Bus(uint8_t dat);
void LCD_WR_REG(uint8_t reg);
void LCD_WR_DATA8(uint8_t dat);
void LCD_WR_DATA(uint16_t dat);

#endif /* __SPI_H */

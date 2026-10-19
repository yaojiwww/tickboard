#include "spi.h"

/* LCD 引脚初始化：SCK/MOSI/CS/DC/RST 均为推挽输出 */
void LCD_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Pin   = LCD_SCK_PIN | LCD_MOSI_PIN | LCD_RES_PIN |
                               LCD_DC_PIN  | LCD_CS_PIN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    LCD_CS_Set();
    LCD_DC_Set();
    LCD_SCK_Clr();
    LCD_MOSI_Clr();
}

/* 软件 SPI 发送一个字节，高位在前，上升沿采样 */
void LCD_WR_Bus(uint8_t dat)
{
    uint8_t i;

    LCD_CS_Clr();
    for (i = 0; i < 8U; i++)
    {
        if (dat & 0x80U)
        {
            LCD_MOSI_Set();
        }
        else
        {
            LCD_MOSI_Clr();
        }

        LCD_SCK_Set();
        dat <<= 1U;
        LCD_SCK_Clr();
    }
    LCD_CS_Set();
}

/* 写命令 */
void LCD_WR_REG(uint8_t reg)
{
    LCD_DC_Clr();
    LCD_WR_Bus(reg);
    LCD_DC_Set();
}

/* 写 8 位数据 */
void LCD_WR_DATA8(uint8_t dat)
{
    LCD_DC_Set();
    LCD_WR_Bus(dat);
    LCD_DC_Set();
}

/* 写 16 位数据（RGB565） */
void LCD_WR_DATA(uint16_t dat)
{
    LCD_DC_Set();
    LCD_WR_Bus((uint8_t)(dat >> 8));
    LCD_WR_Bus((uint8_t)(dat & 0xFFU));
    LCD_DC_Set();
}

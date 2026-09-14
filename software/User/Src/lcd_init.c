#include "lcd_init.h"
#include "stdio.h"

/**
 * @brief       设置光标位置
 * @param       x:坐标列地址
 * @param       y:坐标行地址
 * @retval      无
 */
void LCD_SetCursor(uint16_t x, uint16_t y)
{
    LCD_Address_Set(x, y, x, y);
}

/**
 * @brief       设置显示窗口
 * @param       xs:窗口列起始地址
 * @param       ys:坐标行起始地址
 * @param       xe:窗口列结束地址
 * @param       ye:坐标行结束地址
 * @retval      无
 */
void LCD_Address_Set(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    if(USE_HORIZONTAL==0)
    {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(xs+0x52);
        LCD_WR_DATA(xe+0x52);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(ys+0x12);
        LCD_WR_DATA(ye+0x12);
        LCD_WR_REG(0x2c);//储存器写
    }
    else if(USE_HORIZONTAL==1)
    {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(xs+0x52);
        LCD_WR_DATA(xe+0x52);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(ys+0x12);
        LCD_WR_DATA(ye+0x12);
        LCD_WR_REG(0x2c);//储存器写
    }
    else if(USE_HORIZONTAL==2)
    {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(xs+0x12);
        LCD_WR_DATA(xe+0x12);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(ys+0x52);
        LCD_WR_DATA(ye+0x52);
        LCD_WR_REG(0x2c);//储存器写
    }
    else
    {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(xs+0x12);
        LCD_WR_DATA(xe+0x12);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(ys+0x52);
        LCD_WR_DATA(ye+0x52);
        LCD_WR_REG(0x2c);//储存器写
    }
}

/**
 * @brief       指定颜色填充区域
 * @param       xs:填充区域列起始地址
 * @param       ys:填充区域行起始地址
 * @param       xe:填充区域列结束地址
 * @param       ye:填充区域行结束地址
 * @param       color:填充颜色值
 * @retval      无
 */
void LCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color)
{
    uint16_t i, j;
    LCD_Address_Set(xs, ys, xe - 1, ye - 1);
    for (j = ys; j < ye; j++)
    {
        for (i = xs; i < xe; i++)
        {
            LCD_WR_DATA(color);
        }
    }
}

/**
 * @brief       初始化LCD
 * @param       无
 * @retval      无
 */
void LCD_Init(void)
{
    LCD_GPIOInit();
    LCD_RES_Set();
    HAL_Delay(20);
    LCD_RES_Clr();
    HAL_Delay(200);
    LCD_RES_Set();
    HAL_Delay(120);
    LCD_BLK_Set();

    LCD_WR_REG(0x11);
    HAL_Delay(120);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x33);

    LCD_WR_REG(0xB0);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0xE0);

    LCD_WR_REG(0x36);
    if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
    else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
    else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x70);
    else LCD_WR_DATA8(0xA0);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x05);

    LCD_WR_REG(0xB7);
    LCD_WR_DATA8(0x45);

    LCD_WR_REG(0xBB);
    LCD_WR_DATA8(0x1D);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x2C);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x01);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x19);

    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x20);

    LCD_WR_REG(0xC6);
    LCD_WR_DATA8(0x0F);

    LCD_WR_REG(0xD0);
    LCD_WR_DATA8(0xA4);
    LCD_WR_DATA8(0xA1);

    LCD_WR_REG(0xD6);
    LCD_WR_DATA8(0xA1);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0xD0);
    LCD_WR_DATA8(0x10);
    LCD_WR_DATA8(0x21);
    LCD_WR_DATA8(0x14);
    LCD_WR_DATA8(0x15);
    LCD_WR_DATA8(0x2D);
    LCD_WR_DATA8(0x41);
    LCD_WR_DATA8(0x44);
    LCD_WR_DATA8(0x4F);
    LCD_WR_DATA8(0x28);
    LCD_WR_DATA8(0x0E);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x1D);
    LCD_WR_DATA8(0x1F);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0xD0);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x1B);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x42);
    LCD_WR_DATA8(0x54);
    LCD_WR_DATA8(0x50);
    LCD_WR_DATA8(0x3E);
    LCD_WR_DATA8(0x1A);
    LCD_WR_DATA8(0x18);
    LCD_WR_DATA8(0x22);
    LCD_WR_DATA8(0x25);
    LCD_WR_REG(0x11);
    HAL_Delay(120);
    LCD_WR_REG(0x29);

}

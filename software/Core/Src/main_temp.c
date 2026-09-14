//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//中景园电子
//店铺地址：http://shop73023976.taobao.com
//
//  文 件 名   : main.c
//  版 本 号   : v1.0
//  作    者   : zhaojian
//  生成日期   : 2024-02-17
//  最近修改   : 
//  功能描述   : LCD SPI演示例程(STM32系列)
//               GND  电源地
//               VCC  3.3V电源
//               SCLK PA5
//               MOSI PA7
//               RES  PD2
//               DC   PB5
//               CS   PA4
//              ----------------------------------------------------------------
// 修改历史   :
// 日    期   :
// 作    者   :zhaojian
// 修改内容   :创建文件
//版权所有，盗版必究。
//Copyright(C) 中景园电子2024-02-17
//All rights reserved
//******************************************************************************/

#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "lcd.h"
#include "pic.h"

void SystemClock_Config(void);
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    LCD_Init();
    while(1)
    {
        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        LCD_ShowPicture(86, 0, 105, 56, gImage_1);
        LCD_ShowString(13, 60, "2.25 TFT RESOLUTION:284x76 DRIVER IC:ST7789", RED, WHITE, 12, 0);
        HAL_Delay(1000);
        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        LCD_Fill(0, 0, 37, LCD_H, RED);
        LCD_Fill(37, 0, 72, LCD_H, GREEN);
        LCD_Fill(72, 0, 107, LCD_H, BLUE);
        LCD_Fill(107, 0, 142, LCD_H, YELLOW);
        LCD_Fill(142, 0, 177, LCD_H, WHITE);
        LCD_Fill(177, 0, 209, LCD_H, GRAY);
        LCD_Fill(209, 0, 244, LCD_H, BLACK);
        LCD_Fill(244, 0, 284, LCD_H, MAGENTA);
        HAL_Delay(1000);
        LCD_Fill(0, 0, LCD_W / 2, LCD_H / 2, RED);
        LCD_Fill(LCD_W / 2, 0, LCD_W, LCD_H / 2, GREEN);
        LCD_Fill(0, LCD_H / 2, LCD_W / 2, LCD_H, BLUE);
        LCD_Fill(LCD_W / 2, LCD_H / 2, LCD_W, LCD_H, WHITE);
        HAL_Delay(1000);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

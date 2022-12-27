/*
 * The test project - FreeRTOS + bxCAN
 *
 *  Created on: Dec 21, 2022
 *      Author: Ulad Shumeika
 */

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "main.h"

//---------------------------------------------------------------------------
// Static function prototypes
//---------------------------------------------------------------------------
static void SystemClock_Config(void);

//---------------------------------------------------------------------------
// Main function
//---------------------------------------------------------------------------

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
 	HAL_Init();

	SystemClock_Config();

	// Call init function for freertos objects (in freertos.c)
	MX_FREERTOS_Init();

	// Start scheduler
	osKernelStart();

	while(1)
	{

	}
}

//---------------------------------------------------------------------------
// Others functions
//---------------------------------------------------------------------------

/**
 * @brief printf redirection
 */
int _write(int file, char *ptr, int len)
{
	int DataIdx;

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		//__io_putchar(*ptr++);
		ITM_SendChar(*ptr++);
	}

	return len;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
	__disable_irq();

	while(1)
	{

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

	// Configure the main internal regulator output voltage
 	__HAL_RCC_PWR_CLK_ENABLE();
 	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

 	// Initializes the RCC Oscillators according to the specified parameters in the RCC_OscInitTypeDef structure
 	RCC_OscInitStruct.OscillatorType					= RCC_OSCILLATORTYPE_HSE;
 	RCC_OscInitStruct.HSEState 							= RCC_HSE_ON;
 	RCC_OscInitStruct.PLL.PLLState 						= RCC_PLL_ON;
 	RCC_OscInitStruct.PLL.PLLSource 					= RCC_PLLSOURCE_HSE;
 	RCC_OscInitStruct.PLL.PLLM 							= 4;
 	RCC_OscInitStruct.PLL.PLLN 							= 64;
 	RCC_OscInitStruct.PLL.PLLP 							= RCC_PLLP_DIV2;
 	RCC_OscInitStruct.PLL.PLLQ							= 4;

 	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
 	{
 		Error_Handler();
 	}

 	// Initializes the CPU, AHB and APB buses clocks
 	RCC_ClkInitStruct.ClockType 						= RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
 														  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
 	RCC_ClkInitStruct.SYSCLKSource 						= RCC_SYSCLKSOURCE_PLLCLK;
 	RCC_ClkInitStruct.AHBCLKDivider 					= RCC_SYSCLK_DIV1;
 	RCC_ClkInitStruct.APB1CLKDivider 					= RCC_HCLK_DIV2;
 	RCC_ClkInitStruct.APB2CLKDivider 					= RCC_HCLK_DIV1;

 	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
 	{
 		Error_Handler();
 	}
}

//---------------------------------------------------------------------------
// Callback functions
//---------------------------------------------------------------------------

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM7)
	{
		HAL_IncTick();
	}
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
	printf("Wrong parameters value: file %s on line %ld\r\n", file, line);
}
#endif /* USE_FULL_ASSERT */

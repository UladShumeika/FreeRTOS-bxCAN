//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "UART.h"

//---------------------------------------------------------------------------
// Typedefs
//---------------------------------------------------------------------------
static UART_HandleTypeDef huart1;

//---------------------------------------------------------------------------
// Static function prototypes
//---------------------------------------------------------------------------
static void UART_USART_init(void);

//---------------------------------------------------------------------------
// Descriptions of FreeRTOS elements
//---------------------------------------------------------------------------
static osThreadId UARTSendingMessagesHandle;

//---------------------------------------------------------------------------
// FreeRTOS's threads
//---------------------------------------------------------------------------

/**
* @brief Function implementing the UARTSendingMessages thread.
* @param argument: Not used
* @retval None
*/
void UARTSendingMessagesTask(void const* argument)
{
	UART_USART_init();

	/* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
}

//---------------------------------------------------------------------------
// Initialization functions
//---------------------------------------------------------------------------

/**
  * @brief  USART configuration UART module
  * @param  None
  * @retval None
  */
static void UART_USART_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	// USART1 GPIO Configuration
	// PA9     ------> USART1_TX
	GPIO_InitStruct.Pin 						= GPIO_PIN_9;
	GPIO_InitStruct.Mode 						= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull 						= GPIO_NOPULL;
	GPIO_InitStruct.Speed 						= GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate 					= GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	huart1.Instance 							= USART1;
	huart1.Init.BaudRate 						= 115200;
	huart1.Init.WordLength 						= UART_WORDLENGTH_8B;
	huart1.Init.StopBits 						= UART_STOPBITS_1;
	huart1.Init.Parity 							= UART_PARITY_NONE;
	huart1.Init.Mode 							= UART_MODE_TX;
	huart1.Init.HwFlowCtl 						= UART_HWCONTROL_NONE;
	huart1.Init.OverSampling 					= UART_OVERSAMPLING_16;
	HAL_UART_Init(&huart1);
}

/**
  * @brief  FreeRTOS initialization for UART module
  * @param  None
  * @retval None
  */
void UART_FreeRTOS_init(void)
{
	// Create the thread(s)
	// definition and creation of UARTSendingMessagesTask
	osThreadDef(UARTSending, UARTSendingMessagesTask, osPriorityAboveNormal, 0, 128);
	UARTSendingMessagesHandle = osThreadCreate(osThread(UARTSending), NULL);
}

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "heartbeat.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define PORT_LED_HEARTBEAT		GPIOG
#define PIN_LED_HEARTBEAT		GPIO_PIN_13
#define BLICK_DELAY_HEARTBEAT	(700U)				// ms

//---------------------------------------------------------------------------
// Static function prototypes
//---------------------------------------------------------------------------
static void HEARTBEAT_GPIO_Init(void);

//---------------------------------------------------------------------------
// Descriptions of FreeRTOS elements
//---------------------------------------------------------------------------
static osThreadId HeartbeatHandle;

//---------------------------------------------------------------------------
// FreeRTOS's threads
//---------------------------------------------------------------------------

/**
* @brief Function implementing the heartbeat thread.
* @param argument: Not used
* @retval None
*/
void HeartbeatTask(void const * argument)
{
  HEARTBEAT_GPIO_Init();

  /* Infinite loop */
  for(;;)
  {
	HAL_GPIO_TogglePin(PORT_LED_HEARTBEAT, PIN_LED_HEARTBEAT);
    osDelay(BLICK_DELAY_HEARTBEAT);
  }
}

//---------------------------------------------------------------------------
// Initialization functions
//---------------------------------------------------------------------------

/**
  * @brief  FreeRTOS initialization for heartbeat module
  * @param  None
  * @retval None
  */
void HEARTBEAT_FreeRTOS_init(void)
{
	// Create the thread(s)
	// definition and creation of HeartbeatTask
	osThreadDef(Heartbeat, HeartbeatTask, osPriorityLow, 0, 128);
	HeartbeatHandle = osThreadCreate(osThread(Heartbeat), NULL);
}

/**
  * @brief  GPIO configuration for heartbeat module
  * @param  None
  * @retval None
  */
static void HEARTBEAT_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOG_CLK_ENABLE();

	// Configure GPIO pins : PIN_LED_HEARTBEAT
	GPIO_InitStruct.Pin 			= PIN_LED_HEARTBEAT;
	GPIO_InitStruct.Mode 			= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull 			= GPIO_PULLDOWN;
	GPIO_InitStruct.Speed 			= GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	// Configure GPIO pin Output Level
	HAL_GPIO_WritePin(PORT_LED_HEARTBEAT, PIN_LED_HEARTBEAT, GPIO_PIN_RESET);
}

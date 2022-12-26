//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "UART.h"

//---------------------------------------------------------------------------
// Static function prototypes
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//---------------------------------------------------------------------------
// Functions prototypes
//---------------------------------------------------------------------------
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

// GetIdleTaskMemory prototype (linked to static allocation support)
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
	*ppxIdleTaskStackBuffer = &xIdleStack[0];
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

//---------------------------------------------------------------------------
// Initialization functions
//---------------------------------------------------------------------------

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void)
{

#ifdef HEARTBEAT
	HEARTBEAT_FreeRTOS_init();
#endif

}

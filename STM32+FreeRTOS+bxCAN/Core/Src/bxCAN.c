//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "bxCAN.h"
#include <string.h>

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------

// CAN1 interrupt priorities
#define CAN1_TX_PREEMPPRIORITY					(5U)
#define CAN1_TX_SUBPRIORITY						(0U)

#define CAN1_RX0_PREEMPPRIORITY					(5U)
#define CAN1_RX0_SUBPRIORITY					(0U)

#define CAN1_SCE_PREEMPPRIORITY					(5U)
#define CAN1_SCE_SUBPRIORITY					(0U)

#define AMOUNT_MESSAGES							(20U)

//---------------------------------------------------------------------------
// Typedefs
//---------------------------------------------------------------------------
CAN_HandleTypeDef hcan1;
CAN_TxHeaderTypeDef TxHeader;

//---------------------------------------------------------------------------
// Static function prototypes
//---------------------------------------------------------------------------
static void bxCAN_GPIO_init(void);
static void bxCAN_CAN1_init(void);
static void bxCAN_create_and_add_message(CAN_HandleTypeDef *hcan, const uint32_t* idFrames, const char** messages, uint8_t amountMessages, CAN_TxHeaderTypeDef *pHeader);

//---------------------------------------------------------------------------
// Descriptions of FreeRTOS elements
//---------------------------------------------------------------------------
static osThreadId InterruptHandlingRxFIFO0Handle;
static osThreadId InterruptHandlingErrorHandle;
static osThreadId InterruptHandlingSendHandle;
static osSemaphoreId InterruptRxFIFO0SemHandle;
static osSemaphoreId InterruprtErrorCANSemHandle;
static osSemaphoreId SendingMessagesSemHandle;
extern osMessageQId dataFromCANHandle;
extern osPoolId mpool;

//---------------------------------------------------------------------------
// Variables
//---------------------------------------------------------------------------
uint8_t RxData[DATA_FIELD] = {0,};

const uint32_t idFrames[AMOUNT_MESSAGES] = {0x51F, 0x2B6, 0x9A, 0x7C8, 0x560, 0x622, 0x56E, 0x34E, 0x4B9, 0x42E, 0x38B, 0x1E0,
										   0x3DB, 0x32A, 0x7B9, 0x772, 0x13B, 0x36C, 0x4B5, 0x3D7};
const char* messages[AMOUNT_MESSAGES] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven",
		"twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};

//---------------------------------------------------------------------------
// FreeRTOS's threads
//---------------------------------------------------------------------------

/**
* @brief Function implementing the InterruptHandlingSend thread.
* @param argument: Not used
* @retval None
*/
void InterruptHandlingSendTask(void const* argument)
{
	uint8_t firstStart = 1;

	bxCAN_CAN1_init();

	/* Infinite loop */
	for(;;)
	{
		osSemaphoreWait(SendingMessagesSemHandle, portMAX_DELAY);

		if(firstStart == 1)
		{
			firstStart = 0;
			bxCAN_create_and_add_message(&hcan1, idFrames, messages, AMOUNT_MESSAGES, &TxHeader);
		} else
		{
			bxCAN_create_and_add_message(&hcan1, idFrames, messages, AMOUNT_MESSAGES, &TxHeader);
		}
	}
}

/**
* @brief Function implementing the InterruptHandlingRxFIFO0 thread.
* @param argument: Not used
* @retval None
*/
void InterruptHandlingRxFIFO0Task(void const* argument)
{
	CAN_RxHeaderTypeDef RxHeader;
	bxCAN_message_t *Tmessage;
	uint8_t lengthMessage = 0, previusLengthMessage = 0;

	/* Infinite loop */
	for(;;)
	{
		osSemaphoreWait(InterruptRxFIFO0SemHandle, portMAX_DELAY);

		HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxHeader, RxData);

		Tmessage = osPoolAlloc(mpool);
		Tmessage->id = RxHeader.StdId;
		Tmessage->length = RxHeader.DLC;
		lengthMessage = Tmessage->length;

		for(uint8_t counter = 0; counter < previusLengthMessage; counter++)
		{
			Tmessage->data[counter] = 0;
		}

		for(uint8_t counter = 0; counter < lengthMessage; counter++)
		{
			Tmessage->data[counter] = RxData[counter];
		}

		previusLengthMessage = lengthMessage;

		osMessagePut(dataFromCANHandle, (uint32_t)Tmessage, osWaitForever);

		HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
	}
}

/**
* @brief Function implementing the InterruptHandlingError thread.
* @param argument: Not used
* @retval None
*/
void InterruptHandlingErrorTask(void const* argument)
{
	/* Infinite loop */
	for(;;)
	{
		osSemaphoreWait(InterruprtErrorCANSemHandle, portMAX_DELAY);
		osDelay(1);
	}
}

//---------------------------------------------------------------------------
// Others functions
//---------------------------------------------------------------------------

/**
  * @brief  Create a CAN bus frame. This frame has the following parameters:
  * 		11-bit identifier, data frame, mode transmit global time off
  * @param	hcan pointer to a CAN_HandleTypeDef structure that contains
  * 		the configuration information for the specified CAN.
  * @param	idFrames a pointer to an array with id frames.
  * @param 	messages a pointer to an array with messages for frames.
  * @param	amountMessages amount of messages.
  * @param 	pTxHeader pointer to a CAN_TxHeaderTypeDef structure.
  * @retval None
  */
static void bxCAN_create_and_add_message(CAN_HandleTypeDef* hcan, const uint32_t* idFrames, const char** messages, uint8_t amountMessages, CAN_TxHeaderTypeDef *pHeader)
{
	static uint8_t counter = 0;
	static uint32_t TxMailbox = 0;
	uint32_t lengthMessage = 0;

	pHeader->StdId = idFrames[counter];
	pHeader->ExtId = 0x0000;
	pHeader->IDE = CAN_ID_STD;
	pHeader->RTR = CAN_RTR_DATA;

	lengthMessage = strlen(messages[counter]);
	if(lengthMessage > DATA_FIELD) lengthMessage = DATA_FIELD; // this is a temporary solution since "seventeen" has nine characters
	pHeader->DLC = lengthMessage;

	pHeader->TransmitGlobalTime = DISABLE;

	char message[DATA_FIELD + 2] = {0,}; // +2 added for correct line copying. Since "seventeen" together with \0 has 10 characters

	strcpy(message, &(*messages[counter]));

	counter++;
	if(counter == amountMessages) counter = 0;

	HAL_CAN_AddTxMessage(&(*hcan), &(*pHeader), (uint8_t*)message, &TxMailbox);
}

//---------------------------------------------------------------------------
// Initialization functions
//---------------------------------------------------------------------------

/**
  * @brief  CAN1 configuration for bxCAN module
  * @param  None
  * @retval None
  */
static void bxCAN_CAN1_init(void)
{
	CAN_FilterTypeDef sFilterConfig;

	__HAL_RCC_CAN1_CLK_ENABLE();

	bxCAN_GPIO_init();

	hcan1.Instance 							= CAN1;
	hcan1.Init.Prescaler 					= 20;
	hcan1.Init.Mode 						= CAN_MODE_SILENT_LOOPBACK;
	hcan1.Init.SyncJumpWidth 				= CAN_SJW_1TQ;
	hcan1.Init.TimeSeg1 					= CAN_BS1_13TQ;
	hcan1.Init.TimeSeg2 					= CAN_BS2_2TQ;
	hcan1.Init.TimeTriggeredMode 			= DISABLE;
	hcan1.Init.AutoBusOff 					= ENABLE;
	hcan1.Init.AutoWakeUp 					= DISABLE;
	hcan1.Init.AutoRetransmission 			= ENABLE;
	hcan1.Init.ReceiveFifoLocked 			= DISABLE;
	hcan1.Init.TransmitFifoPriority 		= ENABLE;

	if(HAL_CAN_Init(&hcan1) != HAL_OK)
	{
		Error_Handler();
	}

	sFilterConfig.FilterIdHigh				= 0x0000;
	sFilterConfig.FilterIdLow				= 0x0000;
	sFilterConfig.FilterMaskIdHigh			= 0x0000;
	sFilterConfig.FilterMaskIdLow			= 0x0000;
	sFilterConfig.FilterFIFOAssignment		= CAN_FILTER_FIFO0;
	sFilterConfig.FilterBank				= 0;
	sFilterConfig.FilterMode				= CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale				= CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterActivation			= CAN_FILTER_ENABLE;
	sFilterConfig.SlaveStartFilterBank		= 14;

	if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_CAN_Start(&hcan1);

	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING |
										 CAN_IT_TX_MAILBOX_EMPTY |
										 CAN_IT_ERROR_WARNING |
										 CAN_IT_ERROR_PASSIVE |
										 CAN_IT_BUSOFF |
										 CAN_IT_LAST_ERROR_CODE |
										 CAN_IT_ERROR);
}

/**
  * @brief  GPIO configuration for bxCAN module
  * @param  None
  * @retval None
  */
static void bxCAN_GPIO_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	// CAN1 GPIO Configuration
	// PA11	------> CAN1_RX
	// PA12 ------> CAN1_TX
	GPIO_InitStruct.Pin 				= GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode 				= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull 				= GPIO_NOPULL;
	GPIO_InitStruct.Speed 				= GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate 			= GPIO_AF9_CAN1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// CAN1 interrupt init
	HAL_NVIC_SetPriority(CAN1_TX_IRQn, CAN1_TX_PREEMPPRIORITY, CAN1_TX_SUBPRIORITY);
	HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);

	HAL_NVIC_SetPriority(CAN1_RX0_IRQn, CAN1_RX0_PREEMPPRIORITY, CAN1_RX0_SUBPRIORITY);
	HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);

	HAL_NVIC_SetPriority(CAN1_SCE_IRQn, CAN1_SCE_PREEMPPRIORITY, CAN1_SCE_SUBPRIORITY);
	HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
}

/**
  * @brief  FreeRTOS initialization for bxCAN module
  * @param  None
  * @retval None
  */
void bxCAN_FreeRTOS_init(void)
{
	// Create the thread(s)
	// definition and creation of InterruptHandlingRxFIFO0Task
	osThreadDef(ReceivingMessages, InterruptHandlingRxFIFO0Task, osPriorityNormal, 0, 128);
	InterruptHandlingRxFIFO0Handle = osThreadCreate(osThread(ReceivingMessages), NULL);

	// definition and creation of InterruptHandlingErrorTask
	osThreadDef(ErrorProcessing, InterruptHandlingErrorTask, osPriorityBelowNormal, 0, 128);
	InterruptHandlingErrorHandle = osThreadCreate(osThread(ErrorProcessing), NULL);

	// definition and creation of InterruptHandlingSendTask
	osThreadDef(SendingMessages, InterruptHandlingSendTask, osPriorityBelowNormal, 0, 128);
	InterruptHandlingSendHandle = osThreadCreate(osThread(SendingMessages), NULL);

	// Create the semaphore(s)
	// definition and creation of InterruptRxFIFO0Sem
	osSemaphoreDef(InterruptRxFIFO0Sem);
	InterruptRxFIFO0SemHandle = osSemaphoreCreate(osSemaphore(InterruptRxFIFO0Sem), 1);

	// definition and creation of InterruprtErrorCANSem
	osSemaphoreDef(InterruprtErrorCANSem);
	InterruprtErrorCANSemHandle = osSemaphoreCreate(osSemaphore(InterruprtErrorCANSem), 1);

	// definition and creation of SendingMessagesSem
	osSemaphoreDef(SendingMessagesSem);
	SendingMessagesSemHandle = osSemaphoreCreate(osSemaphore(SendingMessagesSem), 1);

#ifdef DEBUG
	vQueueAddToRegistry(InterruptRxFIFO0SemHandle, "semReceiving");
	vQueueAddToRegistry(SendingMessagesSemHandle, "semSending");
	vQueueAddToRegistry(InterruprtErrorCANSemHandle, "semError");
#endif
}

//---------------------------------------------------------------------------
// Callback functions
//---------------------------------------------------------------------------

/**
  * @brief  Tx Mailbox complete callback
  * @param  hcan: pointer to a CAN_HandleTypeDef structure that contains the configuration information for the
  * 			  specified CAN
  * @retval None
  */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
	osSemaphoreRelease(SendingMessagesSemHandle);
}

/**
  * @brief  Rx FIFO 0 message pending callback
  * @param  hcan: pointer to a CAN_HandleTypeDef structure that contains the configuration information for the
  * 			  specified CAN
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef * hcan)
{
	HAL_CAN_DeactivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
	osSemaphoreRelease(InterruptRxFIFO0SemHandle);
}

/**
  * @brief  Error CAN callback
  * @param  hcan: pointer to a CAN_HandleTypeDef structure that contains the configuration information for the
  * 			  specified CAN
  * @retval None
  */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	osSemaphoreRelease(InterruprtErrorCANSemHandle);
}

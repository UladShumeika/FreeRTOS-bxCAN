//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "bxCAN.h"
#include <string.h>

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------

// Filter configuration -----------------------------------------------------
#define USE_FILTER_MODE1						// from ID1 to ID5 inclusive
#define USE_FILTER_MODE2						// ID6 and ID7
#define USE_FILTER_MODE3						// from ID8 to ID12 inclusive and from ID13 to ID16 inclusive
#define USE_FILTER_MODE4						// ID19 and ID20

#define AMOUNT_MESSAGES						    (20U)

#define	IDENTIFIER_1							0x0000		// zero
#define	IDENTIFIER_2							0x0001		// one
#define	IDENTIFIER_3							0x0002		// two
#define	IDENTIFIER_4							0x0003		// three
#define	IDENTIFIER_5							0x0004		// four
#define	IDENTIFIER_6							0x000B		// five
#define	IDENTIFIER_7							0x000C		// six
#define	IDENTIFIER_8							0x007A		// seven
#define	IDENTIFIER_9							0x007B		// eight
#define	IDENTIFIER_10							0x007C		// nine
#define	IDENTIFIER_11							0x007D		// ten
#define	IDENTIFIER_12							0x007E		// eleven
#define	IDENTIFIER_13							0x07A1		// twelve
#define	IDENTIFIER_14							0x07A2		// thirteen
#define	IDENTIFIER_15							0x07A3		// fourteen
#define	IDENTIFIER_16							0x07A4		// fifteen
#define	IDENTIFIER_17							0x0432		// sixteen
#define	IDENTIFIER_18							0x0543		// seventeen
#define	IDENTIFIER_19							0x0654		// eighteen
#define	IDENTIFIER_20							0x0765		// nineteen

#define FILTER_NUM1								(0U)
#define FILTER_NUM2								(1U)
#define FILTER_NUM3								(2U)
#define FILTER_NUM4								(3U)

// System settings ----------------------------------------------------------
#define QUEUE_SIZE								(5U)
#define ELEMENTS_IN_MEMORY_POOL					(5U)

// CAN1 interrupt priorities
#define CAN1_TX_PREEMPPRIORITY					(5U)
#define CAN1_TX_SUBPRIORITY						(0U)

#define CAN1_RX0_PREEMPPRIORITY					(5U)
#define CAN1_RX0_SUBPRIORITY					(0U)

#define CAN1_SCE_PREEMPPRIORITY					(5U)
#define CAN1_SCE_SUBPRIORITY					(0U)

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
osMessageQId dataFromCANHandle;
osPoolId mpool;

//---------------------------------------------------------------------------
// Variables
//---------------------------------------------------------------------------
const uint32_t idFrames[AMOUNT_MESSAGES] = {IDENTIFIER_1, IDENTIFIER_2, IDENTIFIER_3, IDENTIFIER_4, IDENTIFIER_5, IDENTIFIER_6, IDENTIFIER_7,
											IDENTIFIER_8, IDENTIFIER_9, IDENTIFIER_10, IDENTIFIER_11, IDENTIFIER_12, IDENTIFIER_13,
											IDENTIFIER_14, IDENTIFIER_15, IDENTIFIER_16, IDENTIFIER_17, IDENTIFIER_18, IDENTIFIER_19,
											IDENTIFIER_20};
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
	bxCAN_CAN1_init();

	/* Infinite loop */
	for(;;)
	{
		osSemaphoreWait(SendingMessagesSemHandle, osWaitForever);
		bxCAN_create_and_add_message(&hcan1, idFrames, messages, AMOUNT_MESSAGES, &TxHeader);
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
	uint8_t RxData[DATA_FIELD] = {0,}, lengthMessage = 0, previusLengthMessage = 0;

	/* Infinite loop */
	for(;;)
	{
		osSemaphoreWait(InterruptRxFIFO0SemHandle, osWaitForever);

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
		osSemaphoreWait(InterruprtErrorCANSemHandle, osWaitForever);
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

#ifdef USE_FILTER_MODE1
	sFilterConfig.FilterIdHigh				= (IDENTIFIER_1 << 5); 		// for understanding why the shift is used and why there is such,
	sFilterConfig.FilterIdLow				= 0x0000;					// it's necessary to refer to the appNote RM0090, p. 1088
	sFilterConfig.FilterMaskIdHigh			= (0x07F8 << 5);
	sFilterConfig.FilterMaskIdLow			= 0x0000;
	sFilterConfig.FilterFIFOAssignment		= CAN_FILTER_FIFO0;
	sFilterConfig.FilterBank				= FILTER_NUM1;
	sFilterConfig.FilterMode				= CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale				= CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterActivation			= CAN_FILTER_ENABLE;
	sFilterConfig.SlaveStartFilterBank		= 14;

	if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}
#endif

#ifdef USE_FILTER_MODE2
	sFilterConfig.FilterIdHigh				= (IDENTIFIER_6 << 5);		// for understanding why the shift is used and why there is such,
	sFilterConfig.FilterIdLow				= 0x0000;					// it's necessary to refer to the appNote RM0090, p. 1088
	sFilterConfig.FilterMaskIdHigh			= (IDENTIFIER_7 << 5);
	sFilterConfig.FilterMaskIdLow			= 0x0000;
	sFilterConfig.FilterFIFOAssignment		= CAN_FILTER_FIFO0;
	sFilterConfig.FilterBank				= FILTER_NUM2;
	sFilterConfig.FilterMode				= CAN_FILTERMODE_IDLIST;
	sFilterConfig.FilterScale				= CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterActivation			= CAN_FILTER_ENABLE;
	sFilterConfig.SlaveStartFilterBank		= 14;

	if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}
#endif

#ifdef USE_FILTER_MODE3
	sFilterConfig.FilterIdHigh				= (0x0078 << 5);			// Why is 0x0078 used. Given the features of filter settings,
	sFilterConfig.FilterIdLow				= (IDENTIFIER_13 << 5);		// it's impossible to set up filters from 0x007A to 0x007E identifiers,
	sFilterConfig.FilterMaskIdHigh			= (0x0078 << 5);			// however, it's possible to set the range from 0x0078 to 0x007f. With
	sFilterConfig.FilterMaskIdLow			= (0x07F8 << 5);			// a similar range, the task in this mode is solved, however, if
	sFilterConfig.FilterFIFOAssignment		= CAN_FILTER_FIFO0;			// to add identifiers 0x0079 and 0x007f, they will also pass the filter.
	sFilterConfig.FilterBank				= FILTER_NUM3;				// For understanding why the shift is used and why there is such,
	sFilterConfig.FilterMode				= CAN_FILTERMODE_IDMASK;	// it's necessary to refer to the appNote RM0090, p. 1088
	sFilterConfig.FilterScale				= CAN_FILTERSCALE_16BIT;
	sFilterConfig.FilterActivation			= CAN_FILTER_ENABLE;
	sFilterConfig.SlaveStartFilterBank		= 14;

	if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}
#endif

#ifdef USE_FILTER_MODE4
	sFilterConfig.FilterIdHigh				= (IDENTIFIER_17 << 5);		// for understanding why the shift is used and why there is such,
	sFilterConfig.FilterIdLow				= (IDENTIFIER_18 << 5);		// it's necessary to refer to the appNote RM0090, p. 1088
	sFilterConfig.FilterMaskIdHigh			= (IDENTIFIER_19 << 5);
	sFilterConfig.FilterMaskIdLow			= (IDENTIFIER_20 << 5);
	sFilterConfig.FilterFIFOAssignment		= CAN_FILTER_FIFO0;
	sFilterConfig.FilterBank				= FILTER_NUM4;
	sFilterConfig.FilterMode				= CAN_FILTERMODE_IDLIST;
	sFilterConfig.FilterScale				= CAN_FILTERSCALE_16BIT;
	sFilterConfig.FilterActivation			= CAN_FILTER_ENABLE;
	sFilterConfig.SlaveStartFilterBank		= 14;

	if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}
#endif

#if !defined(USE_FILTER_MODE1) || !defined(USE_FILTER_MODE2) || !defined(USE_FILTER_MODE3) || !defined(USE_FILTER_MODE4)
	sFilterConfig.FilterIdHigh				= 0x0000;
	sFilterConfig.FilterIdLow				= 0x0000;
	sFilterConfig.FilterMaskIdHigh			= 0x0000;
	sFilterConfig.FilterMaskIdLow			= 0x0000;
	sFilterConfig.FilterFIFOAssignment		= CAN_FILTER_FIFO0;
	sFilterConfig.FilterBank				= FILTER_NUM1;
	sFilterConfig.FilterMode				= CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale				= CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterActivation			= CAN_FILTER_ENABLE;
	sFilterConfig.SlaveStartFilterBank		= 14;

	if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}
#endif


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

	// Create the queue(s)
	// definition and creation of dataFromCANQueue
	osMessageQDef(sendDataFromCAN, QUEUE_SIZE, bxCAN_message_t);
	dataFromCANHandle = osMessageCreate(osMessageQ(sendDataFromCAN), NULL);

	// Create the memory pool(s)
	// definition and creation of mpool
	osPoolDef(mpool, ELEMENTS_IN_MEMORY_POOL, bxCAN_message_t);
	mpool = osPoolCreate(osPool(mpool));

#ifdef DEBUG
	vQueueAddToRegistry(InterruptRxFIFO0SemHandle, "semReceiving");
	vQueueAddToRegistry(SendingMessagesSemHandle, "semSending");
	vQueueAddToRegistry(InterruprtErrorCANSemHandle, "semError");
	vQueueAddToRegistry(dataFromCANHandle, "data from CAN");
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

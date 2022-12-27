//---------------------------------------------------------------------------
// Define to prevent recursive inclusion
//---------------------------------------------------------------------------
#ifndef __BXCAN_H
#define __BXCAN_H

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "main.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define DATA_FIELD 								(8U) // bytes

//---------------------------------------------------------------------------
// Typedefs
//---------------------------------------------------------------------------

/**
 * @brief  CAN bus message
 */
typedef struct
{
	uint32_t id;
	uint32_t length;
	uint8_t data[DATA_FIELD];

} bxCAN_message_t;

//---------------------------------------------------------------------------
// External function prototypes
//---------------------------------------------------------------------------
void bxCAN_FreeRTOS_init(void);
void InterruptHandlingSendTask(void const* argument);
void InterruptHandlingRxFIFO0Task(void const* argument);
void InterruptHandlingErrorTask(void const* argument);

#ifdef __cplusplus
}
#endif

#endif /* __BXCAN_H */

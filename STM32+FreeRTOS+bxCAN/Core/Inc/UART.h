//---------------------------------------------------------------------------
// Define to prevent recursive inclusion
//---------------------------------------------------------------------------
#ifndef __UART_H__
#define __UART_H__

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "main.h"

//---------------------------------------------------------------------------
// External function prototypes
//---------------------------------------------------------------------------
void UART_FreeRTOS_init(void);
void UARTSendingMessagesTask(void const* argument);

#ifdef __cplusplus
}
#endif

#endif /*__ __UART_H__ */

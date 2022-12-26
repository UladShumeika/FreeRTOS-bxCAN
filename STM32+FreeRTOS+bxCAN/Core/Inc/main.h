//---------------------------------------------------------------------------
// Define to prevent recursive inclusion
//---------------------------------------------------------------------------
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include <stdio.h>

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define HEARTBEAT
#define bxCAN
#define UART

//---------------------------------------------------------------------------
// Module's includes
//---------------------------------------------------------------------------

#ifdef HEARTBEAT
	#include "heartbeat.h"
#endif

#ifdef bxCAN
	#include "bxCAN.h"
#endif

#ifdef UART
	#include "UART.h"
#endif

//---------------------------------------------------------------------------
// External function prototypes
//---------------------------------------------------------------------------
void Error_Handler(void);
void MX_FREERTOS_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

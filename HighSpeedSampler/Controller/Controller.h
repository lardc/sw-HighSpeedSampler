#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

// Includes
//
#include <stdint.h>

// Definitions
//
#define VALUES_READx_SIZE		2000
#define VALUES_WRITEx_SIZE		2000

// Types
//
typedef enum __DeviceState
{
	DS_None				= 0,
	DS_Fault			= 1,
	DS_Disabled			= 2,
	DS_InProcess		= 3
} DeviceState;

// Variables
//
extern volatile uint64_t CONTROL_TimeCounter;

// Functions
//
void CONTROL_TimerInit();
void CONTROL_Init();
void CONTROL_Idle();

#endif	// __CONTROLLER_H__

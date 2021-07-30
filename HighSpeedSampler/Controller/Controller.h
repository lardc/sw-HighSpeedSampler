#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

// Includes
//
#include <stdint.h>
#include "Controller.h"

// Types
//
typedef enum __DeviceState
{
	DS_None				= 0,
	DS_Fault			= 1,
	DS_Disabled			= 2,
	DS_Ready			= 3,
	DS_InProcess		= 4
} DeviceState;

// Variables
//
extern volatile uint64_t CONTROL_TimeCounter;

// Functions
//
void CONTROL_TimerInit();
bool CONTROL_Init(const char *ScopeSerialVoltage, const char *ScopeSerialCurrent);
void CONTROL_Idle();

#endif	// __CONTROLLER_H__

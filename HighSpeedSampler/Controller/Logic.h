#ifndef __LOGIC_H__
#define __LOGIC_H__

// Includes
//
#include <stdint.h>
#include "Controller\Sampler.h"

// Functions
//
PICO_STATUS LOGIC_PisoScopeInit();
PICO_STATUS LOGIC_PisoScopeActivate(uint16_t MaxCurrent, float ShuntRes, bool EnableVoltage);
PICO_STATUS LOGIC_HandleSamplerData(uint16_t* CalcProblem, uint32_t* Index0, float* Irr, float* trr, float* Qrr, float* dIdt, bool UseVoltage, bool UseTrr050Method, uint32_t* Index0V);
uint16_t LOGIC_GetIData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead);
uint16_t LOGIC_GetVData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead);
uint16_t LOGIC_LoadFragment(uint16_t* BufferI, uint16_t* BufferV, uint16_t Size, uint16_t Scale);
void LOGIC_CurrentToFile();
void LOGIC_VoltageToFile();

#endif	// __LOGIC_H__

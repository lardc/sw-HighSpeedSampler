#ifndef __SAMPLER_H__
#define __SAMPLER_H__

// Includes
//
#include <stdint.h>
#include "External\PicoStatus.h"
#include "External\ps5000aApi.h"

// Functions
//
bool SAMPLER_StatusGood(PICO_STATUS status);
PICO_STATUS SAMPLER_Open(const char *ScopeSerialVoltage, const char *ScopeSerialCurrent);
void SAMPLER_GetHandlers(int16_t* Voltage, int16_t* Current);
PICO_STATUS SAMPLER_Init();
PICO_STATUS SAMPLER_Close();
PICO_STATUS SAMPLER_ConfigureChannels(PS5000A_RANGE VRange, PS5000A_RANGE IRange);
PS5000A_RANGE SAMPLER_SelectRange(float Voltage);
PS5000A_RANGE SAMPLER_GetSavedVRange();
PS5000A_RANGE SAMPLER_GetSavedIRange();
float SAMPLER_GetVRangeCoeff();
float SAMPLER_GetIRangeCoeff();
PICO_STATUS SAMPLER_ActivateSampling();
bool SAMPLING_Finished();
PICO_STATUS SAMPLER_ConnectOutputBuffers(short *BufferI, unsigned int BufferILength, short *BufferV, unsigned int BufferVLength);
PICO_STATUS SAMPLER_GetValues(unsigned int *noOfSamples);
PICO_STATUS SAMPLER_Stop();

#endif	// __SAMPLER_H__

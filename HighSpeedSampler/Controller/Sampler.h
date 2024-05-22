#ifndef __SAMPLER_H__
#define __SAMPLER_H__

// Includes
//
#include <stdint.h>
#include "External\PicoStatus.h"
#include "External\ps5000aApi.h"

// Functions
//
PICO_STATUS SAMPLER_Open();
PICO_STATUS SAMPLER_Init();
PICO_STATUS SAMPLER_Close();
PICO_STATUS SAMPLER_ConfigureChannels(PS5000A_RANGE IRange, bool EnableVoltage);
PS5000A_RANGE SAMPLER_SelectRange(float Voltage);
PS5000A_RANGE SAMPLER_GetSavedIRange();
float SAMPLER_GetRangeCoeff();
PICO_STATUS SAMPLER_ActivateSampling();
bool SAMPLING_Finished();
PICO_STATUS SAMPLER_ConnectOutputBuffers(short *BufferI, unsigned int BufferILength, short *BufferV, unsigned int BufferVLength);
PICO_STATUS SAMPLER_GetValues(unsigned int *noOfSamples);
PICO_STATUS SAMPLER_Stop();

#endif	// __SAMPLER_H__

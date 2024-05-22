// Headers
//
#include "stdafx.h"
#include "Sampler.h"

// Includes
//
#include "Global.h"
#include <math.h>

// Variables
//
static int16_t handle;
static bool SamplingDone = false;
static PS5000A_RANGE SavedIRange = SAMPLING_I_DEF_RANGE;

// Functions
//
PICO_STATUS SAMPLER_Open()
{
	PICO_STATUS ret_val = ps5000aOpenUnit(&handle, NULL, SAMPLING_RESOLUTION);
	if ((ret_val == PICO_POWER_SUPPLY_NOT_CONNECTED || ret_val == PICO_USB3_0_DEVICE_NON_USB3_0_PORT) && !SAMPLING_EXT_POWER)
		return ps5000aChangePowerSource(handle, PICO_POWER_SUPPLY_NOT_CONNECTED);
	else
		return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConfigureSamplingRate()
{
	return ps5000aGetTimebase(handle, SAMPLING_TIME_BASE, SAMPLING_SAMPLES, NULL, NULL, 0);
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConfigureTrigger()
{
	return ps5000aSetSimpleTrigger(handle, 1, TRIGGER_SOURCE, TRIGGER_LEVEL, TRIGGER_MODE, 0, 0);
}
//----------------------------------------------

PICO_STATUS SAMPLER_Init()
{
	PICO_STATUS ret_val = PICO_OK;

	if ((ret_val = SAMPLER_ConfigureChannels(SAMPLING_I_DEF_RANGE, false)) == PICO_OK)
		if ((ret_val = SAMPLER_ConfigureSamplingRate()) == PICO_OK)
			return SAMPLER_ConfigureTrigger();
	
	return ret_val;
}
//----------------------------------------------

PS5000A_RANGE SAMPLER_SelectRange(float Voltage)
{
	float v = (float)fabs(Voltage);

	if (v < 0.01f)
		return PS5000A_10MV;
	else if (v < 0.02f)
		return PS5000A_20MV;
	else if (v < 0.05f)
		return PS5000A_50MV;
	else if (v < 0.1f)
		return PS5000A_100MV;
	else if (v < 0.2f)
		return PS5000A_200MV;
	else if (v < 0.5f)
		return PS5000A_500MV;
	else if (v < 1.0f)
		return PS5000A_1V;
	else if (v < 2.0f)
		return PS5000A_2V;
	else if (v < 5.0f)
		return PS5000A_5V;
	else if (v < 10.0f)
		return PS5000A_10V;
	else
		return PS5000A_20V;
}
//----------------------------------------------

PS5000A_RANGE SAMPLER_GetSavedIRange()
{
	return SavedIRange;
}
//----------------------------------------------

float SAMPLER_GetRangeCoeff()
{
	switch(SavedIRange)
	{
		case PS5000A_10MV:
			return 0.01f;
		case PS5000A_20MV:
			return 0.02f;
		case PS5000A_50MV:
			return 0.05f;
		case PS5000A_100MV:
			return 0.1f;
		case PS5000A_200MV:
			return 0.2f;
		case PS5000A_500MV:
			return 0.5f;
		case PS5000A_1V:
			return 1.0f;
		case PS5000A_2V:
			return 2.0f;
		case PS5000A_5V:
			return 5.0f;
		case PS5000A_10V:
			return 10.0f;
		case PS5000A_20V:
		default:
			return 20.0f;
	}
}
//----------------------------------------------

PICO_STATUS SAMPLER_Close()
{
	return ps5000aCloseUnit(handle);
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConfigureChannels(PS5000A_RANGE IRange, bool EnableVoltage)
{
	PICO_STATUS result;

	SavedIRange = IRange;
	if ((result = ps5000aSetChannel(handle, SAMPLING_I_CHANNEL, 1, PS5000A_DC, SavedIRange, 0)) == PICO_OK)
		result = ps5000aSetChannel(handle, SAMPLING_V_CHANNEL, EnableVoltage ? 1 : 0, PS5000A_DC, SAMPLING_V_RANGE, 0);

	return result;
}
//----------------------------------------------

void PREF4 SAMPLER_CallBack(int16_t handle, PICO_STATUS status, void *pParameter)
{
	SamplingDone = true;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ActivateSampling()
{
	SamplingDone = false;
	return ps5000aRunBlock(handle, 0, SAMPLING_SAMPLES, SAMPLING_TIME_BASE, NULL, 0, SAMPLER_CallBack, NULL);
}
//----------------------------------------------

bool SAMPLING_Finished()
{
	return SamplingDone;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConnectOutputBuffers(short *BufferI, unsigned int BufferILength, short *BufferV, unsigned int BufferVLength)
{
	PICO_STATUS result;

	if ((result = ps5000aSetDataBuffer(handle, SAMPLING_I_CHANNEL, BufferI, BufferILength, 0, PS5000A_RATIO_MODE_NONE)) == PICO_OK)
		result = ps5000aSetDataBuffer(handle, SAMPLING_V_CHANNEL, BufferV, BufferVLength, 0, PS5000A_RATIO_MODE_NONE);

	return result;
}
//----------------------------------------------

PICO_STATUS SAMPLER_GetValues(unsigned int *noOfSamples)
{
	return ps5000aGetValues(handle, 0, noOfSamples, 0, PS5000A_RATIO_MODE_NONE, 0, NULL);
}
//----------------------------------------------

PICO_STATUS SAMPLER_Stop()
{
	return ps5000aStop(handle);
}
//----------------------------------------------

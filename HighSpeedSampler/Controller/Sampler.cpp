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
static int16_t VHandler, IHandler;
static bool SamplingVDone = false, SamplingIDone = false;
static PS5000A_RANGE SavedVRange = SAMPLING_DEFAULT_RANGE, SavedIRange = SAMPLING_DEFAULT_RANGE;

// Forward functions
//
PICO_STATUS SAMPLER_OpenX(const char *SerialNumber, int16_t *Handler);
PICO_STATUS SAMPLER_ConfigureChannelsX(int16_t Handler, PS5000A_RANGE Range);
float SAMPLER_GetRangeCoeff(PS5000A_RANGE Range);

// Functions
//
PICO_STATUS SAMPLER_StatusComb(PICO_STATUS status)
{
	if(status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT || status == PICO_EEPROM_CORRUPT)
		return PICO_OK;
	else
		return status;
}
//----------------------------------------------

PICO_STATUS SAMPLER_OpenX(const char *SerialNumber, int16_t *Handler)
{
	if (DIAG_EMULATE_SCOPES)
	{
		*Handler = 0;
		return PICO_OK;
	}

	PICO_STATUS ret_val = SAMPLER_StatusComb(ps5000aOpenUnit(Handler, (int8_t *)SerialNumber, SAMPLING_RESOLUTION));
	if (ret_val == PICO_POWER_SUPPLY_NOT_CONNECTED || ret_val == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aChangePowerSource(*Handler, PICO_POWER_SUPPLY_NOT_CONNECTED));

	return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_Open(const char *ScopeSerialVoltage, const char *ScopeSerialCurrent)
{
	PICO_STATUS ret_val = PICO_OK;
	VHandler = IHandler = 0;

	if ((ret_val = SAMPLER_OpenX(ScopeSerialVoltage, &VHandler)) == PICO_OK)
		ret_val = SAMPLER_OpenX(ScopeSerialCurrent, &IHandler);

	return ret_val;
}
//----------------------------------------------

void SAMPLER_GetHandlers(int16_t* Voltage, int16_t* Current)
{
	*Voltage = VHandler;
	*Current = IHandler;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConfigureSamplingRate()
{
	if (DIAG_EMULATE_SCOPES)
		return PICO_OK;

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_StatusComb(ps5000aGetTimebase(VHandler, SAMPLING_TIME_BASE, SAMPLING_SAMPLES, NULL, NULL, 0))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aGetTimebase(IHandler, SAMPLING_TIME_BASE, SAMPLING_SAMPLES, NULL, NULL, 0));

	return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConfigureTrigger()
{
	if (DIAG_EMULATE_SCOPES)
		return PICO_OK;

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_StatusComb(ps5000aSetSimpleTrigger(VHandler, 1, TRIGGER_SOURCE, TRIGGER_LEVEL, TRIGGER_MODE, 0, 0))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aSetSimpleTrigger(IHandler, 1, TRIGGER_SOURCE, TRIGGER_LEVEL, TRIGGER_MODE, 0, 0));

	return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_Init()
{
	if (DIAG_EMULATE_SCOPES)
		return PICO_OK;

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_ConfigureChannels(SAMPLING_DEFAULT_RANGE, SAMPLING_DEFAULT_RANGE)) == PICO_OK)
		if ((ret_val = SAMPLER_ConfigureSamplingRate()) == PICO_OK)
			return SAMPLER_ConfigureTrigger();
	
	return ret_val;
}
//----------------------------------------------

PS5000A_RANGE SAMPLER_SelectRange(float Voltage)
{
	float v = (float)fabs(Voltage * SAMPLING_SAFE_RANGE_RATIO);

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

PS5000A_RANGE SAMPLER_GetSavedVRange()
{
	return SavedVRange;
}
//----------------------------------------------

PS5000A_RANGE SAMPLER_GetSavedIRange()
{
	return SavedIRange;
}
//----------------------------------------------

float SAMPLER_GetRangeCoeff(PS5000A_RANGE Range)
{
	switch (Range)
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

float SAMPLER_GetVRangeCoeff()
{
	return SAMPLER_GetRangeCoeff(SavedVRange);
}
//----------------------------------------------

float SAMPLER_GetIRangeCoeff()
{
	return SAMPLER_GetRangeCoeff(SavedIRange);
}
//----------------------------------------------

PICO_STATUS SAMPLER_Close()
{
	if (DIAG_EMULATE_SCOPES)
		return PICO_OK;

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_StatusComb(ps5000aCloseUnit(VHandler))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aCloseUnit(IHandler));

	return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConfigureChannelsX(int16_t Handler, PS5000A_RANGE Range)
{
	if (DIAG_EMULATE_SCOPES)
		return PICO_OK;

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_StatusComb(ps5000aSetChannel(Handler, SAMPLING_ACTIVE_CHANNEL, 1, PS5000A_DC, Range, 0))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aSetChannel(Handler, SAMPLING_OFF_CHANNEL, 0, PS5000A_DC, PS5000A_20V, 0));

	return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConfigureChannels(PS5000A_RANGE VRange, PS5000A_RANGE IRange)
{
	PICO_STATUS ret_val = PICO_OK;

	SavedVRange = VRange;
	SavedIRange = IRange;

	if ((ret_val = SAMPLER_ConfigureChannelsX(VHandler, VRange)) == PICO_OK)
		ret_val = SAMPLER_ConfigureChannelsX(IHandler, IRange);

	return ret_val;
}
//----------------------------------------------

void PREF4 SAMPLER_CallBack(int16_t Handler, PICO_STATUS status, void *pParameter)
{
	if (Handler == VHandler)
		SamplingVDone = true;
	else if (Handler == IHandler)
		SamplingIDone = true;
}
//----------------------------------------------

PICO_STATUS SAMPLER_ActivateSampling()
{
	if (DIAG_EMULATE_SCOPES)
	{
		SamplingVDone = SamplingIDone = true;
		return PICO_OK;
	}

	PICO_STATUS ret_val = PICO_OK;
	SamplingVDone = SamplingIDone = false;

	if ((ret_val = SAMPLER_StatusComb(ps5000aRunBlock(VHandler, 0, SAMPLING_SAMPLES, SAMPLING_TIME_BASE, NULL, 0, SAMPLER_CallBack, NULL))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aRunBlock(IHandler, 0, SAMPLING_SAMPLES, SAMPLING_TIME_BASE, NULL, 0, SAMPLER_CallBack, NULL));

	return ret_val;
}
//----------------------------------------------

bool SAMPLING_Finished()
{
	return (SamplingVDone && SamplingIDone);
}
//----------------------------------------------

PICO_STATUS SAMPLER_ConnectOutputBuffers(short *BufferI, unsigned int BufferILength, short *BufferV, unsigned int BufferVLength)
{
	if (DIAG_EMULATE_SCOPES)
		return PICO_OK;

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_StatusComb(ps5000aSetDataBuffer(VHandler, SAMPLING_ACTIVE_CHANNEL, BufferV, BufferVLength, 0, PS5000A_RATIO_MODE_NONE))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aSetDataBuffer(IHandler, SAMPLING_ACTIVE_CHANNEL, BufferI, BufferILength, 0, PS5000A_RATIO_MODE_NONE));

	return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_GetValues(unsigned int *noOfSamples)
{
	if (DIAG_EMULATE_SCOPES)
	{
		*noOfSamples = 0;
		return PICO_OK;
	}

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_StatusComb(ps5000aGetValues(VHandler, 0, noOfSamples, 0, PS5000A_RATIO_MODE_NONE, 0, NULL))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aGetValues(IHandler, 0, noOfSamples, 0, PS5000A_RATIO_MODE_NONE, 0, NULL));

	return ret_val;
}
//----------------------------------------------

PICO_STATUS SAMPLER_Stop()
{
	if (DIAG_EMULATE_SCOPES)
		return PICO_OK;

	PICO_STATUS ret_val = PICO_OK;
	if ((ret_val = SAMPLER_StatusComb(ps5000aStop(VHandler))) == PICO_OK)
		ret_val = SAMPLER_StatusComb(ps5000aStop(IHandler));

	return ret_val;
}
//----------------------------------------------

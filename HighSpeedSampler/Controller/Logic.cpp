// Headers
//
#include "stdafx.h"
#include "Logic.h"

// Includes
//
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "Controller\Global.h"
#include "Controller\Math\Calculate.h"
#include "Controller\Math\FIR.h"
#include "Platform\DataTable.h"
#include "Platform\DeviceObjectDictionary.h"
#include "Info.h"

// Variables
//
static int16_t MEMBUF_ScopeI[SAMPLING_SAMPLES], MEMBUF_ScopeV[SAMPLING_SAMPLES];
static float MEMBUF_fScopeI[SAMPLING_SAMPLES], MEMBUF_fScopeV[SAMPLING_SAMPLES];
static float MEMBUF_fScopeIFiltered[SAMPLING_SAMPLES], MEMBUF_fScopeVFiltered[SAMPLING_SAMPLES];;
static uint32_t MEMBUF_Scope_Counter, SCOPE_ReadFullCounter;
static float ShuntResCache;

// Functions
//
PICO_STATUS LOGIC_PisoScopeInit()
{
	PICO_STATUS status;

	if ((status = SAMPLER_Open()) == PICO_OK)
		status = SAMPLER_Init();
	
	return status;
}
//----------------------------------------------

PICO_STATUS LOGIC_PisoScopeActivate(uint16_t MaxCurrent, float ShuntRes, bool EnableVoltage)
{
	PICO_STATUS status;
	PS5000A_RANGE curr_v_range;
	float curr_v_max;
	char message[256];

	ShuntResCache = ShuntRes;
	curr_v_max = 0.001f * MaxCurrent * ShuntRes;

	if ((status = SAMPLER_ConfigureChannels(curr_v_range = SAMPLER_SelectRange(curr_v_max), EnableVoltage)) == PICO_OK)
		status = SAMPLER_ActivateSampling();

	// Diagnostic output
	sprintf_s(message, 256, "Shunt, mOhm: %.3f; Range: %d; Max I voltage, V: %.3f", ShuntRes, curr_v_range, curr_v_max);
	InfoPrint(IP_Info, message);

	return status;
}
// ----------------------------------------

PICO_STATUS LOGIC_HandleSamplerData(uint16_t* CalcProblem, uint32_t* Index0, float* Irr, float* trr, float* Qrr, float* dIdt, bool UseVoltage, bool UseTrr050Method, uint32_t* Index0V)
{
	char message[256];

	uint32_t i, Index_0, Index_irr, Index_trr, Index_025, Index_09, Index_0V;
	PICO_STATUS status;
	float invert_mul = (SAMPLING_I_INVERT_INPUT) ? (-1.0f) : (1.0f);
	float Actual_dIdt = 0;

	SCOPE_ReadFullCounter = 0;
	*CalcProblem = PROBLEM_NONE;

	// Get scope data
	if ((status = SAMPLER_ConnectOutputBuffers(MEMBUF_ScopeI, SAMPLING_SAMPLES, MEMBUF_ScopeV, SAMPLING_SAMPLES)) == PICO_OK)
	{
		MEMBUF_Scope_Counter = SAMPLING_SAMPLES;
		if ((status = SAMPLER_GetValues(&MEMBUF_Scope_Counter)) == PICO_OK)
		{
			if ((status = SAMPLER_Stop()) == PICO_OK)
			{
				// Convert to current
				float Kfine = (float)DataTable[REG_I_FINE_N] / DataTable[REG_I_FINE_D];
				float Offset = (float)((int16_t)DataTable[REG_I_FINE_OFFSET]) / 10;

				// Diagnostic output
				sprintf_s(message, 256, "Shunt, mOhm: %.3f; Range: %d; Range-K: %.2f; Kfine: %.3f; Offset: %.1f", ShuntResCache, SAMPLER_GetSavedIRange(), SAMPLER_GetRangeCoeff(), Kfine, Offset);
				InfoPrint(IP_Info, message);
				
				for (i = 0; i < MEMBUF_Scope_Counter; ++i)
					MEMBUF_fScopeI[i] = (SAMPLER_GetRangeCoeff() * MEMBUF_ScopeI[i] * Kfine * invert_mul) / (INT16_MAX * ShuntResCache * 0.001f) + Offset;

				// Convert to voltage
				Kfine = (float)DataTable[REG_V_FINE_N] / DataTable[REG_V_FINE_D];
				Offset = (float)((int16_t)DataTable[REG_V_FINE_OFFSET]) / 10;

				for (i = 0; i < MEMBUF_Scope_Counter; ++i)
					MEMBUF_fScopeV[i] = MEMBUF_ScopeV[i] * SAMPLING_V_COEFFICIENT / INT16_MAX * Kfine + Offset;

				// Filter
				FIR_Apply(MEMBUF_fScopeI, MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter);
				FIR_Apply(MEMBUF_fScopeV, MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter);

				// Main calculations
				try
				{
					sprintf_s(message, 256, "Results:");
					InfoPrint(IP_Info, message);

					// Calculate Index0 and Irr parameters
					if (!CALC_IrrAndZeroCrossingIndex(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, &Index_0, &Index_irr))
						throw PROBLEM_CALC_IRR;

					if (Index0) *Index0 = Index_0;
					if (Irr) *Irr = (float)fabs(MEMBUF_fScopeIFiltered[Index_irr]);

					sprintf_s(message, 256, "Index 0: %d; Index Irr: %d", Index_0, Index_irr);
					InfoPrint(IP_Info, message);

					// Calculate Irr pivot points
					if (!CALC_IrrFractionIndex(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, Index_irr, UseTrr050Method ? 0.5f : 0.25f, &Index_025))
						throw PROBLEM_CALC_IRR_025;

					sprintf_s(message, 256, "Index Irr_low: %d", Index_025);
					InfoPrint(IP_Info, message);

					if (!CALC_IrrFractionIndex(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, Index_irr, 0.9f, &Index_09))
						throw PROBLEM_CALC_IRR_090;

					sprintf_s(message, 256, "Index Irr_high: %d", Index_09);
					InfoPrint(IP_Info, message);

					// Calculate trr and Qrr
					Index_trr = CALC_trrIndex(MEMBUF_fScopeIFiltered[Index_025], MEMBUF_fScopeIFiltered[Index_09], Index_025, Index_09);

					if (trr) *trr = SAMPLING_TIME_FRACTION * ((Index_trr > Index_0) ? (Index_trr - Index_0) : 0);
					if (Qrr) *Qrr = (float)fabs(CALC_Qrr(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, Index_0, Index_trr, SAMPLING_TIME_FRACTION));

					sprintf_s(message, 256, "Index trr: %d", Index_trr);
					InfoPrint(IP_Info, message);

					// Calculate actual dIdt
					if (!CALC_dIdt(MEMBUF_fScopeIFiltered, Index_0, Index_irr, SAMPLING_TIME_FRACTION, &Actual_dIdt))
						throw PROBLEM_CALC_DIDT;

					if (dIdt) *dIdt = Actual_dIdt;

					sprintf_s(message, 256, "Actual dIdt: %.1f", Actual_dIdt);
					InfoPrint(IP_Info, message);

					// Calculate voltage zero crossing
					if (UseVoltage)
					{
						if (!CALC_OSVZeroCrossing(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter, &Index_0V))
							throw PROBLEM_CALC_VZ;

						sprintf_s(message, 256, "Index V0: %d", Index_0V);
						InfoPrint(IP_Info, message);
					}
					else
						Index_0V = 0;
					if (Index0V) *Index0V = Index_0V;
				}
				catch(int problem)
				{
					*CalcProblem = problem;
					sprintf_s(message, 256, "Exception. Problem: %d", problem);
					InfoPrint(IP_Warn, message);
				}
			}
		}
	}

	return status;
}
// ----------------------------------------

uint16_t LOGIC_GetXData(float* SrcBuffer, uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, uint32_t Index0, uint32_t MulFactor, uint32_t ForceSectorRead, uint16_t* SampleTimeSteps)
{
	uint16_t i = 0;
	uint16_t dsRatio, Counter;
	uint32_t TrimmedDataCounter;

	// Trim data in case of successful calculations
	if (ForceSectorRead > 0)
	{
		TrimmedDataCounter = (ForceSectorRead < MEMBUF_Scope_Counter) ? ForceSectorRead : MEMBUF_Scope_Counter;
	}
	else
	{
		// 4 x (fall time from Imax to zero)
		TrimmedDataCounter = ((MulFactor * Index0) < MEMBUF_Scope_Counter && CalcOK && Index0 > 0) ? (MulFactor * Index0) : MEMBUF_Scope_Counter;
	}

	// Downsample ratio
	dsRatio = TrimmedDataCounter / BufferSize + 1;
	Counter = TrimmedDataCounter / dsRatio;

	for (i = 0; i < Counter; ++i)
		Buffer[i] = (uint16_t)((int16_t)SrcBuffer[i * dsRatio]);

	if (SampleTimeSteps)
		*SampleTimeSteps = dsRatio;

	return i;
}
// ----------------------------------------

uint16_t LOGIC_GetIData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead, uint16_t* SampleTimeSteps)
{
	return LOGIC_GetXData(MEMBUF_fScopeIFiltered, Buffer, BufferSize, CalcOK, ModeQrr ? Index0 : Index0V, ModeQrr ? MUL_FACTOR_I : MUL_FACTOR_V, ForceSectorRead, SampleTimeSteps);
}
// ----------------------------------------

uint16_t LOGIC_GetVData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead, uint16_t* SampleTimeSteps)
{
	return LOGIC_GetXData(MEMBUF_fScopeVFiltered, Buffer, BufferSize, CalcOK, ModeQrr ? Index0 : Index0V, ModeQrr ? MUL_FACTOR_I : MUL_FACTOR_V, ForceSectorRead, SampleTimeSteps);
}
// ----------------------------------------

uint16_t LOGIC_LoadFragment(uint16_t* BufferI, uint16_t* BufferV, uint16_t Size, uint16_t Scale)
{
	uint16_t counter = 0;

	while (counter < Size && SCOPE_ReadFullCounter < MEMBUF_Scope_Counter)
	{
		BufferI[counter] = (uint16_t)((int16_t)MEMBUF_fScopeIFiltered[SCOPE_ReadFullCounter]);
		BufferV[counter] = (uint16_t)((int16_t)MEMBUF_fScopeVFiltered[SCOPE_ReadFullCounter]);
		++counter;
		SCOPE_ReadFullCounter += Scale;
	}

	return counter;
}
// ----------------------------------------

void LOGIC_BufferToFile(float* Buffer, uint32_t BufferSize, const char* FileName)
{
	FILE *fPointer;
	char item[32];

	if (fopen_s(&fPointer, FileName, "w") == 0)
	{
		InfoPrint(IP_Info, "File opened");

		for (uint32_t i = 0; i < BufferSize; i++)
		{
			sprintf_s(item, "%.2f", Buffer[i]);
			fprintf(fPointer, "%s\n", item);
		}

		fclose(fPointer);
	}
	else
		InfoPrint(IP_Err, "File not opened");
}
// ----------------------------------------

void LOGIC_CurrentToFile()
{
	LOGIC_BufferToFile(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, "current.csv");
}
// ----------------------------------------

void LOGIC_VoltageToFile()
{
	LOGIC_BufferToFile(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter, "voltage.csv");
}
// ----------------------------------------

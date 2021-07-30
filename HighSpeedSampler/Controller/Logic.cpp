// Headers
//
#include "stdafx.h"
#include "Logic.h"

// Includes
//
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include "Controller\Global.h"
#include "Controller\Math\Calculate.h"
#include "Controller\Math\FIR.h"
#include "Controller\Math\SplineFilter.h"
#include "Platform\DataTable.h"
#include "Platform\DeviceObjectDictionary.h"
#include "Info.h"

// Variables
//
static int16_t MEMBUF_ScopeI[SAMPLING_SAMPLES], MEMBUF_ScopeV[SAMPLING_SAMPLES];
static float MEMBUF_fScopeI[SAMPLING_SAMPLES], MEMBUF_fScopeV[SAMPLING_SAMPLES];
static float MEMBUF_fScopeIFiltered[SAMPLING_SAMPLES], MEMBUF_fScopeVFiltered[SAMPLING_SAMPLES];
static uint32_t MEMBUF_Scope_Counter, SCOPE_ReadFullCounter;
static float ShuntResCache;

// Functions
//
PICO_STATUS LOGIC_PicoScopeInit(const char *ScopeSerialVoltage, const char *ScopeSerialCurrent)
{
	PICO_STATUS status;
	int Attempts = 0;

	InfoPrint(IP_Info, "Attempt to detect scopes");
	while (LOGIC_PicoScopeList() < 2 && Attempts++ < SCOPE_DETECT_ATTEMPTS)
		Sleep(SCOPE_DETECT_WAIT_PAUSE);

	if ((status = SAMPLER_Open(ScopeSerialVoltage, ScopeSerialCurrent)) == PICO_OK)
		status = SAMPLER_Init();
	
	return status;
}
//----------------------------------------------

int16_t LOGIC_PicoScopeList()
{
	char Serials[256], message[256];
	int16_t Count = 0, StringLength = 256;

	ps5000aEnumerateUnits(&Count, (int8_t *)Serials, &StringLength);

	sprintf_s(message, 256, "Detected scopes count: %d", Count);
	InfoPrint(IP_Info, message);

	sprintf_s(message, 256, "Detected scopes serials: %s", Serials);
	InfoPrint(IP_Info, message);

	return Count;
}
//----------------------------------------------

PICO_STATUS LOGIC_PicoScopeActivate()
{
	PICO_STATUS status;
	PS5000A_RANGE iv_range, v_range;
	float CurrentSet, CurrentSetV, VoltageSet;
	char message[256];

	// Current parameters
	ShuntResCache = (float)DataTable[REG_SHUNT_RES_N] / DataTable[REG_SHUNT_RES_D];
	CurrentSet = (float)DataTable[REG_CURRENT_AMPL];
	CurrentSetV = 0.001f * CurrentSet * ShuntResCache;
	
	// Voltage parameters
	float Vdiv = (float)DataTable[REG_VOLTAGE_DIV_N] / DataTable[REG_VOLTAGE_DIV_D];
	float Vmax = (float)fabs(SAMPLING_QRR_VR) * 2;
	if (DataTable[REG_MEASURE_MODE] == MODE_QRR_TQ && DataTable[REG_VOLTAGE_AMPL] > Vmax)
		Vmax = DataTable[REG_VOLTAGE_AMPL];
	VoltageSet = Vdiv * Vmax;

	if ((status = SAMPLER_ConfigureChannels(v_range  = SAMPLER_SelectRange(VoltageSet),
											iv_range = SAMPLER_SelectRange(CurrentSetV))) == PICO_OK)
	{
		status = SAMPLER_ActivateSampling();
	}

	// Diagnostic output
	sprintf_s(message, 256, "Shunt, mOhm: %.3f; Range: %d; Max I voltage, V: %.3f", ShuntResCache, iv_range, CurrentSetV);
	InfoPrint(IP_Info, message);
	sprintf_s(message, 256, "Voltage range: %d; Max voltage, V: %.3f; Max div voltage, V: %.3f", v_range, Vmax, VoltageSet);
	InfoPrint(IP_Info, message);

	return status;
}
// ----------------------------------------

PICO_STATUS LOGIC_HandleSamplerData(uint16_t* CalcProblem, uint32_t* Index0, float* Irr, float* trr, float* Qrr, float* dIdt, float* Id, float* Vd, bool UseVoltage, bool UseTrr050Method, uint32_t* Index0V)
{
	char message[256];

	uint32_t i, Index_0, Index_irr, Index_trr, Index_025, Index_09, Index_0V;
	PICO_STATUS status;
	bool InvertCurrent = (DataTable[REG_INVERT_CURRENT] == 1);
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
				sprintf_s(message, 256, "Shunt, mOhm: %.3f; Range: %d; Range-K: %.2f; Kfine: %.3f; Offset: %.1f", ShuntResCache, SAMPLER_GetSavedIRange(), SAMPLER_GetIRangeCoeff(), Kfine, Offset);
				InfoPrint(IP_Info, message);

				if (InvertCurrent)
				{
					for (i = 0; i < MEMBUF_Scope_Counter; ++i)
						MEMBUF_ScopeI[i] = -MEMBUF_ScopeI[i];
				}
				
				for (i = 0; i < MEMBUF_Scope_Counter; ++i)
					MEMBUF_fScopeI[i] = (SAMPLER_GetIRangeCoeff() * MEMBUF_ScopeI[i] * Kfine) / (INT16_MAX * ShuntResCache * 0.001f) + Offset;

				// Convert to voltage
				float Kvoltage = (float)DataTable[REG_VOLTAGE_DIV_N] / DataTable[REG_VOLTAGE_DIV_D];
				Kfine = (float)DataTable[REG_V_FINE_N] / DataTable[REG_V_FINE_D];
				Offset = (float)((int16_t)DataTable[REG_V_FINE_OFFSET]) / 10;

				// Diagnostic output
				sprintf_s(message, 256, "Voltage range: %d; Range-K: %.2f; Kfine: %.3f; Offset: %.1f", SAMPLER_GetSavedVRange(), SAMPLER_GetVRangeCoeff(), Kfine, Offset);
				InfoPrint(IP_Info, message);

				for (i = 0; i < MEMBUF_Scope_Counter; ++i)
					MEMBUF_fScopeV[i] = (SAMPLER_GetVRangeCoeff() * MEMBUF_ScopeV[i]) / (Kvoltage * INT16_MAX) + Offset;

				// FIR filter
				FIR_Apply(MEMBUF_fScopeI, MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter);
				FIR_Apply(MEMBUF_fScopeV, MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter);

				// Spline filter
				SPLINE_Apply(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter);
				SPLINE_Apply(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter);

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

					// Calculate Id
					*Id = CALC_Id(MEMBUF_fScopeIFiltered, Index_0);

					sprintf_s(message, 256, "Idc: %.1f", *Id);
					InfoPrint(IP_Info, message);

					// Calculate Vd
					*Vd = CALC_Vd(MEMBUF_fScopeVFiltered, SAMPLING_SAMPLES);

					sprintf_s(message, 256, "Vd: %.1f", *Vd);
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

uint16_t LOGIC_GetXData(float* SrcBuffer, uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, uint32_t Index0, uint32_t MulFactor, uint32_t ForceSectorRead)
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

	return i;
}
// ----------------------------------------

uint16_t LOGIC_GetIData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead)
{
	return LOGIC_GetXData(MEMBUF_fScopeIFiltered, Buffer, BufferSize, CalcOK, ModeQrr ? Index0 : Index0V, ModeQrr ? MUL_FACTOR_I : MUL_FACTOR_V, ForceSectorRead);
}
// ----------------------------------------

uint16_t LOGIC_GetVData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK, bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead)
{
	return LOGIC_GetXData(MEMBUF_fScopeVFiltered, Buffer, BufferSize, CalcOK, ModeQrr ? Index0 : Index0V, ModeQrr ? MUL_FACTOR_I : MUL_FACTOR_V, ForceSectorRead);
}
// ----------------------------------------

uint16_t LOGIC_xLoadFragment(uint16_t* BufferI, uint16_t* BufferV, uint16_t Size, uint16_t Scale, bool UseFiltered)
{
	uint16_t counter = 0;

	while (counter < Size && SCOPE_ReadFullCounter < MEMBUF_Scope_Counter)
	{
		if (UseFiltered)
		{
			BufferI[counter] = (uint16_t)((int16_t)MEMBUF_fScopeIFiltered[SCOPE_ReadFullCounter]);
			BufferV[counter] = (uint16_t)((int16_t)MEMBUF_fScopeVFiltered[SCOPE_ReadFullCounter]);
		}
		else
		{
			BufferI[counter] = (uint16_t)(MEMBUF_ScopeI[SCOPE_ReadFullCounter]);
			BufferV[counter] = (uint16_t)(MEMBUF_ScopeV[SCOPE_ReadFullCounter]);
		}

		++counter;
		SCOPE_ReadFullCounter += Scale;
	}

	return counter;
}
// ----------------------------------------

uint16_t LOGIC_LoadFragment(uint16_t* BufferI, uint16_t* BufferV, uint16_t Size, uint16_t Scale)
{
	return LOGIC_xLoadFragment(BufferI, BufferV, Size, Scale, true);
}
// ----------------------------------------

uint16_t LOGIC_LoadRawFragment(uint16_t* BufferI, uint16_t* BufferV, uint16_t Size, uint16_t Scale)
{
	return LOGIC_xLoadFragment(BufferI, BufferV, Size, Scale, false);
}
// ----------------------------------------

void LOGIC_xBufferToFile(void* Buffer, uint32_t BufferSize, const char* FileName, bool UseFloat)
{
	FILE *fPointer;
	char message[256];

	if (fopen_s(&fPointer, FileName, "w") == 0)
	{
		sprintf_s(message, "File %s opened", FileName);
		InfoPrint(IP_Info, message);

		for (uint32_t i = 0; i < BufferSize; i++)
		{
			if (UseFloat)
				fprintf(fPointer, "%.2f\n", ((float*)Buffer)[i]);
			else
				fprintf(fPointer, "%d\n", ((short*)Buffer)[i]);
		}

		fclose(fPointer);

		sprintf_s(message, "Write to %s completed", FileName);
		InfoPrint(IP_Info, message);
	}
	else
	{
		sprintf_s(message, "File %s not opened", FileName);
		InfoPrint(IP_Err, message);
	}
}
// ----------------------------------------

void LOGIC_FloatBufferToFile(float* Buffer, uint32_t BufferSize, const char* FileName)
{
	LOGIC_xBufferToFile((void*)Buffer, BufferSize, FileName, true);
}
// ----------------------------------------

void LOGIC_ShortBufferToFile(short* Buffer, uint32_t BufferSize, const char* FileName)
{
	LOGIC_xBufferToFile((void*)Buffer, BufferSize, FileName, false);
}
// ----------------------------------------

void LOGIC_CurrentToFile()
{
	LOGIC_ShortBufferToFile(MEMBUF_ScopeI, MEMBUF_Scope_Counter, "current_raw.csv");
	LOGIC_FloatBufferToFile(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, "current.csv");
}
// ----------------------------------------

void LOGIC_VoltageToFile()
{
	LOGIC_ShortBufferToFile(MEMBUF_ScopeV, MEMBUF_Scope_Counter, "voltage_raw.csv");
	LOGIC_FloatBufferToFile(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter, "voltage.csv");
}
// ----------------------------------------

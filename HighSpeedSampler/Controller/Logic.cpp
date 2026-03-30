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
#include "Global.h"

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
	char message[256];
	PICO_STATUS status, VOpenStatus = PICO_OK, IOpenStatus = PICO_OK;

	LOGIC_PicoScopeList();

	InfoPrint(IP_Info, "Attempt to open scopes");
	status = SAMPLER_Open(ScopeSerialVoltage, ScopeSerialCurrent, &VOpenStatus, &IOpenStatus);

	sprintf_s(message, 256, "Voltage scope open status: 0x%08x", VOpenStatus);
	InfoPrint(status == PICO_OK ? IP_Info : IP_Warn, message);

	sprintf_s(message, 256, "Current scope open status: 0x%08x", IOpenStatus);
	InfoPrint(status == PICO_OK ? IP_Info : IP_Warn, message);

	if (status == PICO_OK)
	{
		InfoPrint(IP_Info, "Scopes are opened");
		status = SAMPLER_Init();
	}

	int16_t VHandler, IHandler;
	SAMPLER_GetHandlers(&VHandler, &IHandler);
	sprintf_s(message, 256, "Voltage handle: %d, current handle: %d", VHandler, IHandler);
	InfoPrint(status == PICO_OK ? IP_Info : IP_Warn, message);

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

PICO_STATUS LOGIC_HandleSamplerData(uint16_t* CalcProblem, uint32_t* Index0, float* Irr, float* trr, float* Qrr, float* dIdt, float* Id, float* Vd, bool UseVoltage, bool UseTrr050Method, uint32_t* Index0V, float* Time09, float* ts_time, float* tf_time, float* RevVolt)
{
	char message[256];

	uint32_t i, Index_0, Index_irr, Index_trr, Index_025, Index_09, Index_0V;
	float Corr_trr, Corr_trr_P2, Corr_trr_P1, Corr_trr_P0, Index_0_trr;
	PICO_STATUS status;
	bool InvertCurrent = (DataTable[REG_INVERT_CURRENT] == 1);
	float Actual_dIdt = 0;

	SCOPE_ReadFullCounter = 0;
	*CalcProblem = PROBLEM_NONE;

	uint32_t NSamples = LOGIC_GetSamplingSamples();

	// Get scope data
	if ((status = SAMPLER_ConnectOutputBuffers(MEMBUF_ScopeI, NSamples, MEMBUF_ScopeV, NSamples)) == PICO_OK)
	{
		MEMBUF_Scope_Counter = NSamples;

		sprintf_s(message, 256, "NSamplies: %d; TSamplies: %.f", NSamples, NSamples * SAMPLING_TIME_FRACTION);
		InfoPrint(IP_Info, message);

		if ((status = SAMPLER_GetValues(&MEMBUF_Scope_Counter)) == PICO_OK)
		{
			if ((status = SAMPLER_Stop()) == PICO_OK)
			{
				// Convert to current
				uint16_t DToffset = REG_I_3_P2 + (SAMPLER_GetSavedIRange() - 3) * 3;
				float P2_I = (float)(int16_t)DataTable[DToffset] / 1e6f;
				float P1_I = (float)DataTable[DToffset + 1] / 1e3f;
				float P0_I = (float)(int16_t)DataTable[DToffset + 2];

				// Diagnostic output
				sprintf_s(message, 256, "Shunt, mOhm: %.3f; Range: %d; Range-K: %.2f; P2: %.6f; P1: %.3f; P0: %.1f", ShuntResCache, SAMPLER_GetSavedIRange(), SAMPLER_GetIRangeCoeff(), P2_I, P1_I, P0_I);
				InfoPrint(IP_Info, message);

				if (InvertCurrent)
				{
					for (i = 0; i < MEMBUF_Scope_Counter; ++i)
						MEMBUF_ScopeI[i] = -MEMBUF_ScopeI[i];
				}
				
				for (i = 0; i < MEMBUF_Scope_Counter; ++i)
				{
					float ScopeI = (SAMPLER_GetIRangeCoeff() * MEMBUF_ScopeI[i]) / (INT16_MAX * ShuntResCache * 0.001f);
					MEMBUF_fScopeI[i] = ScopeI * ScopeI * P2_I + ScopeI * P1_I + P0_I;
				}

				FIR_Apply(MEMBUF_fScopeI, MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter);
				SPLINE_Apply(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter);

				// Convert to voltage
				float Kvoltage = (float)DataTable[REG_VOLTAGE_DIV_N] / DataTable[REG_VOLTAGE_DIV_D];

				DToffset = REG_U_5_P2 + (SAMPLER_GetSavedVRange() - 5) * 3;
				float P2_U = (float)(int16_t)DataTable[DToffset] / 1e6f;
				float P1_U = (float)DataTable[DToffset + 1] / 1e3f;
				float P0_U = (float)(int16_t)DataTable[DToffset + 2];

				// Diagnostic output
				sprintf_s(message, 256, "Voltage range: %d; Range-K: %.2f; P2: %.6f; P1: %.3f; P0: %.1f", SAMPLER_GetSavedVRange(), SAMPLER_GetVRangeCoeff(), P2_U, P1_U, P0_U);
				InfoPrint(IP_Info, message);

				if (!SCOPE_CURRENT_ONLY)
				{
					for (i = 0; i < MEMBUF_Scope_Counter; ++i)
					{
						float ScopeU = (SAMPLER_GetVRangeCoeff() * MEMBUF_ScopeV[i]) / (Kvoltage * INT16_MAX);
						MEMBUF_fScopeV[i] = ScopeU * ScopeU * P2_U + ScopeU * P1_U + P0_U;
					}

					FIR_Apply(MEMBUF_fScopeV, MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter);
					SPLINE_Apply(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter);
				}

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

					Corr_trr_P2 = (float)(int16_t)DataTable[REG_TRR_P2] / 1e11f;
					Corr_trr_P1 = (float)(int16_t)DataTable[REG_TRR_P1] / 1e4f;
					Corr_trr_P0 = (float)(int16_t)DataTable[REG_TRR_P0] / 1e4f;
					Index_0_trr = (float)(Index_trr - Index_0);

					sprintf_s(message, 256, "Index_0_trr: %f", Index_0_trr);
					InfoPrint(IP_Info, message);

					sprintf_s(message, 256, "Index trr: %d", Index_trr);
					InfoPrint(IP_Info, message);

					sprintf_s(message, 256, "Correct Trr : P2: %.11f ; P1: %.4f ; P0: %.4f", Corr_trr_P2, Corr_trr_P1, Corr_trr_P0);
					InfoPrint(IP_Info, message);
					
					Corr_trr = (float)(Index_0_trr * Index_0_trr * Corr_trr_P2 + Index_0_trr * Corr_trr_P1 + Corr_trr_P0) / (Index_0_trr * SAMPLING_TIME_FRACTION);
					
					sprintf_s(message, 256, "Corr_trr: %.6f", Corr_trr);
					InfoPrint(IP_Info, message);

					*Time09 = (float)(Index_09 - Index_0) * SAMPLING_TIME_FRACTION * Corr_trr;

					sprintf_s(message, 256, "Time 0_90: %.3f ms", *Time09);
					InfoPrint(IP_Info, message);
				
					if (trr) *trr = SAMPLING_TIME_FRACTION * Corr_trr * ((Index_trr > Index_0) ? (Index_trr - Index_0) : 0);
					if (Qrr) *Qrr = (float)fabs(CALC_Qrr(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, Index_0, Index_trr, SAMPLING_TIME_FRACTION * Corr_trr));

					uint16_t Index_0_ts = Index_irr - Index_0;
					if (ts_time) *ts_time = Index_0_ts * SAMPLING_TIME_FRACTION * Corr_trr;

					uint16_t tf = Index_0_trr - Index_0_ts;
					if (tf_time) *tf_time = tf * SAMPLING_TIME_FRACTION * Corr_trr;

					sprintf_s(message, 256, "ts_time: %.3f ms; tf_time: %.3f ms", ts_time, tf_time);
					InfoPrint(IP_Info, message);

					// Calculate actual dIdt
					if (!CALC_dIdt(MEMBUF_fScopeIFiltered, Index_0, Index_irr, SAMPLING_TIME_FRACTION, &Actual_dIdt))
						throw PROBLEM_CALC_DIDT;

					if (dIdt) *dIdt = Actual_dIdt;

					sprintf_s(message, 256, "Actual dIdt: %.2f", Actual_dIdt);
					InfoPrint(IP_Info, message);

					// Calculate Id
					*Id = CALC_Id(MEMBUF_fScopeIFiltered, Index_0);

					sprintf_s(message, 256, "Idc: %.1f", *Id);
					InfoPrint(IP_Info, message);

					// Calculate voltage zero crossing
					if (UseVoltage && !SCOPE_CURRENT_ONLY)
					{
						bool ZeroCrossingCalcOK = CALC_OSVZeroCrossing(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter, &Index_0V, Vd);
						sprintf_s(message, 256, "Vd: %.1f", *Vd);
						InfoPrint(IP_Info, message);

						if (!ZeroCrossingCalcOK)
							throw PROBLEM_CALC_VZ;

						sprintf_s(message, 256, "Index V0: %d", Index_0V);
						InfoPrint(IP_Info, message);
					}
					else
						Index_0V = 0;
					if (Index0V) *Index0V = Index_0V;

					// Calculate reverse voltage amplitude
					if (UseVoltage && !SCOPE_CURRENT_ONLY)
					{
						*RevVolt = Calc_RevVolt(MEMBUF_fScopeVFiltered, Index_0, Index_0V);

						sprintf_s(message, 256, "Reverse Voltage Amplitude: %.1f", *RevVolt);
						InfoPrint(IP_Info, message);
					}
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

uint16_t LOGIC_GetXData(float* SrcBuffer, uint16_t* Buffer, uint16_t BufferSize, bool CalcOK,
	uint32_t Index0, uint32_t MulFactor, uint32_t ForceSectorRead, uint16_t* SampleTimeSteps, float OutMulFactor)
{
	uint16_t Counter, dsRatio, i = 0;
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
		Buffer[i] = (uint16_t)((int16_t)(SrcBuffer[i * dsRatio] * OutMulFactor));

	if (SampleTimeSteps)
		*SampleTimeSteps = dsRatio;

	return i;
}
// ----------------------------------------

uint16_t LOGIC_GetIData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK,
	bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead, uint16_t* SampleTimeSteps)
{
	return LOGIC_GetXData(MEMBUF_fScopeIFiltered, Buffer, BufferSize, CalcOK,
		ModeQrr ? Index0 : Index0V, ModeQrr ? MUL_FACTOR_I : MUL_FACTOR_V,
		ForceSectorRead, SampleTimeSteps, EP_CURRENT_MUL);
}
// ----------------------------------------

uint16_t LOGIC_GetVData(uint16_t* Buffer, uint16_t BufferSize, bool CalcOK,
	bool ModeQrr, uint32_t Index0, uint32_t Index0V, uint32_t ForceSectorRead, uint16_t* SampleTimeSteps)
{
	return LOGIC_GetXData(MEMBUF_fScopeVFiltered, Buffer, BufferSize, CalcOK,
		ModeQrr ? Index0 : Index0V, ModeQrr ? MUL_FACTOR_I : MUL_FACTOR_V,
		ForceSectorRead, SampleTimeSteps, EP_VOLTAGE_MUL);
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

uint32_t LOGIC_GetSamplingSamples()

{
	uint32_t n_const = (uint32_t)(SAMPLING_T_CONST / SAMPLING_TIME_FRACTION);
	float t_fall = (DataTable[REG_DC_FALL_TIME] > 0) ? (float)DataTable[REG_DC_FALL_TIME] : 0.0f;
	uint32_t n_fall = (uint32_t)(t_fall / SAMPLING_TIME_FRACTION);
	return n_const + n_fall;
}
// ----------------------------------------

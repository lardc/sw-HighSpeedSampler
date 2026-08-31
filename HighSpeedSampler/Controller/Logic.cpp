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
	float Vmax = fabsf(SAMPLING_QRR_VR) * 2;
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

PICO_STATUS LOGIC_HandleSamplerData(uint16_t* CalcProblem, uint32_t* Index0, float* Irr, float* trr, float* Qrr,
	float* dIdt, float* Id, float* Vd, bool UseVoltage, bool UseTrr050Method, uint32_t* Index0V, float* Time09, float* trs, float* trf, float* Vr_min, uint16_t SetVd, bool* DutTrig)
{
	char message[256];

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

		sprintf_s(message, 256, "Number of samplies: %d; sampling time: %.3f", NSamples, NSamples * SAMPLING_TIME_FRACTION);
		InfoPrint(IP_Info, message);

		if ((status = SAMPLER_GetValues(&MEMBUF_Scope_Counter)) == PICO_OK)
		{
			if ((status = SAMPLER_Stop()) == PICO_OK)
			{
				// Convert to current
				float P2_I = 0.0f, P1_I = 1.0f, P0_I = 0.0f;
				PS5000A_RANGE IRange = SAMPLER_GetSavedIRange();
				if (IRange >= PS5000A_100MV && IRange <= PS5000A_5V)
				{
					uint16_t DToffset = REG_I_3_P2 + (IRange - PS5000A_100MV) * 3;
					P2_I = (float)(int16_t)DataTable[DToffset] / 1e6f;
					P1_I = (float)DataTable[DToffset + 1] / 1e3f;
					P0_I = (float)(int16_t)DataTable[DToffset + 2];
				}

				// Diagnostic output
				sprintf_s(message, 256, "Shunt, mOhm: %.3f; Range: %d; Range-K: %.2f; P2: %.6f; P1: %.3f; P0: %.1f",
					ShuntResCache, IRange, SAMPLER_GetIRangeCoeff(), P2_I, P1_I, P0_I);
				InfoPrint(IP_Info, message);

				if (InvertCurrent)
				{
					for (uint32_t i = 0; i < MEMBUF_Scope_Counter; ++i)
						MEMBUF_ScopeI[i] = -MEMBUF_ScopeI[i];
				}
				
				for (uint32_t i = 0; i < MEMBUF_Scope_Counter; ++i)
				{
					float ScopeI = (SAMPLER_GetIRangeCoeff() * MEMBUF_ScopeI[i]) / (INT16_MAX * ShuntResCache * 0.001f);
					MEMBUF_fScopeI[i] = ScopeI * ScopeI * P2_I + ScopeI * P1_I + P0_I;
				}

				FIR_Apply(MEMBUF_fScopeI, MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter);
				SPLINE_Apply(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter);

				// Convert to voltage
				float Kvoltage = (float)DataTable[REG_VOLTAGE_DIV_N] / DataTable[REG_VOLTAGE_DIV_D];

				float P2_U = 0.0f, P1_U = 1.0f, P0_U = 0.0f;
				PS5000A_RANGE VRange = SAMPLER_GetSavedVRange();
				if (VRange >= PS5000A_500MV && VRange <= PS5000A_10V)
				{
					uint16_t DToffset = REG_U_5_P2 + (VRange - PS5000A_500MV) * 3;
					P2_U = (float)(int16_t)DataTable[DToffset] / 1e6f;
					P1_U = (float)DataTable[DToffset + 1] / 1e3f;
					P0_U = (float)(int16_t)DataTable[DToffset + 2];
				}

				// Diagnostic output
				sprintf_s(message, 256, "Voltage range: %d; Range-K: %.2f; P2: %.6f; P1: %.3f; P0: %.1f",
					VRange, SAMPLER_GetVRangeCoeff(), P2_U, P1_U, P0_U);
				InfoPrint(IP_Info, message);

				if (!SCOPE_CURRENT_ONLY)
				{
					for (uint32_t i = 0; i < MEMBUF_Scope_Counter; ++i)
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
					uint32_t Index_0 = 0, Index_irr = 0;
					if (!CALC_IrrAndZeroCrossingIndex(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, &Index_0, &Index_irr))
						throw PROBLEM_CALC_IRR;

					if (Index0) *Index0 = Index_0;
					if (Irr) *Irr = fabsf(MEMBUF_fScopeIFiltered[Index_irr]);

					sprintf_s(message, 256, "Index 0: %d; Index Irr: %d", Index_0, Index_irr);
					InfoPrint(IP_Info, message);

					// Calculate Irr pivot points
					uint32_t Index_025 = 0;
					if (!CALC_IrrFractionIndex(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, Index_irr,
						UseTrr050Method ? 0.5f : 0.25f, &Index_025))
						throw PROBLEM_CALC_IRR_025;

					sprintf_s(message, 256, "Index Irr_low (0.25): %d", Index_025);
					InfoPrint(IP_Info, message);

					uint32_t Index_09 = 0;
					if (!CALC_IrrFractionIndex(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, Index_irr, 0.9f, &Index_09))
						throw PROBLEM_CALC_IRR_090;

					sprintf_s(message, 256, "Index Irr_high (0.9): %d", Index_09);
					InfoPrint(IP_Info, message);

					// Calculate trr and fixing sampling time
					uint32_t Index_trr = CALC_trrIndex(MEMBUF_fScopeIFiltered[Index_025], MEMBUF_fScopeIFiltered[Index_09],
						Index_025, Index_09);

					uint32_t trr_ticks = (Index_trr > Index_0) ? Index_trr - Index_0 : 0;
					if (trr_ticks == 0)
						throw PROBLEM_CALC_TRR;

					float trr_P2 = (float)(int16_t)DataTable[REG_TRR_P2] / 1e6f;
					float trr_P1 = (float)DataTable[REG_TRR_P1] / 1e3f;
					float trr_P0 = (float)(int16_t)DataTable[REG_TRR_P0];
					sprintf_s(message, 256, "trr fine tune P2: %e, P1: %e, P0: %e", trr_P2, trr_P1, trr_P0);
					InfoPrint(IP_Info, message);

					float trr_raw = SAMPLING_TIME_FRACTION * trr_ticks;
					float trr_tuned = trr_raw * trr_raw * trr_P2 + trr_raw * trr_P1 + trr_P0;
					if (trr_tuned <= 0.0f)
						throw PROBLEM_CALC_TRR;

					if (trr) *trr = trr_tuned;

					float TunedSamplingTimeFraction = trr_tuned / trr_ticks;

					sprintf_s(message, 256, "Index trr: %d, trr ticks: %d, trr_raw: %.3f, trr_tuned: %.3f, tuned time fraction: %.6f",
						Index_trr, trr_ticks, trr_raw, trr_tuned, TunedSamplingTimeFraction);
					InfoPrint(IP_Info, message);

					// Calculate time from zero crossing to 0.9 irr from rising side
					if (Time09)
					{
						*Time09 = TunedSamplingTimeFraction * (Index_09 - Index_0);
						sprintf_s(message, 256, "Time 0.90: %.3f", *Time09);
						InfoPrint(IP_Info, message);
					}

					// Calculate trs, trf
					float _trs = TunedSamplingTimeFraction * (Index_irr - Index_0);
					float _trf = trr_tuned - _trs;
					sprintf_s(message, 256, "trs: %.3f, trf: %.3f", _trs, _trf);
					InfoPrint(IP_Info, message);
					if (trs) *trs = _trs;
					if (trf) *trf = _trf;
				
					// Calculate Qrr
					float _Qrr = CALC_Qrr(MEMBUF_fScopeIFiltered, MEMBUF_Scope_Counter, Index_0, Index_trr, TunedSamplingTimeFraction);
					if (_Qrr <= 0.0f)
						throw PROBLEM_CALC_QRR;
					sprintf_s(message, 256, "Qrr: %.2f", _Qrr);
					InfoPrint(IP_Info, message);
					if (Qrr) *Qrr = _Qrr;

					// Calculate actual dIdt (unfixed sampling time is used)
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
					uint32_t Index_0V = 0, Index_Vd = 0;
					if (UseVoltage && !SCOPE_CURRENT_ONLY)
					{
						bool ZeroCrossingCalcOK = CALC_OSVZeroCrossing(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter, &Index_0V, Vd, &Index_Vd);

						sprintf_s(message, 256, "SetVd: %u", SetVd);
						InfoPrint(IP_Info, message);

						sprintf_s(message, 256, "Vd: %.1f", *Vd);
						InfoPrint(IP_Info, message);

						if (!ZeroCrossingCalcOK)
							throw PROBLEM_CALC_VZ;

						if (DutTrig)
							*DutTrig = CALC_DUTTrig(MEMBUF_fScopeVFiltered, MEMBUF_Scope_Counter, Index_Vd, SetVd,
								DataTable[REG_FLATTOP_DUT_US], (float)DataTable[REG_FLATTOP_DUT_HYST] / 1000.0f);

						sprintf_s(message, 256, "Index V0: %d", Index_0V);
						InfoPrint(IP_Info, message);
					}
					if (Index0V) *Index0V = Index_0V;

					// Calculate reverse voltage amplitude
					if (UseVoltage && !SCOPE_CURRENT_ONLY)
					{
						if (!CALC_Vr_min(MEMBUF_fScopeVFiltered, Index_0, Index_0V, Vr_min))
							throw PROBLEM_CALC_VR_MIN;

						sprintf_s(message, 256, "Vr_min: %.1f", *Vr_min);
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
	float t_fall = (DataTable[REG_DC_FALL_TIME] > 0) ? (float)DataTable[REG_DC_FALL_TIME] : 0.0f;
	uint32_t samples = (uint32_t)((SAMPLING_T_CONST + t_fall) / SAMPLING_TIME_FRACTION);
	if (samples > (uint32_t)SAMPLING_SAMPLES)
		samples = (uint32_t)SAMPLING_SAMPLES;
	return samples;
}
// ----------------------------------------

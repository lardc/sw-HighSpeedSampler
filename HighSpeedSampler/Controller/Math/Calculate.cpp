// Headers
//
#include "stdafx.h"
#include "Calculate.h"

// Includes
//
#include <math.h>
#include "Controller\Global.h"

// Functions
//
bool CALC_IrrAndZeroCrossingIndex(float* Buffer, uint32_t BufferLength, uint32_t* CrossingIndex, uint32_t* IrrIndex)
{
	uint32_t i, Imin_index;
	float Imin;

	// Find zero crossing index
	for (i = 0; i < BufferLength; ++i)
	{
		if (Buffer[i] < 0)
		{
			*CrossingIndex = i;
			break;
		}
	}

	// Find reverse current
	Imin = Buffer[*CrossingIndex];
	Imin_index = *CrossingIndex;

	for (i = *CrossingIndex; (i < BufferLength) && (i < ((*CrossingIndex) * 3)); ++i)
	{
		if (Buffer[i] < Imin)
		{
			Imin = Buffer[i];
			Imin_index = i;
		}
		else if ((i - Imin_index) > IRR_SEARCH_WND)
			break;
	}

	if (Imin >= 0)
		return false;
	else
	{
		*IrrIndex = Imin_index;
		return true;
	}
}
//----------------------------------------------

bool CALC_IrrFractionIndex(float* Buffer, uint32_t BufferLength, uint32_t IrrIndex, float IrrFraction, uint32_t* IrrFractionIndex)
{
	uint32_t i;

	for (i = IrrIndex; i < BufferLength; ++i)
	{
		if (Buffer[i] > (Buffer[IrrIndex] * IrrFraction))
		{
			*IrrFractionIndex = i;
			return true;
		}
	}

	return false;
}
//----------------------------------------------

uint32_t CALC_trrIndex(float I1, float I2, int x1, int x2)
{
	float k, b;

	k = (I1 - I2) / (x1 - x2);
	b = I1 - k * x1;

	return (uint32_t)fabs(b / k);
}
//----------------------------------------------

float CALC_Qrr(float* Buffer, uint32_t BufferLength, uint32_t t0, uint32_t trr, float TimeFraction)
{
	uint32_t i;
	double Qrr = 0;

	if (t0 >= (BufferLength - 1) || trr >= BufferLength || trr < 1)
		return 0;

	for (i = t0 + 1; i <= trr - 1; ++i)
		Qrr += Buffer[i];
	Qrr += (Buffer[t0] + Buffer[trr]) * 0.5f;

	return (float)(TimeFraction * Qrr);
}
//----------------------------------------------

bool CALC_dIdt(float* Buffer, uint32_t t0, uint32_t trr, float TimeFraction, float* dIdt)
{
	uint32_t i, Id_half, Ir_half;
	float Idc;

	// Find Id_max
	Idc = Buffer[0];
	for (i = 1; i < t0; ++i)
		if (Buffer[i] > Idc) Idc = Buffer[i];

	// Find Id_half
	for (i = 0; i < t0; ++i)
	{
		if (Buffer[i] <= (Idc / 2))
		{
			Id_half = i;
			break;
		}
	}
	if (i == t0) return false;

	// Find Ir_half
	for (i = t0; i < trr; ++i)
	{
		if (Buffer[i] <= (Buffer[trr] / 2))
		{
			Ir_half = i;
			break;
		}
	}
	if (i == trr) return false;

	// Calculate dIdt
	*dIdt = (Buffer[Id_half] - Buffer[Ir_half]) / ((Ir_half - Id_half) * TimeFraction);
	return true;
}
//----------------------------------------------

bool CALC_OSVZeroCrossing(float* Buffer, uint32_t BufferLength, uint32_t* CrossingIndex)
{
	uint32_t i, index = BufferLength;
	bool StartCrossingSearch = false, CalcOK = false;

	for (i = 0; i < BufferLength; ++i)
	{
		if (StartCrossingSearch)
		{
			if (Buffer[i] > OSV_RISE_DETECT_V)
			{
				CalcOK = true;
				*CrossingIndex = i;
				break;
			}
		}
		else if (Buffer[i] < OSV_FALL_DETECT_V)
			StartCrossingSearch = true;
	}

	return CalcOK;
}
//----------------------------------------------

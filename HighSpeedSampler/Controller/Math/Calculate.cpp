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
	float Id;

	// Find Id_max
	Id = CALC_Id(Buffer, t0);

	// Find Id_half
	for (i = 0; i < t0; ++i)
	{
		if (Buffer[i] <= (Id / 2))
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

float CALC_Id(float* Buffer, uint32_t t0)
{
	uint32_t i;
	float Id;

	// Find Id_max
	Id = Buffer[0];
	for (i = 1; i < t0; ++i)
		if (Buffer[i] > Id) Id = Buffer[i];

	return Id;
}
//----------------------------------------------

bool CALC_OSVZeroCrossing(float* Buffer, uint32_t BufferLength, uint32_t* CrossingIndex, float* Vd)
{
	int32_t i, Vdmax_index = 0;

	// Find Vd max
	float Vdmax = Buffer[0];
	for (i = 1; i < (int32_t)BufferLength; ++i)
		if (Buffer[i] > Vdmax)
		{
			Vdmax = Buffer[i];
			Vdmax_index = i;
		}
	*Vd = Vdmax;

	if (Vdmax > OSV_PEAK_DETECT_V && Vdmax_index != 0)
	{
		for (i = Vdmax_index; i >= 0; --i)
			if (Buffer[i] < 0)
			{
				*CrossingIndex = i;
				return true;
			}
	}
	
	return false;
}
//----------------------------------------------

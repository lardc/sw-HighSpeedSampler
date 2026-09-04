// Headers
//
#include "stdafx.h"
#include "SavitzkyGolay.h"

// Definitions
//
#define SG_LENGTH	21
#define SG_HALF		(SG_LENGTH >> 1)

// Variables
//
// Savitzky–Golay smoothing
// Window:		21 samples
// Polynomial:	3
// Derivative:	0 (smoothing)
// Recalc:		python sg_coeff.py
const float SG_COEFF[SG_LENGTH] = {
	-5.59006211e-02f, -2.48447205e-02f,  2.94213795e-03f,  2.74599542e-02f,
	 4.87087283e-02f,  6.66884603e-02f,  8.13991500e-02f,  9.28407976e-02f,
	 1.01013403e-01f,  1.05916966e-01f,  1.07551487e-01f,  1.05916966e-01f,
	 1.01013403e-01f,  9.28407976e-02f,  8.13991500e-02f,  6.66884603e-02f,
	 4.87087283e-02f,  2.74599542e-02f,  2.94213795e-03f, -2.48447205e-02f,
	-5.59006211e-02f
};

// Functions
//
void SG_Apply(float* InputBuffer, float* OutputBuffer, int BufferLength)
{
	int i, j;
	float tmp;

	if (BufferLength < SG_LENGTH)
	{
		for (i = 0; i < BufferLength; ++i)
			OutputBuffer[i] = InputBuffer[i];
		return;
	}

	for (i = 0; i <= BufferLength - SG_LENGTH; ++i)
	{
		tmp = 0.0f;
		for (j = 0; j < SG_LENGTH; ++j)
			tmp += InputBuffer[i + j] * SG_COEFF[j];
		OutputBuffer[i + SG_HALF] = tmp;
	}

	for (i = 0; i < SG_HALF; ++i)
	{
		OutputBuffer[i] = InputBuffer[i];
		OutputBuffer[BufferLength - i - 1] = InputBuffer[BufferLength - i - 1];
	}
}
//----------------------------------------------

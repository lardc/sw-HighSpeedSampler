// Headers
//
#include "stdafx.h"
#include "FIR.h"

// Definitions
//
#define FIR_LENGTH	101

// Variables
//
// FIR
// Sampling frequency:	125 Mhz
// Cut-off frequency:	10 Mhz
// Order:				100
// Rp  = 0.00057565
// Rst = 1e-4
const double FIR[FIR_LENGTH] = {-1.17151865e-004, -1.68374093e-004, -2.07613478e-004, -1.66886221e-004, -1.85597488e-005, 2.20148972e-004, 4.74975320e-004, 6.28631027e-004, 5.63863447e-004, 2.23596453e-004, -3.37167967e-004, -9.37378494e-004, -1.31387251e-003, -1.22021505e-003, -5.49053836e-004, 5.74362913e-004, 1.78092515e-003, 2.55738865e-003, 2.44071328e-003, 1.24167627e-003, -7.97966095e-004, -3.00351510e-003, -4.45951390e-003, -4.35578591e-003, -2.36825915e-003, 1.08679455e-003, 4.86551682e-003, 7.42881262e-003, 7.41785024e-003, 4.27480619e-003, -1.34843032e-003, -7.61090751e-003, -1.20097323e-002, -1.22922379e-002, -7.44843884e-003, 1.61016442e-003, 1.20223797e-002, 1.97338803e-002, 2.08839022e-002, 1.33952990e-002, -1.80495459e-003, -2.04545446e-002, -3.57769765e-002, -4.03319220e-002, -2.83863588e-002, 1.94347576e-003, 4.75261873e-002, 1.00506010e-001, 1.50078109e-001, 1.85285632e-001, 1.98017079e-001, 1.85285632e-001, 1.50078109e-001, 1.00506010e-001, 4.75261873e-002, 1.94347576e-003, -2.83863588e-002, -4.03319220e-002, -3.57769765e-002, -2.04545446e-002, -1.80495459e-003, 1.33952990e-002, 2.08839022e-002, 1.97338803e-002, 1.20223797e-002, 1.61016442e-003, -7.44843884e-003, -1.22922379e-002, -1.20097323e-002, -7.61090751e-003, -1.34843032e-003, 4.27480619e-003, 7.41785024e-003, 7.42881262e-003, 4.86551682e-003, 1.08679455e-003, -2.36825915e-003, -4.35578591e-003, -4.45951390e-003, -3.00351510e-003, -7.97966095e-004, 1.24167627e-003, 2.44071328e-003, 2.55738865e-003, 1.78092515e-003, 5.74362913e-004, -5.49053836e-004, -1.22021505e-003, -1.31387251e-003, -9.37378494e-004, -3.37167967e-004, 2.23596453e-004, 5.63863447e-004, 6.28631027e-004, 4.74975320e-004, 2.20148972e-004, -1.85597488e-005, -1.66886221e-004, -2.07613478e-004, -1.68374093e-004, -1.17151865e-004};

// Functions
//
void FIR_Apply(float* InputBuffer, float* OutputBuffer, int BufferLength)
{
	int i, j;
	double tmp;

	// Position in output
	for (i = 0; i <= BufferLength - FIR_LENGTH; ++i) 
	{
		tmp = 0;

		// Position in coefficients array
		for (j = 0; j < FIR_LENGTH; ++j)
			tmp += (double)InputBuffer[i + j] * FIR[j];

		OutputBuffer[i + (FIR_LENGTH >> 1)] = (float)tmp;
	}

	// Copy unfiltered values
	for (i = 0; i < FIR_LENGTH >> 1; ++i)
	{
		OutputBuffer[i] = InputBuffer[i];
		OutputBuffer[BufferLength - i - 1] = InputBuffer[BufferLength - i - 1];
	}

	for (int i = 0; i < BufferLength; i++)
		OutputBuffer[i] = InputBuffer[i];
}
//----------------------------------------------

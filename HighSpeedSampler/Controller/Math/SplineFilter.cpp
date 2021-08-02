// Headers
//
#include "stdafx.h"
#include "SplineFilter.h"
#include "Controller\Global.h"
#include "Math.h"

// Definitions
//
#define SF_FILTER_POINTS		5
#define SF_FILTER_FACTOR		0.5

// Variables
//
float AvgData[SAMPLING_SAMPLES];


// Functions
//
void SPLINE_Apply(float* InputBuffer, int BufferLength)
{
	float AvgPoint = 0;
	int SF_FilterPoint2 = (int)pow(SF_FILTER_POINTS, 2);

	// Avg filtering
	for (int i = 0; i < (BufferLength - SF_FilterPoint2); ++i)
	{
		AvgPoint = 0;

		for (int j = i; j < (i + SF_FilterPoint2); j += SF_FILTER_POINTS)
			AvgPoint += *(InputBuffer + j);
		AvgData[i] = AvgPoint / SF_FILTER_POINTS;
	}

	// Spline filtering
	for (int i = 0; i < (BufferLength - SF_FilterPoint2 - 3); ++i)
	{
		*(InputBuffer + i) = (float)(pow(1 - SF_FILTER_FACTOR, 3) * AvgData[i] +
			3 * SF_FILTER_FACTOR * pow(1 - SF_FILTER_FACTOR, 2) * AvgData[i + 1] +
			3 * pow(SF_FILTER_FACTOR, 2) * (1 - SF_FILTER_FACTOR) * AvgData[i + 2] +
			pow(SF_FILTER_FACTOR, 3) * AvgData[i + 3]);
	}
}
//----------------------------------------------

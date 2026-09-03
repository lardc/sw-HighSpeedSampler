#ifndef __CALCULATE_H__
#define __CALCULATE_H__

// Includes
//
#include <stdint.h>

// Functions
//
// Find reverse current and zero-crossing index
bool CALC_IrrAndZeroCrossingIndex(float* Buffer, uint32_t BufferLength, uint32_t* CrossingIndex, uint32_t* IrrIndex);
// Find indexes of current fractions on current rise
bool CALC_IrrFractionIndex(float* Buffer, uint32_t BufferLength, uint32_t IrrIndex, float IrrFraction, uint32_t* IrrFractionIndex);
// Calculate trr index
uint32_t CALC_trrIndex(float I1, float I2, int x1, int x2);
// Calculate Qrr
float CALC_Qrr(float* Buffer, uint32_t BufferLength, uint32_t t0, uint32_t trr, float TimeFraction);
// Calculate on-state voltage crossing index
bool CALC_OSVZeroCrossing(float* Buffer, uint32_t BufferLength, uint32_t* CrossingIndex, float* Vd, uint32_t* VdIndex);
// Check DUT open (FlatTop hold)
bool CALC_DUTTrig(float* Buffer, uint32_t BufferLength, uint32_t Index_0V, uint16_t SetVd, uint16_t FlatTopUs, float FlatTopHyst,
	bool* Result);
// Calculate actual dIdt
bool CALC_dIdt(float* Buffer, uint32_t t0, uint32_t trr, float TimeFraction, float* dIdt);
// Calculate Id
float CALC_Id(float* Buffer, uint32_t t0);
// Calculate Vr_min
void CALC_Vr_min(float* Buffer, uint32_t CrossingIndI, uint32_t CrossingIndV, float* Vr_min);

#endif	// __CALCULATE_H__

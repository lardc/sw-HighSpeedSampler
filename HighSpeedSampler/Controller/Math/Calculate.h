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
bool CALC_OSVZeroCrossing(float* Buffer, uint32_t BufferLength, uint32_t* CrossingIndex);
// Calculate actual dIdt
bool CALC_dIdt(float* Buffer, uint32_t t0, uint32_t trr, float TimeFraction, float* dIdt);
// Calculate Id
float CALC_Id(float* Buffer, uint32_t t0);
// Calculate Vd
float CALC_Vd(float* Buffer, uint32_t BufferLength);

#endif	// __CALCULATE_H__

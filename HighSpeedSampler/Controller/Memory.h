#ifndef __MEMORY_H__
#define __MEMORY_H__

// Includes
//
#include <stdint.h>

// Functions
//
void MEMORY_Read(uint16_t dummy, uint16_t* Buffer, uint16_t BufferSize);
void MEMORY_Write(uint16_t dummy, uint16_t* Buffer, uint16_t BufferSize);

#endif	// __MEMORY_H__

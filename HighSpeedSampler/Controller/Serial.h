#ifndef __SERIAL_H__
#define __SERIAL_H__

// Includes
//
#include <stdint.h>
#include <Windows.h>

// Functions
//
bool SERIAL_Init(uint8_t PortNum, DWORD PortBR);
void SERIAL_Purge();
void SERIAL_UpdateReadBuffer();
//
void SERIAL_SendArray16(uint16_t* Buffer, uint16_t BufferSize);
uint16_t SERIAL_ReceiveChar();
void SERIAL_ReceiveArray16(uint16_t* Buffer, uint16_t BufferSize);
uint16_t SERIAL_GetBytesToReceive();

#endif	// __SERIAL_H__

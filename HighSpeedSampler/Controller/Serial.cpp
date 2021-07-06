// Headers
//
#include "stdafx.h"
#include "Serial.h"

// Includes
//
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include "Global.h"

// Variables
//
static HANDLE hSerial = NULL;
static uint8_t SerialReadBuffer[SERIAL_BUFFER_LEN], SerialReadBufferCounter = 0;

// Functions
//
bool SERIAL_Init(uint8_t PortNum, DWORD PortBR)
{
	TCHAR tPort[16] = _T("");
	_stprintf_s(tPort, _T("\\\\.\\COM%d"), PortNum);
			
	// create handle to port
	hSerial = CreateFile(tPort,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	try
	{
		if (hSerial == INVALID_HANDLE_VALUE)
			throw 0;

		// set basic parameters
		DCB dcbSerial = {0};
		dcbSerial.DCBlength = sizeof(dcbSerial);
				
		if (!GetCommState(hSerial, &dcbSerial))
			throw 0;

		dcbSerial.BaudRate = PortBR;
		dcbSerial.ByteSize = 8;
		dcbSerial.StopBits = ONESTOPBIT;
		dcbSerial.Parity = NOPARITY;

		if (!SetCommState(hSerial, &dcbSerial))
			throw 0;

		// set timeouts
		COMMTIMEOUTS timeouts = {0};

		timeouts.ReadIntervalTimeout = MAXDWORD;

		timeouts.ReadTotalTimeoutConstant = SERIAL_READ_TIMEOUT;
		timeouts.ReadTotalTimeoutMultiplier = 0;

		timeouts.WriteTotalTimeoutConstant = SERIAL_WRITE_TIMEOUT;
		timeouts.WriteTotalTimeoutMultiplier = 1;
				
		if (!SetCommTimeouts(hSerial, &timeouts))
			throw 0;

		// clear fifos
		PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
	}
	catch(...)
	{
		CloseHandle(hSerial);
		hSerial = NULL;
		return false;
	}

	return true;
}
//----------------------------------------------

void SERIAL_Purge()
{
	if (hSerial != NULL)
		PurgeComm(hSerial, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);
}
//----------------------------------------------

void SERIAL_UpdateReadBuffer()
{
	if (hSerial != NULL && SerialReadBufferCounter < SERIAL_BUFFER_LEN)
	{
		uint8_t Data;
		DWORD dwBytesRead = 0;
		do
		{
			ReadFile(hSerial, &Data, 1, &dwBytesRead, NULL);
			if (dwBytesRead == 1)
				SerialReadBuffer[SerialReadBufferCounter++] = Data;
			
		} while (dwBytesRead && SerialReadBufferCounter < SERIAL_BUFFER_LEN);
	}
}
//----------------------------------------------

void SERIAL_SendArray16(uint16_t* Buffer, uint16_t BufferSize)
{
	if (hSerial != NULL)
	{
		DWORD dwBytesWritten;
		uint8_t byte[2];
		uint16_t i;

		for (i = 0; i < BufferSize; ++i)
		{
			byte[0] = Buffer[i] >> 8;
			byte[1] = Buffer[i] & 0xFF;
			WriteFile(hSerial, byte, 2, &dwBytesWritten, NULL);
		}
	}
}
//----------------------------------------------

uint16_t SERIAL_ReceiveChar()
{
	uint8_t i, ret = 0;

	if (SerialReadBufferCounter)
	{
		ret = SerialReadBuffer[0];
		for (i = 1; i < SerialReadBufferCounter; ++i)
			SerialReadBuffer[i - 1] = SerialReadBuffer[i];
		--SerialReadBufferCounter;
	}

	return ret;
}
//----------------------------------------------

void SERIAL_ReceiveArray16(uint16_t* Buffer, uint16_t BufferSize)
{
	for (uint16_t i = 0; i < BufferSize; ++i)
	{
		Buffer[i] = SERIAL_ReceiveChar() << 8;
		Buffer[i] |= SERIAL_ReceiveChar();
	}
}
//----------------------------------------------

uint16_t SERIAL_GetBytesToReceive()
{
	return SerialReadBufferCounter;
}
//----------------------------------------------

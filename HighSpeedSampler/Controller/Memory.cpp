// Headers
//
#include "stdafx.h"
#include "Memory.h"

// Definitions
//
#define DATATABLE_FILE_NAME		"DataTable.bin"

// Includes
//
#include <stdio.h>

// Functions
//
void MEMORY_Read(uint16_t dummy, uint16_t* Buffer, uint16_t BufferSize)
{
	FILE *fPointer;

	if (fopen_s(&fPointer, DATATABLE_FILE_NAME, "r") == 0)
	{
		fread_s(Buffer, BufferSize * 2, 2, BufferSize, fPointer);
		fclose(fPointer);
	}
}
//----------------------------------------------

void MEMORY_Write(uint16_t dummy, uint16_t* Buffer, uint16_t BufferSize)
{
	FILE *fPointer;

	if (fopen_s(&fPointer, DATATABLE_FILE_NAME, "w") == 0)
	{
		fwrite(Buffer, 2, BufferSize, fPointer);
		fclose(fPointer);
	}
}
//----------------------------------------------

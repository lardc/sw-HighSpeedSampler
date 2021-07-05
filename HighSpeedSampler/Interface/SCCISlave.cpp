// -----------------------------------------
// SCCI-Slave communication interface
// ----------------------------------------

// Header
#include "stdafx.h"
#include "SCCISlave.h"
#include <string.h>
//
// Includes
#include "CRC16.h"

// Constants
//
enum DispID
{
	DISP_NONE			=	0,
	DISP_R_16			=	1,
	DISP_R_32			=	2,
	DISP_R_16_2			=	3,
	DISP_W_16			=	4,
	DISP_W_32			=	5,
	DISP_W_16_2			=	6,
	DISP_RB_16			=	7,
	DISP_RRB_16			=	8,
	DISP_WB_16			=	9,
	DISP_C				=	10,
	DISP_RBF_16			=	11,
	DISP_RRBF_16		= 	12,
	DISP_RB_32			=	13,
	DISP_RRB_32			=	14
};


// Forward functions
//
static void SCCI_DispatchHeader(pSCCI_Interface Interface);
static void SCCI_DispatchBody(pSCCI_Interface Interface, bool MaskStateChangeOperations);
static void SCCI_SendErrorFrame(pSCCI_Interface Interface, uint16_t ErrorCode, uint16_t Details);
static void SCCI_SendResponseFrame(pSCCI_Interface Interface, uint16_t FrameSize);
//
static void SCCI_HandleRead16(pSCCI_Interface Interface);
static void SCCI_HandleRead32(pSCCI_Interface Interface);
static void SCCI_HandleRead16Double(pSCCI_Interface Interface);
static void SCCI_HandleWrite16(pSCCI_Interface Interface);
static void SCCI_HandleWrite32(pSCCI_Interface Interface);
static void SCCI_HandleWrite16Double(pSCCI_Interface Interface);
static void SCCI_HandleReadBlock16(pSCCI_Interface Interface, bool Repeat);
static void SCCI_HandleWriteBlock16(pSCCI_Interface Interface);
static void SCCI_HandleReadBlockFast16(pSCCI_Interface Interface, bool Repeat);
static void SCCI_HandleCall(pSCCI_Interface Interface);


// Variables
//
static uint16_t ZeroBuffer[xCCI_BUFFER_SIZE] = {0};


// Functions
//
void SCCI_Init(pSCCI_Interface Interface, pSCCI_IOConfig IOConfig, pxCCI_ServiceConfig ServiceConfig,
 			   uint16_t* DataTable, uint16_t DataTableSize, uint32_t MessageTimeoutTicks,
 			   void *ArgumentForCallback16)
{
	uint16_t i;
	
	// Reset fields
	Interface->State = SCCI_STATE_WAIT_STARTBYTE;
	Interface->DispID = DISP_NONE;
	Interface->ExpectedBodyLength = 0;
	Interface->LastTimestampTicks = 0;
	Interface->ProtectionAndEndpoints.ProtectedAreasUsed = 0;
	
	for(i = 0; i < xCCI_BUFFER_SIZE; ++i)
		Interface->MessageBuffer[i] = 0;

	for(i = 0; i < xCCI_MAX_READ_ENDPOINTS + 1; ++i)
	{
		Interface->ProtectionAndEndpoints.ReadEndpoints16[i] = NULL;
		Interface->ProtectionAndEndpoints.ReadEndpoints32[i] = NULL;
	} 

	for(i = 0; i < xCCI_MAX_WRITE_ENDPOINTS + 1; ++i)
	{
		Interface->ProtectionAndEndpoints.WriteEndpoints16[i] = NULL;
		Interface->ProtectionAndEndpoints.WriteEndpoints32[i] = NULL;
	} 

	// Save parameters
	Interface->IOConfig = IOConfig;
	Interface->ServiceConfig = ServiceConfig;
	Interface->DataTableAddress = DataTable;
	Interface->DataTableSize = DataTableSize;
	Interface->TimeoutValueTicks = MessageTimeoutTicks;
	Interface->ArgForEPCallback1 = ArgumentForCallback16;
}
// ----------------------------------------

SCCI_States SCCI_Process(pSCCI_Interface Interface, uint64_t CurrentTickCount, bool MaskStateChangeOperations)
{
	switch(Interface->State)
	{
		case SCCI_STATE_WAIT_STARTBYTE:
			if(Interface->IOConfig->IO_GetBytesToReceive() > 0)
			{
				uint16_t startByte = Interface->IOConfig->IO_ReceiveByte();

				if(startByte == START_BYTE)
					Interface->State = SCCI_STATE_WAIT_HEADER;
			}
			break;
		case SCCI_STATE_WAIT_HEADER:
			if(Interface->IOConfig->IO_GetBytesToReceive() >= 3)
			{
				uint16_t nextByte = Interface->IOConfig->IO_ReceiveByte();
				Interface->MessageBuffer[0] = nextByte | (START_BYTE << 8);

				Interface->IOConfig->IO_ReceiveArray16(Interface->MessageBuffer + 1, 1);
				SCCI_DispatchHeader(Interface);

				if(Interface->State == SCCI_STATE_WAIT_BODY)
					Interface->LastTimestampTicks = CurrentTickCount + Interface->TimeoutValueTicks;
			}
			break;
		case SCCI_STATE_WAIT_BODY:
			if(Interface->IOConfig->IO_GetBytesToReceive() >= Interface->ExpectedBodyLength * 2)
			{
				Interface->IOConfig->IO_ReceiveArray16(Interface->MessageBuffer + 2, Interface->ExpectedBodyLength);
				SCCI_DispatchBody(Interface, MaskStateChangeOperations);
			}
			else if(Interface->TimeoutValueTicks && (CurrentTickCount > Interface->LastTimestampTicks))
				SCCI_SendErrorFrame(Interface, ERR_TIMEOUT, (uint16_t)(CurrentTickCount - Interface->LastTimestampTicks));
			break;
	}
	
	return Interface->State;
}
// ----------------------------------------

static void SCCI_DispatchHeader(pSCCI_Interface Interface)
{
	uint16_t fnc = Interface->MessageBuffer[1] >> 8;

	if((Interface->MessageBuffer[0] & 0xFF) == DEVICE_SCCI_ADDRESS)
	{
		switch((fnc & FUNCTION_CODE_MASK) >> 3)
		{
			case FUNCTION_WRITE:
				switch(fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 3;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_W_16;	
						break;
					case SFUNC_32:
						Interface->ExpectedBodyLength = 4;
						Interface->State = SCCI_STATE_WAIT_BODY;	
						Interface->DispID = DISP_W_32;	
						break;
					case SFUNC_16_2:
						Interface->ExpectedBodyLength = 5;
						Interface->State = SCCI_STATE_WAIT_BODY;	
						Interface->DispID = DISP_W_16_2;	
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_READ:
				switch(fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;	
						Interface->DispID = DISP_R_16;	
						break;
					case SFUNC_32:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;	
						Interface->DispID = DISP_R_32;	
						break;
					case SFUNC_16_2:
						Interface->ExpectedBodyLength = 3;
						Interface->State = SCCI_STATE_WAIT_BODY;	
						Interface->DispID = DISP_R_16_2;	
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_WRITE_BLOCK:
				switch(fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 5;
						Interface->State = SCCI_STATE_WAIT_BODY;	
						Interface->DispID = DISP_WB_16;	
						break;
					case SFUNC_32:
						SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, fnc & FUNCTION_SCODE_MASK);
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_READ_BLOCK:
				switch(fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;	
						Interface->DispID = DISP_RB_16;	
						break;
					case SFUNC_32:
						SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, fnc & FUNCTION_SCODE_MASK);
						break;
					case SFUNC_REP_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RRB_16;
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_CALL:
				if((fnc & FUNCTION_SCODE_MASK) == 0)
				{
					Interface->ExpectedBodyLength = 2;
					Interface->State = SCCI_STATE_WAIT_BODY;
					Interface->DispID = DISP_C;	
				}
				else
					SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
				break;
			case FUNCTION_FAST_READ_BLK:
				switch(fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RBF_16;
						break;
					case SFUNC_32:
						SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, fnc & FUNCTION_SCODE_MASK);
						break;
					case SFUNC_REP_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RRBF_16;
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			default:
				SCCI_SendErrorFrame(Interface, ERR_INVALID_FUNCTION, fnc & FUNCTION_CODE_MASK);
				break;
		}
	}
}
// ----------------------------------------

static void SCCI_DispatchBody(pSCCI_Interface Interface, bool MaskStateChangeOperations)
{
	uint16_t crc_calc, crc_in;
	crc_calc = CRC16_ComputeCRC(Interface->MessageBuffer, Interface->ExpectedBodyLength + 1);
	crc_in = Interface->MessageBuffer[Interface->ExpectedBodyLength + 1];

	if(crc_in != crc_calc)
	{
		SCCI_SendErrorFrame(Interface, ERR_CRC, crc_calc);
		return;
	}

	if(MaskStateChangeOperations &&
	   (Interface->DispID == DISP_W_16 ||
	    Interface->DispID == DISP_W_32 ||
	    Interface->DispID == DISP_W_16_2 ||
	    Interface->DispID == DISP_WB_16))
	{
		SCCI_SendErrorFrame(Interface, ERR_BLOCKED, 0);
		return;
	}

	switch(Interface->DispID)
	{
		case DISP_R_16:
			SCCI_HandleRead16(Interface);
			break;
		case DISP_R_32:
			SCCI_HandleRead32(Interface);
			break;
		case DISP_R_16_2:
			SCCI_HandleRead16Double(Interface);
			break;
		case DISP_W_16:
			SCCI_HandleWrite16(Interface);
			break;
		case DISP_W_32:
			SCCI_HandleWrite32(Interface);
			break;
		case DISP_W_16_2:
			SCCI_HandleWrite16Double(Interface);
			break;
		case DISP_RB_16:
			SCCI_HandleReadBlock16(Interface, false);
			break;
		case DISP_RRB_16:
			SCCI_HandleReadBlock16(Interface, true);
			break;
		case DISP_WB_16:
			SCCI_HandleWriteBlock16(Interface);
			break;
		case DISP_C:
			SCCI_HandleCall(Interface);
			break;
		case DISP_RBF_16:
			SCCI_HandleReadBlockFast16(Interface, false);
			break;
		case DISP_RRBF_16:
			SCCI_HandleReadBlockFast16(Interface, true);
			break;
		default:
			SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, 0);
			break;
	}
}
// ----------------------------------------

static void SCCI_HandleRead16(pSCCI_Interface Interface)
{
	uint16_t addr = Interface->MessageBuffer[2];

	if(addr >= Interface->DataTableSize)
	{
		printf("send resp\n");
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr);
	}
	else
	{
		printf("send err\n");
		Interface->MessageBuffer[3] = Interface->DataTableAddress[addr];
		SCCI_SendResponseFrame(Interface, 5);
	}
}
// ----------------------------------------

static void SCCI_HandleRead32(pSCCI_Interface Interface)
{
	uint16_t addr = Interface->MessageBuffer[2];

	if((addr + 1) >= Interface->DataTableSize)
	{
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr + 1);
	}
	else
	{
		uint32_t data = Interface->ServiceConfig->Read32Service(Interface->DataTableAddress, addr);

		Interface->MessageBuffer[3] = data >> 16;
		Interface->MessageBuffer[4] = data & 0x0000FFFF;

		SCCI_SendResponseFrame(Interface, 6);
	}
}
// ----------------------------------------

static void SCCI_HandleRead16Double(pSCCI_Interface Interface)
{
	uint16_t addr1 = Interface->MessageBuffer[2];
	uint16_t addr2 = Interface->MessageBuffer[3];

	if(addr1 >= Interface->DataTableSize)
	{
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr1);
	}
	else
	{
		Interface->MessageBuffer[3] = Interface->DataTableAddress[addr1];

		if(addr2 >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr2);
		}
		else
		{
			Interface->MessageBuffer[4] = addr2;
			Interface->MessageBuffer[5] = Interface->DataTableAddress[addr2];
			SCCI_SendResponseFrame(Interface, 7);
		}
	}
}
// ----------------------------------------

static void SCCI_HandleWrite16(pSCCI_Interface Interface)
{
	uint16_t addr = Interface->MessageBuffer[2];
	uint16_t data = Interface->MessageBuffer[3];

	if(addr >= Interface->DataTableSize)
	{
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr);
	}
	else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr))
	{
		SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr);
	}
	else if(Interface->ServiceConfig->ValidateCallback16
			&& !Interface->ServiceConfig->ValidateCallback16(addr, data))
	{
		SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr);
	}
	else
	{
		Interface->DataTableAddress[addr] = data;
		SCCI_SendResponseFrame(Interface, 4);
	}
}
// ----------------------------------------

static void SCCI_HandleWrite32(pSCCI_Interface Interface)
{
	uint16_t addr = Interface->MessageBuffer[2];
	uint32_t data = (((uint32_t)(Interface->MessageBuffer[3])) << 16) | Interface->MessageBuffer[4];

	if((addr + 1) >= Interface->DataTableSize)
	{
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr + 1);
	}
	else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr)
			|| xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr + 1))
	{
		SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr);
	}
	else if(Interface->ServiceConfig->ValidateCallback32
			&& !Interface->ServiceConfig->ValidateCallback32(addr, data))
	{
		SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr);
	}
	else
	{
		Interface->ServiceConfig->Write32Service(Interface->DataTableAddress, addr, data);
		SCCI_SendResponseFrame(Interface, 4);
	}
}
// ----------------------------------------

static void SCCI_HandleWrite16Double(pSCCI_Interface Interface)
{
	uint16_t addr1 = Interface->MessageBuffer[2];
	uint16_t addr2 = Interface->MessageBuffer[4];
	uint16_t data1 = Interface->MessageBuffer[3];
	uint16_t data2 = Interface->MessageBuffer[5];

	if(addr1 >= Interface->DataTableSize)
	{
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr1);
	}
	else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr1))
	{
		SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr1);
	}
	else if(Interface->ServiceConfig->ValidateCallback16
			&& !Interface->ServiceConfig->ValidateCallback16(addr1, data1))
	{
		SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr1);
	}
	else
	{
		if(addr2 >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr2);
		}
		else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr2))
		{
			SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr2);
		}
		else if(Interface->ServiceConfig->ValidateCallback16
				&& !Interface->ServiceConfig->ValidateCallback16(addr2, data2))
		{
			SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr2);
		}
		else
		{
			Interface->DataTableAddress[addr1] = data1;
			Interface->DataTableAddress[addr2] = data2;

			Interface->MessageBuffer[3] = addr2;

			SCCI_SendResponseFrame(Interface, 5);
		}
	}
}
// ----------------------------------------

static void SCCI_HandleReadBlock16(pSCCI_Interface Interface, bool Repeat)
{
	uint16_t* src;
	uint16_t length;
	uint16_t epnt = Interface->MessageBuffer[2] >> 8;

	if((epnt < xCCI_MAX_READ_ENDPOINTS + 1) && Interface->ProtectionAndEndpoints.ReadEndpoints16[epnt])
	{
		length = Interface->ProtectionAndEndpoints.ReadEndpoints16[epnt](epnt, &src, false, Repeat,
																		 Interface->ArgForEPCallback1, SCCI_BLOCK_MAX_VAL_16_R);
		// Set to zero
		memset(&Interface->MessageBuffer[3], 0, SCCI_BLOCK_MAX_VAL_16_R * 2);

		if(!length || (length > SCCI_BLOCK_MAX_VAL_16_R))
		{
			Interface->MessageBuffer[2] &= 0xFF00;
		}
		else
		{
			Interface->MessageBuffer[2] = (epnt << 8) | length;
			memcpy(&Interface->MessageBuffer[3], src, length * 2);
		}

		SCCI_SendResponseFrame(Interface, SCCI_BLOCK_MAX_VAL_16_R + 4);
	}
	else
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ENDPOINT, epnt);
}
// ----------------------------------------

static void SCCI_HandleWriteBlock16(pSCCI_Interface Interface)
{
	uint16_t epnt = Interface->MessageBuffer[2] >> 8;
	uint16_t length = Interface->MessageBuffer[2] & 0xFF;

	if(length <= SCCI_BLOCK_MAX_VAL_16_W)
	{
		if((epnt < xCCI_MAX_WRITE_ENDPOINTS + 1) && Interface->ProtectionAndEndpoints.WriteEndpoints16[epnt])
		{
			if(Interface->ProtectionAndEndpoints.WriteEndpoints16[epnt](epnt, &Interface->MessageBuffer[3], false,
																	    length, Interface->ArgForEPCallback1))
			{
				Interface->MessageBuffer[2] &= 0xFF00;
				SCCI_SendResponseFrame(Interface, 4);
			}
			else
				SCCI_SendErrorFrame(Interface, ERR_TOO_LONG, epnt);
		}
		else
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ENDPOINT, epnt);
	}
	else
		SCCI_SendErrorFrame(Interface, ERR_ILLEGAL_SIZE, length);
}
// ----------------------------------------

static void SCCI_HandleReadBlockFast16(pSCCI_Interface Interface, bool Repeat)
{
	uint16_t* src;
	uint16_t length;
	uint16_t epnt = Interface->MessageBuffer[2] >> 8;

	if((epnt < xCCI_MAX_READ_ENDPOINTS + 1) && Interface->ProtectionAndEndpoints.ReadEndpoints16[epnt])
	{
		length = Interface->ProtectionAndEndpoints.ReadEndpoints16[epnt](epnt, &src, true, Repeat,
																		 Interface->ArgForEPCallback1, 0);
		Interface->MessageBuffer[2] = (epnt << 8) | (SCCI_USE_CRC_IN_STREAM ? 1 : 0);

		if(!length || (length > xCCI_BLOCK_MAX_LENGTH))
			length = 0;

		Interface->MessageBuffer[3] = length;

		if(SCCI_USE_CRC_IN_STREAM)
			Interface->MessageBuffer[4] = CRC16_ComputeCRC(src, length);

		SCCI_SendResponseFrame(Interface, 6);

		Interface->IOConfig->IO_SendArray16(src, length);
		Interface->IOConfig->IO_SendArray16(ZeroBuffer, (8 - length % 8) % 8);
	}
	else
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ENDPOINT, epnt);
}
// ----------------------------------------

static void SCCI_HandleCall(pSCCI_Interface Interface)
{
	uint16_t action = Interface->MessageBuffer[2];

	if(Interface->ServiceConfig->UserActionCallback != NULL)
	{
		uint16_t userError = 0;

		if(!Interface->ServiceConfig->UserActionCallback(action, &userError))
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ACTION, action);
		}
		else if(userError != 0)
		{
			SCCI_SendErrorFrame(Interface, ERR_USER, userError);
		}
		else
			SCCI_SendResponseFrame(Interface, 4);
	}
	else
		SCCI_SendErrorFrame(Interface, ERR_INVALID_ACTION, action);
}
// ----------------------------------------

static void SCCI_SendResponseFrame(pSCCI_Interface Interface, uint16_t FrameSize)
{
	Interface->State = SCCI_STATE_WAIT_STARTBYTE;

	Interface->MessageBuffer[0] = (START_BYTE << 8) | DEVICE_SCCI_ADDRESS;
	Interface->MessageBuffer[1] |= RESPONSE_MASK << 8;
	Interface->MessageBuffer[FrameSize - 1] = CRC16_ComputeCRC(Interface->MessageBuffer, FrameSize - 1);

	Interface->IOConfig->IO_SendArray16(Interface->MessageBuffer, FrameSize); 
}
// ----------------------------------------

static void SCCI_SendErrorFrame(pSCCI_Interface Interface, uint16_t ErrorCode, uint16_t Details)
{
	Interface->State = SCCI_STATE_WAIT_STARTBYTE;

	Interface->MessageBuffer[0] = (START_BYTE << 8) | DEVICE_SCCI_ADDRESS;
	Interface->MessageBuffer[1] = (RESPONSE_MASK | (FUNCTION_ERROR << 3)) << 8;
	Interface->MessageBuffer[2] = ErrorCode;
	Interface->MessageBuffer[3] = Details;
	Interface->MessageBuffer[4] = CRC16_ComputeCRC(Interface->MessageBuffer, 4);
	
	Interface->IOConfig->IO_SendArray16(Interface->MessageBuffer, 5);
}
// ----------------------------------------

// No more.

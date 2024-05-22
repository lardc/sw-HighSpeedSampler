// -----------------------------------------
// Common declarations for xCCI interfaces
// ----------------------------------------

#ifndef __xCCI_COMMON_H
#define __xCCI_COMMON_H

// Include
//
#include "xCCIParams.h"
#include <stdint.h>


// Pointers to service functions
typedef uint32_t (*xCCI_FUNC_Read32)(uint16_t* pTable, uint16_t Index);
typedef void (*xCCI_FUNC_Write32)(uint16_t* pTable, uint16_t Index, uint32_t Data);
// Pointers to callback functions
typedef bool (*xCCI_FUNC_CallbackAction)(uint16_t ActionID, uint16_t* UserError);
typedef bool (*xCCI_FUNC_CallbackValidate16)(uint16_t Address, uint16_t Data);
typedef bool (*xCCI_FUNC_CallbackValidate32)(uint16_t Address, uint32_t Data);
typedef uint16_t (*xCCI_FUNC_CallbackReadEndpoint16)(uint16_t Endpoint, uint16_t* *Buffer, bool Streamed, bool RepeatLastTransmission, void *UserArgument, uint16_t MaxNonStreamSize);
typedef uint16_t (*xCCI_FUNC_CallbackReadEndpoint32)(uint16_t Endpoint, uint32_t* *Buffer, bool Streamed, bool RepeatLastTransmission, void *UserArgument, uint16_t MaxNonStreamSize);
typedef bool (*xCCI_FUNC_CallbackWriteEndpoint16)(uint16_t Endpoint, uint16_t* Buffer, bool Streamed, uint16_t Length, void *UserArgument);
typedef bool (*xCCI_FUNC_CallbackWriteEndpoint32)(uint16_t Endpoint, uint32_t* Buffer, bool Streamed, uint16_t Length, void *UserArgument);


// Service configuration
typedef struct __xCCI_ServiceConfig
{
	xCCI_FUNC_Read32 Read32Service;
	xCCI_FUNC_Write32 Write32Service;
	xCCI_FUNC_CallbackAction UserActionCallback;
	xCCI_FUNC_CallbackValidate16 ValidateCallback16;
	xCCI_FUNC_CallbackValidate32 ValidateCallback32;
} xCCI_ServiceConfig, *pxCCI_ServiceConfig;
//
// Protected area data
typedef struct __xCCI_ProtectedArea
{
	uint16_t StartAddress;
	uint16_t EndAddress;
} xCCI_ProtectedArea;
//
typedef struct __xCCI_ProtectionAndEndpoints
{
	uint16_t ProtectedAreasUsed;
	xCCI_ProtectedArea ProtectedAreas[xCCI_MAX_PROTECTED_AREAS];
	xCCI_FUNC_CallbackReadEndpoint16 ReadEndpoints16[xCCI_MAX_READ_ENDPOINTS + 1];
	xCCI_FUNC_CallbackReadEndpoint32 ReadEndpoints32[xCCI_MAX_READ_ENDPOINTS + 1];
	xCCI_FUNC_CallbackWriteEndpoint16 WriteEndpoints16[xCCI_MAX_WRITE_ENDPOINTS + 1];
	xCCI_FUNC_CallbackWriteEndpoint32 WriteEndpoints32[xCCI_MAX_WRITE_ENDPOINTS + 1];
} xCCI_ProtectionAndEndpoints, *pxCCI_ProtectionAndEndpoints;


// Bits
//
#define BIT0  0x1u
#define BIT1  0x2u
#define BIT2  0x4u
#define BIT3  0x8u
#define BIT4  0x10u
#define BIT5  0x20u
#define BIT6  0x40u
#define BIT7  0x80u
#define BIT8  0x100u
#define BIT9  0x200u
#define BIT10 0x400u
#define BIT11 0x800u
#define BIT12 0x1000u
#define BIT13 0x2000u
#define BIT14 0x4000u
#define BIT15 0x8000u
#define BIT16 0x10000u
#define BIT17 0x20000u
#define BIT18 0x40000u
#define BIT19 0x80000u
#define BIT20 0x100000u
#define BIT21 0x200000u
#define BIT22 0x400000u
#define BIT23 0x800000u
#define BIT24 0x1000000u
#define BIT25 0x2000000u
#define BIT26 0x4000000u
#define BIT27 0x8000000u
#define BIT28 0x10000000u
#define BIT29 0x20000000u
#define BIT30 0x40000000u
#define BIT31 0x80000000u


// Constants
//
enum xCCI_ErrorCodes
{
	ERR_NO_ERROR			= 0,
	ERR_TIMEOUT				= 1,
	ERR_CRC					= 2,
	ERR_INVALID_FUNCTION	= 3,
	ERR_INVALID_ADDESS		= 4,
	ERR_INVALID_SFUNCTION	= 5,
	ERR_INVALID_ACTION		= 6,
	ERR_INVALID_ENDPOINT	= 7,
	ERR_ILLEGAL_SIZE		= 8,
	ERR_TOO_LONG			= 9,
	ERR_NOT_SUPPORTED		= 10,
	ERR_PROTECTED			= 11,
	ERR_VALIDATION			= 12,
	ERR_BLOCKED				= 13,
	ERR_FLASH_ERROR			= 14,
	ERR_WRONG_NODE_ID		= 15,
	ERR_PROTOCOL_MISMATCH	= 254,
	ERR_USER				= 255
};


// Functions
//
// Create protected area
uint16_t xCCI_AddProtectedArea(pxCCI_ProtectionAndEndpoints PAE, uint16_t StartAddress, uint16_t EndAddress);
// Remove protected area
bool xCCI_RemoveProtectedArea(pxCCI_ProtectionAndEndpoints PAE, uint16_t AreaIndex);
// Register read endpoint 16-bit source callback
bool xCCI_RegisterReadEndpoint16(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint, xCCI_FUNC_CallbackReadEndpoint16 ReadCallback);
// Register read endpoint 32-bit source callback
bool xCCI_RegisterReadEndpoint32(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint, xCCI_FUNC_CallbackReadEndpoint32 ReadCallback);
// Register write endpoint 16-bit destination callback
bool xCCI_RegisterWriteEndpoint16(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint, xCCI_FUNC_CallbackWriteEndpoint16 WriteCallback);
// Register write endpoint 32-bit destination callback
bool xCCI_RegisterWriteEndpoint32(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint, xCCI_FUNC_CallbackWriteEndpoint32 WriteCallback);
// Check register address
bool xCCI_InProtectedZone(pxCCI_ProtectionAndEndpoints PAE, uint16_t Address);

#endif // __xCCI_COMMON_H

// -----------------------------------------
// Device profile
// ----------------------------------------

// Header
//
#include "stdafx.h"
#include "DeviceProfile.h"

// Includes
//
#include "Controller\Global.h"
#include "Controller\Controller.h"
#include "DeviceObjectDictionary.h"
#include "DataTable.h"
#include "Platform\Constraints.h"
#include <string.h>
#include "Controller\Serial.h"

// Types
//
typedef struct __EPState
{
	uint16_t Size;
	uint16_t ReadCounter;
	uint16_t WriteCounter;
	uint16_t LastReadCounter;
	uint16_t* pDataCounter;
	uint16_t* Data;
} EPState, *pEPState;
//
typedef struct __EPStates
{
	EPState ReadEPs[EP_READ_COUNT];
	EPState WriteEPs[EP_WRITE_COUNT];
} EPStates, *pEPStates;


// Variables
//
SCCI_Interface DEVICE_RS232_Interface;
//
static SCCI_IOConfig RS232_IOConfig;
static xCCI_ServiceConfig X_ServiceConfig;
static xCCI_FUNC_CallbackAction ControllerDispatchFunction;
static EPStates RS232_EPState;
static bool UnlockedForNVWrite = false;
//
static volatile bool *MaskChangesFlag;


// Forward functions
//
static void DEVPROFILE_FillNVPartDefault();
static void DEVPROFILE_FillWRPartDefault();
static bool DEVPROFILE_Validate32(uint16_t Address, uint32_t Data);
static bool DEVPROFILE_Validate16(uint16_t Address, uint16_t Data);
static bool DEVPROFILE_DispatchAction(uint16_t ActionID, uint16_t* UserError);
static uint16_t DEVPROFILE_CallbackReadX(uint16_t Endpoint, uint16_t* *Buffer, bool Streamed,
									   bool RepeatLastTransmission, void *EPStateAddress, uint16_t MaxNonStreamSize);
static bool DEVPROFILE_CallbackWriteX(uint16_t Endpoint, uint16_t* Buffer, bool Streamed, uint16_t Length, void *EPStateAddress);


// Functions
//
void DEVPROFILE_Init(xCCI_FUNC_CallbackAction SpecializedDispatch, volatile bool *MaskChanges)
{
	// Save values
	ControllerDispatchFunction = SpecializedDispatch;
	MaskChangesFlag = MaskChanges;

	// Init interface
	RS232_IOConfig.IO_SendArray16 = &SERIAL_SendArray16;
	RS232_IOConfig.IO_ReceiveArray16 = &SERIAL_ReceiveArray16;
	RS232_IOConfig.IO_GetBytesToReceive = &SERIAL_GetBytesToReceive;
	RS232_IOConfig.IO_ReceiveByte = &SERIAL_ReceiveChar;

	// Init service
	X_ServiceConfig.Read32Service = &DEVPROFILE_ReadValue32;
	X_ServiceConfig.Write32Service = &DEVPROFILE_WriteValue32;
	X_ServiceConfig.UserActionCallback = &DEVPROFILE_DispatchAction;
	X_ServiceConfig.ValidateCallback16 = &DEVPROFILE_Validate16;
	X_ServiceConfig.ValidateCallback32 = &DEVPROFILE_Validate32;

	// Init interface driver
	SCCI_Init(&DEVICE_RS232_Interface, &RS232_IOConfig, &X_ServiceConfig, (uint16_t*)DataTable,
			  DATA_TABLE_SIZE, SCCI_TIMEOUT_TICKS, &RS232_EPState);

	// Set write protection
	SCCI_AddProtectedArea(&DEVICE_RS232_Interface, DATA_TABLE_WP_START, DATA_TABLE_SIZE - 1);
}
// ----------------------------------------

void DEVPROFILE_InitEPService(uint16_t* Indexes, uint16_t* Sizes, uint16_t* *Counters, uint16_t* *Datas)
{
	uint16_t i;

	for(i = 0; i < EP_READ_COUNT; ++i)
	{
		RS232_EPState.ReadEPs[i].Size = Sizes[i];
		RS232_EPState.ReadEPs[i].pDataCounter = Counters[i];
		RS232_EPState.ReadEPs[i].Data = Datas[i];

		RS232_EPState.ReadEPs[i].ReadCounter = RS232_EPState.ReadEPs[i].LastReadCounter = 0;

		SCCI_RegisterReadEndpoint16(&DEVICE_RS232_Interface, Indexes[i], &DEVPROFILE_CallbackReadX);
	}
}
// ----------------------------------------

void DEVPROFILE_InitEPWriteService(uint16_t* Indexes, uint16_t* Sizes, uint16_t* *Counters, uint16_t* *Datas)
{
	uint16_t i;

	for(i = 0; i < EP_WRITE_COUNT; ++i)
	{
		RS232_EPState.WriteEPs[i].Size = Sizes[i];
		RS232_EPState.WriteEPs[i].pDataCounter = Counters[i];
		RS232_EPState.WriteEPs[i].Data = Datas[i];
		RS232_EPState.WriteEPs[i].WriteCounter = 0;

		SCCI_RegisterWriteEndpoint16(&DEVICE_RS232_Interface, Indexes[i], &DEVPROFILE_CallbackWriteX);
	}
}
// ----------------------------------------

void DEVPROFILE_ProcessRequests()
{
	// Handle interface requests
	SCCI_Process(&DEVICE_RS232_Interface, CONTROL_TimeCounter, *MaskChangesFlag);
}
// ----------------------------------------

void DEVPROFILE_ResetEPs()
{
	uint16_t i;

	for(i = 0; i < EP_READ_COUNT; ++i)
	{
		RS232_EPState.ReadEPs[i].ReadCounter = 0;
		RS232_EPState.ReadEPs[i].LastReadCounter = 0;
		//
		*(RS232_EPState.ReadEPs[i].pDataCounter) = 0;
		memset(RS232_EPState.ReadEPs[i].Data, 0, RS232_EPState.ReadEPs[i].Size * 2);
	}

	for(i = 0; i < EP_WRITE_COUNT; ++i)
	{
		RS232_EPState.WriteEPs[i].ReadCounter = 0;
		RS232_EPState.WriteEPs[i].LastReadCounter = 0;
		RS232_EPState.WriteEPs[i].WriteCounter = 0;
		//
		*(RS232_EPState.WriteEPs[i].pDataCounter) = 0;
		memset(RS232_EPState.WriteEPs[i].Data, 0, RS232_EPState.ReadEPs[i].Size * 2);
	}
}
// ----------------------------------------

void DEVPROFILE_ResetControlSection()
{
	DT_ResetWRPart(&DEVPROFILE_FillWRPartDefault);
}
// ----------------------------------------

uint32_t DEVPROFILE_ReadValue32(uint16_t* pTable, uint16_t Index)
{
	return pTable[Index] | (((uint32_t)(pTable[Index + 1])) << 16);
}
// ----------------------------------------

void DEVPROFILE_WriteValue32(uint16_t* pTable, uint16_t Index, uint32_t Data)
{
	pTable[Index] = Data & 0x0000FFFF;
	pTable[Index + 1] = Data >> 16;
}
// ----------------------------------------

static void DEVPROFILE_FillNVPartDefault()
{
	uint16_t i;

	// Write default values to data table
	for(i = 0; i < DATA_TABLE_NV_SIZE; ++i)
		DataTable[DATA_TABLE_NV_START + i] = NVConstraint[i].Default;
}
// ----------------------------------------

static void DEVPROFILE_FillWRPartDefault()
{
	uint16_t i;

	// Write default values to data table
	for(i = 0; i < (DATA_TABLE_WP_START - DATA_TABLE_WR_START); ++i)
		DataTable[DATA_TABLE_WR_START + i] = VConstraint[i].Default;
}
// ----------------------------------------

static bool DEVPROFILE_Validate16(uint16_t Address, uint16_t Data)
{
	if(ENABLE_LOCKING && !UnlockedForNVWrite && (Address < DATA_TABLE_WR_START))
		return false;

	if(Address < DATA_TABLE_WR_START)
	{
		if(NVConstraint[Address - DATA_TABLE_NV_START].Min > NVConstraint[Address - DATA_TABLE_NV_START].Max)
		{
			if(((int16_t)Data) < ((int16_t)(NVConstraint[Address - DATA_TABLE_NV_START].Min)) || ((int16_t)Data) > ((int16_t)(NVConstraint[Address - DATA_TABLE_NV_START].Max)))
					return false;
		}
		else if(Data < NVConstraint[Address - DATA_TABLE_NV_START].Min || Data > NVConstraint[Address - DATA_TABLE_NV_START].Max)
			return false;
	}
	else if(Address < DATA_TABLE_WP_START)
	{
		if(VConstraint[Address - DATA_TABLE_WR_START].Min > VConstraint[Address - DATA_TABLE_WR_START].Max)
		{
			if(((int16_t)Data) < ((int16_t)(VConstraint[Address - DATA_TABLE_WR_START].Min)) || ((int16_t)Data) > ((int16_t)(VConstraint[Address - DATA_TABLE_WR_START].Max)))
					return false;
		}
		else if(Data < VConstraint[Address - DATA_TABLE_WR_START].Min || Data > VConstraint[Address - DATA_TABLE_WR_START].Max)
			return false;
	}

	return true;
}
// ----------------------------------------

static bool DEVPROFILE_Validate32(uint16_t Address, uint32_t Data)
{
	if(ENABLE_LOCKING && !UnlockedForNVWrite && (Address < DATA_TABLE_WR_START))
		return false;

	return false;
}
// ----------------------------------------

static bool DEVPROFILE_DispatchAction(uint16_t ActionID, uint16_t* UserError)
{
	switch(ActionID)
	{
		case ACT_SAVE_TO_ROM:
			{
				if(ENABLE_LOCKING && !UnlockedForNVWrite)
					*UserError = ERR_WRONG_PWD;
				else
					DT_SaveNVPartToEPROM();
			}
			break;
		case ACT_RESTORE_FROM_ROM:
			{
				if(ENABLE_LOCKING && !UnlockedForNVWrite)
					*UserError = ERR_WRONG_PWD;
				else
					DT_RestoreNVPartFromEPROM();
			}
			break;
		case ACT_RESET_TO_DEFAULT:
			{
				if(ENABLE_LOCKING && !UnlockedForNVWrite)
					*UserError = ERR_WRONG_PWD;
				else
					DT_ResetNVPart(&DEVPROFILE_FillNVPartDefault);
			}
			break;
		case ACT_LOCK_NV_AREA:
			UnlockedForNVWrite = false;
			break;
		case ACT_UNLOCK_NV_AREA:
			if(DataTable[REG_PWD_1] == UNLOCK_PWD_1 &&
				DataTable[REG_PWD_2] == UNLOCK_PWD_2 &&
				DataTable[REG_PWD_3] == UNLOCK_PWD_3 &&
				DataTable[REG_PWD_4] == UNLOCK_PWD_4)
			{
				UnlockedForNVWrite = true;
				DataTable[REG_PWD_1] = 0;
				DataTable[REG_PWD_2] = 0;
				DataTable[REG_PWD_3] = 0;
				DataTable[REG_PWD_4] = 0;
			}
			else
				*UserError = ERR_WRONG_PWD;
			break;
		default:
			return (ControllerDispatchFunction) ? ControllerDispatchFunction(ActionID, UserError) : false;
	}

	return true;
}
// ----------------------------------------

static uint16_t DEVPROFILE_CallbackReadX(uint16_t Endpoint, uint16_t* *Buffer, bool Streamed,
									   bool RepeatLastTransmission, void *EPStateAddress, uint16_t MaxNonStreamSize)
{
	uint16_t pLen;
	pEPState epState;
	pEPStates epStates = (pEPStates)EPStateAddress;

	// Validate pointer
	if(!epStates)
		return 0;

	// Get endpoint
	epState = &epStates->ReadEPs[Endpoint - 1];

	// Handle transmission repeat
	if(RepeatLastTransmission)
		epState->ReadCounter = epState->LastReadCounter;

	// Write possible content reference
	*Buffer = epState->Data + epState->ReadCounter;

	// Calculate content length
	if(*(epState->pDataCounter) < epState->ReadCounter)
		pLen = 0;
	else
		pLen = *(epState->pDataCounter) - epState->ReadCounter;

	if(!Streamed)
		pLen = (pLen > MaxNonStreamSize) ? MaxNonStreamSize : pLen;

	// Update content state
	epState->LastReadCounter = epState->ReadCounter;
	epState->ReadCounter += pLen;

	return pLen;
}
// ----------------------------------------

static bool DEVPROFILE_CallbackWriteX(uint16_t Endpoint, uint16_t* Buffer, bool Streamed, uint16_t Length, void *EPStateAddress)
{
	pEPState epState;
	pEPStates epStates = (pEPStates)EPStateAddress;

	// Validate pointer
	if(!epStates)
		return FALSE;

	// Get endpoint
	epState = &epStates->WriteEPs[Endpoint - 1];

	// Check for free space
	if (epState->Size < Length + *(epState->pDataCounter))
		return FALSE;
	else
	{
		memcpy(epState->Data + *(epState->pDataCounter), Buffer, Length * 2);
		*(epState->pDataCounter) += Length;
		return TRUE;
	}
}
// ----------------------------------------

// No more

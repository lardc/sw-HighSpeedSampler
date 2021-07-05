// Headers
//
#include "stdafx.h"
#include "Controller.h"

// Includes
//
#include <windows.h>
#include "Global.h"
#include "Platform\DataTable.h"
#include "Platform\DeviceProfile.h"
#include "Platform\DeviceObjectDictionary.h"
#include "Controller\Serial.h"
#include "Controller\Memory.h"
#include "Controller\Logic.h"
#include "Controller\Info.h"

// Definitions
//
#define VALUES_READx_SIZE		2000
#define VALUES_WRITEx_SIZE		2000

// Types
//
typedef void (*FUNC_AsyncDelegate)();

// Variables
//
volatile uint64_t CONTROL_TimeCounter = 0;
volatile DeviceState CONTROL_State = DS_None;
static HANDLE hTimer = NULL;
static HANDLE hTimerQueue = NULL;
static uint16_t MEMBUF_Values1[VALUES_READx_SIZE],		MEMBUF_Values1_Counter = 0;
static uint16_t MEMBUF_Values2[VALUES_READx_SIZE],		MEMBUF_Values2_Counter = 0;
static uint16_t MEMBUF_ValuesDiag[VALUES_READx_SIZE],	MEMBUF_ValuesDiag_Counter = 0;
static uint16_t MEMBUF_ValuesFullI[VALUES_READx_SIZE],	MEMBUF_ValuesFull_Counter = 0;
static uint16_t MEMBUF_ValuesFullV[VALUES_READx_SIZE];
//
static uint16_t MEMBUF_ValuesWr[VALUES_WRITEx_SIZE],	MEMBUF_ValuesWr_Counter = 0;
//
static volatile bool CycleActive = false; 
static volatile FUNC_AsyncDelegate DPCDelegate = NULL;

// Forward functions 
//
void CALLBACK TimerRoutine(PVOID lpParam, BOOL TimerOrWaitFired);
bool CONTROL_DispatchAction(uint16_t ActionID, uint16_t* UserError);
void CONTROL_SetDeviceState(DeviceState NewState);
void CONTROL_SwitchStateToFault(uint16_t FaultReason, uint16_t FaultReasonEx);
void CONTROL_SwitchStateToDisabled(uint16_t DisableReason, uint16_t DisableReasonEx);
void CONTROL_PisoScopeInit(const char *ScopeSerialVoltage, const char *ScopeSerialCurrent);
void CONTROL_HandleSamplerData();
void CONTROL_FillWPPartDefault();

// Functions
//
void CONTROL_Init(const char *ScopeSerialVoltage, const char *ScopeSerialCurrent)
{
	// Init read endpoints
	uint16_t EPReadIndexes[EP_READ_COUNT] = { EP_READ_I, EP_READ_V, EP_READ_DIAG, EP_READ_FULL_I_PART, EP_READ_FULL_V_PART };
	uint16_t EPReadSized[EP_READ_COUNT] = { VALUES_READx_SIZE, VALUES_READx_SIZE, VALUES_READx_SIZE, VALUES_READx_SIZE, VALUES_READx_SIZE };
	uint16_t* EPReadCounters[EP_READ_COUNT] = { (uint16_t*)&MEMBUF_Values1_Counter, (uint16_t*)&MEMBUF_Values2_Counter, (uint16_t*)&MEMBUF_ValuesDiag_Counter,
												(uint16_t*)&MEMBUF_ValuesFull_Counter, (uint16_t*)&MEMBUF_ValuesFull_Counter };
	uint16_t* EPReadDatas[EP_READ_COUNT] = { MEMBUF_Values1, MEMBUF_Values2, MEMBUF_ValuesDiag, MEMBUF_ValuesFullI, MEMBUF_ValuesFullV };

	// Init write endpoints
	uint16_t EPWriteIndexes[EP_WRITE_COUNT] = { EP_WRITE };
	uint16_t EPWriteSized[EP_WRITE_COUNT] = { VALUES_WRITEx_SIZE };
	uint16_t* EPWriteCounters[EP_WRITE_COUNT] = { (uint16_t*)&MEMBUF_ValuesWr_Counter };
	uint16_t* EPWriteDatas[EP_WRITE_COUNT] = { MEMBUF_ValuesWr };

	// Data-table EPROM service configuration
	EPROMServiceConfig EPROMService = { &MEMORY_Write, &MEMORY_Read };

	// Init data table
	DT_Init(EPROMService, FALSE);
	// Fill state variables with default values
	CONTROL_FillWPPartDefault();

	// Device profile initialization
	DEVPROFILE_Init(&CONTROL_DispatchAction, &CycleActive);
	DEVPROFILE_InitEPService(EPReadIndexes, EPReadSized, EPReadCounters, EPReadDatas);
	DEVPROFILE_InitEPWriteService(EPWriteIndexes, EPWriteSized, EPWriteCounters, EPWriteDatas);
	// Reset control values
	DEVPROFILE_ResetControlSection();

	// Connect scope
	CONTROL_PisoScopeInit(ScopeSerialVoltage, ScopeSerialCurrent);
}
//----------------------------------------------

void CONTROL_TimerInit()
{
	// Create the timer queue.
	hTimerQueue = CreateTimerQueue();
	CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)&TimerRoutine, NULL, 0, TIMER_PERIOD, 0);
}
//----------------------------------------------

void CALLBACK TimerRoutine(PVOID lpParam, BOOL TimerOrWaitFired)
{
	CONTROL_TimeCounter += TIMER_PERIOD;
}
//----------------------------------------------

void CONTROL_RequestDPC(FUNC_AsyncDelegate Action)
{
	DPCDelegate = Action;
}
// ----------------------------------------

void CONTROL_Idle()
{
	// Handle serial communication
	SERIAL_UpdateReadBuffer();
	DEVPROFILE_ProcessRequests();

	// Handle PisoScope data request
	CONTROL_HandleSamplerData();

	// Process deferred procedures
	if(DPCDelegate)
	{
		FUNC_AsyncDelegate del = DPCDelegate;
		DPCDelegate = NULL;
		del();
	}
}
// ----------------------------------------

void CONTROL_SetDeviceState(DeviceState NewState)
{
	// Set new state
	CONTROL_State = NewState;
	DataTable[REG_DEV_STATE] = NewState;
}
// ----------------------------------------

void CONTROL_SwitchStateToFault(uint16_t FaultReason, uint16_t FaultReasonEx)
{
	InfoPrint(IP_Err, "Switched to fault");
	DataTable[REG_FAULT_REASON] = FaultReason;
	DataTable[REG_DF_REASON_EX] = FaultReasonEx;
	CONTROL_SetDeviceState(DS_Fault);
}
// ----------------------------------------

void CONTROL_SwitchStateToDisabled(uint16_t DisableReason, uint16_t DisableReasonEx)
{
	char message[256];
	sprintf_s(message, 256, "Switched to disabled, reason: %d, ext. reason: %d", DisableReason, DisableReasonEx);
	InfoPrint(IP_Err, message);

	DataTable[REG_DISABLE_REASON] = DisableReason;
	DataTable[REG_DF_REASON_EX] = DisableReasonEx;
	CONTROL_SetDeviceState(DS_Disabled);
}
// ----------------------------------------

void CONTROL_PisoScopeInit(const char *ScopeSerialVoltage, const char *ScopeSerialCurrent)
{
	PICO_STATUS status = LOGIC_PisoScopeInit(ScopeSerialVoltage, ScopeSerialCurrent);

	if (status != PICO_OK)
	{
		InfoPrint(IP_Err, "Pisoscope init error");
		CONTROL_SwitchStateToDisabled(DF_PICOSCOPE, status);

		LOGIC_PisoScopeList();
	}
	else
		InfoPrint(IP_Info, "Pisoscope init done");
}
// ----------------------------------------

void CONTROL_HandleSamplerData()
{
	if (CONTROL_State == DS_InProcess)
	{
		if (SAMPLING_Finished())
		{
			bool CalcOK;
			uint16_t CalcProblem = 0;
			uint32_t Index0 = 0, Index0V = 0;
			float Irr = 0, trr = 0, Qrr = 0, dIdt = 0;

			InfoPrint(IP_Info, "Sampling finished");
			PICO_STATUS status = LOGIC_HandleSamplerData(&CalcProblem, &Index0, &Irr, &trr, &Qrr, &dIdt,
														 (DataTable[REG_MEASURE_MODE] == MODE_QRR) ? false : true, (DataTable[REG_TR_050_METHOD] == 0) ? false : true, &Index0V);
			CalcOK = (CalcProblem == PROBLEM_NONE) ? true : false;

			DataTable[REG_RESULT_IRR] =		(uint16_t)Irr;
			DataTable[REG_RESULT_TRR] =		(uint16_t)(trr * 10);
			DataTable[REG_RESULT_QRR] =		(uint16_t)Qrr;
			DataTable[REG_RESULT_ZERO] =	(uint16_t)(Index0 * SAMPLING_TIME_FRACTION * 10);
			DataTable[REG_RESULT_ZERO_V] =	(uint16_t)(Index0V * SAMPLING_TIME_FRACTION * 10);
			DataTable[REG_RESULT_DIDT] =	(uint16_t)(dIdt * 10);

			if (status != PICO_OK)
				CONTROL_SwitchStateToDisabled(DF_PICOSCOPE, status);
			else
			{
				uint32_t forced_sector = (uint32_t)((float)DataTable[REG_DIAG_FORCE_SECTOR_READ] / SAMPLING_TIME_FRACTION);
				MEMBUF_Values1_Counter = LOGIC_GetIData(MEMBUF_Values1, VALUES_READx_SIZE, CalcOK, DataTable[REG_MEASURE_MODE] == MODE_QRR, Index0, Index0V, forced_sector);
				MEMBUF_Values2_Counter = LOGIC_GetVData(MEMBUF_Values2, VALUES_READx_SIZE, CalcOK, DataTable[REG_MEASURE_MODE] == MODE_QRR, Index0, Index0V, forced_sector);

				DataTable[REG_FINISHED] = CalcOK ? OPRESULT_OK : OPRESULT_FAIL;
				DataTable[REG_PROBLEM] = CalcProblem;
				CONTROL_SetDeviceState(DS_None);
			}
		}
	}
}
// ----------------------------------------

void CONTROL_FillWPPartDefault()
{
	CONTROL_SetDeviceState(DS_None);
	DataTable[REG_FAULT_REASON] = DF_NONE;
	DataTable[REG_DISABLE_REASON] = DF_NONE;
	DataTable[REG_WARNING] = WARNING_NONE;
	DataTable[REG_PROBLEM] = PROBLEM_NONE;
	DataTable[REG_DF_REASON_EX] = 0;
	//
	DataTable[REG_FINISHED] = OPRESULT_NONE;
	DataTable[REG_RESULT_IRR] = 0;
	DataTable[REG_RESULT_TRR] = 0;
	DataTable[REG_RESULT_QRR] = 0;
	DataTable[REG_RESULT_ZERO] = 0;
	DataTable[REG_RESULT_ZERO_V] = 0;
}
// ----------------------------------------

void CONTROL_SaveRawData()
{
	LOGIC_CurrentToFile();
	LOGIC_VoltageToFile();
}
// ----------------------------------------

bool CONTROL_DispatchAction(uint16_t ActionID, uint16_t* UserError)
{
	switch(ActionID)
	{
		case ACT_START_TEST:
			{
				InfoPrint(IP_Info, "------------");
				InfoPrint(IP_Info, "Start test request");
				if(CONTROL_State == DS_None)
				{
					PICO_STATUS status = LOGIC_PisoScopeActivate();
					if (status == PICO_OK)
					{
						CONTROL_FillWPPartDefault();
						DEVPROFILE_ResetEPs();
						CONTROL_SetDeviceState(DS_InProcess);
					}
					else
					{
						InfoPrint(IP_Err, "Start error");
						CONTROL_SwitchStateToDisabled(DF_PICOSCOPE, status);
					}
				}
				else
					*UserError = ERR_OPERATION_BLOCKED;
			}
			break;

		case ACT_STOP_TEST:
			{
				InfoPrint(IP_Info, "Stop test request");
				if(CONTROL_State == DS_InProcess)
				{
					PICO_STATUS status = SAMPLER_Stop();
					if (status == PICO_OK)
						CONTROL_SetDeviceState(DS_None);
					else
					{
						InfoPrint(IP_Err, "Stop error");
						CONTROL_SwitchStateToDisabled(DF_PICOSCOPE, status);
					}
				}
			}
			break;

		case ACT_CLR_FAULT:
			{
				if(CONTROL_State == DS_Fault)
				{
					CONTROL_SetDeviceState(DS_None);

					DataTable[REG_WARNING] = WARNING_NONE;
					DataTable[REG_FAULT_REASON] = DF_NONE;
					DataTable[REG_DF_REASON_EX] = 0;
				}
				else if(CONTROL_State == DS_Disabled)
					*UserError = ERR_OPERATION_BLOCKED;
			}
			break;

		case ACT_CLR_WARNING:
			DataTable[REG_WARNING] = WARNING_NONE;
			break;

		case ACT_DIAG_GEN_READ_EP:
			{
				DEVPROFILE_ResetEPs();

				MEMBUF_ValuesDiag_Counter = 500;
				for (int i = 0; i < MEMBUF_ValuesDiag_Counter; i++)
					MEMBUF_ValuesDiag[i] = i % 20;
			}
			break;

		case ACT_DIAG_READ_FRAGMENT:
			{
				DEVPROFILE_ResetEPs();
				MEMBUF_ValuesFull_Counter = LOGIC_LoadFragment(MEMBUF_ValuesFullI, MEMBUF_ValuesFullV, VALUES_READx_SIZE, DataTable[REG_DIAG_FULL_ARR_SCALE]);
			}
			break;

		case ACT_DIAG_READ_RAW_FRAGMENT:
			{
				DEVPROFILE_ResetEPs();
				MEMBUF_ValuesFull_Counter = LOGIC_LoadRawFragment(MEMBUF_ValuesFullI, MEMBUF_ValuesFullV, VALUES_READx_SIZE, DataTable[REG_DIAG_FULL_ARR_SCALE]);
			}
			break;

		case ACT_DIAG_SAVE_TO_FILE:
			{
				CONTROL_RequestDPC(CONTROL_SaveRawData);				
			}
			break;

		default:
			return FALSE;
	}
	
	return TRUE;
}
// ----------------------------------------

// -----------------------------------------
// Device data table
// ----------------------------------------

// Header
//
#include "stdafx.h"
#include "DataTable.h"

// Includes
//
#include "DeviceObjectDictionary.h"


// Constants
//
#define DT_EPROM_ADDRESS	0


// Variables
//
static EPROMServiceConfig EPROMServiceCfg;
//
volatile uint16_t DataTable[DATA_TABLE_SIZE];


// Functions
//
void DT_Init(EPROMServiceConfig EPROMService, bool NoRestore)
{
	uint16_t i;
	
	EPROMServiceCfg = EPROMService;

	for(i = 0; i < DATA_TABLE_SIZE; ++i)
		DataTable[i] = 0;
		
	if(!NoRestore)
		DT_RestoreNVPartFromEPROM();
}
// ----------------------------------------

void DT_RestoreNVPartFromEPROM()
{
	if(EPROMServiceCfg.ReadService)
		EPROMServiceCfg.ReadService(DT_EPROM_ADDRESS, (uint16_t*) &DataTable[DATA_TABLE_NV_START], DATA_TABLE_NV_SIZE);
}
// ----------------------------------------

void DT_SaveNVPartToEPROM()
{
	if(EPROMServiceCfg.WriteService)
		EPROMServiceCfg.WriteService(DT_EPROM_ADDRESS, (uint16_t*) &DataTable[DATA_TABLE_NV_START], DATA_TABLE_NV_SIZE);
}
// ----------------------------------------

void DT_ResetNVPart(FUNC_SetDefaultValues SetFunc)
{
	uint16_t i;
	
	for(i = DATA_TABLE_NV_START; i < (DATA_TABLE_NV_SIZE + DATA_TABLE_NV_START); ++i)
		DataTable[i] = 0;
		
	if(SetFunc)
		SetFunc();
}
// ----------------------------------------

void DT_ResetWRPart(FUNC_SetDefaultValues SetFunc)
{
	uint16_t i;

	for(i = DATA_TABLE_WR_START; i < DATA_TABLE_WP_START; ++i)
		DataTable[i] = 0;

	if(SetFunc)
		SetFunc();
}
// ----------------------------------------

// No more.

// -----------------------------------------
// Device profile
// ----------------------------------------

#ifndef __DEV_PROFILE_H
#define __DEV_PROFILE_H

// Include
#include <stdint.h>
//
#include "Interface\SCCISlave.h"


// Functions
//
// Initialize device profile engine
void DEVPROFILE_Init(xCCI_FUNC_CallbackAction SpecializedDispatch, volatile bool *MaskChanges);
// Initialize endpoint service
void DEVPROFILE_InitEPService(uint16_t* Indexes, uint16_t* Sizes, uint16_t** Counters, uint16_t** Datas);
void DEVPROFILE_InitEPWriteService(uint16_t* Indexes, uint16_t* Sizes, uint16_t **Counters, uint16_t **Datas);
// Process user interface requests
void DEVPROFILE_ProcessRequests();
// Reset EPs data and counters
void DEVPROFILE_ResetEPs();
// Reset user control (WR) section of data table
void DEVPROFILE_ResetControlSection();

// Read 32-bit value from data table
uint32_t DEVPROFILE_ReadValue32(uint16_t* pTable, uint16_t Index);
// Write 32-bit value to data table
void DEVPROFILE_WriteValue32(uint16_t* pTable, uint16_t Index, uint32_t Data);

#endif // __DEV_PROFILE_H

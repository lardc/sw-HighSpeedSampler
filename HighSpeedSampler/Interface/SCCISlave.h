// -----------------------------------------
// SCCI-Slave communication interface
// ----------------------------------------

#ifndef __SCCIS_H
#define __SCCIS_H

// Include
#include <stdint.h>
//
#include "SCCIParams.h"
#include "xCCICommon.h"
#include "SCCITypes.h"


// Functions
//
// Init interface instance
void SCCI_Init(pSCCI_Interface Interface, pSCCI_IOConfig IOConfig, pxCCI_ServiceConfig ServiceConfig,
 			   uint16_t* DataTable, uint16_t DataTableSize, uint32_t MessageTimeoutTicks,
 			   void *ArgumentForCallback16);

// Process packets
void SCCI_Process(pSCCI_Interface Interface, uint64_t CurrentTickCount, bool MaskStateChangeOperations);


// Internal functions
//
// Create protected area
uint16_t inline SCCI_AddProtectedArea(pSCCI_Interface Interface, uint16_t StartAddress, uint16_t EndAddress)
{
	return xCCI_AddProtectedArea(&(Interface->ProtectionAndEndpoints), StartAddress, EndAddress);
}
//
// Remove protected area
bool inline SCCI_RemoveProtectedArea(pSCCI_Interface Interface, uint16_t AreaIndex)
{
	return xCCI_RemoveProtectedArea(&(Interface->ProtectionAndEndpoints), AreaIndex);
}
//
// Register read endpoint 16-bit source callback
bool inline SCCI_RegisterReadEndpoint16(pSCCI_Interface Interface, uint16_t Endpoint,
								    xCCI_FUNC_CallbackReadEndpoint16 ReadCallback)
{
	return xCCI_RegisterReadEndpoint16(&(Interface->ProtectionAndEndpoints), Endpoint, ReadCallback);
}
//
// Register read endpoint 32-bit source callback
bool inline SCCI_RegisterReadEndpoint32(pSCCI_Interface Interface, uint16_t Endpoint,
								    xCCI_FUNC_CallbackReadEndpoint32 ReadCallback)
{
	return xCCI_RegisterReadEndpoint32(&(Interface->ProtectionAndEndpoints), Endpoint, ReadCallback);
}
//
// Register write endpoint 16-bit destination callback
bool inline SCCI_RegisterWriteEndpoint16(pSCCI_Interface Interface, uint16_t Endpoint,
									 xCCI_FUNC_CallbackWriteEndpoint16 WriteCallback)
{
	return xCCI_RegisterWriteEndpoint16(&(Interface->ProtectionAndEndpoints), Endpoint, WriteCallback);
}
//
// Register write endpoint 32-bit destination callback
bool inline SCCI_RegisterWriteEndpoint32(pSCCI_Interface Interface, uint16_t Endpoint,
									 xCCI_FUNC_CallbackWriteEndpoint32 WriteCallback)
{
	return xCCI_RegisterWriteEndpoint32(&(Interface->ProtectionAndEndpoints), Endpoint, WriteCallback);
}


#endif // __SCCIS_H

// -----------------------------------------
// SCCI communication interface
// ----------------------------------------

// Header
#include "stdafx.h"
#include "xCCICommon.h"


uint16_t xCCI_AddProtectedArea(pxCCI_ProtectionAndEndpoints PAE, uint16_t StartAddress, uint16_t EndAddress)
{
	if(PAE->ProtectedAreasUsed == xCCI_MAX_PROTECTED_AREAS)
		return 0;

	// Save parameters
	PAE->ProtectedAreas[PAE->ProtectedAreasUsed].StartAddress = StartAddress;
	PAE->ProtectedAreas[PAE->ProtectedAreasUsed].EndAddress = EndAddress;

	return 1 + (PAE->ProtectedAreasUsed++);
}
// ----------------------------------------

bool xCCI_RemoveProtectedArea(pxCCI_ProtectionAndEndpoints PAE, uint16_t AreaIndex)
{
	uint16_t i;

	if(AreaIndex > PAE->ProtectedAreasUsed)
		return false;

	// Update indexes
	--AreaIndex;
	--(PAE->ProtectedAreasUsed);
	// Shift data
	for(i = AreaIndex; i < PAE->ProtectedAreasUsed; ++i)
		PAE->ProtectedAreas[i] = PAE->ProtectedAreas[i + 1];

	return true;
}
// ----------------------------------------

bool xCCI_RegisterReadEndpoint16(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint,
								    xCCI_FUNC_CallbackReadEndpoint16 ReadCallback)
{
	if(Endpoint < xCCI_MAX_READ_ENDPOINTS + 1)
	{
		PAE->ReadEndpoints16[Endpoint] = ReadCallback;
		return true;
	}

	return false;
}
// ----------------------------------------

bool xCCI_RegisterReadEndpoint32(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint,
								    xCCI_FUNC_CallbackReadEndpoint32 ReadCallback)
{
	if(Endpoint < xCCI_MAX_READ_ENDPOINTS + 1)
	{
		PAE->ReadEndpoints32[Endpoint] = ReadCallback;
		return true;
	}

	return false;
}
// ----------------------------------------

bool xCCI_RegisterWriteEndpoint16(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint,
									 xCCI_FUNC_CallbackWriteEndpoint16 WriteCallback)
{
	if(Endpoint < xCCI_MAX_WRITE_ENDPOINTS + 1)
	{
		PAE->WriteEndpoints16[Endpoint] = WriteCallback;
		return true;
	}

	return false;
}
// ----------------------------------------

bool xCCI_RegisterWriteEndpoint32(pxCCI_ProtectionAndEndpoints PAE, uint16_t Endpoint,
									 xCCI_FUNC_CallbackWriteEndpoint32 WriteCallback)
{
	if(Endpoint < xCCI_MAX_WRITE_ENDPOINTS + 1)
	{
		PAE->WriteEndpoints32[Endpoint] = WriteCallback;
		return true;
	}

	return false;
}
// ----------------------------------------

bool xCCI_InProtectedZone(pxCCI_ProtectionAndEndpoints PAE, uint16_t Address)
{
	uint16_t i;
	bool result = false;

	for(i = 0; i < PAE->ProtectedAreasUsed; ++i)
		if((Address >= PAE->ProtectedAreas[i].StartAddress) && (Address <= PAE->ProtectedAreas[i].EndAddress))
		{
			result = true;
			break;
		}

	return result;
}

// No more

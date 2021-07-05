#ifndef __INFO_H__
#define __INFO_H__

// Includes
//
#include <stdint.h>

// Types
//
typedef enum __InfoPrintParam
{
	IP_Info,
	IP_Warn,
	IP_Err

} InfoPrintParam;

// Functions
//
void InfoPrint(InfoPrintParam Param, char *Message);

#endif	// __INFO_H__
// Headers
//
#include "stdafx.h"
#include "Info.h"

// Includes
//
#include <stdio.h>
#include < Windows.h >

// Variables
char *StrInfo  = "INFO:";
char *StrWarn  = "WARN:";
char *StrError = "ERR :";

// Functions
//
void InfoPrint(InfoPrintParam Param, const char *Message)
{
	char *Description = "";

	switch (Param)
	{
		case IP_Info:
			Description = (char *)StrInfo;
			break;

		case IP_Warn:
			Description = (char *)StrWarn;
			break;

		case IP_Err:
			Description = (char *)StrError;
			break;
	}

	// Подготовка даты
	SYSTEMTIME LocTime;
	GetLocalTime(&LocTime);
	char dt_str[32];
	sprintf_s(dt_str, 32, "%04d-%02d-%02d %02d:%02d:%02d",
		LocTime.wYear, LocTime.wMonth, LocTime.wDay, LocTime.wHour, LocTime.wMinute, LocTime.wSecond);

	printf("%s %s %s\n", dt_str, Description, Message);
}

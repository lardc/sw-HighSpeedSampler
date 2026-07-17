// Headers
//
#include "stdafx.h"
#include "Info.h"

// Includes
//
#include <stdio.h>
#include <Windows.h>

// Variables
char *StrInfo  = "INFO:";
char *StrWarn  = "WARN:";
char *StrError = "ERR :";

// Functions
//
void InfoPrint(InfoPrintParam Param, char *Message)
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

	printf("%s %s\n", Description, Message);
}

void PrintTime()
{
	SYSTEMTIME LocTime;
	GetLocalTime(&LocTime);
	printf("Sample created at %04d-%02d-%02d %02d:%02d:%02d\n", LocTime.wYear, LocTime.wMonth, LocTime.wDay, LocTime.wHour, LocTime.wMinute, LocTime.wSecond);
}
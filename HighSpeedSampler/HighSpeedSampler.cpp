// HighSpeedSampler.cpp : Defines the entry point for the console application.
//
// Headers
//
#include "stdafx.h"
#include <Windows.h>

// Includes
//
#include "Serial.h"
#include "Controller.h"
#include "ConfigFile.h"
#include "Info.h"
#include "Platform\git_info.h"
#include <string>

// Use the executable directory as CWD so sidecar files
// (config, DataTable.bin, csv dumps) are found even if the
// process was started from another folder (e.g. d:\Agent).
static bool SetWorkingDirectoryToExe()
{
	TCHAR modulePath[MAX_PATH];
	DWORD length = GetModuleFileName(NULL, modulePath, MAX_PATH);
	if (length == 0 || length >= MAX_PATH)
		return false;

	TCHAR* lastSlash = _tcsrchr(modulePath, _T('\\'));
	if (lastSlash == NULL)
		lastSlash = _tcsrchr(modulePath, _T('/'));
	if (lastSlash == NULL)
		return false;

	*lastSlash = _T('\0');
	return SetCurrentDirectory(modulePath) != 0;
}

// Functions
//
int _tmain(int argc, _TCHAR* argv[])
{
	int PortNumber, PortBR;

	// Print Firmware Info
	InfoPrint(IP_Info, (std::string("Git Branch: ") + git_branch).c_str());
	InfoPrint(IP_Info, (std::string("Git Commit: ") + git_commit).c_str());
	InfoPrint(IP_Info, (std::string("Commit date: ") + git_date).c_str());
	InfoPrint(IP_Info, (std::string("Project: ") + git_proj).c_str());

	if (!SetWorkingDirectoryToExe())
	{
		InfoPrint(IP_Err, "Failed to set working directory to executable folder");
		getchar();
		return 1;
	}

	// Load configuration
	try
	{
		ConfigFile cf("HighSpeedSampler.config");
		PortNumber = (int)cf.Value("serial", "portnum");
		PortBR = (int)cf.Value("serial", "baudrate");

		InfoPrint(IP_Info, "Config file loaded");
	}
	catch(...)
	{
		InfoPrint(IP_Err, "Load config error");

		getchar();
		return 1;
	}

	// Init serial port
	if(!SERIAL_Init(PortNumber, PortBR))
	{
		InfoPrint(IP_Err, "Serial port init error");

		getchar();
		return 1;
	}
	else
		InfoPrint(IP_Info, "Serial port opened");

	CONTROL_TimerInit();
	CONTROL_Init();

	while(true)
		CONTROL_Idle();

	return 0;
}
//----------------------------------------------


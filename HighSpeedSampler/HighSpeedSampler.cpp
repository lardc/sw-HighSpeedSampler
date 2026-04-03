// HighSpeedSampler.cpp : Defines the entry point for the console application.
//
// Headers
//
#include "stdafx.h"
#include <Windows.h>

// Includes
//
#include "Controller\Serial.h"
#include "Controller\Controller.h"
#include "External\ConfigFile.h"
#include "Controller\Info.h"
#include "Platform\git_info.h"
#include <string>

// Functions
//
int _tmain(int argc, _TCHAR* argv[])
{
	int PortNumber, PortBR;
	std::string ScopeSerialVoltage, ScopeSerialCurrent;

	// Print Firmware Info
	InfoPrint(IP_Info, (std::string("Git Branch: ") + git_branch).c_str());
	InfoPrint(IP_Info, (std::string("Git Commit: ") + git_commit).c_str());
	InfoPrint(IP_Info, (std::string("Commit date: ") + git_date).c_str());
	InfoPrint(IP_Info, (std::string("Project: ") + git_proj).c_str());
	// Load configuration
	try
	{
		ConfigFile cf("HighSpeedSampler.config");
		PortNumber =			(int)cf.Value("serial", "portnum");
		PortBR =				(int)cf.Value("serial", "baudrate");
		ScopeSerialVoltage =	cf.Value("serial", "scope_voltage");
		ScopeSerialCurrent =	cf.Value("serial", "scope_current");

		InfoPrint(IP_Info, "Config file loaded");
	}
	catch(...)
	{
		InfoPrint(IP_Err, "Load config error");

		getchar();
		return 1;
	}

	// Init serial port
	if (!SERIAL_Init(PortNumber, PortBR))
	{
		InfoPrint(IP_Err, "Serial port init error");

		getchar();
		return 1;
	}
	else
		InfoPrint(IP_Info, "Serial port opened");

	CONTROL_TimerInit();
	if (!CONTROL_Init(ScopeSerialVoltage.c_str(), ScopeSerialCurrent.c_str()))
	{
		InfoPrint(IP_Info, "Exit");
		InfoPrint(IP_Info, "--------------------");
		return 1;
	}

	while(true)
		CONTROL_Idle();

	return 0;
}
//----------------------------------------------


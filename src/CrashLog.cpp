//
// CrashLog.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include "CrashLog.h"

HANDLE hCrashLogFile = 0;
char szCrashLogBuffer[CRASH_LOG_BUFFER_LEN] = {};

bool CreateCrashLog(const std::string& filename)
{
	if (hCrashLogFile)
	{
		CloseHandle(hCrashLogFile);
	}

	hCrashLogFile = ::CreateFileA(
		filename.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	return hCrashLogFile != NULL;
}

bool CloseCrashLog()
{
	bool ret = false;

	if (hCrashLogFile)
	{
		ret = CloseHandle(hCrashLogFile) != FALSE;
		hCrashLogFile = 0;
	}

	return ret;
}
//
// CrashLog.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include "CrashLog.h"

HANDLE hLogFile = 0;

#define LOG_BUFFER_LEN 1024
char logBuffer[LOG_BUFFER_LEN] = {};

bool CreateCrashLog(const std::string& filename)
{
	if (hLogFile)
	{
		CloseHandle(hLogFile);
	}

	hLogFile = ::CreateFileA(
		filename.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);

	return hLogFile != 0;
}

bool CloseCrashLog()
{
	bool ret = false;

	if (hLogFile)
	{
		ret = CloseHandle(hLogFile) != 0;
		hLogFile = 0;
	}

	return ret;
}
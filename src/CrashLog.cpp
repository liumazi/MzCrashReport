//
// CrashLog.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include "CrashLog.h"

HANDLE _hCrashLogFile = 0;
char _szCrashLogBuffer[CRASH_LOG_BUFFER_LEN] = {};

bool CreateCrashLog(const std::string& filename)
{
	if (_hCrashLogFile)
	{
		CloseHandle(_hCrashLogFile);
	}

	_hCrashLogFile = CreateFileA(
		filename.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	return _hCrashLogFile != NULL;
}

bool CloseCrashLog()
{
	bool ret = false;

	if (_hCrashLogFile)
	{
		ret = CloseHandle(_hCrashLogFile) != FALSE;
		_hCrashLogFile = 0;
	}

	return ret;
}
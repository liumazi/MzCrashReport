//
// CrashLog.h
//
// Created by liuliu.mz on 20-08-28
//

#pragma once

#include <windows.h>
#include <string>

#define CRASH_LOG_BUFFER_LEN 1024

bool CreateCrashLog(const std::string& filename);

template<typename ...T>
bool AppendCrashLog(const char* format, T... args)
{
	extern char _szCrashLogBuffer[CRASH_LOG_BUFFER_LEN];
	extern HANDLE _hCrashLogFile;

	int n = sprintf(_szCrashLogBuffer, format, args...);
	if (n > 0) // && n < FMT_BUFFER_LEN
	{
		DWORD writed = 0;
		if (WriteFile(_hCrashLogFile, _szCrashLogBuffer, n, &writed, nullptr))
		{
			return DWORD(n) == writed;
		}
	}

	return false;
}

bool CloseCrashLog();
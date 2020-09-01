//
// CrashLog.h
//
// Created by liuliu.mz on 20-08-28
//

#pragma once

#include <windows.h>
#include <string>

bool CrashLogCreate(const std::string& filename);

template <typename... T>
bool CrashLogAppend(const char* format, T... args)
{
#define LOG_BUFFER_LEN 1024
	extern char logBuffer[LOG_BUFFER_LEN];
#undef LOG_BUFFER_LEN
	extern HANDLE hLogFile;

	int n = sprintf(logBuffer, format, args...);
	if (n > 0) // && n < FMT_BUFFER_LEN
	{
		DWORD writed = 0;
		if (WriteFile(hLogFile, logBuffer, n, &writed, nullptr))
		{
			return DWORD(n) == writed;
		}
	}

	return false;
}

bool CrashLogClose();
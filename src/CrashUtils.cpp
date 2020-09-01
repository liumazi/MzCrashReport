//
// CrashUtils.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include <windows.h>
#include <time.h>

#include "CrashUtils.h"

// Dump文件名 不带扩展名 形如 x:\ExeName-2020-08-28-181818
std::string GenDumpFileName()
{
	CHAR PathBuff[MAX_PATH + 20]; // 18-3+1
	DWORD PathLen;

	PathLen = GetModuleFileNameA(0, PathBuff, MAX_PATH);
	if (PathLen < 4)
	{
		PathLen = 0;
	}
	else
	{
		PathLen -= 4;
	}

	time_t utime = time(NULL);
	tm* ltime = localtime(&utime);

	sprintf(&PathBuff[PathLen], "-%04u-%02u-%02u-%02u%02u%02u",
		ltime->tm_year + 1900,
		ltime->tm_mon + 1,
		ltime->tm_mday,
		ltime->tm_hour,
		ltime->tm_min,
		ltime->tm_sec);

	return PathBuff;
}
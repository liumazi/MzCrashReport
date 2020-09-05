//
// CrashUtils.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include <windows.h>
#include <time.h>
#include <locale>
#include <codecvt>

#include "CrashUtils.h"

// Dump文件名 不带扩展名 形如 x:\ExeName-2020-08-28-181818
std::string GenDumpFileName()
{
	CHAR buff[MAX_PATH + 20]; // 18-3+1
	DWORD len;

	len = GetModuleFileNameA(0, buff, MAX_PATH);
	if (len < 4)
	{
		len = 0;
	}
	else
	{
		len -= 4;
	}

	time_t utime = time(NULL);
	tm* ltime = localtime(&utime);

	sprintf(&buff[len], "-%04u-%02u-%02u-%02u%02u%02u",
		ltime->tm_year + 1900,
		ltime->tm_mon + 1,
		ltime->tm_mday,
		ltime->tm_hour,
		ltime->tm_min,
		ltime->tm_sec);

	return buff;
}

std::string WStringToString(const std::wstring& wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvter;
	return cvter.to_bytes(wstr);
}
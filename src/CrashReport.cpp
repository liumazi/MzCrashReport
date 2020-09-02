//
// CrashReport.cpp
//
// Created by liuliu.mz on 20-08-26
//

#include <windows.h>

#include "CrashReport.h"
#include "CrashUtils.h"
#include "CrashHandler.h"

bool InitCrashReport()
{
	bool ret = SetupCrashHanders();
	if (!ret)
	{
		::MessageBoxA(GetActiveWindow(), "InitCrashReport failed.", CRASH_MSGBOX_CAPTION, 0);
	}
	return ret;
}
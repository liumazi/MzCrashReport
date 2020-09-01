//
// CrashReport.cpp
//
// Created by liuliu.mz on 20-08-26
//

#include "CrashReport.h"
#include "CrashUtils.h"
#include "CrashHandler.h"

HWND hMainWindow = 0;

bool InitCrashReport()
{
	bool ret = SetupCrashHanders();
	if (!ret)
	{
		::MessageBoxA(hMainWindow, "InitCrashReport failed.", CRASH_MSGBOX_CAPTION, 0);
	}
	return ret;
}

void SetMainWindow(HWND hWnd)
{
	hMainWindow = hWnd;
}
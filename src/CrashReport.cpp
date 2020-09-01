//
// CrashReport.cpp
//
// Created by liuliu.mz on 20-08-26
//

#include <windows.h>

#include "CrashReport.h"
#include "CrashUtils.h"
#include "CrashHandler.h"
#include "cvconst.h"

// 
bool InitCrashReport()
{
	bool ret = SetupHanders();
	if (!ret)
	{
		::MessageBoxA(0, "InitCrashReport failed.", "MzCrashRpt", 0);
	}
	return ret;
}

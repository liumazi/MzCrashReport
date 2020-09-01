//
// CrashHandler.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include "CrashHandler.h"
#include "CrashDump.h"

LPTOP_LEVEL_EXCEPTION_FILTER _SEH_Handler = nullptr;

LONG WINAPI SEH_Handler(_In_ PEXCEPTION_POINTERS eps)
{
	CrashDumpShow(eps);

	return EXCEPTION_EXECUTE_HANDLER; // ÒÑ±¨¸æ ÍË°É
}

bool SetupHanders()
{
	_SEH_Handler = SetUnhandledExceptionFilter(SEH_Handler);

	return (_SEH_Handler != nullptr);
}
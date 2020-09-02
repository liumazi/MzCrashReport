//
// CrashHandler.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include "CrashHandler.h"
#include "CrashShow.h"

static LPTOP_LEVEL_EXCEPTION_FILTER _SEH_Handler = nullptr;

LONG WINAPI SEH_Handler(_In_ PEXCEPTION_POINTERS eps)
{
	DoCrashShow(eps);

	return EXCEPTION_EXECUTE_HANDLER; // ÒÑ±¨¸æ ÍË°É
}

bool SetupCrashHanders()
{
	_SEH_Handler = SetUnhandledExceptionFilter(SEH_Handler);

	return (_SEH_Handler != nullptr);
}
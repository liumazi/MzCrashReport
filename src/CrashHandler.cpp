//
// CrashHandler.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include "CrashHandler.h"

LONG WINAPI SEH_Handler(_In_ PEXCEPTION_POINTERS eps)
{
	SaveCrashDmp(eps);
	SaveCrashLog(eps);
	return EXCEPTION_EXECUTE_HANDLER; // ÒÑ±¨¸æ ÍË°É
}
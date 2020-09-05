//
// CrashHandler.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include <new.h>

#include "CrashHandler.h"
#include "CrashShow.h"

#define MzExceptionCode 0x66666666

static LPTOP_LEVEL_EXCEPTION_FILTER _SEH_Handler = nullptr;
static _purecall_handler _PureCall_Handler = nullptr;
static _PNH _New_Handler = nullptr;
static _invalid_parameter_handler _InvalidParameter_Handler = nullptr;

LONG WINAPI SEH_Handler(_In_ PEXCEPTION_POINTERS eps)
{
	DoCrashShow(eps);

	return EXCEPTION_EXECUTE_HANDLER; // ÒÑ±¨¸æ ÍË°É
}

#if _MSC_VER >= 1300
// C++ pure virtual call handler
void PureCall_Handler()
{
	RaiseException(MzExceptionCode, 0, 0, nullptr);
}

int New_Handler(size_t sz)
{
	RaiseException(MzExceptionCode, 0, 0, nullptr);
	return 0;
}

void(__cdecl *_invalid_parameter_handler)(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t);
invalid_parameter_handler

#endif 

bool SetupCrashHanders()
{
	_SEH_Handler = SetUnhandledExceptionFilter(SEH_Handler);

#if _MSC_VER >= 1300
	// Catch pure virtual function calls.
	// Because there is one _purecall_handler for the whole process, 
	// calling this function immediately impacts all threads. The last 
	// caller on any thread sets the handler. 
	// http://msdn.microsoft.com/en-us/library/t296ys27.aspx
	_PureCall_Handler = _set_purecall_handler(PureCall_Handler);

	// Catch new operator memory allocation exceptions
	_set_new_mode(1); // Force malloc() to call new handler too
	_New_Handler = _set_new_handler(New_Handler);
#endif

#if _MSC_VER >= 1400
	// Catch invalid parameter exceptions.
	_InvalidParameter_Handler = _set_invalid_parameter_handler(InvalidParameterHandler);
#endif

	return (_SEH_Handler != nullptr);
}
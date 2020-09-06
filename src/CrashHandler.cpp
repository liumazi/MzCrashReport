//
// CrashHandler.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include <new.h>
#include <signal.h>
#include <eh.h>

#include "CrashHandler.h"
#include "CrashShow.h"
#include "CrashUtils.h"

#define MzExceptionBaseCode 0x66666660

static LPTOP_LEVEL_EXCEPTION_FILTER _SEH_Handler = nullptr;
static _purecall_handler _PureCall_Handler = nullptr;
static _PNH _New_Handler = nullptr;
static _invalid_parameter_handler _InvalidParameter_Handler = nullptr;
static void(__cdecl * _SigAbrt_Handler)(int) = nullptr;
static terminate_function _Terminate_Handler = nullptr;
static unexpected_function _Unexpected_Handler = nullptr;

LONG WINAPI SEH_Handler(_In_ PEXCEPTION_POINTERS eps)
{
	// TODO: eps->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW ??
	DoCrashShow(eps);
	return EXCEPTION_EXECUTE_HANDLER; // 886
}

#if _MSC_VER >= 1300
// C++ pure virtual call handler
void PureCall_Handler()
{
	RaiseException(MzExceptionBaseCode + 1, 0, 0, nullptr);
}

int New_Handler(size_t sz)
{
	RaiseException(MzExceptionBaseCode + 2, 0, 0, nullptr);
	return 0;
}
#endif

#if _MSC_VER >= 1400
void InvalidParameter_Handler(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t)
{
	RaiseException(MzExceptionBaseCode + 3, 0, 0, nullptr);
}
#endif

void SigAbrt_Handler(int n)
{
	RaiseException(MzExceptionBaseCode + 4, 0, 0, nullptr);
}

void Terminate_Handler()
{
	RaiseException(MzExceptionBaseCode + 5, 0, 0, nullptr);
}

void Unexpected_Handler()
{
	RaiseException(MzExceptionBaseCode + 6, 0, 0, nullptr);
}

LONG NTAPI Vectored_Handler(PEXCEPTION_POINTERS ExceptionInfo)
{
	MessageBoxA(0, "Vectored_Handler", CRASH_MSGBOX_CAPTION, 0);

	if (ExceptionInfo)
	{
		DWORD exceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;

		bool ignoreException = 
			(exceptionCode == 0xE06D7363) || // CPPExceptionCode
			(exceptionCode == 0x406D1388) || // SetThreadNameExcetpionCode
			(exceptionCode == DBG_PRINTEXCEPTION_C);

		if (ignoreException)
			return EXCEPTION_CONTINUE_SEARCH;

		/*
		_ExceptionCount++;

		if (exceptionCode == DumpExceptionCode)
		{
			DumpExceptionInfo *info = (DumpExceptionInfo*)ExceptionInfo->ExceptionRecord->ExceptionInformation;
			SaveExpceptionDump(*info, ExceptionInfo);

			OnDumpExceptionCallback(info);
		}
		else
		{
			DumpExceptionInfo info;
			info.dumpMode = dump_mode_mini;
			info.dumpReason = dump_reason_catched;
			SaveExpceptionDump(info, ExceptionInfo);
		}
		*/
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

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
	// Catch invalid parameter exceptions. Release下如果安装SIGABRT会进入PureCall_Handler?? 可能是堆栈分析有误; Debug下会弹出错误消息, 然后进入SigAbrt_Handler
	_InvalidParameter_Handler = _set_invalid_parameter_handler(InvalidParameter_Handler);
#endif

#if _MSC_VER >= 1300 && _MSC_VER < 1400    
	// Catch buffer overrun exceptions
	// The _set_security_error_handler is deprecated in VC8 C++ run time library
	_Security_Handler = _set_security_error_handler(Security_Handler);
#endif

	// Set up C++ signal handlers
#if _MSC_VER >= 1400
	//_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

	// Catch an abnormal program termination
	_SigAbrt_Handler = signal(SIGABRT, SigAbrt_Handler); // SIGINT SIGTERM SIGKILL no need

	// only for current thread, and no need, becasue SEH..
	//signal(SIGSEGV, );
	//signal(SIGILL, );
	//signal(SIGFPE, );	

	// Catch terminate() calls. 
	// In a multithreaded environment, terminate functions are maintained 
	// separately for each thread. Each new thread needs to install its own 
	// terminate function. Thus, each thread is in charge of its own termination handling.
	// http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
	_Terminate_Handler = set_terminate(Terminate_Handler); // TODO: 暂未找到方法触发

	// Catch unexpected() calls.
	// In a multithreaded environment, unexpected functions are maintained 
	// separately for each thread. Each new thread needs to install its own 
	// unexpected function. Thus, each thread is in charge of its own unexpected handling.
	// http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
	_Unexpected_Handler = set_unexpected(Unexpected_Handler); // TODO: 暂未找到方法触发

	//AddVectoredExceptionHandler(1, Vectored_Handler);

	return (_SEH_Handler != nullptr);
}
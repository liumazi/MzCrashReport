//
// CrashLog.cpp
//
// Created by liuliu.mz on 20-08-28
//

void SaveDump(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	HANDLE hFile = ::CreateFileA(
		"mz.dmp",
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (INVALID_HANDLE_VALUE != hFile)
	{
		PMINIDUMP_EXCEPTION_INFORMATION pInfo = nullptr;
		MINIDUMP_EXCEPTION_INFORMATION info;
		info.ThreadId = ::GetCurrentThreadId();
		info.ExceptionPointers = ExceptionInfo;
		info.ClientPointers = FALSE;
		pInfo = &info;

		::MiniDumpWriteDump(
			CurrentProcess,
			::GetCurrentProcessId(),
			hFile,
			MiniDumpWithoutOptionalData,
			pInfo,
			NULL,
			NULL);
		::CloseHandle(hFile);
	}
}
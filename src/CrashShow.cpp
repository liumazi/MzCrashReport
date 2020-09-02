//
// CrashShow.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include "CrashShow.h"
#include "CrashUtils.h"
#include "CrashLog.h"

#define MAX_SYM_NAME_LEN 1024

static HANDLE hCurrentProcess = 0;
static CHAR tempBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME_LEN] = {};

bool WriteCrashDmp(const std::string& filename, PEXCEPTION_POINTERS eps)
{
	HANDLE hFile = ::CreateFileA(
		filename.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (INVALID_HANDLE_VALUE != hFile)
	{
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;

		dumpInfo.ThreadId = ::GetCurrentThreadId();
		dumpInfo.ExceptionPointers = eps;
		dumpInfo.ClientPointers = FALSE;

		bool ret = ::MiniDumpWriteDump(
			hCurrentProcess,
			::GetCurrentProcessId(),
			hFile,
			MiniDumpNormal,
			&dumpInfo,
			nullptr,
			nullptr) != FALSE;

		::CloseHandle(hFile);

		return ret;
	}

	return false;
}

// 异常码转字符串
std::string ExceptionCodeToString(DWORD ecode)
{
#define EXCEPTION(x) case EXCEPTION_##x: return (#x);
	switch (ecode)
	{
		EXCEPTION(ACCESS_VIOLATION)
		EXCEPTION(DATATYPE_MISALIGNMENT)
		EXCEPTION(BREAKPOINT)
		EXCEPTION(SINGLE_STEP)
		EXCEPTION(ARRAY_BOUNDS_EXCEEDED)
		EXCEPTION(FLT_DENORMAL_OPERAND)
		EXCEPTION(FLT_DIVIDE_BY_ZERO)
		EXCEPTION(FLT_INEXACT_RESULT)
		EXCEPTION(FLT_INVALID_OPERATION)
		EXCEPTION(FLT_OVERFLOW)
		EXCEPTION(FLT_STACK_CHECK)
		EXCEPTION(FLT_UNDERFLOW)
		EXCEPTION(INT_DIVIDE_BY_ZERO)
		EXCEPTION(INT_OVERFLOW)
		EXCEPTION(PRIV_INSTRUCTION)
		EXCEPTION(IN_PAGE_ERROR)
		EXCEPTION(ILLEGAL_INSTRUCTION)
		EXCEPTION(NONCONTINUABLE_EXCEPTION)
		EXCEPTION(STACK_OVERFLOW)
		EXCEPTION(INVALID_DISPOSITION)
		EXCEPTION(GUARD_PAGE)
		EXCEPTION(INVALID_HANDLE)
	}

	// If not one of the "known" exceptions, try to get the string
	// from NTDLL.DLL's message table.
	FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("NTDLL.DLL"), ecode, 0, tempBuffer, sizeof(tempBuffer), 0);

	return tempBuffer;
}
/*

BasicType GetBasicType(DWORD typeIndex, DWORD64 modBase)
{
	BasicType basicType;
	if (::SymGetTypeInfo(CurrentProcess, modBase, typeIndex,
		TI_GET_BASETYPE, &basicType))
	{
		return basicType;
	}

	// Get the real "TypeId" of the child.  We need this for the
	// SymGetTypeInfo( TI_GET_TYPEID ) call below.
	DWORD typeId;
	if (::SymGetTypeInfo(CurrentProcess, modBase, typeIndex, TI_GET_TYPEID, &typeId))
	{
		if (::SymGetTypeInfo(CurrentProcess, modBase, typeId, TI_GET_BASETYPE,
			&basicType))
		{
			return basicType;
		}
	}

	return btNoType;
}

char* FormatOutputValue(char* pszCurrBuffer, BasicType basicType, DWORD64 length, PVOID pAddress)
{
	// Format appropriately (assuming it's a 1, 2, or 4 bytes (!!!)
	if (length == 1)
		pszCurrBuffer += sprintf(pszCurrBuffer, " = %X", *(PBYTE)pAddress);
	else if (length == 2)
		pszCurrBuffer += sprintf(pszCurrBuffer, " = %X", *(PWORD)pAddress);
	else if (length == 4)
	{
		if (basicType == btFloat)
		{
			pszCurrBuffer += sprintf(pszCurrBuffer, " = %f", *(PFLOAT)pAddress);
		}
		else if (basicType == btChar)
		{
			if (!IsBadStringPtrA(*(PSTR*)pAddress, 32))
			{
				pszCurrBuffer += sprintf(pszCurrBuffer, " = \"%.31s\"",
					*(PDWORD)pAddress);
			}
			else
				pszCurrBuffer += sprintf(pszCurrBuffer, " = %X",
				*(PDWORD)pAddress);
		}
		else
			pszCurrBuffer += sprintf(pszCurrBuffer, " = %X", *(PDWORD)pAddress);
	}
	else if (length == 8)
	{
		if (basicType == btFloat)
		{
			pszCurrBuffer += sprintf(pszCurrBuffer, " = %lf",
				*(double *)pAddress);
		}
		else
			pszCurrBuffer += sprintf(pszCurrBuffer, " = %I64X",
			*(DWORD64*)pAddress);
	}

	return pszCurrBuffer;
}

// If it's a user defined type (UDT), recurse through its members until we're
// at fundamental types.  When he hit fundamental types, return
// bHandled = false, so that FormatSymbolValue() will format them.
char* DumpTypeIndex(char* pszCurrBuffer, DWORD64 modBase, DWORD dwTypeIndex,
	unsigned nestingLevel, DWORD_PTR offset, bool & bHandled, char* Name)
{
	bHandled = false;

	// Get the name of the symbol.  This will either be a Type name (if a UDT),
	// or the structure member name.
	WCHAR * pwszTypeName;
	if (::SymGetTypeInfo(CurrentProcess, modBase, dwTypeIndex, TI_GET_SYMNAME,
		&pwszTypeName))
	{
		pszCurrBuffer += sprintf(pszCurrBuffer, " %ls", pwszTypeName);
		LocalFree(pwszTypeName);
	}

	// Determine how many children this type has.
	DWORD dwChildrenCount = 0;
	::SymGetTypeInfo(CurrentProcess, modBase, dwTypeIndex, TI_GET_CHILDRENCOUNT,
		&dwChildrenCount);

	if (!dwChildrenCount)                                 // If no children, we're done
		return pszCurrBuffer;

	// Prepare to get an array of "TypeIds", representing each of the children.
	// SymGetTypeInfo(TI_FINDCHILDREN) expects more memory than just a
	// TI_FINDCHILDREN_PARAMS struct has.  Use derivation to accomplish this.
	struct FINDCHILDREN : TI_FINDCHILDREN_PARAMS
	{
		ULONG   MoreChildIds[1024];
		FINDCHILDREN(){ Count = sizeof(MoreChildIds) / sizeof(MoreChildIds[0]); }
	} children;

	children.Count = dwChildrenCount;
	children.Start = 0;

	// Get the array of TypeIds, one for each child type
	if (!::SymGetTypeInfo(CurrentProcess, modBase, dwTypeIndex, TI_FINDCHILDREN,
		&children))
	{
		return pszCurrBuffer;
	}

	// Append a line feed
	pszCurrBuffer += sprintf(pszCurrBuffer, "\r\n");

	// Iterate through each of the children
	for (unsigned i = 0; i < dwChildrenCount; i++)
	{
		// Add appropriate indentation level (since this routine is recursive)
		for (unsigned j = 0; j <= nestingLevel + 1; j++)
			pszCurrBuffer += sprintf(pszCurrBuffer, "\t");

		// Recurse for each of the child types
		bool bHandled2;
		BasicType basicType = GetBasicType(children.ChildId[i], modBase);
		pszCurrBuffer += sprintf(pszCurrBuffer, rgBaseType[basicType]);

		pszCurrBuffer = DumpTypeIndex(pszCurrBuffer, modBase,
			children.ChildId[i], nestingLevel + 1,
			offset, bHandled2, ""/*Name * /);

		// If the child wasn't a UDT, format it appropriately
		if (!bHandled2)
		{
			// Get the offset of the child member, relative to its parent
			DWORD dwMemberOffset;
			::SymGetTypeInfo(CurrentProcess, modBase, children.ChildId[i],
				TI_GET_OFFSET, &dwMemberOffset);

			// Get the real "TypeId" of the child.  We need this for the
			// SymGetTypeInfo( TI_GET_TYPEID ) call below.
			DWORD typeId;
			::SymGetTypeInfo(CurrentProcess, modBase, children.ChildId[i],
				TI_GET_TYPEID, &typeId);

			// Get the size of the child member
			ULONG64 length;
			::SymGetTypeInfo(CurrentProcess, modBase, typeId, TI_GET_LENGTH, &length);

			// Calculate the address of the member
			DWORD_PTR dwFinalOffset = offset + dwMemberOffset;
			pszCurrBuffer = FormatOutputValue(pszCurrBuffer, basicType,
				length, (PVOID)dwFinalOffset);

			pszCurrBuffer += sprintf(pszCurrBuffer, "\r\n");
		}
	}

	bHandled = true;
	return pszCurrBuffer;
}

// Given a SYMBOL_INFO representing a particular variable, displays its
// contents.  If it's a user defined type, display the members and their
// values.
bool FormatSymbolValue(PSYMBOL_INFO pSym, STACKFRAME* sf, char* pszBuffer, unsigned cbBuffer)
{
	char* pszCurrBuffer = pszBuffer;

	// Indicate if the variable is a local or parameter
	if (pSym->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER)
		pszCurrBuffer += sprintf(pszCurrBuffer, "Parameter ");
	else if (pSym->Flags & IMAGEHLP_SYMBOL_INFO_LOCAL)
		pszCurrBuffer += sprintf(pszCurrBuffer, "Local ");

	// If it's a function, don't do anything.
	if (pSym->Tag == 5)                                   // SymTagFunction from CVCONST.H from the DIA SDK
		return false;

	DWORD_PTR pVariable = 0;                                // Will point to the variable's data in memory

	if (pSym->Flags & IMAGEHLP_SYMBOL_INFO_REGRELATIVE)
	{
		// if ( pSym->Register == 8 )   // EBP is the value 8 (in DBGHELP 5.1)
		{                                                   //  This may change!!!
			pVariable = sf->AddrFrame.Offset;
			pVariable += (DWORD_PTR)pSym->Address;
		}
		// else
		//  return false;
	}
	else if (pSym->Flags & IMAGEHLP_SYMBOL_INFO_REGISTER)
	{
		return false;                                       // Don't try to report register variable
	}
	else
	{
		pVariable = (DWORD_PTR)pSym->Address;               // It must be a global variable
	}

	// Determine if the variable is a user defined type (UDT).  IF so, bHandled
	// will return true.
	bool bHandled;
	pszCurrBuffer = DumpTypeIndex(pszCurrBuffer, pSym->ModBase, pSym->TypeIndex,
		0, pVariable, bHandled, pSym->Name);

	if (!bHandled)
	{
		// The symbol wasn't a UDT, so do basic, stupid formatting of the
		// variable.  Based on the size, we're assuming it's a char, WORD, or
		// DWORD.
		BasicType basicType = GetBasicType(pSym->TypeIndex, pSym->ModBase);
		pszCurrBuffer += sprintf(pszCurrBuffer, rgBaseType[basicType]);

		// Emit the variable name
		pszCurrBuffer += sprintf(pszCurrBuffer, "\'%s\'", pSym->Name);

		pszCurrBuffer = FormatOutputValue(pszCurrBuffer, basicType, pSym->Size,
			(PVOID)pVariable);
	}

	return true;
}*/

char szBuffer[2048];

BOOL CALLBACK EnumSymbolsProcCallback(PSYMBOL_INFO pSymInfo, ULONG SymSize, PVOID sf)
{
	/*
	// we're only interested in parameters and local variables
	if (pSymInfo->Flags & SYMF_PARAMETER || pSymInfo->Flags & SYMF_LOCAL)
	{
		__try
		{
			if (FormatSymbolValue(pSymInfo, uc->sf, szBuffer, _countof(szBuffer)))
			{
				sprintf(FmtBuffer, "\t%s\n", szBuffer);
				uc->ret.append(FmtBuffer);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
	*/
	// return true to continue enumeration, false would have stopped it
	return TRUE;
}

void RetrieveSymbolParam(const STACKFRAME& sf)
{
	DWORD dwAddrPC = sf.AddrPC.Offset;

	// use SymSetContext to get just the locals/params for this frame

	IMAGEHLP_STACK_FRAME imagehlpStackFrame = {};
	imagehlpStackFrame.InstructionOffset = dwAddrPC;
	if (!::SymSetContext(hCurrentProcess, &imagehlpStackFrame, 0))
	{
		// for symbols from kernel DLL we might not have access to their
		// address, this is not a real error
		AppendCrashLog("%08X, SymSetContext fail\r\n", dwAddrPC);
		return;
	}

	if (!::SymEnumSymbols(
		hCurrentProcess,
		NULL,           // DLL base: use current context
		NULL,           // no mask, get all symbols
		EnumSymbolsProcCallback,
		(PVOID)&sf))    // data parameter for this callback
	{
		AppendCrashLog("%08X, SymSetContext fail\r\n", dwAddrPC);
		return;
	}
}

void RetrieveSymbolName(const STACKFRAME& sf)
{
	DWORD dwAddress = sf.AddrPC.Offset;

	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)tempBuffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME_LEN;

	DWORD64 symDisplacement = 0;
	if (!::SymFromAddr(hCurrentProcess, dwAddress, &symDisplacement, pSymbol))
	{
		// LOG_LAST_ERROR();
		AppendCrashLog("%08X, SymFromAddr Failed \r\n", dwAddress);
		return;
	}

	IMAGEHLP_LINE line = { sizeof(IMAGEHLP_LINE) };
	DWORD dwLineDisplacement = 0;
	if (!::SymGetLineFromAddr(hCurrentProcess, dwAddress, &dwLineDisplacement, &line))
	{
		// it is normal that we don't have source info for some symbols,
		// notably all the ones from the system DLLs...
		AppendCrashLog("%08X, %s()\r\n", dwAddress/*pSymbol->Address*/, pSymbol->Name); // 08I64X
		return;
	}

	AppendCrashLog("%08X, %s(), line %u in\r\n  %s\r\n", dwAddress/*pSymbol->Address*/, pSymbol->Name, line.LineNumber, line.FileName); // 08I64X
}

void WalkCrashCallStack(CONTEXT ct, size_t skip = 0, size_t depth = 20)
{
	// initialize the initial frame: currently we can do it for x86 only
	STACKFRAME sf = {};
#if defined(_M_AMD64)
	sf.AddrPC.Offset = ct.Rip;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Offset = ct.Rsp;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = ct.Rbp;
	sf.AddrFrame.Mode = AddrModeFlat;
	DWORD mt = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86)
	sf.AddrPC.Offset = ct.Eip;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Offset = ct.Esp;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = ct.Ebp;
	sf.AddrFrame.Mode = AddrModeFlat;
	DWORD mt = IMAGE_FILE_MACHINE_I386;
#else
#error "Need to initialize STACKFRAME on non x86"
#endif // _M_IX86

	std::string ret;

	// iterate over all stack frames
	for (size_t nLevel = 0; nLevel < depth; nLevel++)
	{
		if (!::StackWalk(
			mt,
			hCurrentProcess,
			::GetCurrentThread(),
			&sf,
			&ct,     // 注意: ct可能被修改 
			nullptr, // read memory function (default)
			::SymFunctionTableAccess,
			::SymGetModuleBase,
			nullptr))// address translator for 16 bit
		{
			break;
		}

		// Invalid frame
		if (!sf.AddrFrame.Offset)
		{
			break;
		}

		// don't show this frame itself in the output
		// if (nLevel >= skip)
		{
			RetrieveSymbolName(sf);
			//RetrieveSymbolParam(sf);
			AppendCrashLog("\r\n");
		}
	}
}

bool WriteCrashLog(const std::string& filename, PEXCEPTION_POINTERS eps)
{
	CreateCrashLog(filename);

	tempBuffer[0] = 0;
	DWORD code = eps->ExceptionRecord->ExceptionCode;
	PVOID addr = eps->ExceptionRecord->ExceptionAddress;
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(addr, &mbi, sizeof(mbi)))
	{
		if (!GetModuleFileNameA((HMODULE)mbi.AllocationBase, tempBuffer, MAX_PATH))
		{
			tempBuffer[0] = 0;
		}
	}
	AppendCrashLog("ModuleFileName: %s \r\n\r\n", tempBuffer);

	AppendCrashLog("CurrentThreadId: %d\r\n\r\n", ::GetCurrentThreadId());

	AppendCrashLog("ExceptionAddress: 0x%08X\r\n", (DWORD)(addr));

	if (code == EXCEPTION_ACCESS_VIOLATION)
	{
		ULONG_PTR* ExceptionInformation = eps->ExceptionRecord->ExceptionInformation;
		if (ExceptionInformation[0])
		{
			AppendCrashLog("Failed to write address 0x%08X\r\n", ExceptionInformation[1]);
		}
		else
		{
			AppendCrashLog("Failed to read address 0x%08X\r\n", ExceptionInformation[1]);
		}
	}
	AppendCrashLog("\r\n");

	AppendCrashLog("ExceptionCode: 0x%08X (%s)\r\n\r\n", code, ExceptionCodeToString(code).c_str());

	AppendCrashLog(("Call Stack:\r\n------------------------------------------------------\r\n"));

	if (!::SymInitialize(hCurrentProcess, nullptr, TRUE))
	{
		AppendCrashLog("SymInitialize failed");
	}
	else
	{
		WalkCrashCallStack(*eps->ContextRecord);
	}
	
	CloseCrashLog();

	return true;
}

void DoCrashShow(PEXCEPTION_POINTERS eps)
{
	if (!eps)
	{
		::MessageBoxA(GetActiveWindow(), "Invaild eps.", CRASH_MSGBOX_CAPTION, 0);
	}

	hCurrentProcess = ::GetCurrentProcess();
	std::string curFileName = GenDumpFileName();

	std::string dmpFileName = curFileName + ".dmp";
	WriteCrashDmp(dmpFileName, eps);

	std::string logFileName = curFileName + ".log";
	WriteCrashLog(logFileName, eps);

	MessageBoxA(GetActiveWindow(), "Click OK to open the crash log file.", CRASH_MSGBOX_CAPTION, 0);

	ShellExecuteA(0, "open", "explorer.exe", ("/select," + logFileName).c_str(), nullptr, SW_SHOWNORMAL);
}
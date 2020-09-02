//
// CrashShow.cpp
//
// Created by liuliu.mz on 20-08-28
//

#include <locale>
#include <codecvt>
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include "CrashShow.h"
#include "CrashUtils.h"
#include "CrashLog.h"
#include "CrashConst.h"

#define MAX_SYM_NAME_LEN 1024

static HANDLE _hCurrentProcess = 0;
static CHAR _tempBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME_LEN] = {};

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
			_hCurrentProcess,
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
	FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("NTDLL.DLL"), ecode, 0, _tempBuffer, sizeof(_tempBuffer), 0);

	return _tempBuffer;
}

BasicType GetBasicType(DWORD typeIndex, DWORD64 modBase)
{
	BasicType basicType;
	if (::SymGetTypeInfo(_hCurrentProcess, modBase, typeIndex,
		TI_GET_BASETYPE, &basicType))
	{
		return basicType;
	}

	// Get the real "TypeId" of the child.  We need this for the
	// SymGetTypeInfo( TI_GET_TYPEID ) call below.
	DWORD typeId;
	if (::SymGetTypeInfo(_hCurrentProcess, modBase, typeIndex, TI_GET_TYPEID, &typeId))
	{
		if (::SymGetTypeInfo(_hCurrentProcess, modBase, typeId, TI_GET_BASETYPE,
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

#define BASE_TYPE_NONE 0
#define BASE_TYPE_VOID 1
#define BASE_TYPE_BOOL 2
#define BASE_TYPE_CHAR 3
#define BASE_TYPE_UCHAR 4
#define BASE_TYPE_WCHAR 5
#define BASE_TYPE_SHORT 6
#define BASE_TYPE_USHORT 7
#define BASE_TYPE_INT 8
#define BASE_TYPE_UINT 9
#define BASE_TYPE_LONG 10
#define BASE_TYPE_ULONG 11
#define BASE_TYPE_LONGLONG 12
#define BASE_TYPE_ULONGLONG 13
#define BASE_TYPE_FLOAT 14
#define BASE_TYPE_DOUBLE 15
#define BASE_TYPE_END 16

static std::string _baseTypeNames[BASE_TYPE_END] =
{
	"none type",
	"void",
	"bool",
	"char",
	"unsigned char",
	"wchar",
	"short",
	"unsigned short",
	"int",
	"unsigned int",
	"long",
	"unsigned long",
	"long long",
	"unsigned long long",
	"float",
	"double"
};

int GetBaseType(ULONG64 modBase, ULONG typeID)
{
	DWORD baseType;
	::SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_BASETYPE,
		&baseType);

	ULONG64 length;
	::SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_LENGTH,
		&length);

	switch (baseType)
	{
	case btVoid:
		return BASE_TYPE_VOID;

	case btChar:
		return BASE_TYPE_CHAR;

	case btWChar:
		return BASE_TYPE_WCHAR;

	case btInt:
		switch (length)
		{
		case 2:
			return BASE_TYPE_SHORT;

		case 4:
			return BASE_TYPE_INT;

		default:
			return BASE_TYPE_LONGLONG;
		}

	case btUInt:
		switch (length)
		{
		case 1:
			return BASE_TYPE_UCHAR;

		case 2:
			return BASE_TYPE_USHORT;

		case 4:
			return BASE_TYPE_UINT;

		default:
			return BASE_TYPE_ULONGLONG;
		}

	case btFloat:
		switch (length) 
		{
		case 4:
			return BASE_TYPE_FLOAT;

		default:
			return BASE_TYPE_DOUBLE;
		}

	case btBool:
		return BASE_TYPE_BOOL;

	case btLong:
		return BASE_TYPE_LONG;

	case btULong:
		return BASE_TYPE_ULONG;

	default:
		return BASE_TYPE_NONE;
	}
}

std::string GetTypeName(ULONG64 modBase, ULONG typeID); // 前置声明

std::string GetBaseTypeName(ULONG64 modBase, ULONG typeID)
{
	return _baseTypeNames[GetBaseType(modBase, typeID)];
}

std::string GetPointerTypeName(ULONG64 modBase, ULONG typeID)
{
	BOOL isReference;
	::SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_IS_REFERENCE,
		&isReference);

	ULONG innerTypeID;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_TYPEID,
		&innerTypeID);

	return GetTypeName(modBase, innerTypeID) + (isReference == TRUE ? "&" : "*");
}

std::string GetArrayTypeName(ULONG64 modBase, ULONG typeID)
{
	ULONG innerTypeID;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_TYPEID,
		&innerTypeID);

	DWORD elemCount;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_COUNT,
		&elemCount);

	return GetTypeName(modBase, innerTypeID) + "[" + std::to_string(elemCount) + "]";
}

std::string GetFunctionTypeName(ULONG64 modBase, ULONG typeID)
{
	std::string ret;

	DWORD paramCount;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_CHILDRENCOUNT,
		&paramCount);

	ULONG returnTypeID;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_TYPEID,
		&returnTypeID);

	ret += GetTypeName(modBase, returnTypeID);

	BYTE* pBuffer = (BYTE*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * paramCount);
	TI_FINDCHILDREN_PARAMS* pFindParams = (TI_FINDCHILDREN_PARAMS*)pBuffer;
	pFindParams->Count = paramCount;
	pFindParams->Start = 0;

	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_FINDCHILDREN,
		pFindParams);

	ret += "(";

	for (DWORD i = 0; i < paramCount; i++)
	{
		DWORD paramTypeID;
		SymGetTypeInfo(
			_hCurrentProcess,
			modBase,
			pFindParams->ChildId[i],
			TI_GET_TYPEID,
			&paramTypeID);

		if (i != 0) 
		{
			ret += ", ";
		}

		ret += GetTypeName(modBase, paramTypeID);
	}

	free(pBuffer);

	ret += ")";

	return ret;
}

std::string GetNameableTypeName(ULONG64 modBase, ULONG typeID)
{
	WCHAR* symName;

	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_SYMNAME,
		&symName);

	std::wstring ret = symName;

	LocalFree(symName);

	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvter;
	return cvter.to_bytes(ret);
}

std::string GetTypeName(ULONG64 modBase, ULONG typeID)
{
	DWORD symTag;
	::SymGetTypeInfo(_hCurrentProcess, modBase, typeID, TI_GET_SYMTAG, &symTag);

	switch (symTag)
	{
	case SymTagBaseType:
		return GetBaseTypeName(modBase, typeID);

	case SymTagPointerType:
		return GetPointerTypeName(modBase, typeID);

	case SymTagArrayType:
		return GetArrayTypeName(modBase, typeID);

	case SymTagUDT:
		return GetNameableTypeName(modBase, typeID); // GetUDTTypeName

	case SymTagEnum:
		return GetNameableTypeName(modBase, typeID); // GetEnumTypeName

	case SymTagFunctionType:
		return GetFunctionTypeName(modBase, typeID);

	default:
		return "unkown_type";
	}
}


#define MAX_TI_FINDCHILDREN_NUM 66

bool RetrieveFunVarSummary(const SYMBOL_INFO *pSymInfo, DWORD_PTR offset, int level)
{
	bool ret = false;

	for (int i = 0; i < level; i++)
	{
		AppendCrashLog("\t");
	}
	AppendCrashLog("%s %s \n\r", GetTypeName(pSymInfo->ModBase, pSymInfo->TypeIndex).c_str(), pSymInfo->Name);

	/*

	// Get the name of the symbol.
	// This will either be a Type name (if a UDT), or the structure member name.
	WCHAR* pwszTypeName;
	if (::SymGetTypeInfo(_hCurrentProcess, modBase, dwTypeIndex, TI_GET_SYMNAME, &pwszTypeName))
	{
		AppendCrashLog(" %ls", pwszTypeName);
		LocalFree(pwszTypeName);
	}

	// Determine how many children this type has.
	DWORD dwChildrenCount = 0;
	::SymGetTypeInfo(_hCurrentProcess, modBase, dwTypeIndex, TI_GET_CHILDRENCOUNT, &dwChildrenCount);

	// If no children, we're done
	if (!dwChildrenCount)
	{
		return ret;
	}

	if (dwChildrenCount > MAX_TI_FINDCHILDREN_NUM)
	{
		dwChildrenCount = MAX_TI_FINDCHILDREN_NUM;
	}

	// Prepare to get an array of "TypeIds", representing each of the children.
	// SymGetTypeInfo(TI_FINDCHILDREN) expects more memory than just a
	// TI_FINDCHILDREN_PARAMS struct has.  Use derivation to accomplish this.
	struct TI_FINDCHILDREN_PARAMS_EX : TI_FINDCHILDREN_PARAMS
	{
		ULONG MoreChildIds[MAX_TI_FINDCHILDREN_NUM]; // TODO: 固定大小太浪费
	} children;

	children.Count = dwChildrenCount;
	children.Start = 0;

	// Get the array of TypeIds, one for each child type
	if (!::SymGetTypeInfo(_hCurrentProcess, modBase, dwTypeIndex, TI_FINDCHILDREN, &children))
	{
		return ret;
	}

	// Append a line feed
	AppendCrashLog("\r\n");

	// Iterate through each of the children
	for (unsigned i = 0; i < dwChildrenCount; i++)
	{
		// Add appropriate indentation level (since this routine is recursive)
		for (unsigned j = 0; j <= nestingLevel + 1; j++)
		{
			AppendCrashLog("\t");
		}

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
	return pszCurrBuffer;*/

	return true;
}

// Given a SYMBOL_INFO representing a particular variable, displays its contents.
// If it's a user defined type, display the members and their values.
bool RetrieveFunVar(const SYMBOL_INFO *pSymInfo, const STACKFRAME *sf)
{
	// Indicate if the variable is a local or parameter
	if (pSymInfo->Flags & SYMF_PARAMETER)
		AppendCrashLog("Parameter: ");
	else if (pSymInfo->Flags & SYMF_LOCAL)
		AppendCrashLog("Local var: ");
	else
		return false;

	// If it's a function, don't do anything. TODO: ..
	//if (pSymInfo->Tag == SymTagFunction)
	//	return false;

	DWORD_PTR pVariable = 0; // Will point to the variable's data in memory

	if (pSymInfo->Flags & IMAGEHLP_SYMBOL_INFO_REGRELATIVE) // SYMFLAG_REGREL
	{
		//if ( pSymInfo->Register == 8 ) // EBP is the value 8 (in DBGHELP 5.1)
		{                                // This may change !!! ??
			pVariable = sf->AddrFrame.Offset; // TODO: EBP ??
			pVariable += (DWORD_PTR)pSymInfo->Address;
		}
		//else
		//  return false;
	}
	else if (pSymInfo->Flags & IMAGEHLP_SYMBOL_INFO_REGISTER) // SYMF_REGISTER
	{
		return false;                    // Don't try to report register variable, TODO: ..
	}
	else
	{
		pVariable = (DWORD_PTR)pSymInfo->Address; // It must be a global variable
	}

	return RetrieveFunVarSummary(pSymInfo, pVariable, 0);
	/*
	// Determine if the variable is a user defined type (UDT). IF so, will return true.
	if (!)
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
	*/
}

BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO pSymInfo, ULONG uSymSize, PVOID sf)
{
	// we're only interested in parameters and local variables
	if (pSymInfo->Flags & SYMF_PARAMETER || pSymInfo->Flags & SYMF_LOCAL) // if (pSymInfo->Tag == SymTagData) ??
	{
		__try
		{
			AppendCrashLog("\t");
			RetrieveFunVar(pSymInfo, (LPSTACKFRAME)sf);
			AppendCrashLog("\r\n");
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	// return true to continue enumeration, false would have stopped it
	return TRUE;
}

void RetrieveFunVars(const STACKFRAME& sf)
{
	DWORD dwAddress = sf.AddrPC.Offset;

	// use SymSetContext to get just the locals/params for this frame

	IMAGEHLP_STACK_FRAME imagehlpStackFrame = {};
	imagehlpStackFrame.InstructionOffset = dwAddress;
	if (!::SymSetContext(_hCurrentProcess, &imagehlpStackFrame, 0) && GetLastError() != ERROR_SUCCESS)
	{
		// for symbols from kernel DLL we might not have access to their
		// address, this is not a real error
		AppendCrashLog("%08X, SymSetContext fail\r\n", dwAddress);
		return;
	}

	if (!::SymEnumSymbols(
		_hCurrentProcess,
		NULL,           // DLL base: use current context
		NULL,           // no mask, get all symbols
		EnumSymbolsCallback,
		(PVOID)&sf))    // data parameter for this callback
	{
		AppendCrashLog("%08X, SymSetContext fail\r\n", dwAddress);
		return;
	}
}

void RetrieveFunName(const STACKFRAME& sf)
{
	DWORD dwAddress = sf.AddrPC.Offset;

	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)_tempBuffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME_LEN;

	DWORD64 symDisplacement = 0;
	if (!::SymFromAddr(_hCurrentProcess, dwAddress, &symDisplacement, pSymbol))
	{
		// LOG_LAST_ERROR();
		AppendCrashLog("%08X, SymFromAddr Failed \r\n", dwAddress);
		return;
	}

	IMAGEHLP_LINE line = { sizeof(IMAGEHLP_LINE) };
	DWORD dwLineDisplacement = 0;
	if (!::SymGetLineFromAddr(_hCurrentProcess, dwAddress, &dwLineDisplacement, &line))
	{
		// it is normal that we don't have source info for some symbols,
		// notably all the ones from the system DLLs...
		AppendCrashLog("%08X, %s()\r\n", dwAddress/*pSymbol->Address*/, pSymbol->Name); // 08I64X
		return;
	}

	AppendCrashLog("%08X, %s(), line %u in %s\r\n", dwAddress/*pSymbol->Address*/, pSymbol->Name, line.LineNumber, line.FileName); // 08I64X
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
			_hCurrentProcess,
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
			RetrieveFunName(sf);
			RetrieveFunVars(sf);
			AppendCrashLog("\r\n");
		}
	}
}

bool WriteCrashLog(const std::string& filename, PEXCEPTION_POINTERS eps)
{
	CreateCrashLog(filename);

	_tempBuffer[0] = 0;
	DWORD code = eps->ExceptionRecord->ExceptionCode;
	PVOID addr = eps->ExceptionRecord->ExceptionAddress;
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(addr, &mbi, sizeof(mbi)))
	{
		if (!GetModuleFileNameA((HMODULE)mbi.AllocationBase, _tempBuffer, MAX_PATH))
		{
			_tempBuffer[0] = 0;
		}
	}
	AppendCrashLog("ModuleFileName: %s \r\n\r\n", _tempBuffer);

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

	if (!::SymInitialize(_hCurrentProcess, nullptr, TRUE))
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

	_hCurrentProcess = ::GetCurrentProcess();
	std::string curFileName = GenDumpFileName();

	std::string dmpFileName = curFileName + ".dmp";
	WriteCrashDmp(dmpFileName, eps);

	std::string logFileName = curFileName + ".log";
	WriteCrashLog(logFileName, eps);

	MessageBoxA(GetActiveWindow(), "Click OK to open the crash log file.", CRASH_MSGBOX_CAPTION, 0);

	ShellExecuteA(0, "open", "explorer.exe", ("/select," + logFileName).c_str(), nullptr, SW_SHOWNORMAL);
}
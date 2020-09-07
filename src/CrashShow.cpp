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
#include "CrashConst.h"
#include "CrashEHData.h"

#define MAX_SYM_NAME_LEN 1024

static HANDLE _hCurrentProcess = 0;
static CHAR _tempBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME_LEN] = {};

// 前置声明
std::string GetTypeName(ULONG64 modBase, ULONG typeID);
std::string GetTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* address);
//

bool WriteCrashDmp(const std::string& filename, PEXCEPTION_POINTERS eps)
{
	HANDLE hFile = CreateFileA(
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

		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ExceptionPointers = eps;
		dumpInfo.ClientPointers = FALSE;

		bool ret = MiniDumpWriteDump(
			_hCurrentProcess,
			GetCurrentProcessId(),
			hFile,
			MiniDumpNormal,
			&dumpInfo,
			nullptr,
			nullptr) != FALSE;

		CloseHandle(hFile);

		return ret;
	}

	return false;
}

// 异常码转字符串
std::string ExceptionCodeToString(DWORD ecode, MSVC_ThrowInfo* einfo)
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

	case MZ_EXCEPTION_BASE_CODE + 0:
		return "PureCall";

	case MZ_EXCEPTION_BASE_CODE + 1:
		return "New";

	case MZ_EXCEPTION_BASE_CODE + 2:
		return "InvalidParameter";

	case MZ_EXCEPTION_BASE_CODE + 3:
		return "SigAbrt";

	case MZ_EXCEPTION_BASE_CODE + 4:
		return "Terminate";

	case MZ_EXCEPTION_BASE_CODE + 5:
		return "Unexpected";

	case 0xE06D7363:
		{
		std::string ret = "CppException { ";

		for (int i = 0; i < einfo->pCatchableTypeArray->nCatchableTypes; i++)
		{
			ret += "[" + std::to_string(i) + "] = ";
			ret += einfo->pCatchableTypeArray->arrayOfCatchableTypes[i]->pType->name;
			ret += "; ";
		}

		ret += "}";
		return ret;
		}
	}

	// If not one of the "known" exceptions, try to get the string
	// from NTDLL.DLL's message table.
	_tempBuffer[FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("NTDLL.DLL"), ecode, 0, _tempBuffer, sizeof(_tempBuffer), 0)] = 0;
	return _tempBuffer;
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
	"none_type_?",
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
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_BASETYPE,
		&baseType);

	ULONG64 length;
	SymGetTypeInfo(
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

std::string GetBaseTypeName(ULONG64 modBase, ULONG typeID)
{
	return _baseTypeNames[GetBaseType(modBase, typeID)];
}

std::string GetPointerTypeName(ULONG64 modBase, ULONG typeID)
{
	BOOL isReference;
	SymGetTypeInfo(
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
	std::string ret = "";

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

	std::string ret = WStringToString(symName);
	LocalFree(symName);

	return ret;
}

std::string GetTypeName(ULONG64 modBase, ULONG typeID)
{
	DWORD symTag;
	SymGetTypeInfo(_hCurrentProcess, modBase, typeID, TI_GET_SYMTAG, &symTag);

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
		return "unkown_type_?";
	}
}

std::string GetBaseTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* pData)
{
	int baseType = GetBaseType(modBase, typeID);

	switch (baseType)
	{
	case BASE_TYPE_NONE:
		return "none_value_?";

	case BASE_TYPE_VOID:
		return "void_value_?";

	case BASE_TYPE_BOOL:
		return *pData == 0 ? "false" : "true";

	case BASE_TYPE_CHAR:
	{
		char c = *((char*)pData);
		if (c >= 0x21 && c <= 0x7F)
		{
			return "'" + std::string(1, c) + "'";
		}
		else
		{
			sprintf(_tempBuffer, "0x%02X", c & 0x00FF);
			return _tempBuffer;
		}
	}

	case BASE_TYPE_UCHAR:
		sprintf(_tempBuffer, "0x%02X", *((unsigned char*)pData) & 0x00FF);
		return _tempBuffer;

	case BASE_TYPE_WCHAR:		
		sprintf(_tempBuffer, "0x%04X", *((wchar_t*)pData) & 0x00FFFF);
		return _tempBuffer;

	case BASE_TYPE_SHORT:
		sprintf(_tempBuffer, "%d", *((short*)pData));
		return _tempBuffer;

	case BASE_TYPE_USHORT:
		sprintf(_tempBuffer, "%u", *((unsigned short*)pData));
		return _tempBuffer;

	case BASE_TYPE_INT:
		sprintf(_tempBuffer, "%d", *((int*)pData));
		return _tempBuffer;

	case BASE_TYPE_UINT:
		sprintf(_tempBuffer, "%u", *((unsigned int*)pData));
		return _tempBuffer;

	case BASE_TYPE_LONG:
		sprintf(_tempBuffer, "%d", *((long*)pData));
		return _tempBuffer;

	case BASE_TYPE_ULONG:
		sprintf(_tempBuffer, "%u", *((unsigned long*)pData));
		return _tempBuffer;

	case BASE_TYPE_LONGLONG:
		sprintf(_tempBuffer, "%lld", *((long long*)pData));
		return _tempBuffer;

	case BASE_TYPE_ULONGLONG:
		sprintf(_tempBuffer, "%llu", *((unsigned long long*)pData));
		return _tempBuffer;

	case BASE_TYPE_FLOAT:
		sprintf(_tempBuffer, "%f", *((float*)pData));
		return _tempBuffer;

	case BASE_TYPE_DOUBLE:
		sprintf(_tempBuffer, "%lf", *((double*)pData));
		return _tempBuffer;

	default:
		return "unkown_value_?";
	}
}

std::string GetPointerTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* pData)
{
	sprintf(_tempBuffer, "0x%p", *((void**)pData));
	return _tempBuffer;
}

bool VariantEqual(const VARIANT &var, int baseType, const BYTE* pData)
{
	switch (baseType)
	{
	case BASE_TYPE_CHAR:
		return var.cVal == *((char*)pData);

	case BASE_TYPE_UCHAR:
		return var.bVal == *((unsigned char*)pData);

	case BASE_TYPE_SHORT:
		return var.iVal == *((short*)pData);

	case BASE_TYPE_WCHAR:
	case BASE_TYPE_USHORT:
		return var.uiVal == *((unsigned short*)pData);

	case BASE_TYPE_UINT:
		return var.uintVal == *((int*)pData);

	case BASE_TYPE_LONG:
		return var.lVal == *((long*)pData);

	case BASE_TYPE_ULONG:
		return var.ulVal == *((unsigned long*)pData);

	case BASE_TYPE_LONGLONG:
		return var.llVal == *((long long*)pData);

	case BASE_TYPE_ULONGLONG:
		return var.ullVal == *((unsigned long long*)pData);

	case BASE_TYPE_INT:
	default:
		return var.intVal == *((int*)pData);
	}
}

std::string GetEnumTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* pData)
{
	std::string ret = "";

	DWORD childrenCount;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_CHILDRENCOUNT,
		&childrenCount);

	TI_FINDCHILDREN_PARAMS* pFindParams = (TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + childrenCount * sizeof(ULONG));
	pFindParams->Start = 0;
	pFindParams->Count = childrenCount;

	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_FINDCHILDREN,
		pFindParams);

	for (int i = 0; i != childrenCount; i++)
	{
		VARIANT enumValue;
		SymGetTypeInfo(
			_hCurrentProcess,
			modBase,
			pFindParams->ChildId[i],
			TI_GET_VALUE,
			&enumValue);

		int baseType = GetBaseType(modBase, typeID);
		if (VariantEqual(enumValue, baseType, pData))
		{
			WCHAR* pBuffer;
			SymGetTypeInfo(
				_hCurrentProcess,
				modBase,
				pFindParams->ChildId[i],
				TI_GET_SYMNAME,
				&pBuffer);

			ret = WStringToString(pBuffer);
			LocalFree(pBuffer);

			break;
		}
	}

	free(pFindParams);

	if (ret.length() == 0)
	{
		ret = GetBaseTypeValue(modBase, typeID, pData);
	}

	return ret;
}

std::string GetArrayTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* pData)
{
	DWORD elemCount;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_COUNT,
		&elemCount);

	elemCount = elemCount > 6 ? 6 : elemCount; // 数组最多显示前6个

	DWORD innerTypeID;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_TYPEID,
		&innerTypeID);

	ULONG64 elemLen; // TODO: 如果数组成员是struct呢 ??
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		innerTypeID,
		TI_GET_LENGTH,
		&elemLen);

	std::string ret = "{ ";

	for (int i = 0; i != elemCount; i++)
	{
		ret += "[" + std::to_string(i) + "] " + GetTypeValue(modBase, innerTypeID, pData + i * elemLen) + "; ";
	}

	return ret + "}";
}

std::string GetDataMemberInfo(ULONG64 modBase, ULONG memberID, const BYTE* pData)
{
	std::string ret = "";

	DWORD memberTag;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		memberID,
		TI_GET_SYMTAG,
		&memberTag);

	if (memberTag != SymTagData && memberTag != SymTagBaseClass)
	{
		return ret;
	}

	DWORD memberTypeID;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		memberID,
		TI_GET_TYPEID,
		&memberTypeID);

	ret += GetTypeName(modBase, memberTypeID);

	if (memberTag == SymTagData)
	{
		WCHAR* name;
		SymGetTypeInfo(
			_hCurrentProcess,
			modBase,
			memberID,
			TI_GET_SYMNAME,
			&name);

		ret += " " + WStringToString(name) + " ";

		LocalFree(name);
	}
	else
	{
		ret += " <base-class> "; // ??
	}

	/*
	ULONG64 length;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		memberTypeID,
		TI_GET_LENGTH,
		&length);

	ret += "  " + std::to_string(length) + " ";
	*/

	DWORD offset;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		memberID,
		TI_GET_OFFSET,
		&offset);

	const BYTE* childAddress = pData + offset;

	ret += GetTypeValue(modBase, memberTypeID, childAddress);

	return ret;
}

std::string GetUDTTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* pData)
{
	std::string ret = "{ ";

	DWORD memberCount;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_CHILDRENCOUNT,
		&memberCount);

	TI_FINDCHILDREN_PARAMS* pFindParams = (TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + memberCount * sizeof(ULONG));
	pFindParams->Start = 0;
	pFindParams->Count = memberCount;

	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_FINDCHILDREN,
		pFindParams);

	for (int i = 0; i != memberCount; ++i)
	{
		std::string memberValue = GetDataMemberInfo(modBase, pFindParams->ChildId[i], pData);

		if (memberValue.length() > 0)
		{
			ret += memberValue + "; ";
		}
	}

	return ret + "}";
}

std::string GetTypedefTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* address)
{
	DWORD actTypeID;
	SymGetTypeInfo(
		_hCurrentProcess,
		modBase,
		typeID,
		TI_GET_TYPEID,
		&actTypeID);

	return GetTypeValue(modBase, actTypeID, address);
}

std::string GetTypeValue(ULONG64 modBase, ULONG typeID, const BYTE* address)
{
	DWORD symTag;
	SymGetTypeInfo(_hCurrentProcess, modBase, typeID, TI_GET_SYMTAG, &symTag);

	switch (symTag)
	{
	case SymTagBaseType:
		return GetBaseTypeValue(modBase, typeID, address);

	case SymTagPointerType:
		return GetPointerTypeValue(modBase, typeID, address);

	case SymTagEnum:
		return GetEnumTypeValue(modBase, typeID, address);

	case SymTagArrayType:
		return GetArrayTypeValue(modBase, typeID, address);

	case SymTagUDT:
		return GetUDTTypeValue(modBase, typeID, address);

	case SymTagTypedef:
		return GetTypedefTypeValue(modBase, typeID, address);

	default:
		return "unkown_value";
	}
}

#define MAX_TI_FINDCHILDREN_NUM 66

bool RetrieveFunVarDetail(const SYMBOL_INFO *pSymInfo, const BYTE* pVariable)
{
	bool ret = false;

	AppendCrashLog("%s %s %s \n\r", GetTypeName(pSymInfo->ModBase, pSymInfo->TypeIndex).c_str(), pSymInfo->Name, GetTypeValue(pSymInfo->ModBase, pSymInfo->TypeIndex, pVariable).c_str());

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

	const BYTE* pVariable = 0; // Will point to the variable's data in memory

	if (pSymInfo->Flags & IMAGEHLP_SYMBOL_INFO_REGRELATIVE) // SYMFLAG_REGREL
	{
		//if ( pSymInfo->Register == 8 ) // EBP is the value 8 (in DBGHELP 5.1)
		{                                // This may change !!! ??
			pVariable = (const BYTE*)sf->AddrFrame.Offset;           // TODO: esp - 4 or ebp ?? see GetSymbolAddress() in vHandler.cpp of MiniDebugger10
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
		pVariable = (const BYTE*)pSymInfo->Address; // It must be a global variable
	}

	return RetrieveFunVarDetail(pSymInfo, pVariable);
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
	if (!SymSetContext(_hCurrentProcess, &imagehlpStackFrame, 0) && GetLastError() != ERROR_SUCCESS)
	{
		// for symbols from kernel DLL we might not have access to their
		// address, this is not a real error
		AppendCrashLog("%08X, SymSetContext fail\r\n", dwAddress);
		return;
	}

	if (!SymEnumSymbols(
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
	if (!SymFromAddr(_hCurrentProcess, dwAddress, &symDisplacement, pSymbol))
	{
		// LOG_LAST_ERROR();
		AppendCrashLog("%08X, SymFromAddr Failed \r\n", dwAddress);
		return;
	}

	IMAGEHLP_LINE line = { sizeof(IMAGEHLP_LINE) };
	DWORD dwLineDisplacement = 0;
	if (!SymGetLineFromAddr(_hCurrentProcess, dwAddress, &dwLineDisplacement, &line))
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

	std::string ret = "";

	// iterate over all stack frames
	for (size_t nLevel = 0; nLevel < depth; nLevel++)
	{
		if (!StackWalk(
			mt,
			_hCurrentProcess,
			GetCurrentThread(),
			&sf,
			&ct,     // 注意: ct可能被修改 
			nullptr, // read memory function (default)
			SymFunctionTableAccess,
			SymGetModuleBase,
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
		//if (nLevel >= skip)
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
	
	AppendCrashLog("\r\n");
	AppendCrashLog("======================================================\r\n");
	AppendCrashLog("=               MzCrashReport v0.1                   =\r\n");
	AppendCrashLog("======================================================\r\n\r\n");

	DWORD code = eps->ExceptionRecord->ExceptionCode;
	PVOID addr = eps->ExceptionRecord->ExceptionAddress;
	ULONG_PTR* ExceptionInformation = eps->ExceptionRecord->ExceptionInformation;

	_tempBuffer[0] = 0;
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(addr, &mbi, sizeof(mbi)))
	{
		if (!GetModuleFileNameA((HMODULE)mbi.AllocationBase, _tempBuffer, MAX_PATH))
		{
			_tempBuffer[0] = 0;
		}
	}
	AppendCrashLog("ModuleFileName: %s \r\n\r\n", _tempBuffer);

	AppendCrashLog("CurrentThreadId: %d\r\n\r\n", GetCurrentThreadId());

	AppendCrashLog("ExceptionAddress: 0x%08X\r\n", (DWORD)(addr));

	if (code == EXCEPTION_ACCESS_VIOLATION)
	{
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

	AppendCrashLog("ExceptionCode: 0x%08X (%s)\r\n\r\n", code, ExceptionCodeToString(code, (MSVC_ThrowInfo*)ExceptionInformation[2]).c_str());
	
	AppendCrashLog("NumberParameters: %d\r\n\r\n", eps->ExceptionRecord->NumberParameters);

	AppendCrashLog(("Call Stack:\r\n------------------------------------------------------\r\n"));

	if (!SymInitialize(_hCurrentProcess, nullptr, TRUE))
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
		MessageBoxA(GetActiveWindow(), "Invaild eps.", MZ_CRASH_MSGBOX_CAPTION, 0);
	}

	_hCurrentProcess = GetCurrentProcess();
	std::string curFileName = GenDumpFileName();

	std::string dmpFileName = curFileName + ".dmp";
	WriteCrashDmp(dmpFileName, eps);

	std::string logFileName = curFileName + ".log";
	WriteCrashLog(logFileName, eps);

	MessageBoxA(GetActiveWindow(), "Click OK to open the crash log file.", MZ_CRASH_MSGBOX_CAPTION, 0);

	ShellExecuteA(0, "open", "explorer.exe", ("/select," + logFileName).c_str(), nullptr, SW_SHOWNORMAL);
}
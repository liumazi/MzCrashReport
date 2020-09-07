//
// CrashUtils.h
//
// Created by liuliu.mz on 20-08-28
//

#pragma once

#include <string>

#define CRASH_MSGBOX_CAPTION "MzCrashReport"
#define MZ_EXCEPTION_BASE_CODE 0x66666660

std::string GenDumpFileName();
std::string WStringToString(const std::wstring& wstr);

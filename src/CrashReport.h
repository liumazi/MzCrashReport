//
// CrashReport.h
//
// Created by liuliu.mz on 20-08-26
//

#pragma once

static const char* const rgBaseType[] =
{
	" <user defined> ",             // btNoType = 0,
	" void ",                       // btVoid = 1,
	" char* ",                      // btChar = 2,
	" wchar_t* ",                   // btWChar = 3,
	" signed char ",
	" unsigned char ",
	" int ",                        // btInt = 6,
	" unsigned int ",               // btUInt = 7,
	" float ",                      // btFloat = 8,
	" <BCD> ",                      // btBCD = 9,
	" bool ",                       // btBool = 10,
	" short ",
	" unsigned short ",
	" long ",                       // btLong = 13,
	" unsigned long ",              // btULong = 14,
	" __int8 ",
	" __int16 ",
	" __int32 ",
	" __int64 ",
	" __int128 ",
	" unsigned __int8 ",
	" unsigned __int16 ",
	" unsigned __int32 ",
	" unsigned __int64 ",
	" unsigned __int128 ",
	" <currency> ",                 // btCurrency = 25,
	" <date> ",                     // btDate = 26,
	" VARIANT ",                    // btVariant = 27,
	" <complex> ",                  // btComplex = 28,
	" <bit> ",                      // btBit = 29,
	" BSTR ",                       // btBSTR = 30,
	" HRESULT "                     // btHresult = 31
};

bool InitCrashReport();
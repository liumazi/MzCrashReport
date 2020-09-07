//
// CrashEHData.h (ehdata.h)
//
// Created by liuliu.mz on 20-09-08
//
// Copy form http://www.geoffchappell.com/studies/msvc/language/predefined/
//

#pragma once

#pragma pack (push, ehdata, 4)

struct MSVC_PMD
{
	int mdisp;
	int pdisp;
	int vdisp;
};

typedef void(*MSVC_PMFN) (void);

#pragma warning (disable:4200)
#pragma pack (push, _TypeDescriptor, 8)
struct MSVC_TypeDescriptor
{
	const void *pVFTable;
	void *spare;
	char name[];
};
#pragma pack (pop, _TypeDescriptor)
#pragma warning (default:4200)

struct MSVC_CatchableType
{
	unsigned int properties;
	MSVC_TypeDescriptor *pType;
	MSVC_PMD thisDisplacement;
	int sizeOrOffset;
	MSVC_PMFN copyFunction;
};

#pragma warning (disable:4200)
struct MSVC_CatchableTypeArray
{
	int nCatchableTypes;
	MSVC_CatchableType *arrayOfCatchableTypes[];
};
#pragma warning (default:4200)

struct MSVC_ThrowInfo
{
	unsigned int attributes;
	MSVC_PMFN pmfnUnwind;
	int(__cdecl *pForwardCompat) (...);
	MSVC_CatchableTypeArray *pCatchableTypeArray;
};

#pragma pack (pop, ehdata)
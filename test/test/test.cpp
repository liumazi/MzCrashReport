// test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "CrashReport.h"

int _tmain(int argc, _TCHAR* argv[])
{
	InitCrashReport();

	if (argc == 2)
	{
		int crashno = _tstoi(argv[1]);
		printf("crashno %d\r\n", crashno);

		switch (crashno)
		{
		case 0:
			*((int*)6) = 6;
			break;

		default:
			break;
		}
	}

	return 0;
}

